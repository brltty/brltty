/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "brltty.h"
#include "misc.h"
#include "system.h"
#include "notes.h"

static PcmDevice *pcm = NULL;
static int blockSize;
static int sampleRate;
static int channelCount;
static PcmAmplitudeFormat amplitudeFormat;

static unsigned char *blockAddress = NULL;
static size_t blockUsed = 0;

static int openPcm (int errorLevel) {
   if (pcm) return 1;
   if ((pcm = openPcmDevice(errorLevel))) {
      blockSize = getPcmBlockSize(pcm);
      sampleRate = getPcmSampleRate(pcm);
      channelCount = getPcmChannelCount(pcm);
      amplitudeFormat = getPcmAmplitudeFormat(pcm);
      if ((blockAddress = malloc(blockSize))) {
         blockUsed = 0;
         LogPrint(LOG_DEBUG, "PCM opened: blk=%d rate=%d chan=%d fmt=%d",
                  blockSize, sampleRate, channelCount, amplitudeFormat);
         return 1;
      } else {
        LogError("PCM buffer allocation");
      }
      closePcmDevice(pcm);
      pcm = NULL;
   } else {
      LogPrint(LOG_DEBUG, "Cannot open PCM.");
   }
   return 0;
}

static int flushBytes (void) {
   int ok = writePcmData(pcm, blockAddress, blockUsed);
   if (ok) {
      blockUsed = 0;
   } else {
      LogError("PCM write");
   }
   return ok;
}

static int writeBytes (const unsigned char *address, size_t length) {
   while (length > 0) {
      size_t count = blockSize - blockUsed;
      if (length < count) count = length;
      memcpy(&blockAddress[blockUsed], address, count);
      address += count;
      length -= count;
      if ((blockUsed += count) == blockSize)
         if (!flushBytes())
            return 0;
   }
   return 1;
}

static int writeAmplitude (int amplitude) {
   unsigned char sample[4];
   size_t length;
   int channel;
   switch (amplitudeFormat) {
      default:
         length = 0;
         break;
      case PCM_FMT_U8:
         amplitude += 0X8000;
      case PCM_FMT_S8:
         sample[0] = amplitude >> 8;
         length = 1;
         break;
      case PCM_FMT_U16B:
         amplitude += 0X8000;
      case PCM_FMT_S16B:
         sample[0] = amplitude >> 8;
         sample[1] = amplitude;
         length = 2;
         break;
      case PCM_FMT_U16L:
         amplitude += 0X8000;
      case PCM_FMT_S16L:
         sample[0] = amplitude;
         sample[1] = amplitude >> 8;
         length = 2;
         break;
      case PCM_FMT_ULAW: {
         int negative = amplitude < 0;
         int exponent = 0X7;
         unsigned char value;
         if (negative) amplitude = -amplitude;
         while ((exponent > 0) && !(amplitude & 0X4000)) {
            amplitude <<= 1;
            --exponent;
         }
         value = (exponent << 4) | ((amplitude >> 10) & 0XF);
         if (negative) value |= 0X80;
         sample[0] = ~value;
         length = 1;
         break;
      }
   }
   for (channel=0; channel<channelCount; ++channel)
      if (!writeBytes(sample, length))
         return 0;
   return 1;
}

static int playPcm (int note, int duration) {
   if (pcm) {
      long int sampleCount = sampleRate * duration / 1000;
      if (note) {
         /* A triangle waveform sounds nice, is lightweight, and avoids
          * relying too much on floating-point performance and/or on
          * expensive math functions like sin(). Considerations like
	  * these are especially important on PDAs without any FPU.
          */ 
         float maxAmplitude = 32767.0 * prefs.pcmVolume / 100;
         float waveLength = sampleRate / noteFrequencies[note];
         float stepSample = 4 * maxAmplitude / waveLength;
         float currSample = 0;
         if (waveLength <= 2) stepSample = 0;
         LogPrint(LOG_DEBUG, "Tone: msec=%d smct=%lu note=%d",
                  duration, sampleCount, note);
         while (sampleCount > 0) {
            do {
               sampleCount--;
               if (!writeAmplitude(currSample)) return 0;
               currSample += stepSample;
               if (currSample >= maxAmplitude) {
                  currSample = 2*maxAmplitude - currSample;
                  stepSample = -stepSample;
               } else if (currSample <= -maxAmplitude) {
                  currSample = -2*maxAmplitude - currSample;
                  stepSample = -stepSample;
               }
            } while ((stepSample < 0) || (currSample < 0) || (currSample > stepSample));
         }
      } else {
         LogPrint(LOG_DEBUG, "Tone: msec=%d smct=%lu note=%d",
                  duration, sampleCount, note);
         while (sampleCount > 0) {
            if (!writeAmplitude(0)) return 0;
            --sampleCount;
         }
      }
      return 1;
   }
   return 0;
}

static int flushBlock (void) {
   while (blockUsed)
      if (!writeAmplitude(0))
         return 0;
   return 1;
}

static int flushPcm (void) {
   return (pcm)? flushBlock(): 0;
}

static void closePcm (void) {
   if (pcm) {
      flushBlock();
      free(blockAddress);
      blockAddress = NULL;
      closePcmDevice(pcm);
      pcm = NULL;
      LogPrint(LOG_DEBUG, "PCM closed.");
   }
}

const NoteGenerator pcmNoteGenerator = {
   openPcm,
   playPcm,
   flushPcm,
   closePcm
};
