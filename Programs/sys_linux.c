/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

char *
getBootParameters (void) {
  const char *path = "/proc/cmdline";
  FILE *file = fopen(path, "r");
  if (file) {
    char *parameters = strdupWrapper("");
    char buffer[0X1000];
    char *line = fgets(buffer, sizeof(buffer), file);
    if (line) {
      const char *prefix = "brltty=";
      const unsigned int prefixLength = strlen(prefix);
      char *token;
      while ((token = strtok(line, " \n"))) {
        line = NULL;
        if (memcmp(token, prefix, prefixLength) == 0) {
          free(parameters);
          parameters = strdupWrapper(token + prefixLength);
        }
      }
    }
    fclose(file);
    return parameters;
  }
  return NULL;
}

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
    int sampleRate;
    if (ioctl(descriptor, SOUND_PCM_READ_RATE, &sampleRate) != -1) return sampleRate;
  }
  return 8000;
}

int
getPcmChannelCount (int descriptor) {
  if (descriptor != -1) {
    int channelCount;
    if (ioctl(descriptor, SOUND_PCM_READ_CHANNELS, &channelCount) != -1) return channelCount;
  }
  return 1;
}

PcmAmplitudeFormat
getPcmAmplitudeFormat (int descriptor) {
  if (descriptor != -1) {
    int format = AFMT_QUERY;
    if (ioctl(descriptor, SNDCTL_DSP_SETFMT, &format) != -1) {
      switch (format) {
        default:          break;
        case AFMT_U8:     return PCM_FMT_U8;
        case AFMT_S8:     return PCM_FMT_S8;
        case AFMT_U16_BE: return PCM_FMT_U16B;
        case AFMT_S16_BE: return PCM_FMT_S16B;
        case AFMT_U16_LE: return PCM_FMT_U16L;
        case AFMT_S16_LE: return PCM_FMT_S16L;
        case AFMT_MU_LAW: return PCM_FMT_ULAW;
      }
    }
  }
  return PCM_FMT_UNKNOWN;
}
#endif /* ENABLE_PCM_TUNES */

#ifdef ENABLE_MIDI_TUNES
static MidiBufferFlusher flushMidiBuffer;
static int midiDevice;

#ifndef SAMPLE_TYPE_AWE
#  define SAMPLE_TYPE_AWE 0x20
#endif /* SAMPLE_TYPE_AWE */

int
getMidiDevice (int errorLevel, MidiBufferFlusher flushBuffer) {
  const char *path = "/dev/sequencer";
  int descriptor = open(path, O_WRONLY);
  if (descriptor == -1) {
    LogPrint(errorLevel, "Cannot open MIDI device: %s: %s", path, strerror(errno));
  } else {
    flushMidiBuffer = flushBuffer;

    {
      int count;
      int awe = -1;
      int fm = -1;
      int gus = -1;
      int ext = -1;

      if (ioctl(descriptor, SNDCTL_SEQ_NRSYNTHS, &count) != -1) {
        int index;
        for (index=0; index<count; ++index) {
          struct synth_info info;
          info.device = index;
          if (ioctl(descriptor, SNDCTL_SYNTH_INFO, &info) != -1) {
            switch (info.synth_type) {
              case SYNTH_TYPE_SAMPLE:
                switch (info.synth_subtype) {
                  case SAMPLE_TYPE_AWE:
                    awe = index;
                    continue;
                  case SAMPLE_TYPE_GUS:
                    gus = index;
                    continue;
                }
                break;
              case SYNTH_TYPE_FM:
                fm = index;
                continue;
            }
            LogPrint(LOG_DEBUG, "Unknown synthesizer: %d[%d]: %s",
                     info.synth_type, info.synth_subtype, info.name);
          } else {
            LogPrint(errorLevel, "Cannot get description for synthesizer %d: %s",
                     index, strerror(errno));
          }
        }

        if (gus >= 0)
          if (ioctl(descriptor, SNDCTL_SEQ_RESETSAMPLES, &gus) == -1)
            LogPrint(errorLevel, "Cannot reset samples for gus synthesizer %d: %s",
                     gus, strerror(errno));
      } else {
        LogPrint(errorLevel, "Cannot get MIDI synthesizer count: %s",
                 strerror(errno));
      }

      if (ioctl(descriptor, SNDCTL_SEQ_NRMIDIS, &count) != -1) {
        if (count > 0) ext = count - 1;
      } else {
        LogPrint(errorLevel, "Cannot get MIDI device count: %s",
                 strerror(errno));
      }

      midiDevice = (awe >= 0)? awe:
                   (gus >= 0)? gus:
                   (fm >= 0)? fm:
                   (ext >= 0)? ext:
                   0;
    }
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
setMidiInstrument (unsigned char channel, unsigned char instrument) {
  SEQ_SET_PATCH(midiDevice, channel, instrument);
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
startMidiNote (unsigned char channel, unsigned char note, unsigned char volume) {
  SEQ_START_NOTE(midiDevice, channel, note, 0X7F*volume/100);
}

void
stopMidiNote (unsigned char channel) {
  SEQ_STOP_NOTE(midiDevice, channel, 0, 0);
}

void
insertMidiWait (int duration) {
  SEQ_DELTA_TIME((duration + 9) / 10);
}
#endif /* ENABLE_MIDI_TUNES */

int
enablePorts (int errorLevel, unsigned short int base, unsigned short int count) {
#ifdef HAVE_SYS_IO_H
  if (ioperm(base, count, 1) != -1) return 1;
  LogPrint(errorLevel, "Port enable error: %u.%u: %s", base, count, strerror(errno));
#else /* HAVE_SYS_IO_H */
  LogPrint(errorLevel, "I/O ports not supported.");
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
#else /* HAVE_SYS_IO_H */
  return 0;
#endif /* HAVE_SYS_IO_H */
}

void
writePort1 (unsigned short int port, unsigned char value) {
#ifdef HAVE_SYS_IO_H
  outb(value, port);
#endif /* HAVE_SYS_IO_H */
}
