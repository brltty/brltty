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
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include "pcm.h"

typedef struct {
  unsigned char size;
} PcmSampleEntry;

static const PcmSampleEntry pcmSampleTable[] = {
  [PCM_FMT_U8] = {
    .size = 1
  },

  [PCM_FMT_S8] = {
    .size = 1
  },

  [PCM_FMT_U16B] = {
    .size = 2
  },

  [PCM_FMT_S16B] = {
    .size = 2
  },

  [PCM_FMT_U16L] = {
    .size = 2
  },

  [PCM_FMT_S16L] = {
    .size = 2
  },

  [PCM_FMT_U16N] = {
    .size = 2
  },

  [PCM_FMT_S16N] = {
    .size = 2
  },

  [PCM_FMT_ULAW] = {
    .size = 1
  },

  [PCM_FMT_ALAW] = {
    .size = 1
  }
};

size_t
getPcmSampleLength (PcmAmplitudeFormat format) {
  return (format < ARRAY_COUNT(pcmSampleTable))? pcmSampleTable[format].size: 0;
}

size_t
makePcmSample (
  PcmAmplitudeFormat format, int16_t amplitude,
  void *buffer, size_t size
) {
  size_t length = getPcmSampleLength(format);

  if (size >= length) {
    typedef union {
      unsigned char bytes[4];
      int16_t s16N;
    } Sample;

    Sample *sample = buffer;
    const int U2S = 0X8000;

    switch (format) {
      case PCM_FMT_U8:
        amplitude += U2S;
      case PCM_FMT_S8:
        sample->bytes[0] = amplitude >> 8;
        break;

      case PCM_FMT_U16B:
        amplitude += U2S;
      case PCM_FMT_S16B:
        sample->bytes[0] = amplitude >> 8;
        sample->bytes[1] = amplitude;
        break;

      case PCM_FMT_U16L:
        amplitude += U2S;
      case PCM_FMT_S16L:
        sample->bytes[0] = amplitude;
        sample->bytes[1] = amplitude >> 8;
        break;

      case PCM_FMT_U16N:
        amplitude += U2S;
      case PCM_FMT_S16N:
        sample->s16N = amplitude;
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
        sample->bytes[0] = ~value;
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
        sample->bytes[0] = value ^ 0X55;
        break;
      }

      default:
        length = 0;
        break;
    }
  }

  return length;
}
