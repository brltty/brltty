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
  // xgettext: (synth is a common short form for synthesizer)
  [0XA] = strtext("Synth Lead"),

  // xgettext: This is a MIDI sound category.
  // xgettext: (synth is a common short form for synthesizer)
  [0XB] = strtext("Synth Pad"),

  // xgettext: This is a MIDI sound category.
  // xgettext: (synth is a common short form for synthesizer)
  // xgettext: (FM is the acronym for Frequency Modulation)
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
  // xgettext: This is the name of MIDI musical instrument #1 (piano).
  [0X00] = strtext("Acoustic Grand Piano"),

  // xgettext: This is the name of MIDI musical instrument #2 (piano).
  [0X01] = strtext("Bright Acoustic Piano"),

  // xgettext: This is the name of MIDI musical instrument #3 (piano).
  [0X02] = strtext("Electric Grand Piano"),

  // xgettext: This is the name of MIDI musical instrument #4 (piano).
  [0X03] = strtext("Honkytonk Piano"),

  // xgettext: This is the name of MIDI musical instrument #5 (piano).
  [0X04] = strtext("Electric Piano 1"),

  // xgettext: This is the name of MIDI musical instrument #6 (piano).
  [0X05] = strtext("Electric Piano 2"),

  // xgettext: This is the name of MIDI musical instrument #7 (piano).
  [0X06] = strtext("Harpsichord"),

  // xgettext: This is the name of MIDI musical instrument #8 (piano).
  [0X07] = strtext("Clavinet"),

/* Chromatic Percussion */
  // xgettext: This is the name of MIDI musical instrument #9 (chromatic percussion).
  [0X08] = strtext("Celesta"),

  // xgettext: This is the name of MIDI musical instrument #10 (chromatic percussion).
  [0X09] = strtext("Glockenspiel"),

  // xgettext: This is the name of MIDI musical instrument #11 (chromatic percussion).
  [0X0A] = strtext("Music Box"),

  // xgettext: This is the name of MIDI musical instrument #12 (chromatic percussion).
  [0X0B] = strtext("Vibraphone"),

  // xgettext: This is the name of MIDI musical instrument #13 (chromatic percussion).
  [0X0C] = strtext("Marimba"),

  // xgettext: This is the name of MIDI musical instrument #14 (chromatic percussion).
  [0X0D] = strtext("Xylophone"),

  // xgettext: This is the name of MIDI musical instrument #15 (chromatic percussion).
  [0X0E] = strtext("Tubular Bells"),

  // xgettext: This is the name of MIDI musical instrument #16 (chromatic percussion).
  [0X0F] = strtext("Dulcimer"),

/* Organ */
  // xgettext: This is the name of MIDI musical instrument #17 (organ).
  [0X10] = strtext("Drawbar Organ"),

  // xgettext: This is the name of MIDI musical instrument #18 (organ).
  [0X11] = strtext("Percussive Organ"),

  // xgettext: This is the name of MIDI musical instrument #19 (organ).
  [0X12] = strtext("Rock Organ"),

  // xgettext: This is the name of MIDI musical instrument #20 (organ).
  [0X13] = strtext("Church Organ"),

  // xgettext: This is the name of MIDI musical instrument #21 (organ).
  [0X14] = strtext("Reed Organ"),

  // xgettext: This is the name of MIDI musical instrument #22 (organ).
  [0X15] = strtext("Accordion"),

  // xgettext: This is the name of MIDI musical instrument #23 (organ).
  [0X16] = strtext("Harmonica"),

  // xgettext: This is the name of MIDI musical instrument #24 (organ).
  [0X17] = strtext("Tango Accordion"),

/* Guitar */
  // xgettext: This is the name of MIDI musical instrument #25 (guitar).
  [0X18] = strtext("Acoustic Guitar (nylon)"),

  // xgettext: This is the name of MIDI musical instrument #26 (guitar).
  [0X19] = strtext("Acoustic Guitar (steel)"),

  // xgettext: This is the name of MIDI musical instrument #27 (guitar).
  [0X1A] = strtext("Electric Guitar (jazz)"),

  // xgettext: This is the name of MIDI musical instrument #28 (guitar).
  [0X1B] = strtext("Electric Guitar (clean)"),

  // xgettext: This is the name of MIDI musical instrument #29 (guitar).
  [0X1C] = strtext("Electric Guitar (muted)"),

  // xgettext: This is the name of MIDI musical instrument #30 (guitar).
  [0X1D] = strtext("Overdriven Guitar"),

  // xgettext: This is the name of MIDI musical instrument #31 (guitar).
  [0X1E] = strtext("Distortion Guitar"),

  // xgettext: This is the name of MIDI musical instrument #32 (guitar).
  [0X1F] = strtext("Guitar Harmonics"),

