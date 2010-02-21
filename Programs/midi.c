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

#include <string.h>
#include <errno.h>

#include "brltty.h"
#include "log.h"
#include "system.h"
#include "notes.h"

struct NoteDeviceStruct {
  MidiDevice *midi;
  int channelNumber;
};

const char *const midiInstrumentTable[] = {
/* Piano */
  /* 00 */ "Acoustic Grand Piano",
  /* 01 */ "Bright Acoustic Piano",
  /* 02 */ "Electric Grand Piano",
  /* 03 */ "Honkytonk Piano",
  /* 04 */ "Electric Piano 1",
  /* 05 */ "Electric Piano 2",
  /* 06 */ "Harpsichord",
  /* 07 */ "Clavi",
/* Chromatic Percussion */
  /* 08 */ "Celesta",
  /* 09 */ "Glockenspiel",
  /* 0A */ "Music Box",
  /* 0B */ "Vibraphone",
  /* 0C */ "Marimba",
  /* 0D */ "Xylophone",
  /* 0E */ "Tubular Bells",
  /* 0F */ "Dulcimer",
/* Organ */
  /* 10 */ "Drawbar Organ",
  /* 11 */ "Percussive Organ",
  /* 12 */ "Rock Organ",
  /* 13 */ "Church Organ",
  /* 14 */ "Reed Organ",
  /* 15 */ "Accordion",
  /* 16 */ "Harmonica",
  /* 17 */ "Tango Accordion",
/* Guitar */
  /* 18 */ "Acoustic Guitar (nylon)",
  /* 19 */ "Acoustic Guitar (steel)",
  /* 1A */ "Electric Guitar (jazz)",
  /* 1B */ "Electric Guitar (clean)",
  /* 1C */ "Electric Guitar (muted)",
  /* 1D */ "Overdriven Guitar",
  /* 1E */ "Distortion Guitar",
  /* 1F */ "Guitar Harmonics",
/* Bass */
  /* 20 */ "Acoustic Bass",
  /* 21 */ "Electric Bass (finger)",
  /* 22 */ "Electric Bass (pick)",
  /* 23 */ "Fretless Bass",
  /* 24 */ "Slap Bass 1",
  /* 25 */ "Slap Bass 2",
  /* 26 */ "Synth Bass 1",
  /* 27 */ "Synth Bass 2",
/* Strings */
  /* 28 */ "Violin",
  /* 29 */ "Viola",
  /* 2A */ "Cello",
  /* 2B */ "Contrabass",
  /* 2C */ "Tremolo Strings",
  /* 2D */ "Pizzicato Strings",
  /* 2E */ "Orchestral Harp",
  /* 2F */ "Timpani",
/* Ensemble */
  /* 30 */ "String Ensemble 1",
  /* 31 */ "String Ensemble 2",
  /* 32 */ "SynthStrings 1",
  /* 33 */ "SynthStrings 2",
  /* 34 */ "Choir Aahs",
  /* 35 */ "Voice Oohs",
  /* 36 */ "Synth Voice",
  /* 37 */ "Orchestra Hit",
/* Brass */
  /* 38 */ "Trumpet",
  /* 39 */ "Trombone",
  /* 3A */ "Tuba",
  /* 3B */ "Muted Trumpet",
  /* 3C */ "French Horn",
  /* 3D */ "Brass Section",
  /* 3E */ "SynthBrass 1",
  /* 3F */ "SynthBrass 2",
/* Reed */
  /* 40 */ "Soprano Sax",
  /* 41 */ "Alto Sax",
  /* 42 */ "Tenor Sax",
  /* 43 */ "Baritone Sax",
  /* 44 */ "Oboe",
  /* 45 */ "English Horn",
  /* 46 */ "Bassoon",
  /* 47 */ "Clarinet",
/* Pipe */
  /* 48 */ "Piccolo",
  /* 49 */ "Flute",
  /* 4A */ "Recorder",
  /* 4B */ "Pan Flute",
  /* 4C */ "Blown Bottle",
  /* 4D */ "Shakuhachi",
  /* 4E */ "Whistle",
  /* 4F */ "Ocarina",
/* Synth Lead */
  /* 50 */ "Lead 1 (square)",
  /* 51 */ "Lead 2 (sawtooth)",
  /* 52 */ "Lead 3 (calliope)",
  /* 53 */ "Lead 4 (chiff)",
  /* 54 */ "Lead 5 (charang)",
  /* 55 */ "Lead 6 (voice)",
  /* 56 */ "Lead 7 (fifths)",
  /* 57 */ "Lead 8 (bass + lead)",
/* Synth Pad */
  /* 58 */ "Pad 1 (new age)",
  /* 59 */ "Pad 2 (warm)",
  /* 5A */ "Pad 3 (polysynth)",
  /* 5B */ "Pad 4 (choir)",
  /* 5C */ "Pad 5 (bowed)",
  /* 5D */ "Pad 6 (metallic)",
  /* 5E */ "Pad 7 (halo)",
  /* 5F */ "Pad 8 (sweep)",
/* Synth FM */
  /* 60 */ "FX 1 (rain)",
  /* 61 */ "FX 2 (soundtrack)",
  /* 62 */ "FX 3 (crystal)",
  /* 63 */ "FX 4 (atmosphere)",
  /* 64 */ "FX 5 (brightness)",
  /* 65 */ "FX 6 (goblins)",
  /* 66 */ "FX 7 (echoes)",
  /* 67 */ "FX 8 (sci-fi)",
/* Ethnic Instruments */
  /* 68 */ "Sitar",
  /* 69 */ "Banjo",
  /* 6A */ "Shamisen",
  /* 6B */ "Koto",
  /* 6C */ "Kalimba",
  /* 6D */ "Bag Pipe",
  /* 6E */ "Fiddle",
  /* 6F */ "Shanai",
/* Percussive Instruments */
  /* 70 */ "Tinkle Bell",
  /* 71 */ "Agogo",
  /* 72 */ "Steel Drums",
  /* 73 */ "Woodblock",
  /* 74 */ "Taiko Drum",
  /* 75 */ "Melodic Tom",
  /* 76 */ "Synth Drum",
  /* 77 */ "Reverse Cymbal",
/* Sound Effects */
  /* 78 */ "Guitar Fret Noise",
  /* 79 */ "Breath Noise",
  /* 7A */ "Seashore",
  /* 7B */ "Bird Tweet",
  /* 7C */ "Telephone Ring",
  /* 7D */ "Helicopter",
  /* 7E */ "Applause",
  /* 7F */ "Gunshot"
};
const unsigned int midiInstrumentCount = ARRAY_COUNT(midiInstrumentTable);

