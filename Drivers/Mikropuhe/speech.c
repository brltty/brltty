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

/* Mikropuhe/speech.c - Speech library
 * For the Mikropuhe text to speech package
 * Maintained by Dave Mielke <dave@mielke.cc>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

#include "Programs/misc.h"
#include "Programs/system.h"
#include "Programs/queue.h"

typedef enum {
  PARM_NAME,
  PARM_PITCH
} DriverParameter;
#define SPKPARMS "name", "pitch"

#define SPK_HAVE_RATE
#define SPK_HAVE_VOLUME
#include "Programs/spk_driver.h"
#include <mpwrfile.h>

static void *speechLibrary = NULL;
static MPINT_ChannelInitExType mpChannelInitEx = NULL;
static MPINT_ChannelExitType mpChannelExit = NULL;
static MPINT_ChannelSpeakFileType mpChannelSpeakFile = NULL;

typedef struct {
  const char *name;
  const void **address;
} SymbolEntry;
#define SYMBOL_ENTRY(name) {"MPINT_" #name, (const void **)&mp##name}
static const SymbolEntry symbolTable[] = {
  SYMBOL_ENTRY(ChannelInitEx),
  SYMBOL_ENTRY(ChannelExit),
  SYMBOL_ENTRY(ChannelSpeakFile),
  {NULL, NULL}
};

typedef struct {
  int tags;
  int length;
} SpeechSegment;

static Queue *speechQueue = NULL;
static pthread_mutex_t speechMutex;
static pthread_cond_t speechConditional;

static int synthesisThreadStarted = 0;
static pthread_t synthesisThread;

static void *speechChannel = NULL;
static MPINT_SpeakFileParams speechParameters;
static volatile int soundDevice = -1;

static int
openSoundDevice (void) {
  if (soundDevice == -1) {
    if ((soundDevice = getPcmDevice(LOG_WARNING)) == -1) return 0;
    LogPrint(LOG_DEBUG, "Sound device opened: fd=%d", soundDevice);

    speechParameters.nChannels = setPcmChannelCount(soundDevice, 1);
    speechParameters.nSampleFreq = setPcmSampleRate(soundDevice, 22050);
    {
      static const PcmAmplitudeFormat formats[] = {PCM_FMT_S16L, PCM_FMT_U8, PCM_FMT_UNKNOWN};
      const PcmAmplitudeFormat *format = formats;
      while (*format != PCM_FMT_UNKNOWN) {
        if (setPcmAmplitudeFormat(soundDevice, *format) == *format) break;
        ++format;
      }

      switch (*format) {
        case PCM_FMT_U8:
          speechParameters.nBits = 8;
          break;
        case PCM_FMT_S16L:
          speechParameters.nBits = 16;
          break;
        default:
          LogPrint(LOG_WARNING, "No supported sound format.");
          close(soundDevice);
          soundDevice = -1;
          return 0;
      }
    }
    LogPrint(LOG_DEBUG, "Mikropuhe audio configuration: channels=%d rate=%d bits=%d",
             speechParameters.nChannels, speechParameters.nSampleFreq, speechParameters.nBits);
  }
  return 1;
}

static void
closeSoundDevice (void) {
  if (soundDevice != -1) {
    close(soundDevice);
    LogPrint(LOG_DEBUG, "Sound device closed: fd=%d", soundDevice);
    soundDevice = -1;
  }
}

static SpeechSegment *
allocateSpeechSegment (const unsigned char *bytes, int length, int tags) {
  SpeechSegment *segment;
  if ((segment = malloc(sizeof(*segment) + length))) {
    memcpy(segment+1, bytes, length);
    segment->length = length;
    segment->tags = tags;
  }
  return segment;
}

static void
deallocateSpeechSegment (SpeechSegment *segment) {
  free(segment);
}

static void
deallocateSpeechItem (void *item) {
  deallocateSpeechSegment(item);
}

static void
logSynthesisError (int code, const char *action) {
  const char *explanation;
  switch (code) {
    default:
      explanation = "unknown";
      break;
    case MPINT_ERR_GENERAL:
      explanation = "general";
      break;
    case MPINT_ERR_SYNTH:
      explanation = "text synthesis";
      break;
    case MPINT_ERR_MEM:
      explanation = "insufficient memory";
      break;
    case MPINT_ERR_DESTFILEOPEN:
      explanation = "file open";
      break;
    case MPINT_ERR_DESTFILEWRITE:
      explanation = "file write";
      break;
    case MPINT_ERR_EINVAL:
      explanation = "parameter";
      break;
    case MPINT_ERR_INITBADKEY:
      explanation = "invalid key";
      break;
    case MPINT_ERR_INITNOVOICES:
      explanation = "no voices";
      break;
    case MPINT_ERR_INITVOICEFAIL:
      explanation = "voice load";
      break;
    case MPINT_ERR_INITTEXTPARSE:
      explanation = "text parser load";
      break;
    case MPINT_ERR_SOUNDCARD:
      explanation = "sound device";
      break;
  }
  LogPrint(LOG_ERR, "Mikropuhe %s error: %s", action, explanation);
}

static int
writeSound (const void *bytes, unsigned int count, void *data, void *reserved) {
  if (synthesisThreadStarted) {
    int written = safe_write(soundDevice, bytes, count);
    if (written == count) return 0;

    if (written == -1) {
      LogError("Mikropuhe write");
    } else {
      LogPrint(LOG_ERR, "Mikropuhe incomplete write: %d < %d", written, count);
    }
    return MPINT_ERR_GENERAL;
  }
  return 1;
}

static int
synthesizeSpeech (const unsigned char *bytes, int length, int tags) {
  speechParameters.nTags = tags;
  if (mpChannelSpeakFile) {
    int code;
    char string[length+1];
    memcpy(string, bytes, length);
    string[length] = 0;
    if (!(code = mpChannelSpeakFile(speechChannel, string, NULL, &speechParameters))) return 1;
    if (code != 1) logSynthesisError(code, "channel speak");
  }
  return 0;
}

static void *
speechMain (void *data) {
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
  pthread_mutex_lock(&speechMutex);
  while (synthesisThreadStarted) {
    int error;
    SpeechSegment *segment;

    if (soundDevice != -1) {
      struct timeval now;
      struct timespec timeout;
      gettimeofday(&now, NULL);
      timeout.tv_sec = now.tv_sec + 3;
      timeout.tv_nsec = 0;
      error = pthread_cond_timedwait(&speechConditional, &speechMutex, &timeout);
    } else {
      error = pthread_cond_wait(&speechConditional, &speechMutex);
    }
    switch (error) {
      case 0:
        break;
      case ETIMEDOUT:
        closeSoundDevice();
        continue;
      default:
        LogError("pthread_cond_timedwait");
        break;
    }

    while ((segment = dequeueItem(speechQueue))) {
      if (segment->tags) {
        synthesizeSpeech((unsigned char *)(segment+1), segment->length, segment->tags);
      } else if (synthesisThreadStarted && openSoundDevice()) {
        pthread_mutex_unlock(&speechMutex);
        synthesizeSpeech((unsigned char *)(segment+1), segment->length, segment->tags);
        pthread_mutex_lock(&speechMutex);
      }
      deallocateSpeechSegment(segment);
    }
  }
  pthread_mutex_unlock(&speechMutex);
  return NULL;
}

static int
startSynthesisThread (void) {
  int error;
  if (synthesisThreadStarted) return 1;

  synthesisThreadStarted = 1;
  if (!(error = pthread_mutex_init(&speechMutex, NULL))) {
    if (!(error = pthread_cond_init(&speechConditional, NULL))) {
      pthread_attr_t attributes;
      if (!(error = pthread_attr_init(&attributes))) {
        pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE);
        error = pthread_create(&synthesisThread, &attributes, speechMain, NULL);
        pthread_attr_destroy(&attributes);
        if (!error) {
          return 1;
        } else {
          LogPrint(LOG_ERR, "Cannot create speech thread: %s", strerror(error));
        }
      } else {
        LogPrint(LOG_ERR, "Cannot initialize speech thread attributes: %s", strerror(error));
      }

      pthread_cond_destroy(&speechConditional);
    } else {
      LogPrint(LOG_ERR, "Cannot initialize speech conditional: %s", strerror(error));
    }

    pthread_mutex_destroy(&speechMutex);
  } else {
    LogPrint(LOG_ERR, "Cannot initialize speech mutex: %s", strerror(error));
  }
  synthesisThreadStarted = 0;
  return 0;
}

static void
stopSynthesisThread (void) {
  if (synthesisThreadStarted) {
    synthesisThreadStarted = 0;
    pthread_cond_signal(&speechConditional);
    pthread_join(synthesisThread, NULL);
    pthread_cond_destroy(&speechConditional);
    pthread_mutex_destroy(&speechMutex);
  }
}

static int
enqueueSpeech (const unsigned char *bytes, int length, int tags) {
  if (startSynthesisThread()) {
    SpeechSegment *segment;
    if ((segment = allocateSpeechSegment(bytes, length, tags))) {
      Element *element;
      pthread_mutex_lock(&speechMutex);
      element = enqueueItem(speechQueue, segment);
      pthread_mutex_unlock(&speechMutex);
      if (element) {
        pthread_cond_signal(&speechConditional);
        return 1;
      }
      deallocateSpeechSegment(segment);
    }
  }
  return 0;
}

static int
enqueueText (const unsigned char *bytes, int length) {
  return enqueueSpeech(bytes, length, 0);
}

static int
enqueueTag (const char *tag) {
  return enqueueSpeech(tag, strlen(tag), MPINT_TAGS_OWN|MPINT_TAGS_SAPI5);
}

static void
loadSynthesisLibrary (void) {
  if (!speechLibrary) {
    static const char *name = "libmplinux." LIBRARY_EXTENSION;
    char *path = makePath(MIKROPUHE_ROOT, name);
    if (path) {
      if ((speechLibrary = loadSharedObject(path))) {
        const SymbolEntry *symbol = symbolTable;
        while (symbol->name) {
          if (findSharedSymbol(speechLibrary, symbol->name, symbol->address)) {
            LogPrint(LOG_DEBUG, "Mikropuhe symbol: %s -> %p",
                     symbol->name, *symbol->address);
          } else {
            LogPrint(LOG_ERR, "Mikropuhe symbol not found: %s", symbol->name);
          }
          ++symbol;
        }
      } else {
        LogPrint(LOG_ERR, "Mikropuhe library not loaded: %s", path);
      }
      free(path);
    }
  }
}

static void
spk_identify (void) {
  LogPrint(LOG_NOTICE, "Mikropuhe text to speech engine.");
}

static int
spk_open (char **parameters) {
  int code;
  loadSynthesisLibrary();

  if ((speechQueue = newQueue(deallocateSpeechItem, NULL))) {
    if (mpChannelInitEx) {
      if (!(code = mpChannelInitEx(&speechChannel, NULL, NULL, NULL))) {
        memset(&speechParameters, 0, sizeof(speechParameters));
        speechParameters.nWriteWavHeader = 0;
        speechParameters.pfnWrite = writeSound;
        speechParameters.pWriteData = NULL;

        {
          const char *name = parameters[PARM_NAME];
          if (name && *name) {
            char tag[0X100];
            snprintf(tag, sizeof(tag), "<voice name=\"%s\"/>", name);
            enqueueTag(tag);
          }
        }

        {
          const char *pitch = parameters[PARM_PITCH];
          if (pitch && *pitch) {
            int setting = 0;
            static const int minimum = -10;
            static const int maximum = 10;
            if (validateInteger(&setting, "pitch", pitch, &minimum, &maximum)) {
              char tag[0X100];
              snprintf(tag, sizeof(tag), "<pitch absmiddle=\"%d\"/>", setting);
              enqueueTag(tag);
            }
          }
        }

        return 1;
      } else {
        logSynthesisError(code, "channel initialization");
      }
    }
  } else {
    LogPrint(LOG_ERR, "Cannot allocate speech queue.");
  }

  spk_close();
  return 0;
}

static void
spk_close (void) {
  stopSynthesisThread();
  closeSoundDevice();

  if (speechQueue) {
    deallocateQueue(speechQueue);
    speechQueue = NULL;
  }

  if (speechChannel) {
    if (mpChannelExit) mpChannelExit(speechChannel, NULL, 0);
    speechChannel = NULL;
  }

  if (speechLibrary) {
    const SymbolEntry *symbol = symbolTable;
    while (symbol->name)
      *(symbol++)->address = NULL;

    unloadSharedObject(speechLibrary);
    speechLibrary = NULL;
  }
}

static void
spk_say (const unsigned char *buffer, int length) {
  if (enqueueText(buffer, length))
    enqueueTag("<break time=\"none\"/>");
}

static void
spk_mute (void) {
  stopSynthesisThread();
  if (soundDevice != -1) cancelPcmOutput(soundDevice);
}

static void
spk_rate (int setting) {
  char tag[0X40];
  snprintf(tag, sizeof(tag), "<rate absspeed=\"%d\"/>",
           setting * 10 / SPK_DEFAULT_RATE - 10);
  enqueueTag(tag);
}

static void
spk_volume (int setting) {
  char tag[0X40];
  int percentage = setting * 50 / SPK_DEFAULT_VOLUME;
  snprintf(tag, sizeof(tag), "<volume level=\"%d\"/>",
           MIN(100, MAX(1, percentage)));
  enqueueTag(tag);
}
