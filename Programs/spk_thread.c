/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>

#include "log.h"
#include "spk_thread.h"
#include "async_event.h"
#include "async_thread.h"
#include "async_wait.h"

#ifdef ENABLE_SPEECH_SUPPORT
#define DRIVER_THREAD_START_TIMEOUT 15000
#define DRIVER_THREAD_STOP_TIMEOUT 5000
#define SPEECH_REQUEST_WAIT_DURATION 1000000
#define SPEECH_RESPONSE_WAIT_TIMEOUT 5000

typedef enum {
  THD_CONSTRUCTING,
  THD_STARTING,
  THD_READY,
  THD_STOPPING,
  THD_FINISHED
} ThreadState;

typedef struct {
  const char *name;
} ThreadStateEntry;

static const ThreadStateEntry threadStateTable[] = {
  [THD_CONSTRUCTING] = {
    .name = "constructing"
  },

  [THD_STARTING] = {
    .name = "starting"
  },

  [THD_READY] = {
    .name = "ready"
  },

  [THD_STOPPING] = {
    .name = "stopping"
  },

  [THD_FINISHED] {
    .name = "finished"
  },
};

static inline const ThreadStateEntry *
getThreadStateEntry (ThreadState state) {
  if (state >= ARRAY_COUNT(threadStateTable)) return NULL;
  return &threadStateTable[state];
}

typedef enum {
  RSP_PENDING,
  RSP_INTEGER
} SpeechResponseType;

struct SpeechDriverThreadStruct {
  ThreadState threadState;
  pthread_t threadIdentifier;

  SpeechSynthesizer *speechSynthesizer;
  char **driverParameters;

  AsyncEvent *requestEvent;

  struct {
    AsyncEvent *event;
    SpeechResponseType type;

    union {
      int INTEGER;
    } value;
  } response;
};

typedef enum {
  REQ_SAY_TEXT,
  REQ_MUTE_SPEECH,

  REQ_DO_TRACK,
  REQ_GET_TRACK,
  REQ_IS_SPEAKING,

  REQ_SET_VOLUME,
  REQ_SET_RATE,
  REQ_SET_PITCH,
  REQ_SET_PUNCTUATION
} SpeechRequestType;

typedef struct {
  SpeechRequestType type;

  union {
    struct {
      const unsigned char *text;
      size_t length;
      size_t count;
      const unsigned char *attributes;
    } sayText;

    struct {
      unsigned char setting;
    } setVolume;

    struct {
      unsigned char setting;
    } setRate;

    struct {
      unsigned char setting;
    } setPitch;

    struct {
      SpeechPunctuation setting;
    } setPunctuation;
  } arguments;

  unsigned char data[0];
} SpeechRequest;

typedef struct {
  const void *address;
  size_t size;
  unsigned end:1;
} SpeechRequesdtatum;

#define BEGIN_SPEECH_REQUEST_DATA SpeechRequesdtatum data[] = {
#define END_SPEECH_REQUEST_DATA {.end=1} };

static void
setThreadState (SpeechDriverThread *sdt, ThreadState state) {
  const ThreadStateEntry *entry = getThreadStateEntry(state);
  const char *name = entry? entry->name: NULL;

  if (!name) name = "?";
  logMessage(LOG_CATEGORY(SPEECH_EVENTS), "driver thread %s", name);
  sdt->threadState = state;
}

static inline void
setResponsePending (SpeechDriverThread *sdt) {
  sdt->response.type = RSP_PENDING;
}

static int
sendIntegerResponse (SpeechDriverThread *sdt, int value) {
  sdt->response.type = RSP_INTEGER;
  sdt->response.value.INTEGER = value;
  return asyncSignalEvent(sdt->response.event, NULL);
}

