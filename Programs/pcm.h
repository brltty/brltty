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

#ifndef BRLTTY_INCLUDED_PCM
#define BRLTTY_INCLUDED_PCM

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  PCM_FMT_S8,
  PCM_FMT_U8,

  PCM_FMT_S16B,
  PCM_FMT_U16B,

  PCM_FMT_S16L,
  PCM_FMT_U16L,

  PCM_FMT_S16N,
  PCM_FMT_U16N,

  PCM_FMT_ULAW,
  PCM_FMT_ALAW,

  PCM_FMT_UNKNOWN
} PcmAmplitudeFormat;

#define PCM_MAX_SAMPLE_SIZE 2

typedef union {
  uint8_t bytes[PCM_MAX_SAMPLE_SIZE];
} PcmSample;

extern size_t makePcmSample (
  PcmSample *sample, int16_t amplitude, PcmAmplitudeFormat format
);

typedef size_t (*PcmSampleMaker) (PcmSample *sample, int16_t amplitude);
extern PcmSampleMaker getPcmSampleMaker (PcmAmplitudeFormat format);

typedef struct PcmDeviceStruct PcmDevice;

extern PcmDevice *openPcmDevice (int errorLevel, const char *device);
extern void closePcmDevice (PcmDevice *pcm);

extern int getPcmBlockSize (PcmDevice *pcm);

extern int getPcmSampleRate (PcmDevice *pcm);
extern int setPcmSampleRate (PcmDevice *pcm, int rate);

extern int getPcmChannelCount (PcmDevice *pcm);
extern int setPcmChannelCount (PcmDevice *pcm, int channels);

extern PcmAmplitudeFormat getPcmAmplitudeFormat (PcmDevice *pcm);
extern PcmAmplitudeFormat setPcmAmplitudeFormat (PcmDevice *pcm, PcmAmplitudeFormat format);

extern int writePcmData (PcmDevice *pcm, const unsigned char *buffer, int count);
extern void forcePcmOutput (PcmDevice *pcm);
extern void awaitPcmOutput (PcmDevice *pcm);
extern void cancelPcmOutput (PcmDevice *pcm);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PCM */
