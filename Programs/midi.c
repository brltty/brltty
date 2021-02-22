/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include "midi.h"

static const char *const midiInstrumentTypes[] = {
  // xgettext: This is a MIDI sound category.
  [0X0] = strtext("Piano"),

  // xgettext: This is a MIDI sound category.
  [0X1] = strtext("Chromatic Percussion"),

  // xgettext: This is a MIDI sound category.
  [0X2] = strtext("Organ"),

  // xgettext: This is a MIDI sound category.
  [0X3] = strtext("Guitar"),

  // xgettext: This is a MIDI sound category.
  [0X4] = strtext("Bass"),

  // xgettext: This is a MIDI sound category.
  [0X5] = strtext("Strings"),

  // xgettext: This is a MIDI sound category.
  [0X6] = strtext("Ensemble"),

  // xgettext: This is a MIDI sound category.
  [0X7] = strtext("Brass"),

  // xgettext: This is a MIDI sound category.
  [0X8] = strtext("Reed"),

  // xgettext: This is a MIDI sound category.
  [0X9] = strtext("Pipe"),

  // xgettext: This is a MIDI sound category.
  [0XA] = strtext("Synth Lead"),

  // xgettext: This is a MIDI sound category.
  [0XB] = strtext("Synth Pad"),

  // xgettext: This is a MIDI sound category.
  [0XC] = strtext("Synth FM"),

  // xgettext: This is a MIDI sound category.
  [0XD] = strtext("Ethnic Instruments"),

  // xgettext: This is a MIDI sound category.
  [0XE] = strtext("Percussive Instruments"),

  // xgettext: This is a MIDI sound category.
  [0XF] = strtext("Sound Effects")
};

