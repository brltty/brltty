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

#include "prologue.h"

#include <string.h>
#include <errno.h>

#include "brltty.h"
#include "prefs.h"
#include "log.h"
#include "midi.h"
#include "notes.h"

struct NoteDeviceStruct {
  MidiDevice *midi;
  int channelNumber;
};

static NoteDevice *
midiConstruct (int errorLevel) {
  NoteDevice *device;

  if ((device = malloc(sizeof(*device)))) {
    if ((device->midi = openMidiDevice(errorLevel, opt_midiDevice))) {
      device->channelNumber = 0;
      setMidiInstrument(device->midi, device->channelNumber, prefs.midiInstrument);

      logMessage(LOG_DEBUG, "MIDI enabled");
      return device;
    }

    free(device);
  } else {
    logMallocError();
  }

  logMessage(LOG_DEBUG, "MIDI not available");
  return NULL;
}

static int
midiPlay (NoteDevice *device, unsigned char note, unsigned int duration) {
  beginMidiBlock(device->midi);

  if (note) {
    logMessage(LOG_DEBUG, "tone: msec=%d note=%d", duration, note);
    startMidiNote(device->midi, device->channelNumber, note, prefs.midiVolume);
    insertMidiWait(device->midi, duration);
    stopMidiNote(device->midi, device->channelNumber);
  } else {
    logMessage(LOG_DEBUG, "tone: msec=%d", duration);
    insertMidiWait(device->midi, duration);
  }

  endMidiBlock(device->midi);
  return 1;
}

static int
midiFlush (NoteDevice *device) {
  return flushMidiDevice(device->midi);
}

static void
midiDestruct (NoteDevice *device) {
  closeMidiDevice(device->midi);
  free(device);
  logMessage(LOG_DEBUG, "MIDI disabled");
}

const NoteMethods midiMethods = {
  midiConstruct,
  midiPlay,
  midiFlush,
  midiDestruct
};
