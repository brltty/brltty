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

#ifdef HAVE_FUNC_SHL_LOAD
#  include <dl.h>
#endif /* HAVE_FUNC_SHL_LOAD */

#ifdef HAVE_HPUX_AUDIO
#  include <Alib.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#endif /* HAVE_HPUX_AUDIO */

#include "misc.h"
#include "system.h"

char *
getBootParameters (void) {
  return NULL;
}

void *
loadSharedObject (const char *path) {
#ifdef HAVE_FUNC_SHL_LOAD
  shl_t object = shl_load(path, BIND_IMMEDIATE|BIND_VERBOSE|DYNAMIC_PATH, 0L);
  if (object) return object;
  LogPrint(LOG_ERR, "Shared library '%s' not loaded: %s",
           path, strerror(errno));
#endif /* HAVE_HPUX_AUDIO */
  return NULL;
}

void 
unloadSharedObject (void *object) {
#ifdef HAVE_FUNC_SHL_LOAD
  if (shl_unload(object) == -1)
    LogPrint(LOG_ERR, "Shared library unload error: %s",
             strerror(errno));
#endif /* HAVE_HPUX_AUDIO */
}

int 
findSharedSymbol (void *object, const char *symbol, const void **address) {
#ifdef HAVE_FUNC_SHL_LOAD
  shl_t handle = object;
  if (shl_findsym(&handle, symbol, TYPE_UNDEFINED, address) != -1) return 1;
  LogPrint(LOG_ERR, "Shared symbol '%s' not found: %s",
           symbol, strerror(errno));
#endif /* HAVE_HPUX_AUDIO */
  return 0;
}

int
canBeep (void) {
  return 0;
}

int
timedBeep (unsigned short frequency, unsigned short milliseconds) {
  return 0;
}

int
startBeep (unsigned short frequency) {
  return 0;
}

int
stopBeep (void) {
  return 0;
}

#ifdef ENABLE_PCM_TUNES
#ifdef HAVE_HPUX_AUDIO
static Audio *audioServer = NULL;
static ATransID audioTransaction;
static SStream audioStream;

void
logAudioError (int level, long status, const char *action) {
  char message[132];
  AGetErrorText(audioServer, status, message, sizeof(message)-1);
  LogPrint(level, "%s error %ld: %s", action, status, message);
}

static const AudioAttributes *
getAudioAttributes (void) {
  return &audioStream.audio_attr;
}
#endif /* HAVE_HPUX_AUDIO */

int
getPcmDevice (int errorLevel) {
#ifdef HAVE_HPUX_AUDIO
  long status;
  AudioAttrMask mask = 0;
  AudioAttributes attributes;
  SSPlayParams parameters;

  if (!audioServer) {
    char *server = "";
    audioServer = AOpenAudio(server, &status);
    if (status != AENoError) {
      logAudioError(errorLevel, status, "AOpenAudio");
      audioServer = NULL;
      return -1;
    }
    LogPrint(LOG_DEBUG, "connected to audio server: %s", AAudioString(audioServer));

    ASetCloseDownMode(audioServer, AKeepTransactions, &status);
    if (status != AENoError) {
      logAudioError(errorLevel, status, "ASetCloseDownMode");
    }
  }

  memset(&attributes, 0, sizeof(attributes));

  parameters.gain_matrix = *ASimplePlayer(audioServer);
  parameters.play_volume = AUnityGain;
  parameters.priority = APriorityUrgent;
  parameters.event_mask = 0;

  audioTransaction = APlaySStream(audioServer, mask, &attributes, &parameters, &audioStream, &status);
  if (status == AENoError) {
    int descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (descriptor >= 0) {
      if (connect(descriptor, &audioStream.tcp_sockaddr, sizeof(audioStream.tcp_sockaddr)) != -1) {
        return descriptor;
      } else {
        LogError("socket connection");
      }
      close(descriptor);
    } else {
      LogError("socket creation");
    }
  } else {
    logAudioError(errorLevel, status, "APlaySStream");
  }
#endif /* HAVE_HPUX_AUDIO */
  return -1;
}

int
getPcmBlockSize (int descriptor) {
  int size = 0X100;
#ifdef HAVE_HPUX_AUDIO
  size = MIN(size, audioStream.max_block_size);
#endif /* HAVE_HPUX_AUDIO */
  return size;
}

int
getPcmSampleRate (int descriptor) {
#ifdef HAVE_HPUX_AUDIO
  return getAudioAttributes()->attr.sampled_attr.sampling_rate;
#else /* HAVE_HPUX_AUDIO */
  return 8000;
#endif /* HAVE_HPUX_AUDIO */
}

int
setPcmSampleRate (int descriptor, int rate) {
  return getPcmSampleRate(descriptor);
}

int
getPcmChannelCount (int descriptor) {
#ifdef HAVE_HPUX_AUDIO
  return getAudioAttributes()->attr.sampled_attr.channels;
#else /* HAVE_HPUX_AUDIO */
  return 1;
#endif /* HAVE_HPUX_AUDIO */
}

int
setPcmChannelCount (int descriptor, int channels) {
  return getPcmChannelCount(descriptor);
}

PcmAmplitudeFormat
getPcmAmplitudeFormat (int descriptor) {
#ifdef HAVE_HPUX_AUDIO
  switch (getAudioAttributes()->attr.sampled_attr.data_format) {
    default:
      break;
    case ADFLin8:
      return PCM_FMT_S8;
    case ADFLin8Offset:
      return PCM_FMT_U8;
    case ADFLin16:
      return PCM_FMT_S16B;
    case ADFMuLaw:
      return PCM_FMT_ULAW;
  }
#endif /* HAVE_HPUX_AUDIO */
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
