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

static void *alsa_library = NULL;

/* ALSA PCM function pointers */
#define PCM_ALSA_SYMBOL(name) static DEFINE_POINTER_TO(snd_##name, my_)
PCM_ALSA_SYMBOL(strerror);
PCM_ALSA_SYMBOL(pcm_open);
PCM_ALSA_SYMBOL(pcm_close);
PCM_ALSA_SYMBOL(pcm_hw_params_malloc);
PCM_ALSA_SYMBOL(pcm_hw_params_any);
PCM_ALSA_SYMBOL(pcm_hw_params_set_access);
PCM_ALSA_SYMBOL(pcm_hw_params_get_rate);
PCM_ALSA_SYMBOL(pcm_hw_params_get_format);
PCM_ALSA_SYMBOL(pcm_hw_params_set_format);
PCM_ALSA_SYMBOL(pcm_hw_params_get_rate_max);
PCM_ALSA_SYMBOL(pcm_hw_params_set_rate_near);
PCM_ALSA_SYMBOL(pcm_hw_params_get_channels);
PCM_ALSA_SYMBOL(pcm_hw_params_set_channels);
PCM_ALSA_SYMBOL(pcm_hw_params_set_buffer_time_near);
PCM_ALSA_SYMBOL(pcm_hw_params);
PCM_ALSA_SYMBOL(pcm_hw_params_get_sbits);
PCM_ALSA_SYMBOL(pcm_hw_params_get_period_size);
PCM_ALSA_SYMBOL(pcm_hw_params_free);
PCM_ALSA_SYMBOL(pcm_prepare);
PCM_ALSA_SYMBOL(pcm_writei);
PCM_ALSA_SYMBOL(pcm_drain);
PCM_ALSA_SYMBOL(pcm_resume);

static void
logPcmError (int level, const char *action, int code) {
  LogPrint(level, "ALSA PCM %s error: %s", action, my_snd_strerror(code));
}

PcmDevice *
openPcmDevice (int errorLevel, const char *device) {
  PcmDevice *pcm;

  if (!alsa_library) {
    if (!(alsa_library = loadSharedObject("libasound.so.2"))) {
      LogPrint(LOG_ERR, "Unable to load ALSA PCM library.");
      return NULL;
    }

    findSharedSymbol(alsa_library, "snd_strerror", &my_snd_strerror);
    findSharedSymbol(alsa_library, "snd_pcm_open", &my_snd_pcm_open);
    findSharedSymbol(alsa_library, "snd_pcm_close", &my_snd_pcm_close);
    findSharedSymbol(alsa_library, "snd_pcm_hw_params_malloc", &my_snd_pcm_hw_params_malloc);
    findSharedSymbol(alsa_library, "snd_pcm_hw_params_any", &my_snd_pcm_hw_params_any);
    findSharedSymbol(alsa_library, "snd_pcm_hw_params_set_access", &my_snd_pcm_hw_params_set_access);
    findSharedSymbol(alsa_library, "snd_pcm_hw_params_get_channels", &my_snd_pcm_hw_params_get_channels);
    findSharedSymbol(alsa_library, "snd_pcm_hw_params_set_channels", &my_snd_pcm_hw_params_set_channels);
    findSharedSymbol(alsa_library, "snd_pcm_hw_params_get_format", &my_snd_pcm_hw_params_get_format);
    findSharedSymbol(alsa_library, "snd_pcm_hw_params_set_format", &my_snd_pcm_hw_params_set_format);
    findSharedSymbol(alsa_library, "snd_pcm_hw_params_get_rate", &my_snd_pcm_hw_params_get_rate);
    findSharedSymbol(alsa_library, "snd_pcm_hw_params_get_rate_max", &my_snd_pcm_hw_params_get_rate_max);
    findSharedSymbol(alsa_library, "snd_pcm_hw_params_get_period_size", &my_snd_pcm_hw_params_get_period_size);
    findSharedSymbol(alsa_library, "snd_pcm_hw_params_set_rate_near", &my_snd_pcm_hw_params_set_rate_near);
    findSharedSymbol(alsa_library, "snd_pcm_hw_params_set_buffer_time_near", &my_snd_pcm_hw_params_set_buffer_time_near);
    findSharedSymbol(alsa_library, "snd_pcm_hw_params", &my_snd_pcm_hw_params);
    findSharedSymbol(alsa_library, "snd_pcm_hw_params_get_sbits", &my_snd_pcm_hw_params_get_sbits);
    findSharedSymbol(alsa_library, "snd_pcm_hw_params_free", &my_snd_pcm_hw_params_free);
    findSharedSymbol(alsa_library, "snd_pcm_prepare", &my_snd_pcm_prepare);
    findSharedSymbol(alsa_library, "snd_pcm_writei", &my_snd_pcm_writei);
    findSharedSymbol(alsa_library, "snd_pcm_drain", &my_snd_pcm_drain);
    findSharedSymbol(alsa_library, "snd_pcm_resume", &my_snd_pcm_resume);
  }

  if ((pcm = malloc(sizeof(*pcm)))) {
    int result;

    if (!device) device = "hw:0,0";
    if ((result = my_snd_pcm_open(&pcm->handle, device, SND_PCM_STREAM_PLAYBACK, 0)) >= 0) {
      if ((result = my_snd_pcm_hw_params_malloc(&pcm->hwparams)) >= 0) {
        int buffer_time = 1000000 / 4; /* usecs */
        int dir;

        if ((result = my_snd_pcm_hw_params_any(pcm->handle, pcm->hwparams)) == 0) {
          my_snd_pcm_hw_params_set_access(pcm->handle, pcm->hwparams,
                                        SND_PCM_ACCESS_RW_INTERLEAVED);
          {
            int rate;
            if ((result = my_snd_pcm_hw_params_get_rate_max(pcm->hwparams, &rate, 0)) == 0) {
              if ((result = my_snd_pcm_hw_params_set_rate_near(pcm->handle, pcm->hwparams, &rate, 0)) == 0) {
                LogPrint(LOG_DEBUG, "Using maximum rate of %d.", rate);
              } else {
                logPcmError(errorLevel, "set rate near", result);
              }
            } else {
              logPcmError(errorLevel, "get rate max", result);
            }
          }
          my_snd_pcm_hw_params_set_buffer_time_near(pcm->handle, pcm->hwparams, &buffer_time, &dir);
          setPcmAmplitudeFormat(pcm, PCM_FMT_S16L);
          my_snd_pcm_hw_params(pcm->handle, pcm->hwparams);

          return pcm;
        } else {
          logPcmError(errorLevel, "getting hardware parameters", result);
        }
      } else {
        logPcmError(errorLevel, "hardware parameters allocation", result);
      }

      my_snd_pcm_close(pcm->handle);
    } else {
      logPcmError(errorLevel, "open", result);
    }

    free(pcm);
  } else {
    LogError("PCM device allocation");
  }

  return NULL;
}

