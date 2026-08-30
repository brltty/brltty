/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2026 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>
#include <errno.h>

#import <IOBluetooth/objc/IOBluetoothDevice.h>
#import <IOBluetooth/objc/IOBluetoothSDPUUID.h>
#import <IOBluetooth/objc/IOBluetoothSDPServiceRecord.h>
#import <IOBluetooth/objc/IOBluetoothRFCOMMChannel.h>

#include "log.h"
#include "parameters.h"
#include "io_misc.h"
#include "io_bluetooth.h"
#include "bluetooth_internal.h"
#include "system_darwin.h"
#include "async_io.h"
#include "async_handle.h"

@interface ServiceQueryResult: AsynchronousResult
- (void) sdpQueryComplete
  : (IOBluetoothDevice *) device
  status: (IOReturn) status;
@end

/* Observes an openRFCOMMChannelAsync:withChannelID:delegate: request until it
 * completes or the caller gives up waiting (see bthOpenChannel()). It also
 * has to serve as the channel's data listener during that wait, because
 * IOBluetooth documents that the open won't even complete until one is
 * registered. bluetoothConnectionExtension is cleared by the caller if it
 * abandons the wait (timeout), so a late callback delivered after that point
 * becomes a safe no-op instead of touching a torn-down BluetoothConnectionExtension. */
@interface RfcommChannelOpenObserver: AsynchronousResult
@property (assign) BluetoothConnectionExtension *bluetoothConnectionExtension;

- (void) rfcommChannelOpenComplete
  : (IOBluetoothRFCOMMChannel *) rfcommChannel
  status: (IOReturn) error;

- (void) rfcommChannelData
  : (IOBluetoothRFCOMMChannel *) rfcommChannel
  data: (void *) dataPointer
  length: (size_t) dataLength;

- (void) rfcommChannelClosed
  : (IOBluetoothRFCOMMChannel*) rfcommChannel;
@end

/* Observes an openConnection:withPageTimeout:authenticationRequired: request
 * the same way. Never touches anything but its own ivars in its callback, so
 * unlike RfcommChannelOpenObserver a late callback after we've given up
 * waiting is always a safe no-op - no back-pointer to null out first. */
@interface ConnectionOpenResult: AsynchronousResult
- (void) connectionComplete
  : (IOBluetoothDevice *) device
  status: (IOReturn) status;
@end

/* Observes a remoteNameRequest:withPageTimeout: request the same way. The
 * two possible completion selectors (documented inconsistently across SDK
 * versions - see remoteNameRequest: vs remoteNameRequest:withPageTimeout: in
 * IOBluetoothDevice.h) are both implemented defensively. */
@interface RemoteNameRequestResult: AsynchronousResult
- (void) remoteNameRequestComplete
  : (IOBluetoothDevice *) device
  status: (IOReturn) status;

- (void) remoteNameRequestComplete
  : (IOBluetoothDevice *) device
  status: (IOReturn) status
  name: (NSString *) name;
@end

@interface BluetoothConnectionDelegate: NSObject
@property (assign) BluetoothConnectionExtension *bluetoothConnectionExtension;
@end

@interface RfcommChannelDelegate: BluetoothConnectionDelegate
- (void) rfcommChannelData
  : (IOBluetoothRFCOMMChannel *) rfcommChannel
  data: (void *) dataPointer
  length: (size_t) dataLength;

- (void) rfcommChannelClosed
  : (IOBluetoothRFCOMMChannel*) rfcommChannel;

- (IOReturn) attachToChannel;
@end

struct BluetoothConnectionExtensionStruct {
  BluetoothDeviceAddress bluetoothAddress;
  IOBluetoothDevice *bluetoothDevice;

  IOBluetoothRFCOMMChannel *rfcommChannel;
  RfcommChannelDelegate *rfcommDelegate;

  /* Owned for as long as rfcommChannel might still be able to message it as
   * a delegate - see bthDestroyRfcommOpenObserver()/bthDestroyRfcommChannel()
   * for why release is deferred to there instead of happening right after
   * bthOpenChannel() hands off to rfcommDelegate. */
  RfcommChannelOpenObserver *rfcommOpenObserver;

  int inputPipe[2];

