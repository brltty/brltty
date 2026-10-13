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

#include "parameters.h"
#include "async_alarm.h"
#include "async_handle.h"

/* Deliberately not declared via system_darwin.h, and this deliberately
 * isn't part of system_darwin.c - see darwinRequestRunLoopPump()'s own
 * comment there for why: this file must never include anything that pulls
 * in CoreFoundation. */
extern void darwinDrainRunLoop (void);

ASYNC_ALARM_CALLBACK(darwinRunLoopPumpAlarmCallback) {
  darwinDrainRunLoop();
}

int
darwinRequestRunLoopPump (AsyncHandle *handle) {
  if (asyncNewRelativeAlarm(handle, DARWIN_BLUETOOTH_RUN_LOOP_PUMP_INTERVAL, darwinRunLoopPumpAlarmCallback, NULL)) {
    if (asyncResetAlarmInterval(*handle, DARWIN_BLUETOOTH_RUN_LOOP_PUMP_INTERVAL)) return 1;
    asyncCancelRequest(*handle);
    *handle = NULL;
  }

  return 0;
}
