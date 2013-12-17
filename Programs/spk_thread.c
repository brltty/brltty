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
typedef enum {
  OBJ_ALLOCATED,
  OBJ_STARTING,
  OBJ_READY,
  OBJ_STOPPING,
  OBJ_FINISHED
} ObjectState;

typedef struct {
  const char *name;
} ObjectStateEntry;

static const ObjectStateEntry objectStateTable[] = {
  [OBJ_ALLOCATED] = {
    .name = "allocated"
  },

  [OBJ_STARTING] = {
    .name = "starting"
  },

  [OBJ_READY] = {
    .name = "ready"
  },

  [OBJ_STOPPING] = {
    .name = "stopping"
  },

  [OBJ_FINISHED] {
    .name = "finished"
  },
};

static inline const ObjectStateEntry *
getObjectStateEntry (ObjectState state) {
  if (state >= ARRAY_COUNT(objectStateTable)) return NULL;
  return &objectStateTable[state];
}

typedef enum {
  RSP_PENDING,
  RSP_SUCCESS,
  RSP_FAILURE
} SpeechResponseType;

struct SpeechThreadObjectStruct {
  ObjectState objectState;

  SpeechSynthesizer *speechSynthesizer;
  char **driverParameters;

  pthread_t threadIdentifier;
  AsyncEvent *requestEvent;

  AsyncEvent *responseEvent;
  SpeechResponseType responseType;
};

typedef enum {
  REQ_SAY_TEXT,
  REQ_MUTE_SPEECH
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
  } arguments;

  unsigned char data[0];
} SpeechRequest;

typedef struct {
  const void *address;
  size_t size;
} SpeechRequestDatum;

static void
setObjectState (SpeechThreadObject *obj, ObjectState state) {
  const ObjectStateEntry *entry = getObjectStateEntry(state);
  const char *name = entry? entry->name: NULL;

  if (!name) name = "?";
  logMessage(LOG_CATEGORY(SPEECH_EVENTS), "driver thread object %s", name);
  obj->objectState = state;
}

static inline void
setResponsePending (SpeechThreadObject *obj) {
  obj->responseType = RSP_PENDING;
}

static int
sendSpeechResponse (SpeechThreadObject *obj, int success) {
  obj->responseType = success? RSP_SUCCESS: RSP_FAILURE;
  return asyncSignalEvent(obj->responseEvent, NULL);
}

ASYNC_EVENT_CALLBACK(handleSpeechRequest) {
  SpeechThreadObject *obj = parameters->eventData;
  SpeechRequest *req = parameters->signalData;

  if (req) {
    switch (req->type) {
      case REQ_SAY_TEXT:
        speech->say(
          obj->speechSynthesizer,
          req->arguments.sayText.text,
          req->arguments.sayText.length,
          req->arguments.sayText.count,
          req->arguments.sayText.attributes
        );
        sendSpeechResponse(obj, 1);
        break;

      case REQ_MUTE_SPEECH:
        speech->mute(obj->speechSynthesizer);
        sendSpeechResponse(obj, 1);
        break;

      default:
        logMessage(LOG_CATEGORY(SPEECH_EVENTS), "unimplemented speech request type: %u", req->type);
        sendSpeechResponse(obj, 0);
        break;
    }

    free(req);
  } else {
    setObjectState(obj, OBJ_STOPPING);
  }
}

ASYNC_CONDITION_TESTER(testSpeechThreadStopping) {
  SpeechThreadObject *obj = data;

  return obj->objectState == OBJ_STOPPING;
}

ASYNC_THREAD_FUNCTION(runSpeechThread) {
  SpeechThreadObject *obj = argument;

  setObjectState(obj, OBJ_STARTING);

  if ((obj->requestEvent = asyncNewEvent(handleSpeechRequest, obj))) {
    if (speech->construct(obj->speechSynthesizer, obj->driverParameters)) {
      setObjectState(obj, OBJ_READY);
      sendSpeechResponse(obj, 1);

      while (!asyncAwaitCondition(8000, testSpeechThreadStopping, obj)) {
      }

      speech->destruct(obj->speechSynthesizer);
      obj->speechSynthesizer = NULL;
    } else {
      logMessage(LOG_CATEGORY(SPEECH_EVENTS), "driver construction failure");
    }

    asyncDiscardEvent(obj->requestEvent);
    obj->requestEvent = NULL;
  } else {
    logMessage(LOG_CATEGORY(SPEECH_EVENTS), "request event construction failure");
  }

  {
    int ok = obj->objectState = OBJ_STOPPING;

    setObjectState(obj, OBJ_FINISHED);
    sendSpeechResponse(obj, ok);
  }

  return NULL;
}

