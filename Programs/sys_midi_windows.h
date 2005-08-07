/*
 * BRLTTY - A background process providing access to the console screen (when in
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

#ifdef HAVE_LIBWINMM
struct MidiDeviceStruct {
  HMIDIOUT handle;
};

static void
LogMidiOutError(MMRESULT error, int errorLevel, const char *action) {
  char msg[MAXERRORLENGTH];
  midiOutGetErrorText(error, msg, sizeof(msg));
  LogPrint(errorLevel, "%s error %d: %s.", action, error, msg);
}

MidiDevice *
openMidiDevice (int errorLevel, const char *device) {
  MidiDevice *midi;
  MMRESULT mmres;
  long id = 0;

  if (device && *device) {
    char *end;
    id = strtol(device, &end, 0);
    if (*end || id < 0 || id >= midiOutGetNumDevs()) {
      LogPrint(errorLevel, "Invalid MIDI device number: %s", device);
      return NULL;
    }
  }
  if (!(midi = malloc(sizeof(*midi)))) {
    LogError("MIDI device allocation");
    return NULL;
  }

  if ((mmres = midiOutOpen(&midi->handle, id, 0, 0, CALLBACK_NULL))) {
    LogMidiOutError(mmres, errorLevel, "opening MIDI device");
    goto out;
  }
  return midi;

out:
  free(midi);
  return NULL;
}

void
closeMidiDevice (MidiDevice *midi) {
  midiOutClose(midi->handle);
  free(midi);
}

int
flushMidiDevice (MidiDevice *midi) {
  return 1;
}

#define MIDI_PGM_CHANGE	0xC0
#define MIDI_NOTEOFF	0x80
#define MIDI_NOTEON	0x90

int
setMidiInstrument (MidiDevice *midi, unsigned char channel, unsigned char instrument) {
  MMRESULT mmres;
  if ((mmres = midiOutShortMsg(midi->handle, MAKEWORD(MIDI_PGM_CHANGE+channel,instrument))) == MMSYSERR_NOERROR)
    return 1;
  LogMidiOutError(mmres, LOG_ERR, "setting MIDI instrument");
  return 0;
}

int
beginMidiBlock (MidiDevice *midi) {
  return 1;
}

int
endMidiBlock (MidiDevice *midi) {
  return 1;
}

int
startMidiNote (MidiDevice *midi, unsigned char channel, unsigned char note, unsigned char volume) {
  MMRESULT mmres;
  if ((mmres = midiOutShortMsg(midi->handle, MAKELONG(MAKEWORD(MIDI_NOTEON+channel,note),0x7F*volume/100))) == MMSYSERR_NOERROR)
    return 1;
  LogMidiOutError(mmres, LOG_ERR, "starting MIDI note");
  return 0;
}

int
stopMidiNote (MidiDevice *midi, unsigned char channel) {
  MMRESULT mmres;
  if ((mmres = midiOutShortMsg(midi->handle, MIDI_NOTEOFF+channel)) == MMSYSERR_NOERROR)
    return 1;
  LogMidiOutError(mmres, LOG_ERR, "stopping MIDI note");
  return 1;
}

int
insertMidiWait (MidiDevice *midi, int duration) {
  Sleep(duration);
  return 1;
}
#else /* HAVE_LIBWINMM */
#include "sys_midi_none.h"
#endif /* HAVE_LIBWINMM */
