/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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
#include "parameters.h"
#include "prefs.h"
#include "spk_thread.h"
#include "spk.h"
#include "async_wait.h"
#include "async_event.h"
#include "async_thread.h"
#include "queue.h"

#ifdef ENABLE_SPEECH_SUPPORT
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

  [THD_FINISHED] = {
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
  unsigned stopping:1;
  Queue *requestQueue;

  volatile SpeechSynthesizer *speechSynthesizer;
  char **driverParameters;

#ifdef GOT_PTHREADS
  pthread_t threadIdentifier;
  AsyncEvent *requestEvent;
  AsyncEvent *messageEvent;
#endif /* GOT_PTHREADS */

  struct {
    SpeechResponseType type;

    union {
      int INTEGER;
    } value;
  } response;
};

typedef enum {
  REQ_SAY_TEXT,
  REQ_MUTE_SPEECH,

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
      SayOptions options;
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
} SpeechDatum;

#define BEGIN_SPEECH_DATA SpeechDatum data[] = {
#define END_SPEECH_DATA {.end=1} };

typedef enum {
  MSG_REQUEST_FINISHED,
  MSG_SPEECH_FINISHED,
  MSG_SPEECH_LOCATION
} SpeechMessageType;

typedef struct {
  SpeechMessageType type;

  union {
    struct {
      int result;
    } requestFinished;

    struct {
      int location;
    } speechLocation;
  } arguments;

  unsigned char data[0];
} SpeechMessage;

static void sendSpeechRequest (volatile SpeechDriverThread *sdt);

static int
testThreadValidity (volatile SpeechDriverThread *sdt) {
  if (sdt) {
    volatile SpeechSynthesizer *spk = sdt->speechSynthesizer;

    if (spk) {
      if (sdt == spk->driver.thread) {
        if (!sdt->stopping) {
          return 1;
        }
      }
    }
  }

  return 0;
}

static void
setThreadState (volatile SpeechDriverThread *sdt, ThreadState state) {
  const ThreadStateEntry *entry = getThreadStateEntry(state);
  const char *name = entry? entry->name: NULL;

  if (!name) name = "?";
  logMessage(LOG_CATEGORY(SPEECH_EVENTS), "driver thread %s", name);
  sdt->threadState = state;
}

static size_t
getSpeechDataSize (const SpeechDatum *data) {
  size_t size = 0;

  if (data) {
    const SpeechDatum *datum = data;

    while (!datum->end) {
      if (datum->address) size += datum->size;
      datum += 1;
    }
  }

  return size;
}

static void
moveSpeechData (unsigned char *target, SpeechDatum *data) {
  if (data) {
    SpeechDatum *datum = data;

    while (!datum->end) {
      if (datum->address) {
        memcpy(target, datum->address, datum->size);
        datum->address = target;
        target += datum->size;
      }

      datum += 1;
    }
  }
}

static inline void
setResponsePending (volatile SpeechDriverThread *sdt) {
  sdt->response.type = RSP_PENDING;
}

static void
setIntegerResponse (volatile SpeechDriverThread *sdt, int value) {
  sdt->response.type = RSP_INTEGER;
  sdt->response.value.INTEGER = value;
}

static void
handleSpeechMessage (volatile SpeechDriverThread *sdt, SpeechMessage *msg) {
  if (msg) {
    logMessage(LOG_CATEGORY(SPEECH_EVENTS), "handling message: %u", msg->type);

    switch (msg->type) {
      case MSG_REQUEST_FINISHED:
        setIntegerResponse(sdt, msg->arguments.requestFinished.result);
        sendSpeechRequest(sdt);
        break;

      case MSG_SPEECH_FINISHED:
        setSpeechFinished();
        break;

      case MSG_SPEECH_LOCATION:
        setSpeechLocation(msg->arguments.speechLocation.location);
        break;

      default:
        logMessage(LOG_CATEGORY(SPEECH_EVENTS), "unimplemented message: %u", msg->type);
        break;
    }

    free(msg);
  }
}

static int
sendSpeechMessage (volatile SpeechDriverThread *sdt, SpeechMessage *msg) {
  logMessage(LOG_CATEGORY(SPEECH_EVENTS), "sending message: %u", msg->type);

#ifdef GOT_PTHREADS
  return asyncSignalEvent(sdt->messageEvent, msg);
#else /* GOT_PTHREADS */
  handleSpeechMessage(sdt, msg);
  return 1;
#endif /* GOT_PTHREADS */
}

