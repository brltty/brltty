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

#ifndef BRLTTY_INCLUDED_SYSTEM_DARWIN
#define BRLTTY_INCLUDED_SYSTEM_DARWIN

#include <CoreFoundation/CFRunLoop.h>

#include "async_types_handle.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern IOReturn executeRunLoop (int seconds);
extern void darwinDrainRunLoop (void);
extern void addRunLoopSource (CFRunLoopSourceRef source);
extern void removeRunLoopSource (CFRunLoopSourceRef source);

/* Implemented in system_darwin_runloop.c, not system_darwin.c: this header
 * pulls in <CoreFoundation/CFRunLoop.h>, and CoreFoundation's own
 * MacTypes.h typedefs a TimeValue that collides with this codebase's own
 * struct TimeValue (Headers/timing_types.h) - which is exactly what
 * scheduling a repeating alarm (async_alarm.h) needs. Any translation unit
 * that includes this header can therefore never also include async_alarm.h,
 * so the alarm itself has to live in a separate file that never includes
 * this one. Starts (or, on a handle that's already running, is a no-op for)
 * a periodic alarm that drains this thread's CFRunLoop - needed because
 * IOKit/IOBluetooth deliver their asynchronous callbacks by scheduling
 * sources on whatever thread's run loop was current when the request was
 * made, and nothing else in BRLTTY ever pumps that loop on its own. Returns
 * true on success. */
extern int darwinRequestRunLoopPump (AsyncHandle *handle);

#define MAP_DARWIN_ERROR(from,to) case (from): errno = (to); break;
extern void setDarwinSystemError (IOReturn result);

@interface AsynchronousResult: NSObject
@property (assign, readonly) int isFinished;
@property (assign, readonly) IOReturn finalStatus;

/* timeoutMilliseconds: how long to wait for isFinished to become true. */
- (int) wait
  : (int) timeoutMilliseconds;

- (void) setStatus
  : (IOReturn) status;
@end

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SYSTEM_DARWIN */
