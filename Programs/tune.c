/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>

#include "log.h"
#include "parameters.h"
#include "thread.h"
#include "async_event.h"
#include "async_alarm.h"
#include "async_wait.h"
#include "program.h"
#include "tune.h"
#include "notes.h"

static int tuneInitialized = 0;
static AsyncHandle tuneDeviceCloseTimer = NULL;
static int openErrorLevel = LOG_ERR;

static const NoteMethods *noteMethods = NULL;
static NoteDevice *noteDevice = NULL;

static void
closeTuneDevice (void) {
  if (tuneDeviceCloseTimer) {
    asyncCancelRequest(tuneDeviceCloseTimer);
    tuneDeviceCloseTimer = NULL;
  }

  if (noteDevice) {
    noteMethods->destruct(noteDevice);
    noteDevice = NULL;
  }
}

ASYNC_ALARM_CALLBACK(handleTuneDeviceCloseTimeout) {
  if (tuneDeviceCloseTimer) {
    asyncDiscardHandle(tuneDeviceCloseTimer);
    tuneDeviceCloseTimer = NULL;
  }

  closeTuneDevice();
}

static void
exitTunes (void *data) {
  closeTuneDevice();
  tuneInitialized = 0;
}
 
static int
openTuneDevice (void) {
  const int timeout = TUNE_DEVICE_CLOSE_DELAY;

  if (noteDevice) {
    asyncResetAlarmIn(tuneDeviceCloseTimer, timeout);
  } else if ((noteDevice = noteMethods->construct(openErrorLevel)) != NULL) {
    if (asyncSetAlarmIn(&tuneDeviceCloseTimer, timeout, handleTuneDeviceCloseTimeout, NULL)) {
      if (!tuneInitialized) {
        tuneInitialized = 1;
        onProgramExit("tunes", exitTunes, NULL);
      }
    }
  } else {
    return 0;
  }

  return 1;
}

typedef enum {
  TUNE_REQ_PLAY,
  TUNE_REQ_WAIT,
  TUNE_REQ_SET
} TuneRequestType;

typedef struct {
  TuneRequestType type;

  union {
    struct {
      const TuneElement *tune;
    } play;

    struct {
      int time;
    } wait;

    struct {
      const NoteMethods *methods;
    } set;
  } data;
} TuneRequest;

static void
handleTuneRequest_play (const TuneElement *tune) {
  while (tune->duration) {
    if (!openTuneDevice()) return;
    if (!noteMethods->play(noteDevice, tune->note, tune->duration)) return;
    tune += 1;
  }

  noteMethods->flush(noteDevice);
}

static void
handleTuneRequest_wait (int time) {
  asyncWait(time);
}

static void
handleTuneRequest_set (const NoteMethods *methods) {
  if (methods != noteMethods) {
    closeTuneDevice();
    noteMethods = methods;
  }
}

static void
handleTuneRequest (TuneRequest *req) {
  switch (req->type) {
    case TUNE_REQ_PLAY:
      handleTuneRequest_play(req->data.play.tune);
      break;

    case TUNE_REQ_WAIT:
      handleTuneRequest_wait(req->data.wait.time);
      break;

    case TUNE_REQ_SET:
      handleTuneRequest_set(req->data.set.methods);
      break;
  }

  free(req);
}

static TuneRequest *
newTuneRequest (TuneRequestType type) {
  TuneRequest *req;

  if ((req = malloc(sizeof(*req)))) {
    memset(req, 0, sizeof(*req));
    req->type = type;
    return req;
  } else {
    logMallocError();
  }

  return NULL;
}

#ifdef GOT_PTHREADS
typedef enum {
  TUNE_THREAD_NONE,
  TUNE_THREAD_STARTING,
  TUNE_THREAD_RUNNING,
  TUNE_THREAD_STOPPING,
  TUNE_THREAD_STOPPED,
  TUNE_THREAD_FAILED
} TuneThreadState;

static TuneThreadState tuneThreadState = TUNE_THREAD_NONE;
static pthread_t tuneThreadIdentifier;
static AsyncEvent *tuneRequestEvent = NULL;
static AsyncEvent *tuneMessageEvent = NULL;

static void
setTuneThreadState (TuneThreadState newState) {
  TuneThreadState oldState = tuneThreadState;

  logMessage(LOG_DEBUG, "tune thread state change: %d -> %d", oldState, newState);
  tuneThreadState = newState;
}

ASYNC_CONDITION_TESTER(testTuneThreadStarted) {
  return tuneThreadState != TUNE_THREAD_STARTING;
}

ASYNC_CONDITION_TESTER(testTuneThreadStopping) {
  return tuneThreadState == TUNE_THREAD_STOPPING;
}

typedef enum {
  TUNE_MSG_STATE
} TuneMessageType;

