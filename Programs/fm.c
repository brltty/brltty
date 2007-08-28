/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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

#include "brltty.h"
#include "misc.h"
#include "notes.h"
#include "adlib.h"

static int fmEnabled = 0;
static const unsigned int channelNumber = 0;

static int
fmConstruct (int errorLevel) {
  if (!fmEnabled) {
    if (AL_enablePorts(errorLevel)) {
      if (AL_testCard(errorLevel)) {
        fmEnabled = 1;
        LogPrint(LOG_DEBUG, "FM enabled");
      } else {
        AL_disablePorts();
      }
    }

    if (!fmEnabled) {
      LogPrint(LOG_DEBUG, "FM not available");
      return 0;
    }
  }

  return 1;
}

static int
fmPlay (int note, int duration) {
  LogPrint(LOG_DEBUG, "tone: msec=%d note=%d",
           duration, note);

  if (note) {
    AL_playTone(channelNumber, (int)noteFrequencies[note], duration, prefs.fmVolume);
  } else {
    accurateDelay(duration);
  }

  return 1;
}

static int
fmFlush (void) {
  return fmEnabled;
}

static void
fmDestruct (void) {
  if (fmEnabled) {
    AL_disablePorts();
    fmEnabled = 0;
    LogPrint(LOG_DEBUG, "FM disabled");
  }
}

const NoteGenerator fmNoteGenerator = {
  fmConstruct,
  fmPlay,
  fmFlush,
  fmDestruct
};
