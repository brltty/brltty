/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>

#include "brl.h"
#include "common.h"
#include "misc.h"
#include "tones.h"

static int fileDescriptor = -1;
static int deviceNumber = 1;
static int channelNumber = 0;

char *midiInstrumentTable[] = {
/* Piano */
  /* 00 */ "Acoustic Grand Piano",
  /* 01 */ "Bright Acoustic Piano",
  /* 02 */ "Electric Grand Piano",
  /* 03 */ "Honky-tonk Piano",
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
unsigned int midiInstrumentCount = sizeof(midiInstrumentTable) / sizeof(midiInstrumentTable[0]);

static double noteTable[] = {
   /*   0 -5C  */     8.176,
   /*   1 -5C# */     8.662,
   /*   2 -5D  */     9.177,
   /*   3 -5D# */     9.723,
   /*   4 -5E  */    10.301,
   /*   5 -5F  */    10.913,
   /*   6 -5F# */    11.562,
   /*   7 -5G  */    12.250,
   /*   8 -5G# */    12.978,
   /*   9 -5A  */    13.750,
   /*  10 -5A# */    14.568,
   /*  11 -5B  */    15.434,
   /*  12 -4C  */    16.352,
   /*  13 -4C# */    17.324,
   /*  14 -4D  */    18.354,
   /*  15 -4D# */    19.445,
   /*  16 -4E  */    20.602,
   /*  17 -4F  */    21.827,
   /*  18 -4F# */    23.125,
   /*  19 -4G  */    24.500,
   /*  20 -4G# */    25.957,
   /*  21 -4A  */    27.500,
   /*  22 -4A# */    29.135,
   /*  23 -4B  */    30.868,
   /*  24 -3C  */    32.703,
   /*  25 -3C# */    34.648,
   /*  26 -3D  */    36.708,
   /*  27 -3D# */    38.891,
   /*  28 -3E  */    41.203,
   /*  29 -3F  */    43.654,
   /*  30 -3F# */    46.249,
   /*  31 -3G  */    48.999,
   /*  32 -3G# */    51.913,
   /*  33 -3A  */    55.000,
   /*  34 -3A# */    58.270,
   /*  35 -3B  */    61.735,
   /*  36 -2C  */    65.406,
   /*  37 -2C# */    69.296,
   /*  38 -2D  */    73.416,
   /*  39 -2D# */    77.782,
   /*  40 -2E  */    82.407,
   /*  41 -2F  */    87.307,
   /*  42 -2F# */    92.499,
   /*  43 -2G  */    97.999,
   /*  44 -2G# */   103.826,
   /*  45 -2A  */   110.000,
   /*  46 -2A# */   116.541,
   /*  47 -2B  */   123.471,
   /*  48 -1C  */   130.813,
   /*  49 -1C# */   138.591,
   /*  50 -1D  */   146.832,
   /*  51 -1D# */   155.563,
   /*  52 -1E  */   164.814,
   /*  53 -1F  */   174.614,
   /*  54 -1F# */   184.997,
   /*  55 -1G  */   195.998,
   /*  56 -1G# */   207.652,
   /*  57 -1A  */   220.000,
   /*  58 -1A# */   233.082,
   /*  59 -1B  */   246.942,
   /*  60  0C  */   261.626,
   /*  61  0C# */   277.183,
   /*  62  0D  */   293.665,
   /*  63  0D# */   311.127,
   /*  64  0E  */   329.628,
   /*  65  0F  */   349.228,
   /*  66  0F# */   369.994,
   /*  67  0G  */   391.995,
   /*  68  0G# */   415.305,
   /*  69  0A  */   440.000,
   /*  70  0A# */   466.164,
   /*  71  0B  */   493.883,
   /*  72 +1C  */   523.251,
   /*  73 +1C# */   554.365,
   /*  74 +1D  */   587.330,
   /*  75 +1D# */   622.254,
   /*  76 +1E  */   659.255,
   /*  77 +1F  */   698.456,
   /*  78 +1F# */   739.989,
   /*  79 +1G  */   783.991,
   /*  80 +1G# */   830.609,
   /*  81 +1A  */   880.000,
   /*  82 +1A# */   932.328,
   /*  83 +1B  */   987.767,
   /*  84 +2C  */  1046.502,
   /*  85 +2C# */  1108.731,
   /*  86 +2D  */  1174.659,
   /*  87 +2D# */  1244.508,
   /*  88 +2E  */  1318.510,
   /*  89 +2F  */  1396.913,
   /*  90 +2F# */  1479.978,
   /*  91 +2G  */  1567.982,
   /*  92 +2G# */  1661.219,
   /*  93 +2A  */  1760.000,
   /*  94 +2A# */  1864.655,
   /*  95 +2B  */  1975.533,
   /*  96 +3C  */  2093.005,
   /*  97 +3C# */  2217.461,
   /*  98 +3D  */  2349.318,
   /*  99 +3D# */  2489.016,
   /* 100 +3E  */  2637.020,
   /* 101 +3F  */  2793.826,
   /* 102 +3F# */  2959.955,
   /* 103 +3G  */  3135.963,
   /* 104 +3G# */  3322.438,
   /* 105 +3A  */  3520.000,
   /* 106 +3A# */  3729.310,
   /* 107 +3B  */  3951.066,
   /* 108 +4C  */  4186.009,
   /* 109 +4C# */  4434.922,
   /* 110 +4D  */  4698.636,
   /* 111 +4D# */  4978.032,
   /* 112 +4E  */  5274.041,
   /* 113 +4F  */  5587.652,
   /* 114 +4F# */  5919.911,
   /* 115 +4G  */  6271.927,
   /* 116 +4G# */  6644.875,
   /* 117 +4A  */  7040.000,
   /* 118 +4A# */  7458.620,
   /* 119 +4B  */  7902.133,
   /* 120 +5C  */  8372.018,
   /* 121 +5C# */  8869.844,
   /* 122 +5D  */  9397.273,
   /* 123 +5D# */  9956.063,
   /* 124 +5E  */ 10548.082,
   /* 125 +5F  */ 11175.303,
   /* 126 +5F# */ 11839.822,
   /* 127 +5G  */ 12543.854
};
static unsigned int noteCount = sizeof(noteTable) / sizeof(noteTable[0]);

SEQ_DEFINEBUF(0X80);

void seqbuf_dump (void) {
   if (_seqbufptr) {
      write(fileDescriptor,_seqbuf, _seqbufptr);
      _seqbufptr = 0;
   }
}

static int openSequencer (void) {
   if (fileDescriptor == -1) {
      if ((fileDescriptor = open("/dev/sequencer", O_WRONLY)) == -1) {
         return 0;
      }
      setCloseOnExec(fileDescriptor);
      LogPrint(LOG_DEBUG, "Sequencer opened: fd=%d", fileDescriptor);
      SEQ_START_TIMER();
   }
   SEQ_SET_PATCH(deviceNumber, channelNumber, env.midiinstr);
   return 1;
}

static int getNote (double frequency) {
   int minimum = 0;
   int maximum = noteCount - 1;
   while (minimum <= maximum) {
      int note = (minimum + maximum) / 2;
      if (frequency > noteTable[note])
	 minimum = note + 1;
      else
	 maximum = note - 1;
   }
   return minimum;
}
 
static int getDuration (int duration) {
   return (duration + 9) / 10;
}

static int generateSequencer (int frequency, int duration) {
   if (fileDescriptor != -1) {
      duration = getDuration(duration);
      if (frequency) {
	 int note = getNote(frequency);
         SEQ_START_NOTE(deviceNumber, channelNumber, note, 0X7F);
	 SEQ_DELTA_TIME(duration);
         SEQ_STOP_NOTE(deviceNumber, channelNumber, note, 0X7F);
      } else {
	 SEQ_DELTA_TIME(duration);
      }
      seqbuf_dump();
      ioctl(fileDescriptor, SNDCTL_SEQ_SYNC);
      return 1;
   }
   return 0;
}

static int flushSequencer (void) {
   return 1;
}

static void closeSequencer (void) {
   if (fileDescriptor != -1) {
      close(fileDescriptor);
      LogPrint(LOG_DEBUG, "Sequencer closed.");
      fileDescriptor = -1;
   }
}

static ToneGenerator toneGenerator = {
   openSequencer,
   generateSequencer,
   flushSequencer,
   closeSequencer
};

ToneGenerator *toneSequencer (void) {
   return &toneGenerator;
}
