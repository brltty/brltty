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
#include <sys/ioctl.h>
#include <sys/audioio.h>
#include <sys/time.h>
#include <dev/wscons/wsconsio.h>

#ifdef HAVE_FUNC_DLOPEN 
#  include <dlfcn.h>
#endif /* HAVE_FUNC_DLOPEN */

#include "misc.h"
#include "system.h"

char *
getBootParameters (void) {
  return NULL;
}

#define SHARED_OBJECT_LOAD_FLAGS (DL_LAZY)
#include "sys_shlib_dlfcn.h"

int
canBeep (void) {
  if (getConsole() != -1) return 1;
  return 0;
}

int
timedBeep (unsigned short frequency, unsigned short milliseconds) {
  int console = getConsole();
  if (console != -1) {
    struct wskbd_bell_data bell;
    bell.which = WSKBD_BELL_DOALL;
    bell.pitch = frequency;
    bell.period = milliseconds;
    bell.volume = 100;
    if (!bell.period) {
      return 1;
    } else if (ioctl(console, WSKBDIO_COMPLEXBELL, &bell) != -1) {
      return 1;
    } else {
      LogPrint(LOG_WARNING, "ioctl WSKBDIO_COMPLEXBELL failed: %s", strerror(errno));
    }
  }
  return 0;
}

int
startBeep (unsigned short frequency) {
  return 0;
}

int
stopBeep (void) {
  int console = getConsole();
  if (console != -1) {
    struct wskbd_bell_data bell;
    bell.which = WSKBD_BELL_DOVOLUME | WSKBD_BELL_DOPERIOD;
    bell.volume = 0;
    bell.period = 0;
    if (ioctl(console, WSKBDIO_COMPLEXBELL, &bell) != -1) {
      return 1;
    } else {
      LogPrint(LOG_WARNING, "ioctl WSKBDIO_COMPLEXBELL failed: %s", strerror(errno));
    }
  }
  return 0;
}

#ifdef ENABLE_PCM_TUNES
int
getPcmDevice (int errorLevel) {
  int descriptor;
  const char *path = getenv("AUDIODEV");
  if (!path) path = "/dev/audio";
  if ((descriptor = open(path, O_WRONLY|O_NONBLOCK)) != -1) {
    audio_info_t info;
    AUDIO_INITINFO(&info);
    info.mode = AUMODE_PLAY;
    info.play.encoding = AUDIO_ENCODING_SLINEAR;
    info.play.sample_rate = 16000;
    info.play.channels = 1;
    info.play.precision = 16;
    info.play.gain = AUDIO_MAX_GAIN;
    if (ioctl(descriptor, AUDIO_SETINFO, &info) == -1)
      LogPrint(LOG_WARNING, "ioctl AUDIO_SETINFO failed: %s", strerror(errno));
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
	case AUDIO_ENCODING_SLINEAR_BE:
	  if (info.play.precision == 8) return PCM_FMT_S8;
	  if (info.play.precision == 16) return PCM_FMT_S16B;
	  break;
	case AUDIO_ENCODING_SLINEAR_LE:
	  if (info.play.precision == 8) return PCM_FMT_S8;
	  if (info.play.precision == 16) return PCM_FMT_S16L;
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
int
getMidiDevice (int errorLevel, MidiBufferFlusher flushBuffer) {
  LogPrint(errorLevel, "MIDI device not supported.");
  return -1;
}

void
setMidiInstrument (unsigned char channel, unsigned char instrument) {
}

void
beginMidiBlock (int descriptor) {
}

void
endMidiBlock (int descriptor) {
}

void
startMidiNote (unsigned char channel, unsigned char note, unsigned char volume) {
}

void
stopMidiNote (unsigned char channel) {
}

void
insertMidiWait (int duration) {
}
#endif /* ENABLE_MIDI_TUNES */

int
enablePorts (int errorLevel, unsigned short int base, unsigned short int count) {
  LogPrint(errorLevel, "I/O ports not supported.");
  return 0;
}

int
disablePorts (unsigned short int base, unsigned short int count) {
  return 0;
}

unsigned char
readPort1 (unsigned short int port) {
  return 0;
}

void
writePort1 (unsigned short int port, unsigned char value) {
}