  /* See darwinDrainRunLoop() / runLoopPumpAlarmCallback() below: IOBluetooth
   * delivers its asynchronous callbacks (including incoming RFCOMM data) by
   * scheduling a source on whatever thread's run loop was current when the
   * connection was made - here, this process's single main thread - and
   * nothing else in BRLTTY ever pumps that run loop. Without this, those
   * callbacks are simply never delivered, no matter how long the underlying
   * operation is willing to wait (confirmed via a minimal, single-threaded
   * reproduction outside BRLTTY entirely: the exact same IOBluetooth calls
   * receive a real reply immediately once something pumps the run loop of
   * the thread that made them). */
  AsyncHandle runLoopPumpAlarm;

  /* The handle returned by asyncMonitorFileInput() in bthMonitorInput()
   * below, kept so bthReleaseConnectionExtension() can cancel it before
   * bthDestroyInputPipe() closes inputPipe[0]. This used to be discarded
   * (asyncMonitorFileInput(NULL, ...)), unlike every other platform's
   * bthMonitorInput() (e.g. bluetooth_android.c), which does keep its
   * handle for exactly this reason. Without it, the core select()/poll()
   * loop in async_io.c was left with a monitor still registered on an
   * fd we had already closed - confirmed live: BRLTTY went into a
   * permanent, full-CPU "select error 9: Bad file descriptor" spin,
   * logging on every iteration, from the moment a connection that had
   * actually opened was torn down. async_io.c has no code to notice and
   * drop a stale monitor on its own, so the only fix is to never leave
   * one behind. */
  AsyncHandle inputMonitor;
};

static void
bthSetError (IOReturn result, const char *action) {
  setDarwinSystemError(result);
  logSystemError(action);
}

/* BluetoothHCIPageTimeout is in 0.625ms baseband slots (uint16_t, so at
 * most ~40.9 seconds). Clamp rather than wrap on an oversized request. */
static BluetoothHCIPageTimeout
bthPageTimeoutFromTimeout (int timeoutMilliseconds) {
  if (timeoutMilliseconds <= 0) return 1;

  long slots = (long)timeoutMilliseconds * 8L / 5L;
  if (slots > 0XFFFF) slots = 0XFFFF;
  if (slots < 1) slots = 1;
  return (BluetoothHCIPageTimeout)slots;
}

/* See DARWIN_BLUETOOTH_ASYNC_STEP_TIMEOUT's comment (parameters.h). */
static int
bthAsyncStepTimeout (int timeout) {
  return MIN(timeout, DARWIN_BLUETOOTH_ASYNC_STEP_TIMEOUT);
}

static void
bthInitializeRfcommChannel (BluetoothConnectionExtension *bcx) {
  bcx->rfcommChannel = nil;
}

static void
bthInitializeRfcommOpenObserver (BluetoothConnectionExtension *bcx) {
  bcx->rfcommOpenObserver = nil;
}

/* Only safe to call when no RFCOMM channel was ever created for this
 * observer (i.e. openRFCOMMChannelAsync: itself failed to even issue the
 * request) - in every other case, use bthAbandonRfcommOpenObserver() below
 * instead. */
static void
bthReleaseRfcommOpenObserver (BluetoothConnectionExtension *bcx) {
  if (bcx->rfcommOpenObserver) {
    bcx->rfcommOpenObserver.bluetoothConnectionExtension = nil;
    [bcx->rfcommOpenObserver release];
    bthInitializeRfcommOpenObserver(bcx);
  }
}

/* Once a channel object has existed at all, this observer is never released
 * - only abandoned (detached, but deliberately leaked). closeChannel() may
 * invoke a delegate method (e.g. its close notification) on whatever the
 * channel's delegate still is, and it isn't certain that's always delivered
 * synchronously inline rather than queued for a later run loop pump. A
 * queued message landing on a freed object later (e.g. during some
 * unrelated future wait:) would crash the whole daemon, which is far worse
 * than one small leaked object per real connection attempt. */
static void
bthAbandonRfcommOpenObserver (BluetoothConnectionExtension *bcx) {
  if (bcx->rfcommOpenObserver) {
    bcx->rfcommOpenObserver.bluetoothConnectionExtension = nil;
    bcx->rfcommOpenObserver = nil;
  }
}

static void
bthDestroyRfcommChannel (BluetoothConnectionExtension *bcx) {
  if (bcx->rfcommChannel) {
    [bcx->rfcommChannel closeChannel];
    [bcx->rfcommChannel release];
    bthInitializeRfcommChannel(bcx);
  }

  bthAbandonRfcommOpenObserver(bcx);
}