/* Bass */
  // xgettext: This is the name of MIDI musical instrument #33 (bass).
  [0X20] = strtext("Acoustic Bass"),

  // xgettext: This is the name of MIDI musical instrument #34 (bass).
  [0X21] = strtext("Electric Bass (finger)"),

  // xgettext: This is the name of MIDI musical instrument #35 (bass).
  [0X22] = strtext("Electric Bass (pick)"),

  // xgettext: This is the name of MIDI musical instrument #36 (bass).
  [0X23] = strtext("Fretless Bass"),

  // xgettext: This is the name of MIDI musical instrument #37 (bass).
  [0X24] = strtext("Slap Bass 1"),

  // xgettext: This is the name of MIDI musical instrument #38 (bass).
  [0X25] = strtext("Slap Bass 2"),

  // xgettext: This is the name of MIDI musical instrument #39 (bass).
  [0X26] = strtext("Synth Bass 1"),

  // xgettext: This is the name of MIDI musical instrument #40 (bass).
  [0X27] = strtext("Synth Bass 2"),

/* Strings */
  // xgettext: This is the name of MIDI musical instrument #41 (strings).
  [0X28] = strtext("Violin"),

  // xgettext: This is the name of MIDI musical instrument #42 (strings).
  [0X29] = strtext("Viola"),

  // xgettext: This is the name of MIDI musical instrument #43 (strings).
  [0X2A] = strtext("Cello"),

  // xgettext: This is the name of MIDI musical instrument #44 (strings).
  [0X2B] = strtext("Contrabass"),

  // xgettext: This is the name of MIDI musical instrument #45 (strings).
  [0X2C] = strtext("Tremolo Strings"),

  // xgettext: This is the name of MIDI musical instrument #46 (strings).
  [0X2D] = strtext("Pizzicato Strings"),

  // xgettext: This is the name of MIDI musical instrument #47 (strings).
  [0X2E] = strtext("Orchestral Harp"),

  // xgettext: This is the name of MIDI musical instrument #48 (strings).
  [0X2F] = strtext("Timpani"),

/* Ensemble */
  // xgettext: This is the name of MIDI musical instrument #49 (ensemble).
  [0X30] = strtext("String Ensemble 1"),

  // xgettext: This is the name of MIDI musical instrument #50 (ensemble).
  [0X31] = strtext("String Ensemble 2"),

  // xgettext: This is the name of MIDI musical instrument #51 (ensemble).
  [0X32] = strtext("SynthStrings 1"),

  // xgettext: This is the name of MIDI musical instrument #52 (ensemble).
  [0X33] = strtext("SynthStrings 2"),

  // xgettext: This is the name of MIDI musical instrument #53 (ensemble).
  [0X34] = strtext("Choir Aahs"),

  // xgettext: This is the name of MIDI musical instrument #54 (ensemble).
  [0X35] = strtext("Voice Oohs"),

  // xgettext: This is the name of MIDI musical instrument #55 (ensemble).
  [0X36] = strtext("Synth Voice"),

  // xgettext: This is the name of MIDI musical instrument #56 (ensemble).
  [0X37] = strtext("Orchestra Hit"),

/* Brass */
  // xgettext: This is the name of MIDI musical instrument #57 (brass).
  [0X38] = strtext("Trumpet"),

  // xgettext: This is the name of MIDI musical instrument #58 (brass).
  [0X39] = strtext("Trombone"),

  // xgettext: This is the name of MIDI musical instrument #59 (brass).
  [0X3A] = strtext("Tuba"),

  // xgettext: This is the name of MIDI musical instrument #60 (brass).
  [0X3B] = strtext("Muted Trumpet"),

  // xgettext: This is the name of MIDI musical instrument #61 (brass).
  [0X3C] = strtext("French Horn"),

  // xgettext: This is the name of MIDI musical instrument #62 (brass).
  [0X3D] = strtext("Brass Section"),

  // xgettext: This is the name of MIDI musical instrument #63 (brass).
  [0X3E] = strtext("SynthBrass 1"),

  // xgettext: This is the name of MIDI musical instrument #64 (brass).
  [0X3F] = strtext("SynthBrass 2"),

