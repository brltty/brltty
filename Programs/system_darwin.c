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

#include <errno.h>
#include <time.h>
#include <IOKit/IOKitLib.h>

#include "log.h"
#include "parameters.h"
#include "system.h"
#include "system_darwin.h"
#include "async_wait.h"

static inline CFRunLoopRef
getRunLoop (void) {
  return CFRunLoopGetCurrent();
}

static inline CFStringRef
getRunMode (void) {
  return kCFRunLoopDefaultMode;
}

IOReturn
executeRunLoop (int seconds) {
  return CFRunLoopRunInMode(getRunMode(), seconds, 1);
}

void
darwinDrainRunLoop (void) {
  /* Non-blocking: process whatever's already ready on this thread's run
   * loop and return immediately once nothing more is pending, rather than
   * waiting for anything new to arrive - callers own waiting between calls.
   * See DARWIN_DRAIN_RUN_LOOP_ITERATION_LIMIT's comment for the bound. */
  int iterations = 0;
  while (CFRunLoopRunInMode(getRunMode(), 0, true) == kCFRunLoopRunHandledSource) {
    if (++iterations >= DARWIN_DRAIN_RUN_LOOP_ITERATION_LIMIT) {
      logMessage(LOG_WARNING, "darwinDrainRunLoop: iteration limit reached");
      break;
    }
  }
}

void
addRunLoopSource (CFRunLoopSourceRef source) {
  CFRunLoopAddSource(getRunLoop(), source, getRunMode());
}

void
removeRunLoopSource (CFRunLoopSourceRef source) {
  CFRunLoopRemoveSource(getRunLoop(), source, getRunMode());
}

