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
#include "prefs.h"
#include "async_wait.h"
#include "notes.h"
#include "fm.h"

struct NoteDeviceStruct {
  unsigned int channelNumber;
};

static NoteDevice *
fmConstruct (int errorLevel) {
  NoteDevice *device;

  if ((device = malloc(sizeof(*device)))) {
    if (fmEnablePorts(errorLevel)) {
      if (fmTestCard(errorLevel)) {
        device->channelNumber = 0;

        logMessage(LOG_DEBUG, "FM enabled");
        return device;
      }

      fmDisablePorts();
    }

    free(device);
  } else {
    logMallocError();
  }

  logMessage(LOG_DEBUG, "FM not available");
  return NULL;
}

static void
fmDestruct (NoteDevice *device) {
  free(device);
  fmDisablePorts();
  logMessage(LOG_DEBUG, "FM disabled");
}

static int
fmFrequency (NoteDevice *device, unsigned int duration, NOTE_FREQUENCY_TYPE frequency) {
  uint32_t pitch = frequency;
  logMessage(LOG_DEBUG, "tone: MSecs:%u Freq:%"PRIu32,
             duration, pitch);

  if (pitch) {
    fmPlayTone(device->channelNumber, pitch, duration, prefs.fmVolume);
  } else {
    asyncWait(duration);
  }

  return 1;
}

static int
fmNote (NoteDevice *device, unsigned int duration, unsigned char note) {
  return fmFrequency(device, duration, GET_NOTE_FREQUENCY(note));
}

static int
fmFlush (NoteDevice *device) {
  return 1;
}

const NoteMethods fmNoteMethods = {
  .construct = fmConstruct,
  .destruct = fmDestruct,

  .frequency = fmFrequency,
  .note = fmNote,
  .flush = fmFlush
};
