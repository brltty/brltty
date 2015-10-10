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

#include "pcm.h"

size_t
makePcmSample (
  PcmSample *sample, int16_t amplitude, PcmAmplitudeFormat format
) {
  union {
    unsigned char *bytes;
    int16_t *s16;
  } overlay;

  const int16_t U2S = INT16_MIN;
  overlay.bytes = sample->bytes;

  switch (format) {
    case PCM_FMT_U8:
      amplitude += U2S;
    case PCM_FMT_S8:
      overlay.bytes[0] = amplitude >> 8;
      return 1;

    case PCM_FMT_U16B:
      amplitude += U2S;
    case PCM_FMT_S16B:
      overlay.bytes[0] = amplitude >> 8;
      overlay.bytes[1] = amplitude;
      return 2;

    case PCM_FMT_U16L:
      amplitude += U2S;
    case PCM_FMT_S16L:
      overlay.bytes[0] = amplitude;
      overlay.bytes[1] = amplitude >> 8;
      return 2;

    case PCM_FMT_U16N:
      amplitude += U2S;
    case PCM_FMT_S16N:
      *overlay.s16 = amplitude;
      return 2;

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

      overlay.bytes[0] = ~value;
      return 1;
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

      overlay.bytes[0] = value ^ 0X55;
      return 1;
    }

    default:
      return 0;
  }
}