void
setDarwinSystemError (IOReturn result) {
  switch (result) {
    default: errno = EIO; break;

  //MAP_DARWIN_ERROR(KERN_SUCCESS, )
    MAP_DARWIN_ERROR(KERN_INVALID_ADDRESS, EINVAL)
    MAP_DARWIN_ERROR(KERN_PROTECTION_FAILURE, EFAULT)
    MAP_DARWIN_ERROR(KERN_NO_SPACE, ENOSPC)
    MAP_DARWIN_ERROR(KERN_INVALID_ARGUMENT, EINVAL)
  //MAP_DARWIN_ERROR(KERN_FAILURE, )
    MAP_DARWIN_ERROR(KERN_RESOURCE_SHORTAGE, EAGAIN)
  //MAP_DARWIN_ERROR(KERN_NOT_RECEIVER, )
    MAP_DARWIN_ERROR(KERN_NO_ACCESS, EACCES)
    MAP_DARWIN_ERROR(KERN_MEMORY_FAILURE, EFAULT)
    MAP_DARWIN_ERROR(KERN_MEMORY_ERROR, EFAULT)
  //MAP_DARWIN_ERROR(KERN_ALREADY_IN_SET, )
  //MAP_DARWIN_ERROR(KERN_NOT_IN_SET, )
    MAP_DARWIN_ERROR(KERN_NAME_EXISTS, EEXIST)
    MAP_DARWIN_ERROR(KERN_ABORTED, ECANCELED)
    MAP_DARWIN_ERROR(KERN_INVALID_NAME, EINVAL)
    MAP_DARWIN_ERROR(KERN_INVALID_TASK, EINVAL)
    MAP_DARWIN_ERROR(KERN_INVALID_RIGHT, EINVAL)
    MAP_DARWIN_ERROR(KERN_INVALID_VALUE, EINVAL)
  //MAP_DARWIN_ERROR(KERN_UREFS_OVERFLOW, )
    MAP_DARWIN_ERROR(KERN_INVALID_CAPABILITY, EINVAL)
  //MAP_DARWIN_ERROR(KERN_RIGHT_EXISTS, )
    MAP_DARWIN_ERROR(KERN_INVALID_HOST, EINVAL)
  //MAP_DARWIN_ERROR(KERN_MEMORY_PRESENT, )
  //MAP_DARWIN_ERROR(KERN_MEMORY_DATA_MOVED, )
  //MAP_DARWIN_ERROR(KERN_MEMORY_RESTART_COPY, )
    MAP_DARWIN_ERROR(KERN_INVALID_PROCESSOR_SET, EINVAL)
  //MAP_DARWIN_ERROR(KERN_POLICY_LIMIT, )
    MAP_DARWIN_ERROR(KERN_INVALID_POLICY, EINVAL)
    MAP_DARWIN_ERROR(KERN_INVALID_OBJECT, EINVAL)
  //MAP_DARWIN_ERROR(KERN_ALREADY_WAITING, )
  //MAP_DARWIN_ERROR(KERN_DEFAULT_SET, )
  //MAP_DARWIN_ERROR(KERN_EXCEPTION_PROTECTED, )
    MAP_DARWIN_ERROR(KERN_INVALID_LEDGER, EINVAL)
    MAP_DARWIN_ERROR(KERN_INVALID_MEMORY_CONTROL, EINVAL)
    MAP_DARWIN_ERROR(KERN_INVALID_SECURITY, EINVAL)
  //MAP_DARWIN_ERROR(KERN_NOT_DEPRESSED, )
  //MAP_DARWIN_ERROR(KERN_TERMINATED, )
  //MAP_DARWIN_ERROR(KERN_LOCK_SET_DESTROYED, )
  //MAP_DARWIN_ERROR(KERN_LOCK_UNSTABLE, )
  //MAP_DARWIN_ERROR(KERN_LOCK_OWNED, )
  //MAP_DARWIN_ERROR(KERN_LOCK_OWNED_SELF, )
  //MAP_DARWIN_ERROR(KERN_SEMAPHORE_DESTROYED, )
  //MAP_DARWIN_ERROR(KERN_RPC_SERVER_TERMINATED, )
  //MAP_DARWIN_ERROR(KERN_RPC_TERMINATE_ORPHAN, )
  //MAP_DARWIN_ERROR(KERN_RPC_CONTINUE_ORPHAN, )
    MAP_DARWIN_ERROR(KERN_NOT_SUPPORTED, ENOTSUP)
    MAP_DARWIN_ERROR(KERN_NODE_DOWN, EHOSTDOWN)
  //MAP_DARWIN_ERROR(KERN_NOT_WAITING, )
    MAP_DARWIN_ERROR(KERN_OPERATION_TIMED_OUT, ETIMEDOUT)

    MAP_DARWIN_ERROR(kIOReturnSuccess, 0)
  //MAP_DARWIN_ERROR(kIOReturnError, )
    MAP_DARWIN_ERROR(kIOReturnNoMemory, ENOMEM)
    MAP_DARWIN_ERROR(kIOReturnNoResources, EAGAIN)
  //MAP_DARWIN_ERROR(kIOReturnIPCError, )
    MAP_DARWIN_ERROR(kIOReturnNoDevice, ENODEV)
    MAP_DARWIN_ERROR(kIOReturnNotPrivileged, EACCES)
    MAP_DARWIN_ERROR(kIOReturnBadArgument, EINVAL)
    MAP_DARWIN_ERROR(kIOReturnLockedRead, ENOLCK)
    MAP_DARWIN_ERROR(kIOReturnLockedWrite, ENOLCK)
    MAP_DARWIN_ERROR(kIOReturnExclusiveAccess, EBUSY)
  //MAP_DARWIN_ERROR(kIOReturnBadMessageID, )
    MAP_DARWIN_ERROR(kIOReturnUnsupported, ENOTSUP)
  //MAP_DARWIN_ERROR(kIOReturnVMError, )
  //MAP_DARWIN_ERROR(kIOReturnInternalError, )
    MAP_DARWIN_ERROR(kIOReturnIOError, EIO)
    MAP_DARWIN_ERROR(kIOReturnCannotLock, ENOLCK)
    MAP_DARWIN_ERROR(kIOReturnNotOpen, EBADF)
    MAP_DARWIN_ERROR(kIOReturnNotReadable, EACCES)
    MAP_DARWIN_ERROR(kIOReturnNotWritable, EROFS)
  //MAP_DARWIN_ERROR(kIOReturnNotAligned, )
    MAP_DARWIN_ERROR(kIOReturnBadMedia, ENXIO)
  //MAP_DARWIN_ERROR(kIOReturnStillOpen, )
  //MAP_DARWIN_ERROR(kIOReturnRLDError, )
    MAP_DARWIN_ERROR(kIOReturnDMAError, EDEVERR)
    MAP_DARWIN_ERROR(kIOReturnBusy, EBUSY)
    MAP_DARWIN_ERROR(kIOReturnTimeout, ETIMEDOUT)
    MAP_DARWIN_ERROR(kIOReturnOffline, ENXIO)
    MAP_DARWIN_ERROR(kIOReturnNotReady, ENXIO)
    MAP_DARWIN_ERROR(kIOReturnNotAttached, ENXIO)
    MAP_DARWIN_ERROR(kIOReturnNoChannels, EDEVERR)
    MAP_DARWIN_ERROR(kIOReturnNoSpace, ENOSPC)
    MAP_DARWIN_ERROR(kIOReturnPortExists, EADDRINUSE)
    MAP_DARWIN_ERROR(kIOReturnCannotWire, ENOMEM)
  //MAP_DARWIN_ERROR(kIOReturnNoInterrupt, )
    MAP_DARWIN_ERROR(kIOReturnNoFrames, EDEVERR)
    MAP_DARWIN_ERROR(kIOReturnMessageTooLarge, EMSGSIZE)
    MAP_DARWIN_ERROR(kIOReturnNotPermitted, EPERM)
    MAP_DARWIN_ERROR(kIOReturnNoPower, EPWROFF)
    MAP_DARWIN_ERROR(kIOReturnNoMedia, ENXIO)
    MAP_DARWIN_ERROR(kIOReturnUnformattedMedia, ENXIO)
    MAP_DARWIN_ERROR(kIOReturnUnsupportedMode, ENOSYS)
    MAP_DARWIN_ERROR(kIOReturnUnderrun, EDEVERR)
    MAP_DARWIN_ERROR(kIOReturnOverrun, EDEVERR)
    MAP_DARWIN_ERROR(kIOReturnDeviceError, EDEVERR)
  //MAP_DARWIN_ERROR(kIOReturnNoCompletion, )
    MAP_DARWIN_ERROR(kIOReturnAborted, ECANCELED)
    MAP_DARWIN_ERROR(kIOReturnNoBandwidth, EDEVERR)
    MAP_DARWIN_ERROR(kIOReturnNotResponding, EDEVERR)
    MAP_DARWIN_ERROR(kIOReturnIsoTooOld, EDEVERR)
    MAP_DARWIN_ERROR(kIOReturnIsoTooNew, EDEVERR)
    MAP_DARWIN_ERROR(kIOReturnNotFound, ENOENT)
  //MAP_DARWIN_ERROR(kIOReturnInvalid, )
  }
}