ASYNC_EVENT_CALLBACK(handleSpeechRequest) {
  SpeechDriverThread *sdt = parameters->eventData;
  SpeechRequest *req = parameters->signalData;

  if (req) {
    switch (req->type) {
      case REQ_SAY_TEXT: {
        speech->say(
          sdt->speechSynthesizer,
          req->arguments.sayText.text, req->arguments.sayText.length,
          req->arguments.sayText.count, req->arguments.sayText.attributes
        );

        sendIntegerResponse(sdt, 1);
        break;
      }

      case REQ_MUTE_SPEECH: {
        speech->mute(sdt->speechSynthesizer);

        sendIntegerResponse(sdt, 1);
        break;
      }

      case REQ_DO_TRACK: {
        speech->doTrack(sdt->speechSynthesizer);

        sendIntegerResponse(sdt, 1);
        break;
      }

      case REQ_GET_TRACK: {
        int result = speech->getTrack(sdt->speechSynthesizer);

        sendIntegerResponse(sdt, result);
        break;
      }

      case REQ_IS_SPEAKING: {
        int result = speech->isSpeaking(sdt->speechSynthesizer);

        sendIntegerResponse(sdt, result);
        break;
      }

      case REQ_SET_VOLUME: {
        speech->setVolume(
          sdt->speechSynthesizer,
          req->arguments.setVolume.setting
        );

        sendIntegerResponse(sdt, 1);
        break;
      }

      case REQ_SET_RATE: {
        speech->setRate(
          sdt->speechSynthesizer,
          req->arguments.setRate.setting
        );

        sendIntegerResponse(sdt, 1);
        break;
      }

      case REQ_SET_PITCH: {
        speech->setPitch(
          sdt->speechSynthesizer,
          req->arguments.setPitch.setting
        );

        sendIntegerResponse(sdt, 1);
        break;
      }

      case REQ_SET_PUNCTUATION: {
        speech->setPunctuation(
          sdt->speechSynthesizer,
          req->arguments.setPunctuation.setting
        );

        sendIntegerResponse(sdt, 1);
        break;
      }

      default:
        logMessage(LOG_CATEGORY(SPEECH_EVENTS), "unimplemented speech request type: %u", req->type);
        sendIntegerResponse(sdt, 0);
        break;
    }

    free(req);
  } else {
    setThreadState(sdt, THD_STOPPING);
  }
}

ASYNC_CONDITION_TESTER(testDriverThreadStopping) {
  SpeechDriverThread *sdt = data;

  return sdt->threadState == THD_STOPPING;
}

ASYNC_THREAD_FUNCTION(runSpeechDriverThread) {
  SpeechDriverThread *sdt = argument;

  setThreadState(sdt, THD_STARTING);

  if ((sdt->requestEvent = asyncNewEvent(handleSpeechRequest, sdt))) {
    if (speech->construct(sdt->speechSynthesizer, sdt->driverParameters)) {
      setThreadState(sdt, THD_READY);
      sendIntegerResponse(sdt, 1);

      while (!asyncAwaitCondition(SPEECH_REQUEST_WAIT_DURATION,
                                  testDriverThreadStopping, sdt)) {
      }

      speech->destruct(sdt->speechSynthesizer);
      sdt->speechSynthesizer = NULL;
    } else {
      logMessage(LOG_CATEGORY(SPEECH_EVENTS), "driver construction failure");
    }

    asyncDiscardEvent(sdt->requestEvent);
    sdt->requestEvent = NULL;
  } else {
    logMessage(LOG_CATEGORY(SPEECH_EVENTS), "request event construction failure");
  }

  {
    int ok = sdt->threadState = THD_STOPPING;

    setThreadState(sdt, THD_FINISHED);
    sendIntegerResponse(sdt, ok);
  }

  return NULL;
}

ASYNC_CONDITION_TESTER(testSpeechResponseReceived) {
  SpeechDriverThread *sdt = data;

  return sdt->response.type != RSP_PENDING;
}