static void
bthInitializeRfcommDelegate (BluetoothConnectionExtension *bcx) {
  bcx->rfcommDelegate = nil;
}

static void
bthDestroyRfcommDelegate (BluetoothConnectionExtension *bcx) {
  if (bcx->rfcommDelegate) {
    [bcx->rfcommDelegate release];
    bthInitializeRfcommDelegate(bcx);
  }
}

static void
bthInitializeBluetoothDevice (BluetoothConnectionExtension *bcx) {
  bcx->bluetoothDevice = nil;
}

static void
bthDestroyBluetoothDevice (BluetoothConnectionExtension *bcx) {
  if (bcx->bluetoothDevice) {
    /* Deliberately not calling [bcx->bluetoothDevice closeConnection] here:
     * this runs at the end of every connection attempt, successful or not
     * (via bthReleaseConnectionExtension(), called once per driver-activation
     * retry cycle), and closeConnection() tears down the whole baseband
     * (ACL) connection to the device, not anything scoped to this extension.
     * Classic Bluetooth has exactly one ACL link per bonded device, shared
     * by every RFCOMM channel and SDP query - hanging it up unconditionally
     * here was silently severing any other in-progress or established
     * connection to the same device (e.g. one the peripheral itself
     * initiated) on every single retry, whether or not this attempt ever
     * opened anything of its own. Releasing our own reference to the device
     * object is sufficient cleanup; it does not require also disconnecting
     * the shared link. One consequence: BRLTTY itself never explicitly
     * closes the ACL link it opens, even at clean exit - it relies on
     * process teardown (or the peripheral itself) to release it. */
    [bcx->bluetoothDevice release];
    bthInitializeBluetoothDevice(bcx);
  }
}

static void
bthInitializeInputPipe (BluetoothConnectionExtension *bcx) {
  bcx->inputPipe[0] = bcx->inputPipe[1] = INVALID_FILE_DESCRIPTOR;
}

static void
bthCancelInputMonitor (BluetoothConnectionExtension *bcx) {
  if (bcx->inputMonitor) {
    asyncCancelRequest(bcx->inputMonitor);
    bcx->inputMonitor = NULL;
  }
}

static void
bthDestroyInputPipe (BluetoothConnectionExtension *bcx) {
  /* Must be cancelled before inputPipe[0] is closed below - see the
   * inputMonitor field comment on BluetoothConnectionExtensionStruct. */
  bthCancelInputMonitor(bcx);

  int *fileDescriptor = bcx->inputPipe;
  const int *end = fileDescriptor + ARRAY_COUNT(bcx->inputPipe);

  while (fileDescriptor < end) {
    closeFile(fileDescriptor);
    fileDescriptor += 1;
  }
}

static void
bthMakeAddress (BluetoothDeviceAddress *address, uint64_t bda) {
  unsigned int index = sizeof(address->data);

  while (index > 0) {
    address->data[--index] = bda & 0XFF;
    bda >>= 8;
  }
}

BluetoothConnectionExtension *
bthNewConnectionExtension (uint64_t bda) {
  BluetoothConnectionExtension *bcx;

  if ((bcx = malloc(sizeof(*bcx)))) {
    memset(bcx, 0, sizeof(*bcx));
    bthInitializeInputPipe(bcx);
    bthMakeAddress(&bcx->bluetoothAddress, bda);

    if ((bcx->bluetoothDevice = [IOBluetoothDevice deviceWithAddress:&bcx->bluetoothAddress])) {
      [bcx->bluetoothDevice retain];

      /* See the runLoopPumpAlarm field comment. Scoped to the lifetime of
       * this one connection rather than added globally (e.g. in the shared
       * core event loop) to keep this fix narrowly targeted at the actual
       * problem, with no risk to unrelated subsystems. */
      bthStartDarwinRunLoopPump(&bcx->runLoopPumpAlarm);

      return bcx;
    }

    free(bcx);
  } else {
    logMallocError();
  }

  return NULL;
}

void
bthReleaseConnectionExtension (BluetoothConnectionExtension *bcx) {
  if (bcx->runLoopPumpAlarm) {
    asyncCancelRequest(bcx->runLoopPumpAlarm);
    bcx->runLoopPumpAlarm = NULL;
  }

  bthDestroyRfcommChannel(bcx);
  bthDestroyRfcommDelegate(bcx);
  bthDestroyBluetoothDevice(bcx);
  bthDestroyInputPipe(bcx);
  free(bcx);
}

