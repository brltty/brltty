/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#include "prologue.h"

#include <string.h>
#include <errno.h>

#include "brltty.h"
#include "misc.h"
#include "system.h"
#include "notes.h"

struct NoteDeviceStruct {
  PcmDevice *pcm;
  int blockSize;
  int sampleRate;
  int channelCount;
  PcmAmplitudeFormat amplitudeFormat;
  unsigned char *blockAddress;
  size_t blockUsed;
};

static NoteDevice *
pcmConstruct (int errorLevel) {
  NoteDevice *device;

  if ((device = malloc(sizeof(*device)))) {
    if ((device->pcm = openPcmDevice(errorLevel, opt_pcmDevice))) {
      device->blockSize = getPcmBlockSize(device->pcm);
      device->sampleRate = getPcmSampleRate(device->pcm);
      device->channelCount = getPcmChannelCount(device->pcm);
      device->amplitudeFormat = getPcmAmplitudeFormat(device->pcm);
      device->blockUsed = 0;

      if ((device->blockAddress = malloc(device->blockSize))) {
        LogPrint(LOG_DEBUG, "PCM enabled: blk=%d rate=%d chan=%d fmt=%d",
                 device->blockSize, device->sampleRate, device->channelCount, device->amplitudeFormat);
        return device;
      } else {
        LogError("malloc");
      }

      closePcmDevice(device->pcm);
    }

    free(device);
  } else {
    LogError("malloc");
  }

  LogPrint(LOG_DEBUG, "PCM not available");
  return NULL;
}

static int
flushBytes (NoteDevice *device) {
  int ok = writePcmData(device->pcm, device->blockAddress, device->blockUsed);
  if (ok) device->blockUsed = 0;
  return ok;
}

static int
writeBytes (NoteDevice *device, const unsigned char *address, size_t length) {
  while (length > 0) {
    size_t count = device->blockSize - device->blockUsed;
    if (length < count) count = length;
    memcpy(&device->blockAddress[device->blockUsed], address, count);
    address += count;
    length -= count;

    if ((device->blockUsed += count) == device->blockSize)
      if (!flushBytes(device))
        return 0;
  }

  return 1;
}

static int
writeSample (NoteDevice *device, int amplitude) {
  unsigned char sample[4];
  size_t length;

  switch (device->amplitudeFormat) {
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
      const unsigned int bias = 0X84;
      const unsigned int clip = 0X7FFF - bias;

      if (negative) amplitude = -amplitude;
      if (amplitude > clip) amplitude = clip;
      amplitude += bias;

      while ((exponent > 0) && !(amplitude & 0X4000)) {
        amplitude <<= 1;
        --exponent;
      }

      value = (exponent << 4) | ((amplitude >> 10) & 0X0F);
      if (negative) value |= 0X80;
      sample[0] = ~value;
      length = 1;
      break;
    }

    case PCM_FMT_ALAW: {
      int negative = amplitude < 0;
      int exponent = 0X7;
      unsigned char value;

      if (negative) amplitude = -amplitude;

      while ((exponent > 0) && !(amplitude & 0X4000)) {
        amplitude <<= 1;
        --exponent;
      }

      if (!exponent) amplitude >>= 1;
      value = (exponent << 4) | ((amplitude >> 10) & 0X0F);
      if (negative) value |= 0X80;
      sample[0] = value ^ 0X55;
      length = 1;
      break;
    }
  }

  {
    int channel;
    for (channel=0; channel<device->channelCount; ++channel) {
      if (!writeBytes(device, sample, length)) return 0;
    }
  }

  return 1;
}

static int
pcmPlay (NoteDevice *device, int note, int duration) {
  long int sampleCount = device->sampleRate * duration / 1000;

  if (note) {
    /* A triangle waveform sounds nice, is lightweight, and avoids
     * relying too much on floating-point performance and/or on
     * expensive math functions like sin(). Considerations like
     * these are especially important on PDAs without any FPU.
     */ 
    float maxAmplitude = 32767.0 * prefs.pcmVolume / 100;
    float waveLength = device->sampleRate / noteFrequencies[note];
    float stepSample = 4 * maxAmplitude / waveLength;
    float currSample = 0;

    if (waveLength <= 2) stepSample = 0;
    LogPrint(LOG_DEBUG, "tone: msec=%d smct=%lu note=%d",
             duration, sampleCount, note);

    while (sampleCount > 0) {
      do {
        sampleCount--;
        if (!writeSample(device, currSample)) return 0;
        currSample += stepSample;

        if (currSample >= maxAmplitude) {
          currSample = (2 * maxAmplitude) - currSample;
          stepSample = -stepSample;
        } else if (currSample <= -maxAmplitude) {
          currSample = (-2 * maxAmplitude) - currSample;
          stepSample = -stepSample;
        }
      } while ((stepSample < 0) || (currSample < 0) || (currSample > stepSample));
    }
  } else {
    LogPrint(LOG_DEBUG, "tone: msec=%d smct=%lu note=%d",
             duration, sampleCount, note);

    while (sampleCount > 0) {
      if (!writeSample(device, 0)) return 0;
      --sampleCount;
    }
  }

  return 1;
}

static int
flushBlock (NoteDevice *device) {
  while (device->blockUsed)
    if (!writeSample(device, 0))
      return 0;

  return 1;
}

static int
pcmFlush (NoteDevice *device) {
  return flushBlock(device);
}

static void
pcmDestruct (NoteDevice *device) {
  flushBlock(device);
  free(device->blockAddress);
  closePcmDevice(device->pcm);
  free(device);
  LogPrint(LOG_DEBUG, "PCM disabled");
}

const NoteMethods pcmMethods = {
  pcmConstruct,
  pcmPlay,
  pcmFlush,
  pcmDestruct
};