static int
awaitSpeechResponse (SpeechDriverThread *sdt, int timeout) {
  return asyncAwaitCondition(timeout, testSpeechResponseReceived, sdt);
}

static inline int
getIntegerResult (SpeechDriverThread *sdt) {
  if (awaitSpeechResponse(sdt, SPEECH_RESPONSE_WAIT_TIMEOUT)) {
    if (sdt->response.type == RSP_INTEGER) {
      return sdt->response.value.INTEGER;
    }
  }

  return 0;
}

static SpeechRequest *
newSpeechRequest (SpeechRequestType type, SpeechRequesdtatum *data) {
  SpeechRequest *req;
  size_t size = sizeof(*req);

  if (data) {
    const SpeechRequesdtatum *datum = data;

    while (!datum->end) {
      if (datum->address) size += datum->size;
      datum += 1;
    }
  }

  if ((req = malloc(size))) {
    memset(req, 0, sizeof(*req));
    req->type = type;

    if (data) {
      SpeechRequesdtatum *datum = data;
      unsigned char *target = req->data;

      while (!datum->end) {
        if (datum->address) {
          memcpy(target, datum->address, datum->size);
          datum->address = target;
          target += datum->size;
        }

        datum += 1;
      }
    }

    return req;
  } else {
    logMallocError();
  }

  return NULL;
}

static int
sendSpeechRequest (SpeechDriverThread *sdt, SpeechRequest *req) {
  if (!sdt) return 0;
  setResponsePending(sdt);
  return asyncSignalEvent(sdt->requestEvent, req);
}

int
speechFunction_sayText (
  SpeechDriverThread *sdt,
  const char *text, size_t length,
  size_t count, const unsigned char *attributes
) {
  SpeechRequest *req;

  BEGIN_SPEECH_REQUEST_DATA
    {.address=text, .size=length+1},
    {.address=attributes, .size=count},
  END_SPEECH_REQUEST_DATA

  if ((req = newSpeechRequest(REQ_SAY_TEXT, data))) {
    req->arguments.sayText.text = data[0].address;
    req->arguments.sayText.length = length;
    req->arguments.sayText.count = count;
    req->arguments.sayText.attributes = data[1].address;
    if (sendSpeechRequest(sdt, req)) return getIntegerResult(sdt);

    free(req);
  }

  return 0;
}

int
speechFunction_muteSpeech (
  SpeechDriverThread *sdt
) {
  SpeechRequest *req;

  if ((req = newSpeechRequest(REQ_MUTE_SPEECH, NULL))) {
    if (sendSpeechRequest(sdt, req)) return getIntegerResult(sdt);

    free(req);
  }

  return 0;
}

int
speechFunction_doTrack (
  SpeechDriverThread *sdt
) {
  SpeechRequest *req;

  if ((req = newSpeechRequest(REQ_DO_TRACK, NULL))) {
    if (sendSpeechRequest(sdt, req)) return getIntegerResult(sdt);

    free(req);
  }

  return 0;
}

int
speechFunction_getTrack (
  SpeechDriverThread *sdt
) {
  SpeechRequest *req;

  if ((req = newSpeechRequest(REQ_GET_TRACK, NULL))) {
    if (sendSpeechRequest(sdt, req)) return getIntegerResult(sdt);

    free(req);
  }

  return 0;
}

int
speechFunction_isSpeaking (
  SpeechDriverThread *sdt
) {
  SpeechRequest *req;

  if ((req = newSpeechRequest(REQ_IS_SPEAKING, NULL))) {
    if (sendSpeechRequest(sdt, req)) return getIntegerResult(sdt);

    free(req);
  }

  return 0;
}

int
speechFunction_setVolume (
  SpeechDriverThread *sdt,
  unsigned char setting
) {
  SpeechRequest *req;

  if ((req = newSpeechRequest(REQ_SET_VOLUME, NULL))) {
    req->arguments.setVolume.setting = setting;
    if (sendSpeechRequest(sdt, req)) return getIntegerResult(sdt);

    free(req);
  }

  return 0;
}