static SpeechMessage *
newSpeechMessage (SpeechMessageType type, SpeechDatum *data) {
  SpeechMessage *msg;
  size_t size = sizeof(*msg) + getSpeechDataSize(data);

  if ((msg = malloc(size))) {
    memset(msg, 0, sizeof(*msg));
    msg->type = type;
    moveSpeechData(msg->data, data);
    return msg;
  } else {
    logMallocError();
  }

  return NULL;
}

static int
speechMessage_requestFinished (
  volatile SpeechDriverThread *sdt,
  int result
) {
  SpeechMessage *msg;

  if ((msg = newSpeechMessage(MSG_REQUEST_FINISHED, NULL))) {
    msg->arguments.requestFinished.result = result;
    if (sendSpeechMessage(sdt, msg)) return 1;

    free(msg);
  }

  return 0;
}

int
speechMessage_speechFinished (
  volatile SpeechDriverThread *sdt
) {
  SpeechMessage *msg;

  if ((msg = newSpeechMessage(MSG_SPEECH_FINISHED, NULL))) {
    if (sendSpeechMessage(sdt, msg)) return 1;

    free(msg);
  }

  return 0;
}

int
speechMessage_speechLocation (
  volatile SpeechDriverThread *sdt,
  int location
) {
  SpeechMessage *msg;

  if ((msg = newSpeechMessage(MSG_SPEECH_LOCATION, NULL))) {
    msg->arguments.speechLocation.location = location;
    if (sendSpeechMessage(sdt, msg)) return 1;

    free(msg);
  }

  return 0;
}

static int
sendIntegerResponse (volatile SpeechDriverThread *sdt, int result) {
  return speechMessage_requestFinished(sdt, result);
}

static void
handleSpeechRequest (volatile SpeechDriverThread *sdt, SpeechRequest *req) {
  if (req) {
    logMessage(LOG_CATEGORY(SPEECH_EVENTS), "handling request: %u", req->type);

    switch (req->type) {
      case REQ_SAY_TEXT: {
        volatile SpeechSynthesizer *spk = sdt->speechSynthesizer;
        SayOptions options = req->arguments.sayText.options;
        int restorePitch = 0;
        int restorePunctuation = 0;

        if (options & SAY_OPT_MUTE_FIRST) {
          speech->mute(spk);
        }

        if (options & SAY_OPT_HIGHER_PITCH) {
          unsigned char pitch = prefs.speechPitch + 7;

          if (pitch > SPK_PITCH_MAXIMUM) pitch = SPK_PITCH_MAXIMUM;

          if (pitch != prefs.speechPitch) {
            spk->setPitch(spk, pitch);
            restorePitch = 1;
          }
        }

        if (options && SAY_OPT_ALL_PUNCTUATION) {
          unsigned char punctuation = SPK_PUNCTUATION_ALL;

          if (punctuation != prefs.speechPunctuation) {
            spk->setPunctuation(spk, punctuation);
            restorePunctuation = 1;
          }
        }

        speech->say(
          spk,
          req->arguments.sayText.text, req->arguments.sayText.length,
          req->arguments.sayText.count, req->arguments.sayText.attributes
        );

        if (restorePunctuation) spk->setPunctuation(spk, prefs.speechPunctuation);
        if (restorePitch) spk->setPitch(spk, prefs.speechPitch);

        sendIntegerResponse(sdt, 1);
        break;
      }

      case REQ_MUTE_SPEECH: {
        speech->mute(sdt->speechSynthesizer);

        sendIntegerResponse(sdt, 1);
        break;
      }

      case REQ_SET_VOLUME: {
        sdt->speechSynthesizer->setVolume(
          sdt->speechSynthesizer,
          req->arguments.setVolume.setting
        );

        sendIntegerResponse(sdt, 1);
        break;
      }

      case REQ_SET_RATE: {
        sdt->speechSynthesizer->setRate(
          sdt->speechSynthesizer,
          req->arguments.setRate.setting
        );

        sendIntegerResponse(sdt, 1);
        break;
      }

      case REQ_SET_PITCH: {
        sdt->speechSynthesizer->setPitch(
          sdt->speechSynthesizer,
          req->arguments.setPitch.setting
        );

        sendIntegerResponse(sdt, 1);
        break;
      }

      case REQ_SET_PUNCTUATION: {
        sdt->speechSynthesizer->setPunctuation(
          sdt->speechSynthesizer,
          req->arguments.setPunctuation.setting
        );

        sendIntegerResponse(sdt, 1);
        break;
      }

      default:
        logMessage(LOG_CATEGORY(SPEECH_EVENTS), "unimplemented request: %u", req->type);
        sendIntegerResponse(sdt, 0);
        break;
    }

    free(req);
  } else {
    logMessage(LOG_CATEGORY(SPEECH_EVENTS), "handling request: stop");
    setThreadState(sdt, THD_STOPPING);
    sendIntegerResponse(sdt, 1);
  }
}