const char *const midiInstrumentTable[] = {
/* Piano */
  // xgettext: This is a MIDI musical instrument name (piano).
  [0X00] = strtext("Acoustic Grand Piano"),

  // xgettext: This is a MIDI musical instrument name (piano).
  [0X01] = strtext("Bright Acoustic Piano"),

  // xgettext: This is a MIDI musical instrument name (piano).
  [0X02] = strtext("Electric Grand Piano"),

  // xgettext: This is a MIDI musical instrument name (piano).
  [0X03] = strtext("Honkytonk Piano"),

  // xgettext: This is a MIDI musical instrument name (piano).
  [0X04] = strtext("Electric Piano 1"),

  // xgettext: This is a MIDI musical instrument name (piano).
  [0X05] = strtext("Electric Piano 2"),

  // xgettext: This is a MIDI musical instrument name (piano).
  [0X06] = strtext("Harpsichord"),

  // xgettext: This is a MIDI musical instrument name (piano).
  [0X07] = strtext("Clavi"),

/* Chromatic Percussion */
  // xgettext: This is a MIDI musical instrument name (chromatic percussion).
  [0X08] = strtext("Celesta"),

  // xgettext: This is a MIDI musical instrument name (chromatic percussion).
  [0X09] = strtext("Glockenspiel"),

  // xgettext: This is a MIDI musical instrument name (chromatic percussion).
  [0X0A] = strtext("Music Box"),

  // xgettext: This is a MIDI musical instrument name (chromatic percussion).
  [0X0B] = strtext("Vibraphone"),

  // xgettext: This is a MIDI musical instrument name (chromatic percussion).
  [0X0C] = strtext("Marimba"),

  // xgettext: This is a MIDI musical instrument name (chromatic percussion).
  [0X0D] = strtext("Xylophone"),

  // xgettext: This is a MIDI musical instrument name (chromatic percussion).
  [0X0E] = strtext("Tubular Bells"),

  // xgettext: This is a MIDI musical instrument name (chromatic percussion).
  [0X0F] = strtext("Dulcimer"),

/* Organ */
  // xgettext: This is a MIDI musical instrument name (organ).
  [0X10] = strtext("Drawbar Organ"),

  // xgettext: This is a MIDI musical instrument name (organ).
  [0X11] = strtext("Percussive Organ"),

  // xgettext: This is a MIDI musical instrument name (organ).
  [0X12] = strtext("Rock Organ"),

  // xgettext: This is a MIDI musical instrument name (organ).
  [0X13] = strtext("Church Organ"),

  // xgettext: This is a MIDI musical instrument name (organ).
  [0X14] = strtext("Reed Organ"),

  // xgettext: This is a MIDI musical instrument name (organ).
  [0X15] = strtext("Accordion"),

  // xgettext: This is a MIDI musical instrument name (organ).
  [0X16] = strtext("Harmonica"),

  // xgettext: This is a MIDI musical instrument name (organ).
  [0X17] = strtext("Tango Accordion"),

/* Guitar */
  // xgettext: This is a MIDI musical instrument name (guitar).
  [0X18] = strtext("Acoustic Guitar (nylon)"),

  // xgettext: This is a MIDI musical instrument name (guitar).
  [0X19] = strtext("Acoustic Guitar (steel)"),

  // xgettext: This is a MIDI musical instrument name (guitar).
  [0X1A] = strtext("Electric Guitar (jazz)"),

  // xgettext: This is a MIDI musical instrument name (guitar).
  [0X1B] = strtext("Electric Guitar (clean)"),

  // xgettext: This is a MIDI musical instrument name (guitar).
  [0X1C] = strtext("Electric Guitar (muted)"),

  // xgettext: This is a MIDI musical instrument name (guitar).
  [0X1D] = strtext("Overdriven Guitar"),

  // xgettext: This is a MIDI musical instrument name (guitar).
  [0X1E] = strtext("Distortion Guitar"),

  // xgettext: This is a MIDI musical instrument name (guitar).
  [0X1F] = strtext("Guitar Harmonics"),

/* Bass */
  // xgettext: This is a MIDI musical instrument name (bass).
  [0X20] = strtext("Acoustic Bass"),

  // xgettext: This is a MIDI musical instrument name (bass).
  [0X21] = strtext("Electric Bass (finger)"),

  // xgettext: This is a MIDI musical instrument name (bass).
  [0X22] = strtext("Electric Bass (pick)"),

  // xgettext: This is a MIDI musical instrument name (bass).
  [0X23] = strtext("Fretless Bass"),

  // xgettext: This is a MIDI musical instrument name (bass).
  [0X24] = strtext("Slap Bass 1"),

  // xgettext: This is a MIDI musical instrument name (bass).
  [0X25] = strtext("Slap Bass 2"),

  // xgettext: This is a MIDI musical instrument name (bass).
  [0X26] = strtext("Synth Bass 1"),

  // xgettext: This is a MIDI musical instrument name (bass).
  [0X27] = strtext("Synth Bass 2"),

/* Strings */
  // xgettext: This is a MIDI musical instrument name (strings).
  [0X28] = strtext("Violin"),

  // xgettext: This is a MIDI musical instrument name (strings).
  [0X29] = strtext("Viola"),

  // xgettext: This is a MIDI musical instrument name (strings).
  [0X2A] = strtext("Cello"),

  // xgettext: This is a MIDI musical instrument name (strings).
  [0X2B] = strtext("Contrabass"),

  // xgettext: This is a MIDI musical instrument name (strings).
  [0X2C] = strtext("Tremolo Strings"),

  // xgettext: This is a MIDI musical instrument name (strings).
  [0X2D] = strtext("Pizzicato Strings"),

  // xgettext: This is a MIDI musical instrument name (strings).
  [0X2E] = strtext("Orchestral Harp"),

  // xgettext: This is a MIDI musical instrument name (strings).
  [0X2F] = strtext("Timpani"),

/* Ensemble */
  // xgettext: This is a MIDI musical instrument name (ensemble).
  [0X30] = strtext("String Ensemble 1"),

  // xgettext: This is a MIDI musical instrument name (ensemble).
  [0X31] = strtext("String Ensemble 2"),

  // xgettext: This is a MIDI musical instrument name (ensemble).
  [0X32] = strtext("SynthStrings 1"),

  // xgettext: This is a MIDI musical instrument name (ensemble).
  [0X33] = strtext("SynthStrings 2"),

  // xgettext: This is a MIDI musical instrument name (ensemble).
  [0X34] = strtext("Choir Aahs"),

  // xgettext: This is a MIDI musical instrument name (ensemble).
  [0X35] = strtext("Voice Oohs"),

  // xgettext: This is a MIDI musical instrument name (ensemble).
  [0X36] = strtext("Synth Voice"),

  // xgettext: This is a MIDI musical instrument name (ensemble).
  [0X37] = strtext("Orchestra Hit"),

/* Brass */
  // xgettext: This is a MIDI musical instrument name (brass).
  [0X38] = strtext("Trumpet"),

  // xgettext: This is a MIDI musical instrument name (brass).
  [0X39] = strtext("Trombone"),

  // xgettext: This is a MIDI musical instrument name (brass).
  [0X3A] = strtext("Tuba"),

  // xgettext: This is a MIDI musical instrument name (brass).
  [0X3B] = strtext("Muted Trumpet"),

  // xgettext: This is a MIDI musical instrument name (brass).
  [0X3C] = strtext("French Horn"),

  // xgettext: This is a MIDI musical instrument name (brass).
  [0X3D] = strtext("Brass Section"),

  // xgettext: This is a MIDI musical instrument name (brass).
  [0X3E] = strtext("SynthBrass 1"),

  // xgettext: This is a MIDI musical instrument name (brass).
  [0X3F] = strtext("SynthBrass 2"),

/* Reed */
  // xgettext: This is a MIDI musical instrument name (reed).
  [0X40] = strtext("Soprano Sax"),

  // xgettext: This is a MIDI musical instrument name (reed).
  [0X41] = strtext("Alto Sax"),

  // xgettext: This is a MIDI musical instrument name (reed).
  [0X42] = strtext("Tenor Sax"),

  // xgettext: This is a MIDI musical instrument name (reed).
  [0X43] = strtext("Baritone Sax"),

  // xgettext: This is a MIDI musical instrument name (reed).
  [0X44] = strtext("Oboe"),

  // xgettext: This is a MIDI musical instrument name (reed).
  [0X45] = strtext("English Horn"),

  // xgettext: This is a MIDI musical instrument name (reed).
  [0X46] = strtext("Bassoon"),

  // xgettext: This is a MIDI musical instrument name (reed).
  [0X47] = strtext("Clarinet"),

/* Pipe */
  // xgettext: This is a MIDI musical instrument name (pipe).
  [0X48] = strtext("Piccolo"),

  // xgettext: This is a MIDI musical instrument name (pipe).
  [0X49] = strtext("Flute"),

  // xgettext: This is a MIDI musical instrument name (pipe).
  [0X4A] = strtext("Recorder"),

  // xgettext: This is a MIDI musical instrument name (pipe).
  [0X4B] = strtext("Pan Flute"),

  // xgettext: This is a MIDI musical instrument name (pipe).
  [0X4C] = strtext("Blown Bottle"),

  // xgettext: This is a MIDI musical instrument name (pipe).
  [0X4D] = strtext("Shakuhachi"),

  // xgettext: This is a MIDI musical instrument name (pipe).
  [0X4E] = strtext("Whistle"),

  // xgettext: This is a MIDI musical instrument name (pipe).
  [0X4F] = strtext("Ocarina"),

/* Synth Lead */
  // xgettext: This is a MIDI musical instrument name (synth lead).
  [0X50] = strtext("Lead 1 (square)"),

  // xgettext: This is a MIDI musical instrument name (synth lead).
  [0X51] = strtext("Lead 2 (sawtooth)"),

  // xgettext: This is a MIDI musical instrument name (synth lead).
  [0X52] = strtext("Lead 3 (calliope)"),

  // xgettext: This is a MIDI musical instrument name (synth lead).
  [0X53] = strtext("Lead 4 (chiff)"),

  // xgettext: This is a MIDI musical instrument name (synth lead).
  [0X54] = strtext("Lead 5 (charang)"),

  // xgettext: This is a MIDI musical instrument name (synth lead).
  [0X55] = strtext("Lead 6 (voice)"),

  // xgettext: This is a MIDI musical instrument name (synth lead).
  [0X56] = strtext("Lead 7 (fifths)"),

  // xgettext: This is a MIDI musical instrument name (synth lead).
  [0X57] = strtext("Lead 8 (bass + lead)"),

/* Synth Pad */
  // xgettext: This is a MIDI musical instrument name (synth pad).
  [0X58] = strtext("Pad 1 (new age)"),

  // xgettext: This is a MIDI musical instrument name (synth pad).
  [0X59] = strtext("Pad 2 (warm)"),

  // xgettext: This is a MIDI musical instrument name (synth pad).
  [0X5A] = strtext("Pad 3 (polysynth)"),

  // xgettext: This is a MIDI musical instrument name (synth pad).
  [0X5B] = strtext("Pad 4 (choir)"),

  // xgettext: This is a MIDI musical instrument name (synth pad).
  [0X5C] = strtext("Pad 5 (bowed)"),

  // xgettext: This is a MIDI musical instrument name (synth pad).
  [0X5D] = strtext("Pad 6 (metallic)"),

  // xgettext: This is a MIDI musical instrument name (synth pad).
  [0X5E] = strtext("Pad 7 (halo)"),

  // xgettext: This is a MIDI musical instrument name (synth pad).
  [0X5F] = strtext("Pad 8 (sweep)"),

/* Synth FM */
  // xgettext: This is a MIDI musical instrument name (synth FM).
  [0X60] = strtext("FX 1 (rain)"),

  // xgettext: This is a MIDI musical instrument name (synth FM).
  [0X61] = strtext("FX 2 (soundtrack)"),

  // xgettext: This is a MIDI musical instrument name (synth FM).
  [0X62] = strtext("FX 3 (crystal)"),

  // xgettext: This is a MIDI musical instrument name (synth FM).
  [0X63] = strtext("FX 4 (atmosphere)"),

  // xgettext: This is a MIDI musical instrument name (synth FM).
  [0X64] = strtext("FX 5 (brightness)"),

  // xgettext: This is a MIDI musical instrument name (synth FM).
  [0X65] = strtext("FX 6 (goblins)"),

  // xgettext: This is a MIDI musical instrument name (synth FM).
  [0X66] = strtext("FX 7 (echoes)"),

  // xgettext: This is a MIDI musical instrument name (synth FM).
  [0X67] = strtext("FX 8 (sci-fi)"),

/* Ethnic Instruments */
  // xgettext: This is a MIDI musical instrument name (ethnic).
  [0X68] = strtext("Sitar"),

  // xgettext: This is a MIDI musical instrument name (ethnic).
  [0X69] = strtext("Banjo"),

  // xgettext: This is a MIDI musical instrument name (ethnic).
  [0X6A] = strtext("Shamisen"),

  // xgettext: This is a MIDI musical instrument name (ethnic).
  [0X6B] = strtext("Koto"),

  // xgettext: This is a MIDI musical instrument name (ethnic).
  [0X6C] = strtext("Kalimba"),

  // xgettext: This is a MIDI musical instrument name (ethnic).
  [0X6D] = strtext("Bag Pipe"),

  // xgettext: This is a MIDI musical instrument name (ethnic).
  [0X6E] = strtext("Fiddle"),

  // xgettext: This is a MIDI musical instrument name (ethnic).
  [0X6F] = strtext("Shanai"),

/* Percussive Instruments */
  // xgettext: This is a MIDI musical instrument name (percussive).
  [0X70] = strtext("Tinkle Bell"),

  // xgettext: This is a MIDI musical instrument name (percussive).
  [0X71] = strtext("Agogo"),

  // xgettext: This is a MIDI musical instrument name (percussive).
  [0X72] = strtext("Steel Drums"),

  // xgettext: This is a MIDI musical instrument name (percussive).
  [0X73] = strtext("Woodblock"),

  // xgettext: This is a MIDI musical instrument name (percussive).
  [0X74] = strtext("Taiko Drum"),

  // xgettext: This is a MIDI musical instrument name (percussive).
  [0X75] = strtext("Melodic Tom"),

  // xgettext: This is a MIDI musical instrument name (percussive).
  [0X76] = strtext("Synth Drum"),

  // xgettext: This is a MIDI musical instrument name (percussive).
  [0X77] = strtext("Reverse Cymbal"),

/* Sound Effects */
  // xgettext: This is a MIDI sound effect name.
  [0X78] = strtext("Guitar Fret Noise"),

  // xgettext: This is a MIDI sound effect name.
  [0X79] = strtext("Breath Noise"),

  // xgettext: This is a MIDI sound effect name.
  [0X7A] = strtext("Seashore"),

  // xgettext: This is a MIDI sound effect name.
  [0X7B] = strtext("Bird Tweet"),

  // xgettext: This is a MIDI sound effect name.
  [0X7C] = strtext("Telephone Ring"),

  // xgettext: This is a MIDI sound effect name.
  [0X7D] = strtext("Helicopter"),

  // xgettext: This is a MIDI sound effect name.
  [0X7E] = strtext("Applause"),

  // xgettext: This is a MIDI sound effect name.
  [0X7F] = strtext("Gunshot")
};
const unsigned int midiInstrumentCount = ARRAY_COUNT(midiInstrumentTable);

const char *
midiGetInstrumentType (unsigned char instrument) {
  return midiInstrumentTypes[instrument >> 3];
}
