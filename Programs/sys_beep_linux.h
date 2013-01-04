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

#include "log.h"
#include "device.h"

#define BEEP_DIVIDEND 1193180

static void
enableBeeps (void) {
  static int status = 0;
  installKernelModule("pcspkr", &status);
}

int
canBeep (void) {
  enableBeeps();
  return !!getConsole();
}

int
synchronousBeep (unsigned short frequency, unsigned short milliseconds) {
  return 0;
}

int
asynchronousBeep (unsigned short frequency, unsigned short milliseconds) {
  FILE *console = getConsole();
  if (console) {
    if (ioctl(fileno(console), KDMKTONE, ((milliseconds << 0X10) | (BEEP_DIVIDEND / frequency))) != -1) return 1;
    logSystemError("ioctl KDMKTONE");
  }
  return 0;
}

int
startBeep (unsigned short frequency) {
  FILE *console = getConsole();
  if (console) {
    if (ioctl(fileno(console), KIOCSOUND, BEEP_DIVIDEND/frequency) != -1) return 1;
    logSystemError("ioctl KIOCSOUND");
  }
  return 0;
}

int
stopBeep (void) {
  FILE *console = getConsole();
  if (console) {
    if (ioctl(fileno(console), KIOCSOUND, 0) != -1) return 1;
    logSystemError("ioctl KIOCSOUND");
  }
  return 0;
}

void
endBeep (void) {
}
