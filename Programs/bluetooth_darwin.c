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

@interface BluetoothConnectionDelegate: AsynchronousTask
@property (assign) BluetoothConnectionExtension *bluetoothConnectionExtension;
@end

@interface RfcommChannelDelegate: BluetoothConnectionDelegate
- (void) rfcommChannelData
  : (IOBluetoothRFCOMMChannel *) rfcommChannel
  data: (void *) dataPointer
  length: (size_t) dataLength;

- (void) rfcommChannelClosed
  : (IOBluetoothRFCOMMChannel*) rfcommChannel;

- (IOReturn) run;
@end

struct BluetoothConnectionExtensionStruct {
  BluetoothDeviceAddress bluetoothAddress;
  IOBluetoothDevice *bluetoothDevice;

  IOBluetoothRFCOMMChannel *rfcommChannel;
  RfcommChannelDelegate *rfcommDelegate;

  int inputPipe[2];

  /* The handle returned by asyncMonitorFileInput() in bthMonitorInput()
   * below, kept so it can be cancelled before inputPipe[0] is closed (see
   * bthDestroyInputPipe()), and so a second registration can cancel a
   * still-active previous one instead of orphaning it (see
   * bthMonitorInput()) - matching bluetooth_android.c's own
   * bthMonitorInput(), which already does both for the same reason. */
  AsyncHandle inputMonitor;
};

static void
bthSetError (IOReturn result, const char *action) {
  setDarwinSystemError(result);
  logSystemError(action);
}

static void
bthInitializeRfcommChannel (BluetoothConnectionExtension *bcx) {
  bcx->rfcommChannel = nil;
}

static void
bthDestroyRfcommChannel (BluetoothConnectionExtension *bcx) {
  if (bcx->rfcommChannel) {
    [bcx->rfcommChannel closeChannel];
    [bcx->rfcommChannel release];
    bthInitializeRfcommChannel(bcx);
  }
}

static void
bthInitializeRfcommDelegate (BluetoothConnectionExtension *bcx) {
  bcx->rfcommDelegate = nil;
}

