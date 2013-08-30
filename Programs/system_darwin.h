/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_SYSTEM_DARWIN
#define BRLTTY_INCLUDED_SYSTEM_DARWIN

#include <CoreFoundation/CFRunLoop.h>

#import <Foundation/NSThread.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern IOReturn executeRunLoop (int seconds);
extern void addRunLoopSource (CFRunLoopSourceRef source);
extern void removeRunLoopSource (CFRunLoopSourceRef source);

#define MAP_DARWIN_ERROR(from,to) case (from): errno = (to); break;
extern void setDarwinSystemError (IOReturn result);

@interface AsynchronousResult: NSObject
  {
    int isFinished;
    IOReturn finalStatus;
  }

- (int) wait
  : (int) timeout;

- (void) setStatus
  : (IOReturn) status;

- (IOReturn) getStatus;
@end

@interface AsynchronousTask: AsynchronousResult
  {
    NSThread *thread;
    CFRunLoopRef runLoop;
  }

- (IOReturn) run;

- (int) start;

- (void) stop;

- (NSThread *) getThread;

- (CFRunLoopRef) getRunLoop;
@end

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SYSTEM_DARWIN */