void
initializeSystemObject (void) {
}

@interface AsynchronousResult ()
@property (assign, readwrite) int isFinished;
@property (assign, readwrite) IOReturn finalStatus;
@end

/* asyncAwaitCondition()'s tester, called from within BRLTTY's own event
 * loop (async_wait.c's awaitAction()) between servicing due alarms and I/O -
 * see wait: below for why going through that loop, rather than pumping the
 * CFRunLoop directly, is the actual fix here, not just a style change. */
static int
darwinAsynchronousResultFinished (void *data) {
  AsynchronousResult *result = (AsynchronousResult *)data;
  darwinDrainRunLoop();
  return result.isFinished;
}

@implementation AsynchronousResult
@synthesize isFinished;
@synthesize finalStatus;

- (int) wait
  : (int) timeoutMilliseconds
  {
    if (self.isFinished) return 1;

    /* This used to pump only this thread's CFRunLoop directly
     * (CFRunLoopRunInMode()), never BRLTTY's own event loop
     * (Programs/async_wait.c's awaitAction(), which services alarms and
     * other file descriptors - the update alarm, the screen driver's own
     * monitored input, BrlAPI, and so on) - confirmed live to starve all of
     * those for as long as this wait ran. On every other platform, the
     * equivalent blocking wait inside a driver callback (e.g.
     * bluetooth_linux.c's connect wait) goes through
     * asyncAwaitCondition()/awaitSocketInput(), a *recursive* re-entry into
     * that same event loop, so alarms and other I/O keep running while it
     * blocks. Looping asyncAwaitCondition() here in small slices - each one
     * draining the CFRunLoop via darwinAsynchronousResultFinished() - makes
     * this wait behave the same way. The slicing (rather than one call for
     * the whole timeout) matters on its own: asyncAwaitCondition() only
     * calls its tester between due alarms, so a single call would drain the
     * CFRunLoop only as often as some *other* alarm happens to fire -
     * confirmed live, a wait with no Bluetooth connection open yet (so no
     * pump alarm running - see bluetooth_darwin.c's runLoopPumpAlarm) barely
     * polled at all. */
    int remaining = timeoutMilliseconds;

    while (remaining > 0) {
      int slice = remaining < DARWIN_WAIT_POLL_INTERVAL? remaining: DARWIN_WAIT_POLL_INTERVAL;
      if (asyncAwaitCondition(slice, darwinAsynchronousResultFinished, self)) break;
      remaining -= slice;
    }

    return self.isFinished;
  }

- (void) setStatus
  : (IOReturn) status
  {
    self.finalStatus = status;
    self.isFinished = 1;
  }
@end

