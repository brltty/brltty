/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include <dev/wscons/wsconsio.h>

int
canBeep (void) {
  return getConsole() != -1;
}

int
synchronousBeep (unsigned short frequency, unsigned short milliseconds) {
  return 0;
}

int
asynchronousBeep (unsigned short frequency, unsigned short milliseconds) {
  int console = getConsole();
  if (console != -1) {
    struct wskbd_bell_data bell;
    if (!(bell.period = milliseconds)) return 1;
    bell.pitch = frequency;
    bell.volume = 100;
    bell.which = WSKBD_BELL_DOALL;
    if (ioctl(console, WSKBDIO_COMPLEXBELL, &bell) != -1) return 1;
    LogError("ioctl WSKBDIO_COMPLEXBELL");
  }
  return 0;
}

int
startBeep (unsigned short frequency) {
  return 0;
}

int
stopBeep (void) {
  int console = getConsole();
  if (console != -1) {
    struct wskbd_bell_data bell;
    bell.which = WSKBD_BELL_DOVOLUME | WSKBD_BELL_DOPERIOD;
    bell.volume = 0;
    bell.period = 0;
    if (ioctl(console, WSKBDIO_COMPLEXBELL, &bell) != -1) return 1;
    LogError("ioctl WSKBDIO_COMPLEXBELL");
  }
  return 0;
}

void
endBeep (void) {
}