/* Reed */
  // xgettext: This is the name of MIDI musical instrument #65 (reed).
  // xgettext: (sax is a common short form for saxophone)
  [0X40] = strtext("Soprano Sax"),

  // xgettext: This is the name of MIDI musical instrument #66 (reed).
  // xgettext: (sax is a common short form for saxophone)
  [0X41] = strtext("Alto Sax"),

  // xgettext: This is the name of MIDI musical instrument #67 (reed).
  // xgettext: (sax is a common short form for saxophone)
  [0X42] = strtext("Tenor Sax"),

  // xgettext: This is the name of MIDI musical instrument #68 (reed).
  // xgettext: (sax is a common short form for saxophone)
  [0X43] = strtext("Baritone Sax"),

  // xgettext: This is the name of MIDI musical instrument #69 (reed).
  [0X44] = strtext("Oboe"),

  // xgettext: This is the name of MIDI musical instrument #70 (reed).
  [0X45] = strtext("English Horn"),

  // xgettext: This is the name of MIDI musical instrument #71 (reed).
  [0X46] = strtext("Bassoon"),

  // xgettext: This is the name of MIDI musical instrument #72 (reed).
  [0X47] = strtext("Clarinet"),

/* Pipe */
  // xgettext: This is the name of MIDI musical instrument #73 (pipe).
  [0X48] = strtext("Piccolo"),

  // xgettext: This is the name of MIDI musical instrument #74 (pipe).
  [0X49] = strtext("Flute"),

  // xgettext: This is the name of MIDI musical instrument #75 (pipe).
  [0X4A] = strtext("Recorder"),

  // xgettext: This is the name of MIDI musical instrument #76 (pipe).
  [0X4B] = strtext("Pan Flute"),

  // xgettext: This is the name of MIDI musical instrument #77 (pipe).
  [0X4C] = strtext("Blown Bottle"),

  // xgettext: This is the name of MIDI musical instrument #78 (pipe).
  [0X4D] = strtext("Shakuhachi"),

  // xgettext: This is the name of MIDI musical instrument #79 (pipe).
  [0X4E] = strtext("Whistle"),

  // xgettext: This is the name of MIDI musical instrument #80 (pipe).
  [0X4F] = strtext("Ocarina"),

/* Synth Lead */
  // xgettext: This is the name of MIDI musical instrument #81 (synth lead).
  [0X50] = strtext("Lead 1 (square)"),

  // xgettext: This is the name of MIDI musical instrument #82 (synth lead).
  [0X51] = strtext("Lead 2 (sawtooth)"),

  // xgettext: This is the name of MIDI musical instrument #83 (synth lead).
  [0X52] = strtext("Lead 3 (calliope)"),

  // xgettext: This is the name of MIDI musical instrument #84 (synth lead).
  [0X53] = strtext("Lead 4 (chiff)"),

  // xgettext: This is the name of MIDI musical instrument #85 (synth lead).
  [0X54] = strtext("Lead 5 (charang)"),

  // xgettext: This is the name of MIDI musical instrument #86 (synth lead).
  [0X55] = strtext("Lead 6 (voice)"),

  // xgettext: This is the name of MIDI musical instrument #87 (synth lead).
  [0X56] = strtext("Lead 7 (fifths)"),

  // xgettext: This is the name of MIDI musical instrument #88 (synth lead).
  [0X57] = strtext("Lead 8 (bass + lead)"),

/* Synth Pad */
  // xgettext: This is the name of MIDI musical instrument #89 (synth pad).
  [0X58] = strtext("Pad 1 (new age)"),

  // xgettext: This is the name of MIDI musical instrument #90 (synth pad).
  [0X59] = strtext("Pad 2 (warm)"),

  // xgettext: This is the name of MIDI musical instrument #91 (synth pad).
  [0X5A] = strtext("Pad 3 (polysynth)"),

  // xgettext: This is the name of MIDI musical instrument #92 (synth pad).
  [0X5B] = strtext("Pad 4 (choir)"),

  // xgettext: This is the name of MIDI musical instrument #93 (synth pad).
  [0X5C] = strtext("Pad 5 (bowed)"),

  // xgettext: This is the name of MIDI musical instrument #94 (synth pad).
  [0X5D] = strtext("Pad 6 (metallic)"),

  // xgettext: This is the name of MIDI musical instrument #95 (synth pad).
  [0X5E] = strtext("Pad 7 (halo)"),

  // xgettext: This is the name of MIDI musical instrument #96 (synth pad).
  [0X5F] = strtext("Pad 8 (sweep)"),

