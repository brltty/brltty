/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

struct PcmDeviceStruct {
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *hardwareParameters;
  unsigned int channelCount;
  unsigned int sampleRate;
  unsigned int bufferTime;
  unsigned int periodTime;
};

static void *alsaLibrary;

/* ALSA PCM function pointers */
#define PCM_ALSA_SYMBOL(name) static DEFINE_POINTER_TO(snd_##name, my_)
PCM_ALSA_SYMBOL(strerror);
PCM_ALSA_SYMBOL(pcm_open);
PCM_ALSA_SYMBOL(pcm_close);
PCM_ALSA_SYMBOL(pcm_nonblock);
PCM_ALSA_SYMBOL(pcm_hw_params_malloc);
PCM_ALSA_SYMBOL(pcm_hw_params_any);
PCM_ALSA_SYMBOL(pcm_hw_params_set_access);
PCM_ALSA_SYMBOL(pcm_hw_params_get_rate);
PCM_ALSA_SYMBOL(pcm_hw_params_get_sbits);
PCM_ALSA_SYMBOL(pcm_hw_params_get_format);
PCM_ALSA_SYMBOL(pcm_hw_params_set_format);
PCM_ALSA_SYMBOL(pcm_hw_params_get_rate_min);
PCM_ALSA_SYMBOL(pcm_hw_params_get_rate_max);
PCM_ALSA_SYMBOL(pcm_hw_params_set_rate_near);
PCM_ALSA_SYMBOL(pcm_hw_params_get_channels);
PCM_ALSA_SYMBOL(pcm_hw_params_get_channels_min);
PCM_ALSA_SYMBOL(pcm_hw_params_get_channels_max);
PCM_ALSA_SYMBOL(pcm_hw_params_set_channels_near);
PCM_ALSA_SYMBOL(pcm_hw_params_set_buffer_time_near);
PCM_ALSA_SYMBOL(pcm_hw_params_set_period_time_near);
PCM_ALSA_SYMBOL(pcm_hw_params);
PCM_ALSA_SYMBOL(pcm_hw_params_get_sbits);
PCM_ALSA_SYMBOL(pcm_hw_params_get_period_size);
PCM_ALSA_SYMBOL(pcm_hw_params_free);
PCM_ALSA_SYMBOL(pcm_prepare);
PCM_ALSA_SYMBOL(pcm_writei);
PCM_ALSA_SYMBOL(pcm_drain);
PCM_ALSA_SYMBOL(pcm_drop);
PCM_ALSA_SYMBOL(pcm_resume);

static void
logPcmError (int level, const char *action, int code) {
  LogPrint(level, "ALSA PCM %s error: %s", action, my_snd_strerror(code));
}

static int
configurePcmSampleFormat (PcmDevice *pcm, int errorLevel) {
  static const snd_pcm_format_t formats[] = {
    SND_PCM_FORMAT_S16, SND_PCM_FORMAT_U16,
    SND_PCM_FORMAT_U8, SND_PCM_FORMAT_S8,
    SND_PCM_FORMAT_MU_LAW,
    SND_PCM_FORMAT_UNKNOWN
  };
  const snd_pcm_format_t *format = formats;

  while (*format != SND_PCM_FORMAT_UNKNOWN) {
    int result = my_snd_pcm_hw_params_set_format(pcm->handle, pcm->hardwareParameters, *format);
    if (result >= 0) return 1;

    if (result != -EINVAL) {
      logPcmError(errorLevel, "set format", result);
      return 0;
    }

    ++format;
  }

  LogPrint(errorLevel, "Unsupported PCM sample format.");
  return 0;
}

static int
configurePcmSampleRate (PcmDevice *pcm, int errorLevel) {
  int result;
  unsigned int minimum;
  unsigned int maximum;

  if ((result = my_snd_pcm_hw_params_get_rate_min(pcm->hardwareParameters, &minimum, NULL)) < 0) {
    logPcmError(errorLevel, "get rate min", result);
    return 0;
  }

  if ((result = my_snd_pcm_hw_params_get_rate_max(pcm->hardwareParameters, &maximum, NULL)) < 0) {
    logPcmError(errorLevel, "get rate max", result);
    return 0;
  }

  if ((minimum > maximum) || (minimum < 1)) {
    LogPrint(errorLevel, "Invalid PCM rate range: %u-%u", minimum, maximum);
    return 0;
  }

  pcm->sampleRate = maximum;
  if ((result = my_snd_pcm_hw_params_set_rate_near(pcm->handle, pcm->hardwareParameters, &pcm->sampleRate, NULL)) < 0) {
    logPcmError(errorLevel, "set rate near", result);
    return 0;
  }

  return 1;
}

