/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include "bell.h"
#include "alert.h"
#include "prefs.h"
#include "tune_types.h"

int
setBellInterception (int on) {
  if (on) return startInterceptingBell();

  stopInterceptingBell();
  return 1;
}

void
alertBell (void) {
  if (prefs.tuneDevice != tdBeeper) alert(ALERT_CONSOLE_BELL);
}