void
closePcmDevice (PcmDevice *pcm) {
  my_snd_pcm_close(pcm->handle);
  my_snd_pcm_hw_params_free(pcm->hwparams);
  free(pcm);
}

int
writePcmData (PcmDevice *pcm, const unsigned char *buffer, int count) {
  int frameSize = getPcmChannelCount(pcm) * (my_snd_pcm_hw_params_get_sbits(pcm->hwparams) / 8);
  int framesLeft = count / frameSize;

  while (framesLeft > 0) {
    int result;

    if ((result = my_snd_pcm_writei(pcm->handle, buffer, framesLeft)) > 0) {
      framesLeft -= result;
      buffer += result * frameSize;
    } else {
      switch (result) {
        case -EPIPE:
          if ((result = my_snd_pcm_prepare(pcm->handle)) < 0) {
            logPcmError(LOG_WARNING, "underrun recovery - prepare", result);
            return 0;
          }
          continue;

        case -ESTRPIPE:
          while ((result = my_snd_pcm_resume(pcm->handle)) == -EAGAIN) sleep(1);

          if (result < 0) {
            if ((result = my_snd_pcm_prepare(pcm->handle)) < 0) {
              logPcmError(LOG_WARNING, "resume - prepare", result);
              return 0;
            }
          }
          continue;
      }
    }
  }
  return 1;
}

int
getPcmBlockSize (PcmDevice *pcm) {
  snd_pcm_sframes_t buffer_size;
  int result, dir;

  if ((result = my_snd_pcm_hw_params_get_period_size(pcm->hwparams, &buffer_size,
                                                   &dir)) == 0) {
    return buffer_size;
  } else {
    logPcmError(LOG_ERR, "set rate near", result);
  }
  return 65535;
}

int
getPcmSampleRate (PcmDevice *pcm) {
  int rate, dir, err;
  err = my_snd_pcm_hw_params_get_rate(pcm->hwparams, &rate, &dir);
  if (err < 0) {
    LogPrint(LOG_ERR, "Sample rate %d unavailable: %s", rate, my_snd_strerror(err));
    rate = 8000;
  }
  return rate;
}

int
setPcmSampleRate (PcmDevice *pcm, int rate) {
  my_snd_pcm_hw_params_set_rate_near(pcm->handle, pcm->hwparams, &rate, 0);
  return rate;
}

int
getPcmChannelCount (PcmDevice *pcm) {
  int channels = my_snd_pcm_hw_params_get_channels(pcm->hwparams, 0);
  if (channels > 0) return channels;
  return 1;
}

int
setPcmChannelCount (PcmDevice *pcm, int channels) {
  int err;

  err = my_snd_pcm_hw_params_set_channels(pcm->handle, pcm->hwparams, channels);
  if (err < 0) {
    LogPrint(LOG_ERR, "Channels count (%i) not available for playback: %s",
             channels, my_snd_strerror(err));
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
  err = my_snd_pcm_hw_params_get_format(pcm->hwparams, &format);
  if (err < 0) {
    LogPrint(LOG_ERR, "Unable to set format: %s", my_snd_strerror(err));
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
  my_snd_pcm_hw_params_set_format(pcm->handle, pcm->hwparams, entry->external);
  return entry->external;
}

void
forcePcmOutput (PcmDevice *pcm) {
  my_snd_pcm_drain(pcm->handle);
}

void
awaitPcmOutput (PcmDevice *pcm) {
}

void
cancelPcmOutput (PcmDevice *pcm) {
}