/* Explicitly opens the baseband connection to bcx's device, rather than
 * letting a later call (SDP query, RFCOMM open) trigger it implicitly.
 * Confirmed live: a query issued against a not-yet-connected device races
 * macOS's own automatic per-device "identification" SDP query (bluetoothd
 * completes its own copy in under a second, but BRLTTY's own
 * performSDPQuery: against the same still-connecting device gets no
 * completion callback, repeatably); once the device is already connected
 * first (confirmed by the case where something else connected to it
 * before BRLTTY did), BRLTTY's own SDP query reliably gets its callback.
 * Safe to call again later (e.g. from bthOpenChannel()) once already
 * connected - IOBluetooth documents that as a fast, effectively-idempotent
 * success rather than a fresh page. Async + bounded wait, like every other
 * Bluetooth call in this file; callers proceed regardless of the outcome
 * here.
 *
 * authenticationRequired: passing NO here does not mean the resulting link
 * is unencrypted - confirmed live, comparing Console logs side by side for
 * YES and NO against the same already-bonded device: both produce the
 * identical sequence (bluetoothd's own deferred security enforcement for
 * the RFCOMM/SerialPort service classes - "Send authentication request",
 * "Send set encryption on", "HCIEvent EncryptionChange: Encryption E0
 * enabled") ending in "Security level 2: state ENCRYPTED". macOS enforces
 * that security level for these service classes as its own policy,
 * independent of what this call requests. NO is used because it makes no
 * observed difference to the outcome, not because encryption is being
 * traded away. */
static void
bthEnsureConnectionOpen (BluetoothConnectionExtension *bcx, int timeout) {
  IOReturn result;
  ConnectionOpenResult *connectionResult = [ConnectionOpenResult new];

  if (connectionResult) {
    if ((result = [bcx->bluetoothDevice openConnection:connectionResult withPageTimeout:bthPageTimeoutFromTimeout(timeout) authenticationRequired:NO]) == kIOReturnSuccess) {
      /* See DARWIN_BLUETOOTH_ASYNC_STEP_TIMEOUT's comment (parameters.h) -
       * this bounds only how long this process waits for its own
       * completion callback, not the real page timeout given to the OS
       * above (bthPageTimeoutFromTimeout(timeout)), which is left at the
       * caller's full budget since that one genuinely governs how long a
       * real hardware page attempt gets. */
      int waitTimeout = bthAsyncStepTimeout(timeout);
      if ([connectionResult wait:waitTimeout]) {
        if ((result = connectionResult.finalStatus) != kIOReturnSuccess) {
          logMessage(LOG_CATEGORY(BLUETOOTH_IO), "authenticated connection failed, trying anyway");
        }

        [connectionResult release];
      } else {
        /* Timed out - as elsewhere in this file, deliberately leak
         * rather than release: the callback only ever touches its
         * own ivars, so a late delivery is a safe no-op, but it
         * isn't certain none is still owed. */
        logMessage(LOG_CATEGORY(BLUETOOTH_IO), "authenticated connection timed out, trying anyway");
      }
    } else {
      logMessage(LOG_CATEGORY(BLUETOOTH_IO), "authenticated connection request failed, trying anyway");
      [connectionResult release];
    }
  } else {
    logMallocError();
  }
}

