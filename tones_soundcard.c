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
#include <math.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>

#include "misc.h"
#include "tones.h"

static int fileDescriptor = -1;
static int blockSize;
static int bytesWritten;
static int sampleRate;
static int channelCount;

#define QUERY_SOUND(v, f, d) (v = ((ioctl(fileDescriptor, f, &setting) != -1)? setting: (d)))
static int openSoundCard (void) {
   if (fileDescriptor == -1) {
      char *device = "/dev/dsp";
      if ((fileDescriptor = open(device, O_WRONLY|O_NONBLOCK)) == -1) {
         LogPrint(LOG_ERR, "Cannot open sound card: %s: %s", device, strerror(errno));
         return 0;
      }
      setCloseOnExec(fileDescriptor);
      {
	 int fragmentCount = (1 << 0X10) - 1;
	 int fragmentShift = 7;
	 int fragmentSize = 1 << fragmentShift;
	 int setting = (fragmentCount << 0X10) | fragmentShift;
	 ioctl(fileDescriptor, SNDCTL_DSP_SETFRAGMENT, &setting);
	 QUERY_SOUND(blockSize, SNDCTL_DSP_GETBLKSIZE, fragmentSize);
	 bytesWritten = 0;
	 QUERY_SOUND(sampleRate, SOUND_PCM_READ_RATE, 8000);
	 QUERY_SOUND(channelCount, SOUND_PCM_READ_CHANNELS, 1);
      }
      LogPrint(LOG_DEBUG, "Sound card opened: fd=%d blk=%d rate=%d chan=%d",
               fileDescriptor, blockSize, sampleRate, channelCount);
   }
   return 1;
}

static int writeSample (unsigned char sample) {
   int channelNumber;
   for (channelNumber=0; channelNumber<channelCount; ++channelNumber) {
      while (1) {
	 int count = write(fileDescriptor, &sample, 1);
	 if (count == -1) {
	    if (errno != EAGAIN) {
	       LogPrint(LOG_ERR, "Cannot write to sound card: %s", strerror(errno));
	       return 0;
	    }
	    count = 0;
	 }
	 bytesWritten += count;
	 if (count) break;
	 delay(10);
      }
   }
   return 1;
}

static int generateSoundCard (int note, int duration) {
   if (fileDescriptor != -1) {
      double waveLength = note? (double)sampleRate / noteFrequencies[note]: HUGE_VAL;
      unsigned long int sampleCount = sampleRate * duration / 1000;
      int sampleNumber;
      LogPrint(LOG_DEBUG, "Tone: msec=%d note=%d wvln=%.2E smct=%lu",
               duration, note, waveLength, sampleCount);
      for (sampleNumber=0; sampleNumber<sampleCount; ++sampleNumber) {
         unsigned char sample = rint(sin(((double)sampleNumber / waveLength) * (2.0 * M_PI)) * 127.0) + 128;
	 if (!writeSample(sample)) return 0;
      }
      return 1;
   }
   return 0;
}

static int flushFragment (void) {
   while (bytesWritten % blockSize) {
      if (!writeSample(0X80)) return 0;
   }
   return 1;
}

static int flushSoundCard (void) {
   return (fileDescriptor != -1)? flushFragment(): 0;
}

static void closeSoundCard (void) {
   if (fileDescriptor != -1) {
      flushFragment();
      close(fileDescriptor);
      LogPrint(LOG_DEBUG, "Sound card closed.");
      fileDescriptor = -1;
   }
}

static ToneGenerator toneGenerator = {
   openSoundCard,
   generateSoundCard,
   flushSoundCard,
   closeSoundCard
};

ToneGenerator *toneSoundCard (void) {
   return &toneGenerator;
}
