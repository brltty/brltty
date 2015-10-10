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

  if (note) {
    /* A triangle waveform sounds nice, is lightweight, and avoids
     * relying too much on floating-point performance and/or on
     * expensive math functions like sin(). Considerations like
     * these are especially important on PDAs without any FPU.
     */ 

    const int32_t positiveShiftsPerFullWave = (INT32_MAX >> 1) + INT32_C(1);
    const int32_t positiveShiftsPerHalfWave = positiveShiftsPerFullWave >> 1;
    const int32_t positiveShiftsPerQuarterWave = positiveShiftsPerHalfWave >> 1;

    const int32_t negativeShiftsPerHalfWave = -positiveShiftsPerHalfWave;
    const int32_t negativeShiftsPerQuarterWave = -positiveShiftsPerQuarterWave;

    /* We need to know how many shifts to make from one sample to the next.
     * shiftsPerSample = shiftsPerWave * wavesPerSecond / samplesPerSecond
     *                 = shiftsPerWave * frequency / sampleRate
     *                 = shiftsPerWave / sampleRate * frequency
     */
    const int32_t shiftsPerSample = (NOTE_FREQUENCY_TYPE)positiveShiftsPerFullWave 
                                  / (NOTE_FREQUENCY_TYPE)device->sampleRate
                                  * GET_NOTE_FREQUENCY(note);

    /* We need to know the maximum amplitude based on the volume percentage.
     * The percentage needs to be squared since we perceive loudness exponentially.
     */
    const int32_t maximumAmplitude = INT16_MAX
                                   * (prefs.pcmVolume * prefs.pcmVolume)
                                   / (100 * 100);

    /* We start, of course, with no shifts having been made yet. */
    int32_t currentShifts = 0;

    logMessage(LOG_DEBUG, "tone: msec=%u smct=%ld note=%u",
               duration, sampleCount, note);

    /* This loop iterates once per sample. */
    while (sampleCount > 0) {
      /* This loop iterates once per wave. */
      do {
        /* Start by calculating an amplitude that descends straight from
         * twice as high to twice as low as it should. This already makes
         * amplitudes within the second and third quarter waves correct.
         */
        int32_t amplitude = positiveShiftsPerHalfWave - currentShifts;

        /* Fix the amplitudes within the first and fourth quarter waves. */
        if (amplitude > positiveShiftsPerQuarterWave) {
          /* Amplitudes within the first quarter wave need to be flipped. */
          amplitude = positiveShiftsPerHalfWave - amplitude;
        } else if (amplitude < negativeShiftsPerQuarterWave) {
          /* Amplitudes within the fourth quarter wave need to be flipped. */
          amplitude = negativeShiftsPerHalfWave - amplitude;
        }

        /* Convert from our amplitude to a 16-bit audio amplitude. */
        amplitude >>= 12;

        /* Multiply the full amplitude by the maximum amplitude as set by
         * the currently set volume.
         */
        amplitude *= maximumAmplitude;

        /* Multiplying two 16-bit amplitudes together yields a 32-bit
         * amplitude which must be converted back to a 16-bit amplitude.
         */
        amplitude >>= 16;

        /* Clip the amplitude in case we've gone out of range. A known
         * case is the most positive amplitude being one value too high.
         */
        if (amplitude > INT16_MAX) {
          amplitude = INT16_MAX;
        } else if (amplitude < INT16_MIN) {
          amplitude = INT16_MIN;
        }

        if (!pcmWriteSample(device, amplitude)) return 0;
        sampleCount -= 1;
        currentShifts += shiftsPerSample;
      } while (currentShifts < positiveShiftsPerFullWave);

      /* Bring the current shift count back into range such that the next
       * wave will begin exactly where the previous one left off.
       */
      do {
        currentShifts -= positiveShiftsPerFullWave;
      } while (currentShifts >= positiveShiftsPerFullWave);
    }
  } else {
    /* generate silence */
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