typedef struct {
  TuneMessageType type;

  union {
    struct {
      TuneThreadState state;
    } state;
  } data;
} TuneMessage;

static void
handleTuneMessage (TuneMessage *msg) {
  switch (msg->type) {
    case TUNE_MSG_STATE:
      setTuneThreadState(msg->data.state.state);
      break;
  }

  free(msg);
}

ASYNC_EVENT_CALLBACK(handleTuneMessageEvent) {
  TuneMessage *msg = parameters->signalData;

  handleTuneMessage(msg);
}

static int
sendTuneMessage (TuneMessage *msg) {
  return asyncSignalEvent(tuneMessageEvent, msg);
}

static TuneMessage *
newTuneMessage (TuneMessageType type) {
  TuneMessage *msg;

  if ((msg = malloc(sizeof(*msg)))) {
    memset(msg, 0, sizeof(*msg));
    msg->type = type;
    return msg;
  } else {
    logMallocError();
  }

  return NULL;
}

static void
sendTuneThreadState (TuneThreadState state) {
  TuneMessage *msg;

  if ((msg = newTuneMessage(TUNE_MSG_STATE))) {
    msg->data.state.state = state;
    if (!sendTuneMessage(msg)) free(msg);
  }
}

ASYNC_EVENT_CALLBACK(handleTuneRequestEvent) {
  TuneRequest *req = parameters->signalData;

  handleTuneRequest(req);
}

THREAD_FUNCTION(runTuneThread) {
  if ((tuneRequestEvent = asyncNewEvent(handleTuneRequestEvent, NULL))) {
    sendTuneThreadState(TUNE_THREAD_RUNNING);
    asyncWaitFor(testTuneThreadStopping, NULL);

    asyncDiscardEvent(tuneRequestEvent);
    tuneRequestEvent = NULL;
  }

  sendTuneThreadState(TUNE_THREAD_STOPPED);
  return NULL;
}

static int
startTuneThread (void) {
  if (tuneThreadState == TUNE_THREAD_NONE) {
    setTuneThreadState(TUNE_THREAD_STARTING);

    if ((tuneMessageEvent = asyncNewEvent(handleTuneMessageEvent, NULL))) {
      int creationError = createThread("tune-thread", &tuneThreadIdentifier,
                                       NULL, runTuneThread, NULL);

      if (!creationError) {
        asyncWaitFor(testTuneThreadStarted, NULL);
      } else {
        logActionError(creationError, "tune thread creation");
        setTuneThreadState(TUNE_THREAD_FAILED);
      }

      asyncDiscardEvent(tuneMessageEvent);
      tuneMessageEvent = NULL;
    }
  }

  return tuneThreadState == TUNE_THREAD_RUNNING;
}
#endif /* GOT_PTHREADS */

static int
sendTuneRequest (TuneRequest *req) {
#ifdef GOT_PTHREADS
  if (startTuneThread()) {
    return asyncSignalEvent(tuneRequestEvent, req);
  }
#endif /* GOT_PTHREADS */

  handleTuneRequest(req);
  return 1;
}

void
playTune (const TuneElement *tune) {
  TuneRequest *req;

  if ((req = newTuneRequest(TUNE_REQ_PLAY))) {
    req->data.play.tune = tune;
    if (!sendTuneRequest(req)) free(req);
  }
}

void
waitTune (int time) {
  TuneRequest *req;

  if ((req = newTuneRequest(TUNE_REQ_WAIT))) {
    req->data.wait.time = time;
    if (!sendTuneRequest(req)) free(req);
  }
}

int
setTuneDevice (TuneDevice device) {
  const NoteMethods *methods;

  switch (device) {
    default:
      return 0;

#ifdef HAVE_BEEP_SUPPORT
    case tdBeeper:
      methods = &beepNoteMethods;
      break;
#endif /* HAVE_BEEP_SUPPORT */

#ifdef HAVE_PCM_SUPPORT
    case tdPcm:
      methods = &pcmNoteMethods;
      break;
#endif /* HAVE_PCM_SUPPORT */

#ifdef HAVE_MIDI_SUPPORT
    case tdMidi:
      methods = &midiNoteMethods;
      break;
#endif /* HAVE_MIDI_SUPPORT */

#ifdef HAVE_FM_SUPPORT
    case tdFm:
      methods = &fmNoteMethods;
      break;
#endif /* HAVE_FM_SUPPORT */
  }

  {
    TuneRequest *req;

    if ((req = newTuneRequest(TUNE_REQ_SET))) {
      req->data.set.methods = methods;
      if (!sendTuneRequest(req)) free(req);
    }
  }

  return 1;
}

void
suppressTuneDeviceOpenErrors (void) {
  openErrorLevel = LOG_DEBUG;
}