static NoteDevice *
midiConstruct (int errorLevel) {
  NoteDevice *device;

  if ((device = malloc(sizeof(*device)))) {
    if ((device->midi = openMidiDevice(errorLevel, opt_midiDevice))) {
      device->channelNumber = 0;
      setMidiInstrument(device->midi, device->channelNumber, prefs.midiInstrument);

      LogPrint(LOG_DEBUG, "MIDI enabled");
      return device;
    }

    free(device);
  } else {
    LogError("malloc");
  }

  LogPrint(LOG_DEBUG, "MIDI not available");
  return NULL;
}

static int
midiPlay (NoteDevice *device, int note, int duration) {
  beginMidiBlock(device->midi);

  if (note) {
    LogPrint(LOG_DEBUG, "tone: msec=%d note=%d", duration, note);
    startMidiNote(device->midi, device->channelNumber, note, prefs.midiVolume);
    insertMidiWait(device->midi, duration);
    stopMidiNote(device->midi, device->channelNumber);
  } else {
    LogPrint(LOG_DEBUG, "tone: msec=%d", duration);
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
  LogPrint(LOG_DEBUG, "MIDI disabled");
}

const NoteMethods midiMethods = {
  midiConstruct,
  midiPlay,
  midiFlush,
  midiDestruct
};