static void
bthDestroyRfcommDelegate (BluetoothConnectionExtension *bcx) {
  if (bcx->rfcommDelegate) {
    [bcx->rfcommDelegate stop];
    [bcx->rfcommDelegate wait:5];
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
    [bcx->bluetoothDevice closeConnection];
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
  bthDestroyRfcommChannel(bcx);
  bthDestroyRfcommDelegate(bcx);
  bthDestroyBluetoothDevice(bcx);
  bthDestroyInputPipe(bcx);
  free(bcx);
}

int
bthOpenChannel (BluetoothConnectionExtension *bcx, uint8_t channel, int timeout) {
  IOReturn result;

  if (pipe(bcx->inputPipe) != -1) {
    if (setBlockingIo(bcx->inputPipe[0], 0)) {
      if ((bcx->rfcommDelegate = [RfcommChannelDelegate new])) {
        bcx->rfcommDelegate.bluetoothConnectionExtension = bcx;

        if ((result = [bcx->bluetoothDevice openRFCOMMChannelSync:&bcx->rfcommChannel withChannelID:channel delegate:nil]) == kIOReturnSuccess) {
          if ([bcx->rfcommDelegate start]) {
            return 1;
          }

          bthDestroyRfcommChannel(bcx);
        } else {
          bthSetError(result, "RFCOMM channel open");
        }

        bthDestroyRfcommDelegate(bcx);
      }
    }

    bthDestroyInputPipe(bcx);
  } else {
    logSystemError("pipe");
  }

  return 0;
}

/* Explicitly opens the baseband connection to bcx's device, rather than
 * letting performSDPQuery: below trigger it implicitly. Confirmed live: a
 * query issued against a not-yet-connected device races macOS's own
 * automatic per-device "identification" SDP query (bluetoothd completes
 * its own copy in under a second, but BRLTTY's own performSDPQuery: against
 * the same still-connecting device gets no completion callback,
 * repeatably); once the device is already connected first (confirmed by
 * the case where something else connected to it before BRLTTY did),
 * BRLTTY's own SDP query reliably gets its callback.
 *
 * Calling this again when already connected is safe (IOBluetooth's header
 * documents openConnection as returning success in that case too, at least
 * prior to macOS 10.7 - since 10.7 it explicitly may instead report a
 * "connection exists" error, which this function treats the same as any
 * other failure: logged, then proceeds anyway), but is NOT a fast no-op
 * like a page-timeout-bounded async call would be: this is fully
 * synchronous, blocking this process's single thread for as long as
 * IOBluetooth's own default page timeout allows, with no BRLTTY-imposed
 * cap of its own (the caller-supplied discovery timeout isn't threaded
 * through here). Accepted as a one-time cost per outer connection attempt,
 * not per retry-loop iteration, since bounding it further would require
 * the same async-observer machinery bthPerformServiceQuery() below already
 * uses for its own wait - out of scope for this fix, which is about
 * ordering, not about making every step here asynchronous. */
static void
bthEnsureConnectionOpen (BluetoothConnectionExtension *bcx) {
  IOReturn result = [bcx->bluetoothDevice openConnection];
  if (result != kIOReturnSuccess) {
    logMessage(LOG_CATEGORY(BLUETOOTH_IO), "connection open failed, trying anyway");
  }
}

/* Returns true unless the live query got a definitive negative answer (a
 * real error status, not just a dropped/timed-out callback) - in that
 * case, bthDiscoverChannel() below skips bthLookUpCachedChannel() entirely
 * rather than risk returning a stale cached record instead of correctly
 * falling through to the caller's own hardcoded fallback channel. On
 * success the cache still needs to be consulted (a successful query only
 * means the cache is now populated, not that the channel number is known
 * yet), and on a timeout it's still worth trying - see
 * bthLookUpCachedChannel()'s own comment for why a dropped callback
 * doesn't rule out a usable cached record. */
static int
bthPerformServiceQuery (BluetoothConnectionExtension *bcx) {
  IOReturn result;
  ServiceQueryResult *target = [ServiceQueryResult new];
  int tryCache = 1;

  if (target) {
    if ((result = [bcx->bluetoothDevice performSDPQuery:target]) == kIOReturnSuccess) {
      if ([target wait:10]) {
        if ((result = target.finalStatus) != kIOReturnSuccess) {
          bthSetError(result, "service discovery response");
          tryCache = 0;
        }

        [target release];
      } else {
        /* Timed out. IOBluetooth may still deliver the completion later,
         * arbitrarily, on this thread's run loop - deliberately leak
         * rather than release an object it might still call back into.
         * ServiceQueryResult only ever touches its own ivars in that
         * callback, so a late delivery is a safe no-op. */
        logMessage(LOG_CATEGORY(BLUETOOTH_IO), "service discovery response timed out");
      }
    } else {
      bthSetError(result, "service discovery request");
      tryCache = 0;
      [target release];
    }
  } else {
    logMallocError();
    tryCache = 0;
  }

  return tryCache;
}

/* performSDPQuery:'s completion callback can go permanently missing - not
 * failing, just never arriving - on a connection that is still being
 * established: confirmed live via Console logging Apple's own "This
 * currently won't trigger SDP delegate" on exactly that path.
 * bthEnsureConnectionOpen() above avoids that by holding the link open
 * first, but doesn't fully eliminate it. When it does time out,
 * bthLookUpCachedChannel() below is the fallback, not a same-process retry
 * here - there's no evidence a second attempt behaves differently. */

/* Looks up an already-cached SDP record for uuidBytes without asking for a
 * fresh query - getServiceRecordForUUID: only consults records already
 * queried, so this is synchronous, free, and cannot time out. Called after
 * a successful or merely-timed-out bthPerformServiceQuery() (see its own
 * comment for why bthDiscoverChannel() skips this entirely after a
 * definitive failure instead): macOS's automatic per-device identification
 * pass can populate this same cache even when this process's own query
 * never got a completion callback for it. A record found this way could be
 * stale, but that is still at least as good a guess as the hardcoded
 * fallback used when this returns 0. */
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
  bthEnsureConnectionOpen(bcx);
  if (!bthPerformServiceQuery(bcx)) return 0;
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

  bthMakeAddress(&address, bda);

  {
    IOBluetoothDevice *device = [IOBluetoothDevice deviceWithAddress:&address];

    if (device != nil) {
      if ((result = [device remoteNameRequest:nil]) == kIOReturnSuccess) {
        NSString *nsName = device.name;

        if (nsName != nil) {
          const char *utf8Name = [nsName UTF8String];

          if (utf8Name != NULL) {
            char *name = strdup(utf8Name);

            if (name != NULL) {
              return name;
            }
          }
        }
      } else {
        bthSetError(result, "device name query");
      }

      [device closeConnection];
    }
  }

  return NULL;
}

@implementation ServiceQueryResult
- (void) sdpQueryComplete
  : (IOBluetoothDevice *) device
  status: (IOReturn) status
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

- (IOReturn) run
  {
    IOReturn result;
    logMessage(LOG_CATEGORY(BLUETOOTH_IO), "RFCOMM channel delegate started");

    {
      BluetoothConnectionExtension *bcx = self.bluetoothConnectionExtension;

      if ((result = [bcx->rfcommChannel setDelegate:self]) == kIOReturnSuccess) {
        CFRunLoopRun();
        result = kIOReturnSuccess;
      } else {
        bthSetError(result, "RFCOMM channel delegate set");
      }
    }

    logMessage(LOG_CATEGORY(BLUETOOTH_IO), "RFCOMM channel delegate finished");
    return result;
  }
@end

void
bthProcessDiscoveredDevices (
  DiscoveredBluetoothDeviceTester *testDevice, void *data
) {
}
