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

#include <alsa/asoundlib.h>

struct PcmDeviceStruct {
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *hwparams;
};

PcmDevice *
openPcmDevice (int errorLevel) {
  PcmDevice *pcm;
  if ((pcm = malloc(sizeof(*pcm)))) {
    int err;

    if ((err = snd_pcm_open(&pcm->handle, "plughw:0,0", SND_PCM_STREAM_PLAYBACK, 0)) >= 0) {
      int buffer_time = 1000000 / 4; /* usecs */
      int dir;
      snd_pcm_hw_params_alloca(&pcm->hwparams);
      snd_pcm_hw_params_any(pcm->handle, pcm->hwparams);
      snd_pcm_hw_params_set_access(pcm->handle, pcm->hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
      setPcmSampleRate(pcm, 44100);
      snd_pcm_hw_params_set_buffer_time_near(pcm->handle, pcm->hwparams, &buffer_time, &dir);
      snd_pcm_hw_params(pcm->handle, pcm->hwparams);
      return pcm;
    } else {
      LogPrint(errorLevel, "Cannot open PCM device: %s", snd_strerror(err));
    }
    free(pcm);
  } else {
    LogError("PCM device allocation");
  }
  return NULL;
}

void
closePcmDevice (PcmDevice *pcm) {
  snd_pcm_close(pcm->handle);
  free(pcm);
}

int
writePcmData (PcmDevice *pcm, const unsigned char *buffer, int count) {
  int samples = count / getPcmChannelCount(pcm);
  int written;
  written = snd_pcm_writei(pcm->handle, buffer, samples);
  return written == samples;
}

int
getPcmBlockSize (PcmDevice *pcm) {
  snd_pcm_sframes_t buffer_size;
  if (snd_pcm_hw_params_get_buffer_size(pcm->hwparams, &buffer_size) < 0) {
    return 65535;
  } else {
    return (int)buffer_size;
  }
}

int
getPcmSampleRate (PcmDevice *pcm) {
  int rate, dir, err;
  err = snd_pcm_hw_params_get_rate(pcm->hwparams, &rate, &dir);
  if (err < 0) {
    LogPrint(LOG_ERR, "Sample rate %d unavailable: %s", rate, snd_strerror(err));
    rate = 8000;
  }
  return rate;
}

int
setPcmSampleRate (PcmDevice *pcm, int rate) {
  snd_pcm_hw_params_set_rate_near(pcm->handle, pcm->hwparams, &rate, 0);
  return rate;
}

int
getPcmChannelCount (PcmDevice *pcm) {
  int channels = snd_pcm_hw_params_get_channels(pcm->hwparams, 0);
  if (channels > 0) return channels;
  return 1;
}

int
setPcmChannelCount (PcmDevice *pcm, int channels) {
  int err;

  err = snd_pcm_hw_params_set_channels(pcm->handle, pcm->hwparams, channels);
  if (err < 0) {
    LogPrint(LOG_ERR, "Channels count (%i) not available for playback: %s",
	     channels, snd_strerror(err));
    return 0;
  }
  return channels;
}

typedef struct {
  PcmAmplitudeFormat internal;
  snd_pcm_format_t external;
} AmplitudeFormatEntry;
static const AmplitudeFormatEntry amplitudeFormatTable[] = {
  {PCM_FMT_U8     , SND_PCM_FORMAT_U8     },
  {PCM_FMT_S8     , SND_PCM_FORMAT_S8     },
  {PCM_FMT_U16B   , SND_PCM_FORMAT_U16_BE },
  {PCM_FMT_S16B   , SND_PCM_FORMAT_S16_BE },
  {PCM_FMT_U16L   , SND_PCM_FORMAT_U16_LE },
  {PCM_FMT_S16L   , SND_PCM_FORMAT_S16_LE },
  {PCM_FMT_ULAW   , SND_PCM_FORMAT_MU_LAW },
  {PCM_FMT_UNKNOWN, SND_PCM_FORMAT_UNKNOWN}
};

PcmAmplitudeFormat
getPcmAmplitudeFormat (PcmDevice *pcm) {
  snd_pcm_format_t format;
  int err;
  err = snd_pcm_hw_params_get_format(pcm->hwparams, &format);
  if (err < 0) {
    LogPrint(LOG_ERR, "Unable to set format: %s", snd_strerror(err));
  } else {
    const AmplitudeFormatEntry *entry = amplitudeFormatTable;
    while (entry->internal != PCM_FMT_UNKNOWN) {
      if (entry->external == format) return entry->internal;
      ++entry;
    }
  }
  return PCM_FMT_UNKNOWN;
}

PcmAmplitudeFormat
setPcmAmplitudeFormat (PcmDevice *pcm, PcmAmplitudeFormat format) {
  const AmplitudeFormatEntry *entry = amplitudeFormatTable;
  while (entry->internal != PCM_FMT_UNKNOWN) {
    if (entry->internal == format) break;
    ++entry;
  }
  snd_pcm_hw_params_set_format(pcm->handle, pcm->hwparams, entry->external);
  return entry->external;
}

void
forcePcmOutput (PcmDevice *pcm) {
}

void
awaitPcmOutput (PcmDevice *pcm) {
}

void
cancelPcmOutput (PcmDevice *pcm) {
}
