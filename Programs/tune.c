/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

#include <string.h>

#include "log.h"
#include "parameters.h"
#include "async_alarm.h"
#include "program.h"
#include "tune.h"
#include "notes.h"

static const NoteMethods *noteMethods = NULL;
static NoteDevice *noteDevice = NULL;
static AsyncHandle tuneDeviceCloseTimer = NULL;
static int openErrorLevel = LOG_ERR;

void
suppressTuneDeviceOpenErrors (void) {
  openErrorLevel = LOG_DEBUG;
}

void
closeTuneDevice (void) {
  if (tuneDeviceCloseTimer) {
    asyncCancelRequest(tuneDeviceCloseTimer);
    tuneDeviceCloseTimer = NULL;
  }

  if (noteDevice) {
    noteMethods->destruct(noteDevice);
    noteDevice = NULL;
  }
}

ASYNC_ALARM_CALLBACK(handleTuneDeviceCloseTimeout) {
  if (tuneDeviceCloseTimer) {
    asyncDiscardHandle(tuneDeviceCloseTimer);
    tuneDeviceCloseTimer = NULL;
  }

  closeTuneDevice();
}

static int tuneInitialized = 0;

static void
exitTunes (void *data) {
  closeTuneDevice();
  tuneInitialized = 0;
}
 
static int
openTuneDevice (void) {
  const int timeout = TUNE_DEVICE_CLOSE_DELAY;

  if (noteDevice) {
    asyncResetAlarmIn(tuneDeviceCloseTimer, timeout);
  } else if ((noteDevice = noteMethods->construct(openErrorLevel)) != NULL) {
    if (asyncSetAlarmIn(&tuneDeviceCloseTimer, timeout, handleTuneDeviceCloseTimeout, NULL)) {
      if (!tuneInitialized) {
        tuneInitialized = 1;
        onProgramExit("tunes", exitTunes, NULL);
      }
    }
  } else {
    return 0;
  }

  return 1;
}

int
setTuneDevice (TuneDevice device) {
  const NoteMethods *methods;

  switch (device) {
    default:
      methods = NULL;
      break;

#ifdef HAVE_BEEP_SUPPORT
    case tdBeeper:
      methods = &beepNoteMethods;
      break;
#endif /* HAVE_BEEP_SUPPORT */

#ifdef HAVE_PCM_SUPPORT
    case tdPcm:
      methods = &pcmNoteMethods;
      break;
#endif /* HAVE_PCM_SUPPORT */

#ifdef HAVE_MIDI_SUPPORT
    case tdMidi:
      methods = &midiNoteMethods;
      break;
#endif /* HAVE_MIDI_SUPPORT */

#ifdef HAVE_FM_SUPPORT
    case tdFm:
      methods = &fmNoteMethods;
      break;
#endif /* HAVE_FM_SUPPORT */
  }
  if (!methods) return 0;

  if (methods != noteMethods) {
    closeTuneDevice();
    noteMethods = methods;
  }

  return 1;
}

int
playTune (const TuneElement *tune) {
  while (tune->duration) {
    if (!openTuneDevice()) return 0;
    if (!noteMethods->play(noteDevice, tune->note, tune->duration)) return 0;
    tune += 1;
  }

  return noteMethods->flush(noteDevice);
}
