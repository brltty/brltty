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
  PcmAmplitudeFormat format = device->amplitudeFormat;
  PcmSample sample;
  size_t length = makePcmSample(&sample, amplitude, format);

  {
    int channel;

    for (channel=0; channel<device->channelCount; channel+=1) {
      if (!pcmWriteBytes(device, &sample, length)) return 0;
    }
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

  if (note) {
    /* A triangle waveform sounds nice, is lightweight, and avoids
     * relying too much on floating-point performance and/or on
     * expensive math functions like sin(). Considerations like
     * these are especially important on PDAs without any FPU.
     */ 

    const int32_t positiveShiftsPerQuarterWave = (INT32_MAX >> 3) + INT32_C(1);
    const int32_t negativeShiftsPerQuarterWave = -positiveShiftsPerQuarterWave;

    const int32_t positiveShiftsPerHalfWave = positiveShiftsPerQuarterWave << 1;
    const int32_t negativeShiftsPerHalfWave = -positiveShiftsPerHalfWave;

    const int32_t positiveShiftsPerFullWave = positiveShiftsPerHalfWave << 1;

    const int32_t shiftsPerSample = (NOTE_FREQUENCY_TYPE)positiveShiftsPerFullWave 
                                  / (NOTE_FREQUENCY_TYPE)device->sampleRate
                                  * GET_NOTE_FREQUENCY(note);

    const int32_t maximumAmplitude = INT16_MAX * prefs.pcmVolume * prefs.pcmVolume / 10000;
    int32_t currentShift = 0;

    logMessage(LOG_DEBUG, "tone: msec=%u smct=%ld note=%u",
               duration, sampleCount, note);

    while (sampleCount > 0) {
      do {
        {
          int32_t normalizedAmplitude = positiveShiftsPerHalfWave - currentShift;

          if (normalizedAmplitude > positiveShiftsPerQuarterWave) {
            normalizedAmplitude = positiveShiftsPerHalfWave - normalizedAmplitude;
          } else if (normalizedAmplitude < negativeShiftsPerQuarterWave) {
            normalizedAmplitude = negativeShiftsPerHalfWave - normalizedAmplitude;
          }

          {
            const int16_t actualAmplitude =
              ((normalizedAmplitude >> 12) * maximumAmplitude) >> 16;

            if (!pcmWriteSample(device, actualAmplitude)) return 0;
            sampleCount -= 1;
          }
        }
      } while ((currentShift += shiftsPerSample) < positiveShiftsPerFullWave);

      do {
        currentShift -= positiveShiftsPerFullWave;
      } while (currentShift >= positiveShiftsPerFullWave);
    }
  } else {
    logMessage(LOG_DEBUG, "tone: msec=%u smct=%ld note=%u",
               duration, sampleCount, note);

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
