/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* Mikropuhe/speech.c - Speech library
 * For the Mikropuhe text to speech package
 * Maintained by Dave Mielke <dave@mielke.cc>
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include "log.h"
#include "file.h"
#include "parse.h"
#include "thread.h"
#include "queue.h"
#include "notes.h"
#include "pcm.h"
#include "dynld.h"
#include "core.h"

typedef enum {
  PARM_NAME,
  PARM_PITCH
} DriverParameter;
#define SPKPARMS "name", "pitch"

#include "spk_driver.h"
#include <mpwrfile.h>

#ifdef ENABLE_SHARED_OBJECTS
static void *speechLibrary = NULL;
#endif /* ENABLE_SHARED_OBJECTS */

static MPINT_ChannelInitExType mpChannelInitEx = NULL;
static MPINT_ChannelExitType mpChannelExit = NULL;
static MPINT_ChannelSpeakFileType mpChannelSpeakFile = NULL;

typedef struct {
  const char *name;
  void *address;
} SymbolEntry;
#define SYMBOL_ENTRY(name) {"MPINT_" #name, &mp##name}
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
static PcmDevice *pcm = NULL;

static int
openSoundDevice (void) {
  if (!pcm) {
    if (!(pcm = openPcmDevice(LOG_WARNING, opt_pcmDevice))) return 0;

    speechParameters.nChannels = setPcmChannelCount(pcm, 1);
    speechParameters.nSampleFreq = setPcmSampleRate(pcm, 22050);
    {
      typedef struct {
        PcmAmplitudeFormat internal;
        int external;
      } FormatEntry;
      static const FormatEntry formatTable[] = {
        {PCM_FMT_S16L   , 16},
        {PCM_FMT_U8     ,  8},
        {PCM_FMT_UNKNOWN,  0}
      };
      const FormatEntry *format = formatTable;
      while (format->internal != PCM_FMT_UNKNOWN) {
        if (setPcmAmplitudeFormat(pcm, format->internal) == format->internal) break;
        ++format;
      }
      if (format->internal == PCM_FMT_UNKNOWN) {
        logMessage(LOG_WARNING, "No supported sound format.");
        closePcmDevice(pcm);
        pcm = NULL;
        return 0;
      }
      speechParameters.nBits = format->external;
    }
    logMessage(LOG_DEBUG, "Mikropuhe audio configuration: channels=%d rate=%d bits=%d",
               speechParameters.nChannels, speechParameters.nSampleFreq, speechParameters.nBits);
  }
  return 1;
}

static void
closeSoundDevice (void) {
  if (pcm) {
    closePcmDevice(pcm);
    pcm = NULL;
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
deallocateSpeechItem (void *item, void *data) {
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
  logMessage(LOG_ERR, "Mikropuhe %s error: %s", action, explanation);
}

static int
writeSound (const void *bytes, unsigned int count, void *data, void *reserved) {
  if (synthesisThreadStarted) {
    if (writePcmData(pcm, bytes, count)) return 0;
    logSystemError("Mikropuhe write");
    return MPINT_ERR_GENERAL;
  }
  return 1;
}

static int
synthesizeSpeech (const unsigned char *bytes, int length, int tags) {
  speechParameters.nTags = tags;
  if (mpChannelSpeakFile) {
    int error;
    char string[length+1];
    memcpy(string, bytes, length);
    string[length] = 0;
    if (!(error = mpChannelSpeakFile(speechChannel, string, NULL, &speechParameters))) return 1;
    if (error != 1) logSynthesisError(error, "channel speak");
  }
  return 0;
}

static void
synthesizeSpeechSegment (const SpeechSegment *segment) {
  synthesizeSpeech((unsigned char *)(segment+1), segment->length, segment->tags);
}

static void
synthesizeSpeechSegments (void) {
  SpeechSegment *segment;
  while ((segment = dequeueItem(speechQueue))) {
    if (segment->tags) {
      synthesizeSpeechSegment(segment);
    } else if (synthesisThreadStarted && openSoundDevice()) {
      pthread_mutex_unlock(&speechMutex);
      synthesizeSpeechSegment(segment);
      pthread_mutex_lock(&speechMutex);
    }
    deallocateSpeechSegment(segment);
  }
}

static int
awaitSpeechSegment (void) {
  while (synthesisThreadStarted) {
    int error;

    if (pcm) {
      struct timeval now;
      struct timespec timeout;
      gettimeofday(&now, NULL);
      timeout.tv_sec = now.tv_sec + 3;
      timeout.tv_nsec = now.tv_usec * 1000;
      error = pthread_cond_timedwait(&speechConditional, &speechMutex, &timeout);
    } else {
      error = pthread_cond_wait(&speechConditional, &speechMutex);
    }

    switch (error) {
      case 0:
        return 1;

      case ETIMEDOUT:
        closeSoundDevice();
        continue;

      default:
        logSystemError("pthread_cond_timedwait");
        return 0;
    }
  }
  return 0;
}

THREAD_FUNCTION(mpSpeechSynthesisThread) {
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
  pthread_mutex_lock(&speechMutex);

  while (synthesisThreadStarted) {
    synthesizeSpeechSegments();
    awaitSpeechSegment();
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
        error = createThread("driver-speech-Mikropuhe",
                                  &synthesisThread, &attributes,
                                  mpSpeechSynthesisThread, NULL);
        pthread_attr_destroy(&attributes);
        if (!error) {
          return 1;
        } else {
          logMessage(LOG_ERR, "Cannot create speech thread: %s", strerror(error));
        }
      } else {
        logMessage(LOG_ERR, "Cannot initialize speech thread attributes: %s", strerror(error));
      }

      pthread_cond_destroy(&speechConditional);
    } else {
      logMessage(LOG_ERR, "Cannot initialize speech conditional: %s", strerror(error));
    }

    pthread_mutex_destroy(&speechMutex);
  } else {
    logMessage(LOG_ERR, "Cannot initialize speech mutex: %s", strerror(error));
  }
  synthesisThreadStarted = 0;
  return 0;
}