static int
configurePcmChannelCount (PcmDevice *pcm, int errorLevel) {
  int result;
  unsigned int minimum;
  unsigned int maximum;

  if ((result = my_snd_pcm_hw_params_get_channels_min(pcm->hardwareParameters, &minimum)) < 0) {
    logPcmError(errorLevel, "get channels min", result);
    return 0;
  }

  if ((result = my_snd_pcm_hw_params_get_channels_max(pcm->hardwareParameters, &maximum)) < 0) {
    logPcmError(errorLevel, "get channels max", result);
    return 0;
  }

  if ((minimum > maximum) || (minimum < 1)) {
    LogPrint(errorLevel, "Invalid PCM channel range: %u-%u", minimum, maximum);
    return 0;
  }

  pcm->channelCount = minimum;
  if ((result = my_snd_pcm_hw_params_set_channels_near(pcm->handle, pcm->hardwareParameters, &pcm->channelCount)) < 0) {
    logPcmError(errorLevel, "set channels near", result);
    return 0;
  }

  return 1;
}

PcmDevice *
openPcmDevice (int errorLevel, const char *device) {
  PcmDevice *pcm;

  if (!alsaLibrary) {
    if (!(alsaLibrary = loadSharedObject("libasound.so.2"))) {
      LogPrint(LOG_ERR, "Unable to load ALSA PCM library.");
      return NULL;
    }

    findSharedSymbol(alsaLibrary, "snd_strerror", &my_snd_strerror);
    findSharedSymbol(alsaLibrary, "snd_pcm_open", &my_snd_pcm_open);
    findSharedSymbol(alsaLibrary, "snd_pcm_close", &my_snd_pcm_close);
    findSharedSymbol(alsaLibrary, "snd_pcm_nonblock", &my_snd_pcm_nonblock);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_malloc", &my_snd_pcm_hw_params_malloc);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_any", &my_snd_pcm_hw_params_any);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_set_access", &my_snd_pcm_hw_params_set_access);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_get_channels", &my_snd_pcm_hw_params_get_channels);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_get_channels_min", &my_snd_pcm_hw_params_get_channels_min);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_get_channels_max", &my_snd_pcm_hw_params_get_channels_max);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_set_channels_near", &my_snd_pcm_hw_params_set_channels_near);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_get_format", &my_snd_pcm_hw_params_get_format);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_set_format", &my_snd_pcm_hw_params_set_format);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_get_rate", &my_snd_pcm_hw_params_get_rate);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_get_sbits", &my_snd_pcm_hw_params_get_sbits);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_get_rate_min", &my_snd_pcm_hw_params_get_rate_min);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_get_rate_max", &my_snd_pcm_hw_params_get_rate_max);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_get_period_size", &my_snd_pcm_hw_params_get_period_size);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_set_rate_near", &my_snd_pcm_hw_params_set_rate_near);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_set_buffer_time_near", &my_snd_pcm_hw_params_set_buffer_time_near);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_set_period_time_near", &my_snd_pcm_hw_params_set_period_time_near);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params", &my_snd_pcm_hw_params);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_get_sbits", &my_snd_pcm_hw_params_get_sbits);
    findSharedSymbol(alsaLibrary, "snd_pcm_hw_params_free", &my_snd_pcm_hw_params_free);
    findSharedSymbol(alsaLibrary, "snd_pcm_prepare", &my_snd_pcm_prepare);
    findSharedSymbol(alsaLibrary, "snd_pcm_writei", &my_snd_pcm_writei);
    findSharedSymbol(alsaLibrary, "snd_pcm_drain", &my_snd_pcm_drain);
    findSharedSymbol(alsaLibrary, "snd_pcm_drop", &my_snd_pcm_drop);
    findSharedSymbol(alsaLibrary, "snd_pcm_resume", &my_snd_pcm_resume);
  }

  if ((pcm = malloc(sizeof(*pcm)))) {
    int result;

    if (!device) device = "hw:0,0";
    if ((result = my_snd_pcm_open(&pcm->handle, device,
                                  SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) >= 0) {
      my_snd_pcm_nonblock(pcm->handle, 0);

      if ((result = my_snd_pcm_hw_params_malloc(&pcm->hardwareParameters)) >= 0) {
        if ((result = my_snd_pcm_hw_params_any(pcm->handle, pcm->hardwareParameters)) >= 0) {
          if ((result = my_snd_pcm_hw_params_set_access(pcm->handle, pcm->hardwareParameters, SND_PCM_ACCESS_RW_INTERLEAVED)) >= 0) {
            if (configurePcmSampleFormat(pcm, errorLevel)) {
              if (configurePcmSampleRate(pcm, errorLevel)) {
                if (configurePcmChannelCount(pcm, errorLevel)) {
                  pcm->bufferTime = 500000;
                  if ((result = my_snd_pcm_hw_params_set_buffer_time_near(pcm->handle, pcm->hardwareParameters, &pcm->bufferTime, NULL)) >= 0) {
                    pcm->periodTime = pcm->bufferTime / 8;
                    if ((result = my_snd_pcm_hw_params_set_period_time_near(pcm->handle, pcm->hardwareParameters, &pcm->periodTime, NULL)) >= 0) {
                      if ((result = my_snd_pcm_hw_params(pcm->handle, pcm->hardwareParameters)) >= 0) {
                        LogPrint(LOG_DEBUG, "ALSA PCM: Chan=%u Rate=%u BufTim=%u PerTim=%u", pcm->channelCount, pcm->sampleRate, pcm->bufferTime, pcm->periodTime);
                        return pcm;
                      } else {
                        logPcmError(errorLevel, "set hardware parameters", result);
                      }
                    } else {
                      logPcmError(errorLevel, "set period time near", result);
                    }
                  } else {
                    logPcmError(errorLevel, "set buffer time near", result);
                  }
                }
              }
            }
          } else {
            logPcmError(errorLevel, "set access", result);
          }
        } else {
          logPcmError(errorLevel, "get hardware parameters", result);
        }

        my_snd_pcm_hw_params_free(pcm->hardwareParameters);
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
  awaitPcmOutput(pcm);
  my_snd_pcm_close(pcm->handle);
  my_snd_pcm_hw_params_free(pcm->hardwareParameters);
  free(pcm);
}

static int
getPcmFrameSize (PcmDevice *pcm) {
  return getPcmChannelCount(pcm) * (my_snd_pcm_hw_params_get_sbits(pcm->hardwareParameters) / 8);
}

int
writePcmData (PcmDevice *pcm, const unsigned char *buffer, int count) {
  int frameSize = getPcmFrameSize(pcm);
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
          while ((result = my_snd_pcm_resume(pcm->handle)) == -EAGAIN) approximateDelay(1);

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
  snd_pcm_uframes_t frames;
  int result;

  if ((result = my_snd_pcm_hw_params_get_period_size(pcm->hardwareParameters, &frames, NULL)) >= 0) {
    return frames * getPcmFrameSize(pcm);
  } else {
    logPcmError(LOG_ERR, "get period size", result);
  }
  return 65535;
}

int
getPcmSampleRate (PcmDevice *pcm) {
  return pcm->sampleRate;
}

int
setPcmSampleRate (PcmDevice *pcm, int rate) {
  int result;

  pcm->sampleRate = rate;
  if ((result = my_snd_pcm_hw_params_set_rate_near(pcm->handle, pcm->hardwareParameters, &pcm->sampleRate, NULL)) < 0) {
    logPcmError(LOG_ERR, "set rate near", result);
  }

  return getPcmSampleRate(pcm);
}

int
getPcmChannelCount (PcmDevice *pcm) {
  return pcm->channelCount;
}

int
setPcmChannelCount (PcmDevice *pcm, int channels) {
  int result;

  pcm->channelCount = channels;
  if ((result = my_snd_pcm_hw_params_set_channels_near(pcm->handle, pcm->hardwareParameters, &pcm->channelCount)) < 0) {
    logPcmError(LOG_ERR, "set channels near", result);
  }

  return getPcmChannelCount(pcm);
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
  int result;

  if ((result = my_snd_pcm_hw_params_get_format(pcm->hardwareParameters, &format)) < 0) {
    logPcmError(LOG_ERR, "get format", result);
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
  int result;

  while (entry->internal != PCM_FMT_UNKNOWN) {
    if (entry->internal == format) break;
    ++entry;
  }

  if ((result = my_snd_pcm_hw_params_set_format(pcm->handle, pcm->hardwareParameters, entry->external)) < 0) {
    logPcmError(LOG_ERR, "set format", result);
    return getPcmAmplitudeFormat(pcm);
  }

  return entry->internal;
}

void
forcePcmOutput (PcmDevice *pcm) {
}

void
awaitPcmOutput (PcmDevice *pcm) {
  int result;
  if ((result = my_snd_pcm_drain(pcm->handle)) < 0) logPcmError(LOG_WARNING, "drain", result);
}

void
cancelPcmOutput (PcmDevice *pcm) {
  int result;
  if ((result = my_snd_pcm_drop(pcm->handle)) < 0) logPcmError(LOG_WARNING, "drop", result);
}
