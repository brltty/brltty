/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#include "prologue.h"

#include "brltty.h"
#include "log.h"
#include "timing.h"
#include "notes.h"
#include "adlib.h"

struct NoteDeviceStruct {
  unsigned int channelNumber;
};

static NoteDevice *
fmConstruct (int errorLevel) {
  NoteDevice *device;

  if ((device = malloc(sizeof(*device)))) {
    if (AL_enablePorts(errorLevel)) {
      if (AL_testCard(errorLevel)) {
        device->channelNumber = 0;

        LogPrint(LOG_DEBUG, "FM enabled");
        return device;
      }

      AL_disablePorts();
    }

    free(device);
  } else {
    LogError("malloc");
  }

  LogPrint(LOG_DEBUG, "FM not available");
  return NULL;
}

static int
fmPlay (NoteDevice *device, int note, int duration) {
  LogPrint(LOG_DEBUG, "tone: msec=%d note=%d",
           duration, note);

  if (note) {
    AL_playTone(device->channelNumber, (int)noteFrequencies[note], duration, prefs.fmVolume);
  } else {
    accurateDelay(duration);
  }

  return 1;
}

static int
fmFlush (NoteDevice *device) {
  return 1;
}

static void
fmDestruct (NoteDevice *device) {
  free(device);
  AL_disablePorts();
  LogPrint(LOG_DEBUG, "FM disabled");
}

const NoteMethods fmMethods = {
  fmConstruct,
  fmPlay,
  fmFlush,
  fmDestruct
};
