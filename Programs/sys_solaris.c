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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/audioio.h>

#ifdef HAVE_FUNC_DLOPEN 
#  include <dlfcn.h>
#endif /* HAVE_FUNC_DLOPEN */

#include "misc.h"
#include "system.h"

#include "sys_boot_none.h"

#define SHARED_OBJECT_LOAD_FLAGS (RTLD_NOW | RTLD_GLOBAL)
#include "sys_shlib_dlfcn.h"

#include "sys_beep_none.h"

#ifdef ENABLE_PCM_TUNES
int
getPcmDevice (int errorLevel) {
  int descriptor;
  const char *path = getenv("AUDIODEV");
  if (!path) path = "/dev/audio";
  if ((descriptor = open(path, O_WRONLY|O_NONBLOCK)) != -1) {
    audio_info_t info;
    AUDIO_INITINFO(&info);
    info.play.encoding = AUDIO_ENCODING_LINEAR;
    info.play.sample_rate = 16000;
    info.play.channels = 1;
    info.play.precision = 16;
    /* info.play.gain = AUDIO_MAX_GAIN; */
    ioctl(descriptor, AUDIO_SETINFO, &info);
  } else {
    LogPrint(errorLevel, "Cannot open PCM device: %s: %s", path, strerror(errno));
  }
  return descriptor;
}

int
getPcmBlockSize (int descriptor) {
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
	case AUDIO_ENCODING_LINEAR:
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
#endif /* ENABLE_PCM_TUNES */

#ifdef ENABLE_MIDI_TUNES
#include "sys_midi_none.h"
#endif /* ENABLE_MIDI_TUNES */

#include "sys_ports_none.h"
