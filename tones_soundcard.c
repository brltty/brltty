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
#include <math.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>

#include "misc.h"
#include "tones.h"

static int fileDescriptor = -1;
static int sampleRate;
static int channelCount;

#define QUERY_SOUND(v, f, d) (v = ((ioctl(fileDescriptor, f, &setting) != -1)? setting: (d)))
static int openSoundCard (void) {
   if (fileDescriptor == -1) {
      if ((fileDescriptor = open("/dev/dsp", O_WRONLY)) == -1) {
         return 0;
      }
      setCloseOnExec(fileDescriptor);
      {
	 int setting = 0X7FFF0008;
	 ioctl(fileDescriptor, SNDCTL_DSP_SETFRAGMENT, &setting);
	 QUERY_SOUND(sampleRate, SOUND_PCM_READ_RATE, 8000);
	 QUERY_SOUND(channelCount, SOUND_PCM_READ_CHANNELS, 1);
      }
   }
   return 1;
}

static int generateSoundCard (int frequency, int duration) {
   if (fileDescriptor != -1) {
      double waveLength = frequency? (double)sampleRate / (double)frequency: HUGE_VAL;
      unsigned int sampleCount = sampleRate * duration / 1000;
      int sampleNumber;
      for (sampleNumber=0; sampleNumber<sampleCount; ++sampleNumber) {
         unsigned char amplitude = rint(sin(((double)sampleNumber / waveLength) * (2.0 * M_PI)) * 127.0) + 128;
	 int channelNumber;
	 for (channelNumber=0; channelNumber<channelCount; ++channelNumber) {
	    if (write(fileDescriptor, &amplitude, 1) == -1) {
	       return 0;
	    }
	 }
      }
      return 1;
   }
   return 0;
}

static void closeSoundCard (void) {
   if (fileDescriptor != -1) {
      close(fileDescriptor);
      fileDescriptor = -1;
   }
}

static ToneGenerator toneGenerator = {
   openSoundCard,
   generateSoundCard,
   closeSoundCard
};

ToneGenerator *toneSoundCard (void) {
   return &toneGenerator;
}