int
speechFunction_setRate (
  SpeechDriverThread *sdt,
  unsigned char setting
) {
  SpeechRequest *req;

  if ((req = newSpeechRequest(REQ_SET_RATE, NULL))) {
    req->arguments.setRate.setting = setting;
    if (sendSpeechRequest(sdt, req)) return getIntegerResult(sdt);

    free(req);
  }

  return 0;
}

int
speechFunction_setPitch (
  SpeechDriverThread *sdt,
  unsigned char setting
) {
  SpeechRequest *req;

  if ((req = newSpeechRequest(REQ_SET_PITCH, NULL))) {
    req->arguments.setPitch.setting = setting;
    if (sendSpeechRequest(sdt, req)) return getIntegerResult(sdt);

    free(req);
  }

  return 0;
}

int
speechFunction_setPunctuation (
  SpeechDriverThread *sdt,
  SpeechPunctuation setting
) {
  SpeechRequest *req;

  if ((req = newSpeechRequest(REQ_SET_PUNCTUATION, NULL))) {
    req->arguments.setPunctuation.setting = setting;
    if (sendSpeechRequest(sdt, req)) return getIntegerResult(sdt);

    free(req);
  }

  return 0;
}

static void
awaitDriverThreadTermination (SpeechDriverThread *sdt) {
  void *result;

  pthread_join(sdt->threadIdentifier, &result);
}

ASYNC_EVENT_CALLBACK(handleSpeechResponse) {
}

SpeechDriverThread *
newSpeechDriverThread (
  SpeechSynthesizer *spk,
  char **parameters
) {
  SpeechDriverThread *sdt;

  if ((sdt = malloc(sizeof(*sdt)))) {
    memset(sdt, 0, sizeof(*sdt));
    setThreadState(sdt, THD_CONSTRUCTING);
    setResponsePending(sdt);

    sdt->speechSynthesizer = spk;
    sdt->driverParameters = parameters;

    if ((sdt->response.event = asyncNewEvent(handleSpeechResponse, sdt))) {
      int createError = asyncCreateThread("speech-driver",
                                          &sdt->threadIdentifier, NULL,
                                          runSpeechDriverThread, sdt);

      if (!createError) {
        if (awaitSpeechResponse(sdt, DRIVER_THREAD_START_TIMEOUT)) {
          if (sdt->response.type == RSP_INTEGER) {
            if (sdt->response.value.INTEGER) {
              return sdt;
            }
          }

          logMessage(LOG_CATEGORY(SPEECH_EVENTS), "driver thread initialization failure");
          awaitDriverThreadTermination(sdt);
        } else {
          logMessage(LOG_CATEGORY(SPEECH_EVENTS), "driver thread initialization timeout");
        }
      } else {
        logMessage(LOG_CATEGORY(SPEECH_EVENTS), "driver thread creation failure: %s", strerror(createError));
      }

      asyncDiscardEvent(sdt->response.event);
      sdt->response.event = NULL;
    } else {
      logMessage(LOG_CATEGORY(SPEECH_EVENTS), "response event construction failure");
    }

    free(sdt);
  } else {
    logMallocError();
  }

  return NULL;
}

ASYNC_CONDITION_TESTER(testDriverThreadFinished) {
  SpeechDriverThread *sdt = data;

  return sdt->threadState == THD_FINISHED;
}

void
destroySpeechDriverThread (
  SpeechDriverThread *sdt
) {
  if (sendSpeechRequest(sdt, NULL)) {
    asyncAwaitCondition(DRIVER_THREAD_STOP_TIMEOUT, testDriverThreadFinished, sdt);
    awaitDriverThreadTermination(sdt);
  }

  if (sdt->response.event) asyncDiscardEvent(sdt->response.event);
  free(sdt);
}
#endif /* ENABLE_SPEECH_SUPPORT */