typedef struct {
  SpeechRequestType const type;
} TestSpeechRequestData;

static int
testSpeechRequest (const void *item, void *data) {
  const SpeechRequest *req = item;
  const TestSpeechRequestData *tsr = data;

  return req->type == tsr->type;
}

static Element *
findSpeechRequestElement (volatile SpeechDriverThread *sdt, SpeechRequestType type) {
  TestSpeechRequestData tsr = {
    .type = type
  };

  if (!testThreadValidity(sdt)) return NULL;
  return findElement(sdt->requestQueue, testSpeechRequest, &tsr);
}

static void
removeSpeechRequests (volatile SpeechDriverThread *sdt, SpeechRequestType type) {
  Element *element;

  while ((element = findSpeechRequestElement(sdt, type))) deleteElement(element);
}

static void
sendSpeechRequest (volatile SpeechDriverThread *sdt) {
  while (getQueueSize(sdt->requestQueue) > 0) {
    SpeechRequest *req = dequeueItem(sdt->requestQueue);

    if (req) {
      logMessage(LOG_CATEGORY(SPEECH_EVENTS), "sending request: %u", req->type);
    } else {
      logMessage(LOG_CATEGORY(SPEECH_EVENTS), "sending request: stop");
    }

    setResponsePending(sdt);

#ifdef GOT_PTHREADS
    if (!asyncSignalEvent(sdt->requestEvent, req)) {
      if (req) free(req);
      setIntegerResponse(sdt, 0);
      continue;
    }
#else /* GOT_PTHREADS */
    handleSpeechRequest(sdt, req);
#endif /* GOT_PTHREADS */
    return;
  }
}

static int
enqueueSpeechRequest (volatile SpeechDriverThread *sdt, SpeechRequest *req) {
  if (testThreadValidity(sdt)) {
    if (req) {
      logMessage(LOG_CATEGORY(SPEECH_EVENTS), "enqueuing request: %u", req->type);
    } else {
      logMessage(LOG_CATEGORY(SPEECH_EVENTS), "enqueuing request: stop");
    }

    if (enqueueItem(sdt->requestQueue, req)) {
      if (sdt->response.type != RSP_PENDING) {
        if (getQueueSize(sdt->requestQueue) == 1) {
          sendSpeechRequest(sdt);
        }
      }

      return 1;
    }
  }

  return 0;
}

static SpeechRequest *
newSpeechRequest (SpeechRequestType type, SpeechDatum *data) {
  SpeechRequest *req;
  size_t size = sizeof(*req) + getSpeechDataSize(data);

  if ((req = malloc(size))) {
    memset(req, 0, sizeof(*req));
    req->type = type;
    moveSpeechData(req->data, data);
    return req;
  } else {
    logMallocError();
  }

  return NULL;
}

int
speechRequest_sayText (
  volatile SpeechDriverThread *sdt,
  const char *text, size_t length,
  size_t count, const unsigned char *attributes,
  SayOptions options
) {
  SpeechRequest *req;

  BEGIN_SPEECH_DATA
    {.address=text, .size=length+1},
    {.address=attributes, .size=count},
  END_SPEECH_DATA

  if ((req = newSpeechRequest(REQ_SAY_TEXT, data))) {
    req->arguments.sayText.text = data[0].address;
    req->arguments.sayText.length = length;
    req->arguments.sayText.count = count;
    req->arguments.sayText.attributes = data[1].address;
    req->arguments.sayText.options = options;

    if (options & SAY_OPT_MUTE_FIRST) {
      removeSpeechRequests(sdt, REQ_SAY_TEXT);
      removeSpeechRequests(sdt, REQ_MUTE_SPEECH);
    }

    if (enqueueSpeechRequest(sdt, req)) return 1;

    free(req);
  }

  return 0;
}