ASYNC_CONDITION_TESTER(testSpeechRequestComplete) {
  SpeechThreadObject *obj = data;

  return obj->responseType != RSP_PENDING;
}

static int
awaitSpeechResponse (SpeechThreadObject *obj, int timeout) {
  return asyncAwaitCondition(timeout, testSpeechRequestComplete, obj);
}

static inline int
getSpeechResult (SpeechThreadObject *obj) {
  if (awaitSpeechResponse(obj, 5000)) {
    if (obj->responseType == RSP_SUCCESS) {
      return 1;
    }
  }

  return 0;
}

static SpeechRequest *
newSpeechRequest (SpeechRequestType type, SpeechRequestDatum *data) {
  SpeechRequest *req;
  size_t size = sizeof(*req);

  if (data) {
    const SpeechRequestDatum *datum = data;

    while (datum->address) size += (datum++)->size;
  }

  if ((req = malloc(size))) {
    memset(req, 0, sizeof(*req));
    req->type = type;

    if (data) {
      SpeechRequestDatum *datum = data;
      unsigned char *target = req->data;

      while (datum->address) {
        memcpy(target, datum->address, datum->size);
        datum->address = target;
        target += datum->size;
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
sendSpeechRequest (SpeechThreadObject *obj, SpeechRequest *req) {
  setResponsePending(obj);
  return asyncSignalEvent(obj->requestEvent, req);
}

int
sendSpeechRequest_sayText (
  SpeechThreadObject *obj,
  const char *text,
  size_t length,
  size_t count,
  const unsigned char *attributes
) {
  SpeechRequest *req;

  SpeechRequestDatum data[] = {
    {.address=text, .size=length},
    {.address=attributes, .size=length},
    {.address=NULL}
  };

  if ((req = newSpeechRequest(REQ_SAY_TEXT, data))) {
    req->arguments.sayText.text = data[0].address;
    req->arguments.sayText.length = length;
    req->arguments.sayText.count = count;
    req->arguments.sayText.attributes = data[1].address;

    if (sendSpeechRequest(obj, req)) {
      return getSpeechResult(obj);
    }

    free(req);
  }

  return 0;
}

int
sendSpeechRequest_muteSpeech (
  SpeechThreadObject *obj
) {
  SpeechRequest *req;

  if ((req = newSpeechRequest(REQ_MUTE_SPEECH, NULL))) {
    if (sendSpeechRequest(obj, req)) {
      return getSpeechResult(obj);
    }

    free(req);
  }

  return 0;
}

static void
awaitSpeechThreadTermination (SpeechThreadObject *obj) {
  void *result;

  pthread_join(obj->threadIdentifier, &result);
}

ASYNC_EVENT_CALLBACK(handleSpeechResponse) {
}

SpeechThreadObject *
newSpeechThreadObject (SpeechSynthesizer *spk, char **parameters) {
  SpeechThreadObject *obj;

  if ((obj = malloc(sizeof(*obj)))) {
    memset(obj, 0, sizeof(*obj));
    setObjectState(obj, OBJ_ALLOCATED);
    setResponsePending(obj);

    obj->speechSynthesizer = spk;
    obj->driverParameters = parameters;

    if ((obj->responseEvent = asyncNewEvent(handleSpeechResponse, obj))) {
      int createError = asyncCreateThread("speech-driver",
                                          &obj->threadIdentifier, NULL,
                                          runSpeechThread, obj);

      if (!createError) {
        if (awaitSpeechResponse(obj, 15000)) {
          if (obj->responseType == RSP_SUCCESS) return obj;
          logMessage(LOG_CATEGORY(SPEECH_EVENTS), "driver thread initialization failure");
          awaitSpeechThreadTermination(obj);
        } else {
          logMessage(LOG_CATEGORY(SPEECH_EVENTS), "driver thread initialization timeout");
        }
      }

      asyncDiscardEvent(obj->responseEvent);
      obj->responseEvent = NULL;
    } else {
      logMessage(LOG_CATEGORY(SPEECH_EVENTS), "response event construction failure");
    }

    free(obj);
  } else {
    logMallocError();
  }

  return NULL;
}

ASYNC_CONDITION_TESTER(testSpeechThreadFinished) {
  SpeechThreadObject *obj = data;

  return obj->objectState == OBJ_FINISHED;
}

void
destroySpeechThreadObject (SpeechThreadObject *obj) {
  if (sendSpeechRequest(obj, NULL)) {
    asyncAwaitCondition(12000, testSpeechThreadFinished, obj);
    awaitSpeechThreadTermination(obj);
  }

  if (obj->responseEvent) asyncDiscardEvent(obj->responseEvent);
  free(obj);
}
#endif /* ENABLE_SPEECH_SUPPORT */
