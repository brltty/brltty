/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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
#include <linux/kd.h>
#include <linux/soundcard.h>

#ifdef HAVE_FUNC_DLOPEN 
#  include <dlfcn.h>
#endif /* HAVE_FUNC_DLOPEN */

#ifdef HAVE_SYS_IO_H
#  include <sys/io.h>
#endif /* HAVE_SYS_IO_H */

#include "misc.h"
#include "system.h"

void *
loadSharedObject (const char *path) {
#ifdef HAVE_FUNC_DLOPEN 
  void *object = dlopen(path, RTLD_NOW|RTLD_GLOBAL);
  if (object) return object;
  LogPrint(LOG_ERR, "%s", dlerror());
#endif /* HAVE_FUNC_DLOPEN */
  return NULL;
}

void 
unloadSharedObject (void *object) {
#ifdef HAVE_FUNC_DLOPEN 
  dlclose(object);
#endif /* HAVE_FUNC_DLOPEN */
}

int 
findSharedSymbol (void *object, const char *symbol, const void **address) {
#ifdef HAVE_FUNC_DLOPEN 
  const char *error;
  *address = dlsym(object, symbol);
  if (!(error = dlerror())) return 1;
  LogPrint(LOG_ERR, "%s", error);
#endif /* HAVE_FUNC_DLOPEN */
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

#ifdef ENABLE_DAC_TUNES
int
getDacDevice (void) {
  const char *path = "/dev/dsp";
  int descriptor = open(path, O_WRONLY|O_NONBLOCK);
  if (descriptor == -1) {
    LogPrint(LOG_ERR, "Cannot open DAC device: %s: %s", path, strerror(errno));
  }
  return descriptor;
}

int
getDacBlockSize (int descriptor) {
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
getDacSampleRate (int descriptor) {
  if (descriptor != -1) {
    int sampleRate;
    if (ioctl(descriptor, SOUND_PCM_READ_RATE, &sampleRate) != -1) return sampleRate;
  }
  return 8000;
}

int
getDacChannelCount (int descriptor) {
  if (descriptor != -1) {
    int channelCount;
    if (ioctl(descriptor, SOUND_PCM_READ_CHANNELS, &channelCount) != -1) return channelCount;
  }
  return 1;
}

DacAmplitudeFormat
getDacAmplitudeFormat (int descriptor) {
  if (descriptor != -1) {
    int format = AFMT_QUERY;
    if (ioctl(descriptor, SNDCTL_DSP_SETFMT, &format) != -1) {
      switch (format) {
        default:          break;
        case AFMT_U8:     return DAC_FMT_U8;
        case AFMT_S8:     return DAC_FMT_S8;
        case AFMT_U16_BE: return DAC_FMT_U16B;
        case AFMT_S16_BE: return DAC_FMT_S16B;
        case AFMT_U16_LE: return DAC_FMT_U16L;
        case AFMT_S16_LE: return DAC_FMT_S16L;
        case AFMT_MU_LAW: return DAC_FMT_ULAW;
      }
    }
  }
  return DAC_FMT_UNKNOWN;
}
#endif /* ENABLE_DAC_TUNES */

#ifdef ENABLE_MIDI_TUNES
static MidiBufferFlusher flushMidiBuffer;
int
getMidiDevice (MidiBufferFlusher flushBuffer) {
  const char *path = "/dev/sequencer";
  int descriptor = open(path, O_WRONLY);
  if (descriptor == -1) {
    LogPrint(LOG_ERR, "Cannot open MIDI device: %s: %s", path, strerror(errno));
  } else {
    flushMidiBuffer = flushBuffer;
  }
  return descriptor;
}

SEQ_DEFINEBUF(0X80);
void
seqbuf_dump (void) {
  flushMidiBuffer(_seqbuf, _seqbufptr);
  _seqbufptr = 0;
}

void
setMidiInstrument (unsigned char device, unsigned char channel, unsigned char instrument) {
  SEQ_SET_PATCH(device, channel, instrument);
}

void
beginMidiBlock (int descriptor) {
  SEQ_START_TIMER();
}

void
endMidiBlock (int descriptor) {
  SEQ_STOP_TIMER();
  seqbuf_dump();
  ioctl(descriptor, SNDCTL_SEQ_SYNC);
}

void
startMidiNote (unsigned char device, unsigned char channel, unsigned char note) {
  SEQ_START_NOTE(device, channel, note, 0X7F);
}

void
stopMidiNote (unsigned char device, unsigned char channel, unsigned char note) {
  SEQ_STOP_NOTE(device, channel, note, 0X7F);
}

void
insertMidiWait (int duration) {
  SEQ_DELTA_TIME((duration + 9) / 10);
}
#endif /* ENABLE_MIDI_TUNES */

int
enablePorts (unsigned short int base, unsigned short int count) {
#ifdef HAVE_SYS_IO_H
  if (ioperm(base, count, 1) != -1) return 1;
  LogPrint(LOG_ERR, "Port enable error: %u.%u: %s", base, count, strerror(errno));
#endif /* HAVE_SYS_IO_H */
  return 0;
}

int
disablePorts (unsigned short int base, unsigned short int count) {
#ifdef HAVE_SYS_IO_H
  if (ioperm(base, count, 0) != -1) return 1;
  LogPrint(LOG_ERR, "Port disable error: %u.%u: %s", base, count, strerror(errno));
#endif /* HAVE_SYS_IO_H */
  return 0;
}

unsigned char
readPort1 (unsigned short int port) {
#ifdef HAVE_SYS_IO_H
  return inb(port);
#else
  return 0;
#endif /* HAVE_SYS_IO_H */
}

void
writePort1 (unsigned short int port, unsigned char value) {
#ifdef HAVE_SYS_IO_H
  outb(value, port);
#endif /* HAVE_SYS_IO_H */
}
