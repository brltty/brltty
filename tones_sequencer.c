/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>

#include "common.h"
#include "misc.h"
#include "tones.h"

static int fileDescriptor = -1;
static int deviceNumber = 1;
static int channelNumber = 0;

const char *midiInstrumentTable[] = {
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
unsigned int midiInstrumentCount = sizeof(midiInstrumentTable) / sizeof(midiInstrumentTable[0]);

SEQ_DEFINEBUF(0X80);

void seqbuf_dump (void) {
   if (_seqbufptr) {
      if (write(fileDescriptor, _seqbuf, _seqbufptr) == -1) {
         LogPrint(LOG_ERR, "Cannot write to sequencer: %s", strerror(errno));
      }
      _seqbufptr = 0;
   }
}

static int openSequencer (void) {
   if (fileDescriptor == -1) {
      char *device = "/dev/sequencer";
      if ((fileDescriptor = open(device, O_WRONLY)) == -1) {
         LogPrint(LOG_ERR, "Cannot open sequencer: %s: %s", device, strerror(errno));
         return 0;
      }
      setCloseOnExec(fileDescriptor);
      LogPrint(LOG_DEBUG, "Sequencer opened: fd=%d", fileDescriptor);
   }
   SEQ_SET_PATCH(deviceNumber, channelNumber, prefs.midiinstr);
   return 1;
}

static int getDuration (int duration) {
   return (duration + 9) / 10;
}

static int generateSequencer (int note, int duration) {
   if (fileDescriptor != -1) {
      int time = getDuration(duration);
      SEQ_START_TIMER();
      if (note) {
	 LogPrint(LOG_DEBUG, "Tone: msec=%d time=%d note=%d",
	          duration, time, note);
         SEQ_START_NOTE(deviceNumber, channelNumber, note, 0X7F);
	 SEQ_DELTA_TIME(time);
         SEQ_STOP_NOTE(deviceNumber, channelNumber, note, 0X7F);
      } else {
	 LogPrint(LOG_DEBUG, "Tone: msec=%d time=%d",
	          duration, time);
	 SEQ_DELTA_TIME(time);
      }
      SEQ_STOP_TIMER();
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