/* Synth FM */
  // xgettext: This is the name of MIDI musical instrument #97 (synth FM).
  [0X60] = strtext("FX 1 (rain)"),

  // xgettext: This is the name of MIDI musical instrument #98 (synth FM).
  [0X61] = strtext("FX 2 (soundtrack)"),

  // xgettext: This is the name of MIDI musical instrument #99 (synth FM).
  [0X62] = strtext("FX 3 (crystal)"),

  // xgettext: This is the name of MIDI musical instrument #100 (synth FM).
  [0X63] = strtext("FX 4 (atmosphere)"),

  // xgettext: This is the name of MIDI musical instrument #101 (synth FM).
  [0X64] = strtext("FX 5 (brightness)"),

  // xgettext: This is the name of MIDI musical instrument #102 (synth FM).
  [0X65] = strtext("FX 6 (goblins)"),

  // xgettext: This is the name of MIDI musical instrument #103 (synth FM).
  [0X66] = strtext("FX 7 (echoes)"),

  // xgettext: This is the name of MIDI musical instrument #104 (synth FM).
  // xgettext: (sci-fi is a common short form for science fiction)
  [0X67] = strtext("FX 8 (sci-fi)"),

/* Ethnic Instruments */
  // xgettext: This is the name of MIDI musical instrument #105 (ethnic).
  [0X68] = strtext("Sitar"),

  // xgettext: This is the name of MIDI musical instrument #106 (ethnic).
  [0X69] = strtext("Banjo"),

  // xgettext: This is the name of MIDI musical instrument #107 (ethnic).
  [0X6A] = strtext("Shamisen"),

  // xgettext: This is the name of MIDI musical instrument #108 (ethnic).
  [0X6B] = strtext("Koto"),

  // xgettext: This is the name of MIDI musical instrument #109 (ethnic).
  [0X6C] = strtext("Kalimba"),

  // xgettext: This is the name of MIDI musical instrument #110 (ethnic).
  [0X6D] = strtext("Bag Pipe"),

  // xgettext: This is the name of MIDI musical instrument #111 (ethnic).
  [0X6E] = strtext("Fiddle"),

  // xgettext: This is the name of MIDI musical instrument #112 (ethnic).
  [0X6F] = strtext("Shanai"),

/* Percussive Instruments */
  // xgettext: This is the name of MIDI musical instrument #113 (percussive).
  [0X70] = strtext("Tinkle Bell"),

  // xgettext: This is the name of MIDI musical instrument #114 (percussive).
  [0X71] = strtext("Agogo"),

  // xgettext: This is the name of MIDI musical instrument #115 (percussive).
  [0X72] = strtext("Steel Drums"),

  // xgettext: This is the name of MIDI musical instrument #116 (percussive).
  [0X73] = strtext("Woodblock"),

  // xgettext: This is the name of MIDI musical instrument #117 (percussive).
  [0X74] = strtext("Taiko Drum"),

  // xgettext: This is the name of MIDI musical instrument #118 (percussive).
  [0X75] = strtext("Melodic Tom"),

  // xgettext: This is the name of MIDI musical instrument #119 (percussive).
  [0X76] = strtext("Synth Drum"),

  // xgettext: This is the name of MIDI musical instrument #120 (percussive).
  [0X77] = strtext("Reverse Cymbal"),

/* Sound Effects */
  // xgettext: This is the name of MIDI sound effect #121.
  [0X78] = strtext("Guitar Fret Noise"),

  // xgettext: This is the name of MIDI sound effect #122.
  [0X79] = strtext("Breath Noise"),

  // xgettext: This is the name of MIDI sound effect #123.
  [0X7A] = strtext("Seashore"),

  // xgettext: This is the name of MIDI sound effect #124.
  [0X7B] = strtext("Bird Tweet"),

  // xgettext: This is the name of MIDI sound effect #125.
  [0X7C] = strtext("Telephone Ring"),

  // xgettext: This is the name of MIDI sound effect #126.
  [0X7D] = strtext("Helicopter"),

  // xgettext: This is the name of MIDI sound effect #127.
  [0X7E] = strtext("Applause"),

  // xgettext: This is the name of MIDI sound effect #128.
  [0X7F] = strtext("Gunshot")
};
const unsigned int midiInstrumentCount = ARRAY_COUNT(midiInstrumentTable);

const char *
midiGetInstrumentType (unsigned char instrument) {
  return midiInstrumentTypes[instrument >> 3];
}