int
speechRequest_muteSpeech (
  volatile SpeechDriverThread *sdt
) {
  SpeechRequest *req;

  if ((req = newSpeechRequest(REQ_MUTE_SPEECH, NULL))) {
    removeSpeechRequests(sdt, REQ_SAY_TEXT);
    removeSpeechRequests(sdt, REQ_MUTE_SPEECH);
    if (enqueueSpeechRequest(sdt, req)) return 1;

    free(req);
  }

  return 0;
}

int
speechRequest_setVolume (
  volatile SpeechDriverThread *sdt,
  unsigned char setting
) {
  SpeechRequest *req;

  if ((req = newSpeechRequest(REQ_SET_VOLUME, NULL))) {
    req->arguments.setVolume.setting = setting;
    if (enqueueSpeechRequest(sdt, req)) return 1;

    free(req);
  }

  return 0;
}

int
speechRequest_setRate (
  volatile SpeechDriverThread *sdt,
  unsigned char setting
) {
  SpeechRequest *req;

  if ((req = newSpeechRequest(REQ_SET_RATE, NULL))) {
    req->arguments.setRate.setting = setting;
    if (enqueueSpeechRequest(sdt, req)) return 1;

    free(req);
  }

  return 0;
}

int
speechRequest_setPitch (
  volatile SpeechDriverThread *sdt,
  unsigned char setting
) {
  SpeechRequest *req;

  if ((req = newSpeechRequest(REQ_SET_PITCH, NULL))) {
    req->arguments.setPitch.setting = setting;
    if (enqueueSpeechRequest(sdt, req)) return 1;

    free(req);
  }

  return 0;
}

int
speechRequest_setPunctuation (
  volatile SpeechDriverThread *sdt,
  SpeechPunctuation setting
) {
  SpeechRequest *req;

  if ((req = newSpeechRequest(REQ_SET_PUNCTUATION, NULL))) {
    req->arguments.setPunctuation.setting = setting;
    if (enqueueSpeechRequest(sdt, req)) return 1;

    free(req);
  }

  return 0;
}

static void
setThreadReady (volatile SpeechDriverThread *sdt) {
  setThreadState(sdt, THD_READY);
  sendIntegerResponse(sdt, 1);
}

static int
startSpeechDriver (volatile SpeechDriverThread *sdt) {
  logMessage(LOG_CATEGORY(SPEECH_EVENTS), "starting driver");
  return speech->construct(sdt->speechSynthesizer, sdt->driverParameters);
}

static void
stopSpeechDriver (volatile SpeechDriverThread *sdt) {
  logMessage(LOG_CATEGORY(SPEECH_EVENTS), "stopping driver");
  speech->destruct(sdt->speechSynthesizer);
}

#ifdef GOT_PTHREADS
ASYNC_CONDITION_TESTER(testSpeechResponseReceived) {
  volatile SpeechDriverThread *sdt = data;

  return sdt->response.type != RSP_PENDING;
}

static int
awaitSpeechResponse (volatile SpeechDriverThread *sdt, int timeout) {
  return asyncAwaitCondition(timeout, testSpeechResponseReceived, (void *)sdt);
}

ASYNC_CONDITION_TESTER(testSpeechDriverThreadStopping) {
  volatile SpeechDriverThread *sdt = data;

  return sdt->threadState == THD_STOPPING;
}

ASYNC_CONDITION_TESTER(testSpeechDriverThreadFinished) {
  volatile SpeechDriverThread *sdt = data;

  return sdt->threadState == THD_FINISHED;
}

ASYNC_EVENT_CALLBACK(handleSpeechMessageEvent) {
  volatile SpeechDriverThread *sdt = parameters->eventData;
  SpeechMessage *msg = parameters->signalData;

  handleSpeechMessage(sdt, msg);
}

ASYNC_EVENT_CALLBACK(handleSpeechRequestEvent) {
  volatile SpeechDriverThread *sdt = parameters->eventData;
  SpeechRequest *req = parameters->signalData;

  handleSpeechRequest(sdt, req);
}

static void
awaitSpeechDriverThreadTermination (volatile SpeechDriverThread *sdt) {
  void *result;

  pthread_join(sdt->threadIdentifier, &result);
}

