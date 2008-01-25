/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#include "prologue.h"

#include "misc.h"
#include "system.h"
#include "notes.h"

struct NoteDeviceStruct {
  char mustHaveAtLeastOneField;
};

static NoteDevice *
beeperConstruct (int errorLevel) {
  NoteDevice *device;

  if ((device = malloc(sizeof(*device)))) {
    if (canBeep()) {
      LogPrint(LOG_DEBUG, "beeper enabled");
      return device;
    }

    free(device);
  } else {
    LogError("malloc");
  }

  LogPrint(LOG_DEBUG, "beeper not available");
  return NULL;
}

static int
beeperPlay (NoteDevice *device, int note, int duration) {
  LogPrint(LOG_DEBUG, "tone: msec=%d note=%d",
           duration, note);

  if (!note) {
    accurateDelay(duration);
    return 1;
  }

  if (asynchronousBeep((int)noteFrequencies[note], duration*4)) {
    accurateDelay(duration);
    stopBeep();
    return 1;
  }

  if (startBeep((int)noteFrequencies[note])) {
    accurateDelay(duration);
    stopBeep();
    return 1;
  }

  return synchronousBeep((int)noteFrequencies[note], duration);
}

static int
beeperFlush (NoteDevice *device) {
  return 1;
}

static void
beeperDestruct (NoteDevice *device) {
  endBeep();
  free(device);
  LogPrint(LOG_DEBUG, "beeper disabled");
}

const NoteMethods beeperMethods = {
  beeperConstruct,
  beeperPlay,
  beeperFlush,
  beeperDestruct
};
