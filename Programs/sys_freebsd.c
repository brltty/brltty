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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/kbio.h>
#include <sys/soundcard.h>

#include "misc.h"
#include "system.h"

char *
getBootParameters (void) {
  return NULL;
}

void *
loadSharedObject (const char *path) {
  void *object = dlopen(path, RTLD_NOW|RTLD_GLOBAL);
  if (object) return object;
  LogPrint(LOG_ERR, "%s", dlerror());
  return NULL;
}

void 
unloadSharedObject (void *object) {
  dlclose(object);
}

int 
findSharedSymbol (void *object, const char *symbol, const void **address) {
  const char *error;
  *address = dlsym(object, symbol);
  if (!(error = dlerror())) return 1;
  LogPrint(LOG_ERR, "%s", error);
  return 0;
}

#define BEEP_DIVIDEND 1193180

int
canBeep (void) {
  if (getConsole() != -1) return 1;
  return 0;
}

int
timedBeep (unsigned short frequency, unsigned short milliseconds) {
  int console = getConsole();
  if (console != -1) {
    if (ioctl(console, KDMKTONE, ((milliseconds << 0X10) | (BEEP_DIVIDEND / frequency))) != -1) return 1;
    LogError("ioctl KDMKTONE");
  }
  return 0;
}

int
startBeep (unsigned short frequency) {
  int console = getConsole();
  if (console != -1) {
    if (ioctl(console, KIOCSOUND, BEEP_DIVIDEND/frequency) != -1) return 1;
    LogError("ioctl KIOCSOUND");
  }
  return 0;
}

int
stopBeep (void) {
  int console = getConsole();
  if (console != -1) {
    if (ioctl(console, KIOCSOUND, 0) != -1) return 1;
    LogError("ioctl KIOCSOUND");
  }
  return 0;
}

#ifdef ENABLE_PCM_TUNES
int
getPcmDevice (int errorLevel) {
  const char *path = "/dev/dsp";
  int descriptor = open(path, O_WRONLY|O_NONBLOCK);
  if (descriptor == -1) LogPrint(errorLevel, "Cannot open PCM device: %s: %s", path, strerror(errno));
  return descriptor;
}

int
getPcmBlockSize (int descriptor) {
  int fragmentCount = (1 << 0X10) - 1;
  int fragmentShift = 7;
  int fragmentSize = 1 << fragmentShift;
  int fragmentSetting = (fragmentCount << 0X10) | fragmentShift;
  if (descriptor != -1) {
    int blockSize;
    ioctl(descriptor, SNDCTL_DSP_SETFRAGMENT, &fragmentSetting);
    if (ioctl(descriptor, SNDCTL_DSP_GETBLKSIZE, &blockSize) != -1) return blockSize;
  }
  return fragmentSize;
}

int
getPcmSampleRate (int descriptor) {
  if (descriptor != -1) {
    int rate;
    if (ioctl(descriptor, SOUND_PCM_READ_RATE, &rate) != -1) return rate;
  }
  return 8000;
}

int
setPcmSampleRate (int descriptor, int rate) {
  if (descriptor != -1) {
    if (ioctl(descriptor, SOUND_PCM_WRITE_RATE, &rate) != -1) return rate;
  }
  return getPcmSampleRate(descriptor);
}

int
getPcmChannelCount (int descriptor) {
  if (descriptor != -1) {
    int channels;
    if (ioctl(descriptor, SOUND_PCM_READ_CHANNELS, &channels) != -1) return channels;
  }
  return 1;
}

int
setPcmChannelCount (int descriptor, int channels) {
  if (descriptor != -1) {
    if (ioctl(descriptor, SOUND_PCM_WRITE_CHANNELS, &channels) != -1) return channels;
  }
  return getPcmChannelCount(descriptor);
}

typedef struct {
  PcmAmplitudeFormat internal;
  int external;
} AmplitudeFormatEntry;
static const AmplitudeFormatEntry amplitudeFormatTable[] = {
  {PCM_FMT_U8     , AFMT_U8    },
  {PCM_FMT_S8     , AFMT_S8    },
  {PCM_FMT_U16B   , AFMT_U16_BE},
  {PCM_FMT_S16B   , AFMT_S16_BE},
  {PCM_FMT_U16L   , AFMT_U16_LE},
  {PCM_FMT_S16L   , AFMT_S16_LE},
  {PCM_FMT_ULAW   , AFMT_MU_LAW},
  {PCM_FMT_UNKNOWN, AFMT_QUERY }
};

static PcmAmplitudeFormat
doPcmAmplitudeFormat (int descriptor, int format) {
  if (descriptor != -1) {
    if (ioctl(descriptor, SNDCTL_DSP_SETFMT, &format) != -1) {
      const AmplitudeFormatEntry *entry = amplitudeFormatTable;
      while (entry->internal != PCM_FMT_UNKNOWN) {
        if (entry->external == format) return entry->internal;
        ++entry;
      }
    }
  }
  return PCM_FMT_UNKNOWN;
}

PcmAmplitudeFormat
getPcmAmplitudeFormat (int descriptor) {
  return doPcmAmplitudeFormat(descriptor, AFMT_QUERY);
}

PcmAmplitudeFormat
setPcmAmplitudeFormat (int descriptor, PcmAmplitudeFormat format) {
  const AmplitudeFormatEntry *entry = amplitudeFormatTable;
  while (entry->internal != PCM_FMT_UNKNOWN) {
    if (entry->internal == format) break;
    ++entry;
  }
  return doPcmAmplitudeFormat(descriptor, entry->external);
}

void
forcePcmOutput (int descriptor) {
  ioctl(descriptor, SNDCTL_DSP_POST, 0);
}

void
awaitPcmOutput (int descriptor) {
  ioctl(descriptor, SNDCTL_DSP_SYNC, 0);
}

void
cancelPcmOutput (int descriptor) {
  ioctl(descriptor, SNDCTL_DSP_RESET, 0);
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