ASYNC_THREAD_FUNCTION(runSpeechDriverThread) {
  volatile SpeechDriverThread *sdt = argument;

  setThreadState(sdt, THD_STARTING);

  if ((sdt->requestEvent = asyncNewEvent(handleSpeechRequestEvent, (void *)sdt))) {
    if (startSpeechDriver(sdt)) {
      setThreadReady(sdt);

      while (!asyncAwaitCondition(SPEECH_REQUEST_WAIT_DURATION,
                                  testSpeechDriverThreadStopping, (void *)sdt)) {
      }

      stopSpeechDriver(sdt);
    } else {
      logMessage(LOG_CATEGORY(SPEECH_EVENTS), "driver construction failure");
    }

    asyncDiscardEvent(sdt->requestEvent);
    sdt->requestEvent = NULL;
  } else {
    logMessage(LOG_CATEGORY(SPEECH_EVENTS), "request event construction failure");
  }

  {
    int ok = sdt->threadState == THD_STOPPING;

    setThreadState(sdt, THD_FINISHED);
    sendIntegerResponse(sdt, ok);
  }

  return NULL;
}
#endif /* GOT_PTHREADS */

static void
deallocateSpeechRequest (void *item, void *data) {
  SpeechRequest *req = item;

  logMessage(LOG_CATEGORY(SPEECH_EVENTS), "unqueuing request: %u", req->type);
  free(req);
}

volatile SpeechDriverThread *
newSpeechDriverThread (
  volatile SpeechSynthesizer *spk,
  char **parameters
) {
  volatile SpeechDriverThread *sdt;

  if ((sdt = malloc(sizeof(*sdt)))) {
    memset((void *)sdt, 0, sizeof(*sdt));
    sdt->stopping = 0;
    setThreadState(sdt, THD_CONSTRUCTING);
    setResponsePending(sdt);

    sdt->speechSynthesizer = spk;
    sdt->driverParameters = parameters;

    if ((sdt->requestQueue = newQueue(deallocateSpeechRequest, NULL))) {
      spk->driver.thread = sdt;

#ifdef GOT_PTHREADS
      if ((sdt->messageEvent = asyncNewEvent(handleSpeechMessageEvent, (void *)sdt))) {
        pthread_t threadIdentifier;
        int createError = asyncCreateThread("speech-driver",
                                            &threadIdentifier, NULL,
                                            runSpeechDriverThread, (void *)sdt);

        if (!createError) {
          sdt->threadIdentifier = threadIdentifier;

          if (awaitSpeechResponse(sdt, SPEECH_DRIVER_THREAD_START_TIMEOUT)) {
            if (sdt->response.type == RSP_INTEGER) {
              if (sdt->response.value.INTEGER) {
                return sdt;
              }
            }

            logMessage(LOG_CATEGORY(SPEECH_EVENTS), "driver thread initialization failure");
            awaitSpeechDriverThreadTermination(sdt);
          } else {
            logMessage(LOG_CATEGORY(SPEECH_EVENTS), "driver thread initialization timeout");
          }
        } else {
          logMessage(LOG_CATEGORY(SPEECH_EVENTS), "driver thread creation failure: %s", strerror(createError));
        }

        asyncDiscardEvent(sdt->messageEvent);
        sdt->messageEvent = NULL;
      } else {
        logMessage(LOG_CATEGORY(SPEECH_EVENTS), "response event construction failure");
      }
#else /* GOT_PTHREADS */
      if (startSpeechDriver(sdt)) {
        setThreadReady(sdt);
        return sdt;
      }
#endif /* GOT_PTHREADS */

      spk->driver.thread = NULL;
      deallocateQueue(sdt->requestQueue);
    }

    free((void *)sdt);
  } else {
    logMallocError();
  }

  return NULL;
}

void
destroySpeechDriverThread (volatile SpeechDriverThread *sdt) {
  deleteElements(sdt->requestQueue);

#ifdef GOT_PTHREADS
  if (enqueueSpeechRequest(sdt, NULL)) {
    awaitSpeechResponse(sdt, SPEECH_DRIVER_THREAD_STOP_TIMEOUT);
    awaitSpeechDriverThreadTermination(sdt);
  }

  if (sdt->messageEvent) asyncDiscardEvent(sdt->messageEvent);
#else /* GOT_PTHREADS */
  stopSpeechDriver(sdt);
  setThreadState(sdt, THD_FINISHED);
#endif /* GOT_PTHREADS */

  sdt->speechSynthesizer->driver.thread = NULL;
  deallocateQueue(sdt->requestQueue);
  free((void *)sdt);
}
#endif /* ENABLE_SPEECH_SUPPORT */