int
bthOpenChannel (BluetoothConnectionExtension *bcx, uint8_t channel, int timeout) {
  IOReturn result;

  if (pipe(bcx->inputPipe) != -1) {
    if (setBlockingIo(bcx->inputPipe[0], 0)) {
      bcx->rfcommOpenObserver = [RfcommChannelOpenObserver new];

      if (bcx->rfcommOpenObserver) {
        bcx->rfcommOpenObserver.bluetoothConnectionExtension = bcx;
        bthEnsureConnectionOpen(bcx, timeout);

        if ((result = [bcx->bluetoothDevice openRFCOMMChannelAsync:&bcx->rfcommChannel withChannelID:channel delegate:bcx->rfcommOpenObserver]) == kIOReturnSuccess) {
          /* See DARWIN_BLUETOOTH_ASYNC_STEP_TIMEOUT's comment (parameters.h)
           * for why this step specifically uses a shorter bound than the
           * caller-supplied timeout, never a longer one. */
          int openTimeout = bthAsyncStepTimeout(timeout);
          if ([bcx->rfcommOpenObserver wait:openTimeout]) {
            /* The open has completed (success or failure). Either way,
             * bthDestroyRfcommChannel() below always abandons rather than
             * releases bcx->rfcommOpenObserver once a channel has existed at
             * all - see its comment for why: the handoff to bcx->rfcommDelegate
             * just below is synchronous, so the observer only remains the
             * channel's delegate here if that handoff itself never happened
             * or failed (open failed, or attachToChannel failed) - but it
             * isn't certain that closeChannel()'s own close notification (if
             * any) is always delivered synchronously inline rather than
             * queued for a later run loop pump, so treat it the same way
             * regardless. */
            if ((result = bcx->rfcommOpenObserver.finalStatus) == kIOReturnSuccess) {
              if ((bcx->rfcommDelegate = [RfcommChannelDelegate new])) {
                bcx->rfcommDelegate.bluetoothConnectionExtension = bcx;
                if ([bcx->rfcommDelegate attachToChannel] == kIOReturnSuccess) return 1;
                bthDestroyRfcommDelegate(bcx);
              }
            } else {
              bthSetError(result, "RFCOMM channel open");
            }

            bthDestroyRfcommChannel(bcx);
          } else {
            /* Timed out. IOBluetooth may still deliver the completion
             * asynchronously, arbitrarily later, on this (the main) thread's
             * run loop - the next time anything pumps it (e.g. the next
             * wait: call, maybe for an unrelated device). bthDestroyRfcommChannel()
             * below abandons (never releases) bcx->rfcommOpenObserver for
             * exactly this reason - a small, bounded leak (at most one per
             * real connection attempt, roughly one per
             * BRAILLE_DRIVER_START_RETRY_INTERVAL while retrying) beats a
             * deferred use-after-free. closeChannel is the
             * documented way to abandon the channel itself, independent of
             * the observer's own lifetime. */
            logMessage(LOG_CATEGORY(BLUETOOTH_IO), "RFCOMM channel open timed out");
            bthSetError(kIOReturnTimeout, "RFCOMM channel open");
            bthDestroyRfcommChannel(bcx);
          }
        } else {
          bthSetError(result, "RFCOMM channel open request");

          /* No channel was ever created, so there's no possibility of a
           * channel-related delegate callback, past or future - safe to
           * release outright rather than abandon. */
          bthReleaseRfcommOpenObserver(bcx);
        }
      }
    }

    bthDestroyInputPipe(bcx);
  } else {
    logSystemError("pipe");
  }

  return 0;
}

static void
bthPerformServiceQuery (BluetoothConnectionExtension *bcx, int timeout) {
  IOReturn result;
  ServiceQueryResult *target = [ServiceQueryResult new];

  if (target) {
    if ((result = [bcx->bluetoothDevice performSDPQuery:target]) == kIOReturnSuccess) {
      /* See DARWIN_BLUETOOTH_ASYNC_STEP_TIMEOUT's comment (parameters.h). */
      int waitTimeout = bthAsyncStepTimeout(timeout);
      if ([target wait:waitTimeout]) {
        if ((result = target.finalStatus) != kIOReturnSuccess) {
          bthSetError(result, "service discovery response");
        }
      } else {
        bthSetError(kIOReturnTimeout, "service discovery response");
      }
    } else {
      bthSetError(result, "service discovery request");
    }

    [target release];
  }
}

/* performSDPQuery:'s completion callback can go permanently missing - not
 * failing, just never arriving - on a connection that is still being
 * established: confirmed live via Console logging Apple's own
 * "This currently won't trigger SDP delegate" on exactly that path.
 * bthEnsureConnectionOpen() above avoids that by holding the link open
 * first, but doesn't fully eliminate it. Do not switch to the UUID-scoped
 * performSDPQuery:uuids: variant to work around this - it is independently
 * reported as less reliable, not more. When this does time out,
 * bthLookUpCachedChannel() below is the fallback, not a same-process
 * retry here - there's no evidence a second attempt behaves differently. */

/* Looks up an already-cached SDP record for uuidBytes without asking for a
 * fresh query - getServiceRecordForUUID: only consults records already
 * queried, so this is synchronous, free, and cannot time out. Called
 * unconditionally after bthPerformServiceQuery() regardless of whether that
 * query's own wait succeeded: macOS's automatic per-device identification
 * pass can populate this same cache even when this process's own query
 * never got a completion callback for it (see bthPerformServiceQuery()'s
 * comment). A record found this way could be stale, but that is still at
 * least as good a guess as the hardcoded fallback used when this returns 0. */
