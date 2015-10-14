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

#include "log.h"
#include "pcm.h"

#define UNSIGNED_TO_SIGNED amplitude += INT16_MIN

static size_t
makePcmSample_S8 (PcmSample *sample, int16_t amplitude) {
  sample->bytes[0] = amplitude >> 8;
  return 1;
}

static size_t
makePcmSample_U8 (PcmSample *sample, int16_t amplitude) {
  UNSIGNED_TO_SIGNED;
  return makePcmSample_S8(sample, amplitude);
}

static size_t
makePcmSample_S16B (PcmSample *sample, int16_t amplitude) {
  sample->bytes[0] = amplitude >> 8;
  sample->bytes[1] = amplitude;
  return 2;
}

static size_t
makePcmSample_U16B (PcmSample *sample, int16_t amplitude) {
  UNSIGNED_TO_SIGNED;
  return makePcmSample_S16B(sample, amplitude);
}

static size_t
makePcmSample_S16L (PcmSample *sample, int16_t amplitude) {
  sample->bytes[0] = amplitude;
  sample->bytes[1] = amplitude >> 8;
  return 2;
}

static size_t
makePcmSample_U16L (PcmSample *sample, int16_t amplitude) {
  UNSIGNED_TO_SIGNED;
  return makePcmSample_S16L(sample, amplitude);
}

static size_t
makePcmSample_S16N (PcmSample *sample, int16_t amplitude) {
  union {
    unsigned char *bytes;
    int16_t *s16;
  } overlay;

  overlay.bytes = sample->bytes;
  *overlay.s16 = amplitude;
  return 2;
}

static size_t
makePcmSample_U16N (PcmSample *sample, int16_t amplitude) {
  UNSIGNED_TO_SIGNED;
  return makePcmSample_S16N(sample, amplitude);
}

static size_t
makePcmSample_ULAW (PcmSample *sample, int16_t amplitude) {
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
  return 1;
}

static size_t
makePcmSample_ALAW (PcmSample *sample, int16_t amplitude) {
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
  return 1;
}

static size_t
makePcmSample_UNKNOWN (PcmSample *sample, int16_t amplitude) {
  return 0;
}

PcmSampleMaker
getPcmSampleMaker (PcmAmplitudeFormat format) {
#define PCM_SAMPLE_MAKER_ENTRY(format) [PCM_FMT_##format] = makePcmSample_##format

  static PcmSampleMaker const pcmSampleMakers[] = {
    PCM_SAMPLE_MAKER_ENTRY(S8),
    PCM_SAMPLE_MAKER_ENTRY(U8),

    PCM_SAMPLE_MAKER_ENTRY(S16B),
    PCM_SAMPLE_MAKER_ENTRY(U16B),

    PCM_SAMPLE_MAKER_ENTRY(S16L),
    PCM_SAMPLE_MAKER_ENTRY(U16L),

    PCM_SAMPLE_MAKER_ENTRY(S16N),
    PCM_SAMPLE_MAKER_ENTRY(U16N),

    PCM_SAMPLE_MAKER_ENTRY(ULAW),
    PCM_SAMPLE_MAKER_ENTRY(ALAW),

    PCM_SAMPLE_MAKER_ENTRY(UNKNOWN)
  };

  switch (format) {
#ifdef WORDS_BIGENDIAN
    case PCM_FMT_S16B:
      format = PCM_FMT_S16N;
      break;

    case PCM_FMT_U16B:
      format = PCM_FMT_U16N;
      break;
#else /* WORDS_BIGENDIAN */
    case PCM_FMT_S16L:
      format = PCM_FMT_S16N;
      break;

    case PCM_FMT_U16L:
      format = PCM_FMT_U16N;
      break;
#endif /* WORDS_BIGENDIAN */

    default:
      break;
  }

  if (format < ARRAY_COUNT(pcmSampleMakers)) {
    PcmSampleMaker sampleMaker = pcmSampleMakers[format];
    if (sampleMaker) return sampleMaker;
  }

  logMessage(LOG_WARNING, "unsupported PCM format: %d", format);
  return pcmSampleMakers[PCM_FMT_UNKNOWN];
}

size_t
makePcmSample (
  PcmSample *sample, int16_t amplitude, PcmAmplitudeFormat format
) {
  return getPcmSampleMaker(format)(sample, amplitude);
}
