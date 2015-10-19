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

#include "log.h"
#include "tune_utils.h"
#include "tune_types.h"
#include "parse.h"
#include "prefs.h"

const char *tuneDeviceNames[] = {
  "beeper",
  "pcm",
  "midi",
  "fm",
  NULL
};

int
setOutputVolume (const char *setting) {
  if (setting && *setting) {
    static const int minimum = 0;
    static const int maximum = 100;
    int volume;

    if (!validateInteger(&volume, setting, &minimum, &maximum)) {
      logMessage(LOG_ERR, "%s: %s", "invalid volume percentage", setting);
      return 0;
    }

    switch (prefs.tuneDevice) {
      case tdPcm:
        prefs.pcmVolume = volume;
        break;

      case tdMidi:
        prefs.midiVolume = volume;
        break;

      case tdFm:
        prefs.fmVolume = volume;
        break;

      default:
        break;
    }
  }

  return 1;
}