static int
bthLookUpCachedChannel (
  uint8_t *channel, BluetoothConnectionExtension *bcx,
  const void *uuidBytes, size_t uuidLength
) {
  IOBluetoothSDPUUID *uuid = [IOBluetoothSDPUUID uuidWithBytes:uuidBytes length:uuidLength];

  if (uuid) {
    IOBluetoothSDPServiceRecord *record = [bcx->bluetoothDevice getServiceRecordForUUID:uuid];

    if (record) {
      IOReturn result = [record getRFCOMMChannelID:channel];
      if (result == kIOReturnSuccess) return 1;
      bthSetError(result, "RFCOMM channel lookup");
    }
  }

  return 0;
}

int
bthDiscoverChannel (
  uint8_t *channel, BluetoothConnectionExtension *bcx,
  const void *uuidBytes, size_t uuidLength,
  int timeout
) {
  bthEnsureConnectionOpen(bcx, timeout);
  bthPerformServiceQuery(bcx, timeout);
  return bthLookUpCachedChannel(channel, bcx, uuidBytes, uuidLength);
}

int
bthMonitorInput (BluetoothConnection *connection, AsyncMonitorCallback *callback, void *data) {
  BluetoothConnectionExtension *bcx = connection->extension;

  /* GIO deregisters by calling this again with callback == NULL (see
   * gioDestroyHandleInputObject()) - matches bluetooth_android.c's own
   * bthMonitorInput(). Without the early return, that call would register
   * a fresh monitor with a NULL callback instead of just cancelling the
   * real one, and the first invocation of that null callback would delete
   * the operation out from under inputMonitor, leaving it dangling. */
  bthCancelInputMonitor(bcx);
  if (!callback) return 1;
  return asyncMonitorFileInput(&bcx->inputMonitor, bcx->inputPipe[0], callback, data);
}

int
bthPollInput (BluetoothConnectionExtension *bcx, int timeout) {
  return awaitFileInput(bcx->inputPipe[0], timeout);
}

ssize_t
bthGetData (
  BluetoothConnectionExtension *bcx, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  return readFile(bcx->inputPipe[0], buffer, size, initialTimeout, subsequentTimeout);
}

ssize_t
bthPutData (BluetoothConnectionExtension *bcx, const void *buffer, size_t size) {
  IOReturn result = [bcx->rfcommChannel writeSync:(void *)buffer length:size];

  if (result == kIOReturnSuccess) return size;
  bthSetError(result, "RFCOMM channel write");
  return -1;
}

char *
bthObtainDeviceName (uint64_t bda, int timeout) {
  IOReturn result;
  BluetoothDeviceAddress address;
  char *name = NULL;

  /* Captures the real failure, if any, independent of whatever errno holds
   * by the time this function actually returns - intervening calls
   * ([target release], device.name, UTF8String) aren't guaranteed to leave
   * errno untouched. 0 means "no real failure": a successful query that
   * simply found no name to report must not be reported as an error via a
   * leftover errno value. */
  int reportedError = 0;

  bthMakeAddress(&address, bda);

  {
    IOBluetoothDevice *device = [IOBluetoothDevice deviceWithAddress:&address];

    if (device != nil) {
      RemoteNameRequestResult *target = [RemoteNameRequestResult new];

      if (target) {
        if ((result = [device remoteNameRequest:target withPageTimeout:bthPageTimeoutFromTimeout(timeout)]) == kIOReturnSuccess) {
          if ([target wait:timeout]) {
            if ((result = target.finalStatus) == kIOReturnSuccess) {
              NSString *nsName = device.name;

              if (nsName != nil) {
                const char *utf8Name = [nsName UTF8String];
                if (utf8Name != NULL) {
                  if (!(name = strdup(utf8Name))) reportedError = ENOMEM;
                }
              }
            } else {
              bthSetError(result, "device name query");
              reportedError = errno;
            }

            [target release];
          } else {
            /* Timed out. As in bthOpenChannel(): IOBluetooth may still
             * deliver the completion later, arbitrarily, on this thread's
             * run loop - deliberately leak rather than release an object it
             * might still call back into. RemoteNameRequestResult only ever
             * touches its own ivars in that callback, so unlike the RFCOMM
             * open case there's no external state to null out first. */
            logMessage(LOG_CATEGORY(BLUETOOTH_IO), "device name query timed out");
            bthSetError(kIOReturnTimeout, "device name query");
            reportedError = errno;
          }
        } else {
          bthSetError(result, "device name query");
          reportedError = errno;
          [target release];
        }
      } else {
        logMallocError();
        reportedError = ENOMEM;
      }

      /* Deliberately not calling [device closeConnection] here - see
       * bthDestroyBluetoothDevice()'s comment for why. */
    } else {
      reportedError = ENODEV;
    }
  }

  errno = reportedError;
  return name;
}