static void
stopSynthesisThread (void) {
  if (synthesisThreadStarted) {
    synthesisThreadStarted = 0;
    pthread_mutex_lock(&speechMutex);
    pthread_cond_signal(&speechConditional);
    pthread_mutex_unlock(&speechMutex);
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
      if (element) {
        pthread_cond_signal(&speechConditional);
        pthread_mutex_unlock(&speechMutex);
        return 1;
      }
      pthread_mutex_unlock(&speechMutex);
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
  return enqueueSpeech((unsigned char *)tag, strlen(tag), MPINT_TAGS_OWN|MPINT_TAGS_SAPI5);
}

static void
loadSynthesisLibrary (void) {
#ifdef ENABLE_SHARED_OBJECTS
  if (!speechLibrary) {
    static const char *name = "libmplinux." LIBRARY_EXTENSION;
    char *path = makePath(MIKROPUHE_ROOT, name);
    if (path) {
      if ((speechLibrary = loadSharedObject(path))) {
        const SymbolEntry *symbol = symbolTable;
        while (symbol->name) {
          void **address = symbol->address;
          if (findSharedSymbol(speechLibrary, symbol->name, address)) {
            logMessage(LOG_DEBUG, "Mikropuhe symbol: %s -> %p",
                       symbol->name, *address);
          } else {
            logMessage(LOG_ERR, "Mikropuhe symbol not found: %s", symbol->name);
          }
          ++symbol;
        }
      } else {
        logMessage(LOG_ERR, "Mikropuhe library not loaded: %s", path);
      }
      free(path);
    }
  }
#endif /* ENABLE_SHARED_OBJECTS */
}

static void
spk_say (volatile SpeechSynthesizer *spk, const unsigned char *buffer, size_t length, size_t count, const unsigned char *attributes) {
  if (enqueueText(buffer, length))
    enqueueTag("<break time=\"none\"/>");
}

static void
spk_mute (volatile SpeechSynthesizer *spk) {
  stopSynthesisThread();
  if (pcm) cancelPcmOutput(pcm);
}

static void
spk_setVolume (volatile SpeechSynthesizer *spk, unsigned char setting) {
  char tag[0X40];
  unsigned int percentage = getIntegerSpeechVolume(setting, 100);
  snprintf(tag, sizeof(tag), "<volume level=\"%d\"/>",
           MAX(percentage, 1));
  enqueueTag(tag);
}

static void
spk_setRate (volatile SpeechSynthesizer *spk, unsigned char setting) {
  char tag[0X40];
  snprintf(tag, sizeof(tag), "<rate absspeed=\"%d\"/>",
           getIntegerSpeechRate(setting, 10)-10);
  enqueueTag(tag);
}

static int
spk_construct (volatile SpeechSynthesizer *spk, char **parameters) {
  int code;

  spk->setVolume = spk_setVolume;
  spk->setRate = spk_setRate;

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
            if (validateInteger(&setting, pitch, &minimum, &maximum)) {
              char tag[0X100];
              snprintf(tag, sizeof(tag), "<pitch absmiddle=\"%d\"/>", setting);
              enqueueTag(tag);
            } else {
              logMessage(LOG_WARNING, "%s: %s", "invalid pitch specification", pitch);
            }
          }
        }

        return 1;
      } else {
        logSynthesisError(code, "channel initialization");
      }
    }
  } else {
    logMessage(LOG_ERR, "Cannot allocate speech queue.");
  }

  spk_destruct(spk);
  return 0;
}

static void
spk_destruct (volatile SpeechSynthesizer *spk) {
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

#ifdef ENABLE_SHARED_OBJECTS
  if (speechLibrary) {
    const SymbolEntry *symbol = symbolTable;
    while (symbol->name) {
      void **address = (symbol++)->address;
      *address = NULL;
    }

    unloadSharedObject(speechLibrary);
    speechLibrary = NULL;
  }
#endif /* ENABLE_SHARED_OBJECTS */
}
