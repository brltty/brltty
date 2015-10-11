/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>
#include <errno.h>

#include "prefs.h"
#include "log.h"
#include "pcm.h"
#include "notes.h"

char *opt_pcmDevice;

struct NoteDeviceStruct {
  PcmDevice *pcm;
  int blockSize;
  int sampleRate;
  int channelCount;
  PcmAmplitudeFormat amplitudeFormat;
  unsigned char *blockAddress;
  size_t blockUsed;
};

static int
pcmFlushBytes (NoteDevice *device) {
  int ok = writePcmData(device->pcm, device->blockAddress, device->blockUsed);
  if (ok) device->blockUsed = 0;
  return ok;
}

static int
pcmWriteBytes (NoteDevice *device, const void *address, size_t length) {
  while (length > 0) {
    size_t count = device->blockSize - device->blockUsed;
    if (length < count) count = length;
    memcpy(&device->blockAddress[device->blockUsed], address, count);
    address += count;
    length -= count;

    if ((device->blockUsed += count) == device->blockSize)
      if (!pcmFlushBytes(device))
        return 0;
  }

  return 1;
}

static int
pcmWriteSample (NoteDevice *device, int16_t amplitude) {
  PcmSample sample;
  size_t size = makePcmSample(&sample, amplitude, device->amplitudeFormat);

  for (int channel=0; channel<device->channelCount; channel+=1) {
    if (!pcmWriteBytes(device, sample.bytes, size)) return 0;
  }

  return 1;
}

static int
pcmFlushBlock (NoteDevice *device) {
  while (device->blockUsed)
    if (!pcmWriteSample(device, 0))
      return 0;

  return 1;
}

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
        logMessage(LOG_DEBUG, "PCM enabled: blk=%d rate=%d chan=%d fmt=%d",
                   device->blockSize, device->sampleRate, device->channelCount, device->amplitudeFormat);
        return device;
      } else {
        logMallocError();
      }

      closePcmDevice(device->pcm);
    }

    free(device);
  } else {
    logMallocError();
  }

  logMessage(LOG_DEBUG, "PCM not available");
  return NULL;
}

static void
pcmDestruct (NoteDevice *device) {
  pcmFlushBlock(device);
  free(device->blockAddress);
  closePcmDevice(device->pcm);
  free(device);
  logMessage(LOG_DEBUG, "PCM disabled");
}

static int
pcmPlay (NoteDevice *device, unsigned char note, unsigned int duration) {
  long int sampleCount = device->sampleRate * duration / 1000;

  logMessage(LOG_DEBUG, "tone: msec=%u smct=%ld note=%u",
             duration, sampleCount, note);

  if (note) {
    /* A triangle waveform sounds nice, is lightweight, and avoids
     * relying too much on floating-point performance and/or on
     * expensive math functions like sin(). Considerations like
     * these are especially important on PDAs without any FPU.
     */ 

    const int32_t positiveStepsPerQuarterWave = INT32_C(1) << (16 + 12);
    const int32_t positiveStepsPerHalfWave = positiveStepsPerQuarterWave << 1;
    const int32_t positiveStepsPerFullWave = positiveStepsPerHalfWave << 1;

    const int32_t negativeStepsPerHalfWave = -positiveStepsPerHalfWave;
    const int32_t negativeStepsPerQuarterWave = -positiveStepsPerQuarterWave;

    /* We need to know how many steps to make from one sample to the next.
     * stepsPerSample = stepsPerWave * wavesPerSecond / samplesPerSecond
     *                = stepsPerWave * frequency / sampleRate
     *                = stepsPerWave / sampleRate * frequency
     */
    const int32_t stepsPerSample = (NOTE_FREQUENCY_TYPE)positiveStepsPerFullWave 
                                 / (NOTE_FREQUENCY_TYPE)device->sampleRate
                                 * GET_NOTE_FREQUENCY(note);

    /* We need to know the maximum amplitude based on the volume percentage.
     * The percentage needs to be squared since we perceive loudness exponentially.
     */
    const unsigned char fullVolume = 100;
    const unsigned char currentVolume = MIN(fullVolume, prefs.pcmVolume);
    const int32_t maximumAmplitude = INT16_MAX
                                   * (currentVolume * currentVolume)
                                   / (fullVolume * fullVolume);

    /* We start with an offset of 0. */
    int32_t currentOffset = 0;

    /* This loop iterates once per wave till the note is complete. */
    while (sampleCount > 0) {

#define writeSample() { \
  int32_t amplitude = ((currentOffset >> 12) * maximumAmplitude) >> 16; \
  if (!pcmWriteSample(device, amplitude)) return 0; \
  sampleCount -= 1; \
}

      /* This loop iterates once per sample for the first quarter wave. It
       * writes samples that ascend from 0 to the positive peak.
       */
      while (currentOffset < positiveStepsPerQuarterWave) {
        writeSample();
        currentOffset += stepsPerSample;
      }

      /* The offset has gone too high so prepare for the next part of the
       * wave by recalculating the offset for descending samples.
       */
      currentOffset = positiveStepsPerHalfWave - currentOffset;

      /* This loop iterates once per sample for the second and third
       * quarter waves. It writes samples that descend from the positive
       * peak to the negative peak.
       */
      while (currentOffset > negativeStepsPerQuarterWave) {
        writeSample();
        currentOffset -= stepsPerSample;
      }

      /* The offset has gone too low so prepare for the next part of the
       * wave by recalculating the offset for ascending samples.
       */
      currentOffset = negativeStepsPerHalfWave - currentOffset;

      /* This loop iterates once per sample for the fourth quarter wave. It
       * writes samples that ascend from the negative peak to 0.
       */
      while (currentOffset < 0) {
        writeSample();
        currentOffset += stepsPerSample;
      }
    }
  } else {
    /* generate silence */
    while (sampleCount > 0) {
      if (!pcmWriteSample(device, 0)) return 0;
      sampleCount -= 1;
    }
  }

  return 1;
}

static int
pcmFlush (NoteDevice *device) {
  return pcmFlushBlock(device);
}

const NoteMethods pcmNoteMethods = {
  .construct = pcmConstruct,
  .destruct = pcmDestruct,

  .play = pcmPlay,
  .flush = pcmFlush
};