@implementation ServiceQueryResult
- (void) sdpQueryComplete
  : (IOBluetoothDevice *) device
  status: (IOReturn) status
  {
    [self setStatus:status];
  }
@end

@implementation RfcommChannelOpenObserver
@synthesize bluetoothConnectionExtension;

- (void) rfcommChannelOpenComplete
  : (IOBluetoothRFCOMMChannel *) rfcommChannel
  status: (IOReturn) error
  {
    [self setStatus:error];
  }

- (void) rfcommChannelData
  : (IOBluetoothRFCOMMChannel *) rfcommChannel
  data: (void *) dataPointer
  length: (size_t) dataLength
  {
    /* Only reachable while a connect attempt is still being waited on
     * (bthOpenChannel() nulls this out before abandoning a timed-out wait),
     * so bcx and its inputPipe are still valid here. */
    BluetoothConnectionExtension *bcx = self.bluetoothConnectionExtension;
    if (bcx) writeFile(bcx->inputPipe[1], dataPointer, dataLength);
  }

- (void) rfcommChannelClosed
  : (IOBluetoothRFCOMMChannel*) rfcommChannel
  {
    logMessage(LOG_CATEGORY(BLUETOOTH_IO), "RFCOMM channel closed before delegate started");
  }
@end

@implementation ConnectionOpenResult
- (void) connectionComplete
  : (IOBluetoothDevice *) device
  status: (IOReturn) status
  {
    [self setStatus:status];
  }
@end

@implementation RemoteNameRequestResult
- (void) remoteNameRequestComplete
  : (IOBluetoothDevice *) device
  status: (IOReturn) status
  {
    [self setStatus:status];
  }

- (void) remoteNameRequestComplete
  : (IOBluetoothDevice *) device
  status: (IOReturn) status
  name: (NSString *) name
  {
    [self setStatus:status];
  }
@end

@implementation BluetoothConnectionDelegate
@synthesize bluetoothConnectionExtension;
@end

@implementation RfcommChannelDelegate
- (void) rfcommChannelData
  : (IOBluetoothRFCOMMChannel *) rfcommChannel
  data: (void *) dataPointer
  length: (size_t) dataLength
  {
    writeFile(self.bluetoothConnectionExtension->inputPipe[1], dataPointer, dataLength);
  }

- (void) rfcommChannelClosed
  : (IOBluetoothRFCOMMChannel*) rfcommChannel
  {
    logMessage(LOG_NOTICE, "RFCOMM channel closed");
  }

/* This used to spawn a whole background thread whose only job was to keep
 * its own run loop pumped, on the theory that IOBluetooth might deliver
 * rfcommChannelData: callbacks there. Confirmed live it does not: every
 * callback observed, across multiple full connections, arrived on the main
 * thread (the runLoopPumpAlarm mechanism already pumps that - see
 * BluetoothConnectionExtensionStruct's runLoopPumpAlarm field) - matching
 * this file's own established understanding that IOBluetooth binds
 * callback delivery to whichever thread was current when the channel was
 * created, not to whatever thread later calls setDelegate:. The dedicated
 * thread was doing nothing but consuming a thread's worth of resources.
 * Removing it also closes a real race the old code had: the handoff to
 * this delegate happened *asynchronously*, whenever that background thread
 * next got scheduled, so a callback in the gap between the RFCOMM open
 * completing and the thread actually running could still have gone to the
 * old open-observer delegate. Calling this directly and synchronously,
 * right after the open completes, closes that gap. */
- (IOReturn) attachToChannel
  {
    IOReturn result = [self.bluetoothConnectionExtension->rfcommChannel setDelegate:self];
    if (result != kIOReturnSuccess) bthSetError(result, "RFCOMM channel delegate set");
    return result;
  }
@end

void
bthProcessDiscoveredDevices (
  DiscoveredBluetoothDeviceTester *testDevice, void *data
) {
}
