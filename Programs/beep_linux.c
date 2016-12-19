/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2016 by The BRLTTY Developers.
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

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include "log.h"
#include "beep.h"
#include "system_linux.h"

#define BEEP_DIVIDEND 1193180

static int consoleDevice = INVALID_FILE_DESCRIPTOR;

static inline BeepFrequency
getWaveLength (BeepFrequency frequency) {
  return frequency? (BEEP_DIVIDEND / frequency): 0;
}

static void
enableBeeps (void) {
  static unsigned char status = 0;

  installKernelModule("pcspkr", &status);
}

int
canBeep (void) {
  if (consoleDevice == INVALID_FILE_DESCRIPTOR) {
    const char *path = "/dev/tty0";
    int device = open(path, O_WRONLY);

    if (device == -1) {
      logMessage(LOG_WARNING, "can't open console: %s: %s", path, strerror(errno));
      return 0;
    }

    consoleDevice = device;
    enableBeeps();
  }

  return 1;
}

int
synchronousBeep (BeepFrequency frequency, BeepDuration duration) {
  return 0;
}

int
asynchronousBeep (BeepFrequency frequency, BeepDuration duration) {
  if (consoleDevice != INVALID_FILE_DESCRIPTOR) {
    if (ioctl(consoleDevice, KDMKTONE, ((duration << 0X10) | getWaveLength(frequency))) != -1) return 1;
    logSystemError("ioctl[KDMKTONE]");
  }

  return 0;
}

int
startBeep (BeepFrequency frequency) {
  if (consoleDevice != INVALID_FILE_DESCRIPTOR) {
    if (ioctl(consoleDevice, KIOCSOUND, getWaveLength(frequency)) != -1) return 1;
    logSystemError("ioctl[KIOCSOUND]");
  }

  return 0;
}

int
stopBeep (void) {
  return startBeep(0);
}

void
endBeep (void) {
  if (consoleDevice != INVALID_FILE_DESCRIPTOR) {
    close(consoleDevice);
    consoleDevice = INVALID_FILE_DESCRIPTOR;
  }
}
