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

int
getPcmDevice (int errorLevel) {
  int descriptor;
  const char *path = getenv("AUDIODEV");
  if (!path) path = "/dev/audio";
  if ((descriptor = open(path, O_WRONLY|O_NONBLOCK)) != -1) {
    audio_info_t info;
    AUDIO_INITINFO(&info);
#ifdef AUMODE_PLAY
    info.mode = AUMODE_PLAY;
#endif /* AUMODE_PLAY */
#ifdef AUDIO_ENCODING_SLINEAR
    info.play.encoding = AUDIO_ENCODING_SLINEAR;
#else /* AUDIO_ENCODING_SLINEAR */
    info.play.encoding = AUDIO_ENCODING_LINEAR;
#endif /* AUDIO_ENCODING_SLINEAR */
    info.play.sample_rate = 16000;
    info.play.channels = 1;
    info.play.precision = 16;
    info.play.gain = AUDIO_MAX_GAIN;
    if (ioctl(descriptor, AUDIO_SETINFO, &info) == -1)
      LogPrint(errorLevel, "Cannot set audio info: %s", strerror(errno));
  } else {
    LogPrint(errorLevel, "Cannot open PCM device: %s: %s", path, strerror(errno));
  }
  return descriptor;
}

int
getPcmBlockSize (int descriptor) {
  if (descriptor != -1) {
    audio_info_t info;
    if (ioctl(descriptor, AUDIO_GETINFO, &info) != -1) return info.play.buffer_size;
  }
  return 0X100;
}

int
getPcmSampleRate (int descriptor) {
  if (descriptor != -1) {
    audio_info_t info;
    if (ioctl(descriptor, AUDIO_GETINFO, &info) != -1) return info.play.sample_rate;
  }
  return 8000;
}

int
setPcmSampleRate (int descriptor, int rate) {
  return getPcmSampleRate(descriptor);
}

int
getPcmChannelCount (int descriptor) {
  if (descriptor != -1) {
    audio_info_t info;
    if (ioctl(descriptor, AUDIO_GETINFO, &info) != -1) return info.play.channels;
  }
  return 1;
}

int
setPcmChannelCount (int descriptor, int channels) {
  return getPcmChannelCount(descriptor);
}

PcmAmplitudeFormat
getPcmAmplitudeFormat (int descriptor) {
  if (descriptor != -1) {
    audio_info_t info;
    if (ioctl(descriptor, AUDIO_GETINFO, &info) != -1) {
      switch (info.play.encoding) {
        default:
          break;
#ifdef AUDIO_ENCODING_SLINEAR
        case AUDIO_ENCODING_SLINEAR_LE:
          if (info.play.precision == 8) return PCM_FMT_S8;
          if (info.play.precision == 16) return PCM_FMT_S16L;
          break;
        case AUDIO_ENCODING_SLINEAR_BE:
#else /* AUDIO_ENCODING_SLINEAR */
        case AUDIO_ENCODING_LINEAR:
#endif /* AUDIO_ENCODING_SLINEAR */
          if (info.play.precision == 8) return PCM_FMT_S8;
          if (info.play.precision == 16) return PCM_FMT_S16B;
          break;
        case AUDIO_ENCODING_ULAW:
          return PCM_FMT_ULAW;
        case AUDIO_ENCODING_LINEAR8:
          return PCM_FMT_U8;
      }
    }
  }
  return PCM_FMT_UNKNOWN;
}

PcmAmplitudeFormat
setPcmAmplitudeFormat (int descriptor, PcmAmplitudeFormat format) {
  return getPcmAmplitudeFormat(descriptor);
}

void
forcePcmOutput (int descriptor) {
}

void
awaitPcmOutput (int descriptor) {
}

void
cancelPcmOutput (int descriptor) {
}
