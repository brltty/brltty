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
#include "async_signal.h"
#include "async_internal.h"

#ifdef ASYNC_CAN_HANDLE_SIGNALS
typedef struct {
  int number;
  Queue *handlers;

  sighandler_t oldHandler;
  unsigned wasBlocked:1;
} SignalEntry;

typedef struct {
  int signalNumber;
} SignalKey;

typedef struct {
  SignalEntry *signalEntry;
  AsyncSignalHandler *handlerFunction;
  void *handlerData;
} HandlerEntry;

static int
setSignalMask (int how, const sigset_t *new, sigset_t *old) {
#ifdef HAVE_POSIX_THREADS
  int error = pthread_sigmask(how, new, old);

  if (!error) return 1;
  logActionError(error, "pthread_setmask");
#else /* HAVE_POSIX_THREADS */
  if (sigprocmask(how, new, old) != -1) return 1;
  logSystemError("sigprocmask");
#endif /* HAVE_POSIX_THREADS */

  return 0;
}

static int
getSignalMask (sigset_t *mask) {
  return setSignalMask(SIG_SETMASK, NULL, mask);
}

static int
isSignalBlocked (int signalNumber) {
  sigset_t signalMask;

  if (getSignalMask(&signalMask)) {
    int result = sigismember(&signalMask, signalNumber);

    if (result != -1) return result;
    logSystemError("sigismember");
  }

  return 0;
}

static int
makeSignalMask (sigset_t *signalMask, int signalNumber) {
  if (sigemptyset(signalMask) != -1) {
    if (sigaddset(signalMask, signalNumber) != -1) {
      return 1;
    } else {
      logSystemError("sigaddset");
    }
  } else {
    logSystemError("sigemptyset");
  }

  return 0;
}

static int
setSignalBlocked (int signalNumber, int state) {
  sigset_t mask;

  if (makeSignalMask(&mask, signalNumber)) {
    if (setSignalMask((state? SIG_BLOCK: SIG_UNBLOCK), &mask, NULL)) {
      return 1;
    }
  }

  return 0;
}

static int
setSignalHandler (int number, sighandler_t newHandler, sighandler_t *oldHandler) {
#ifdef HAVE_SIGACTION
  struct sigaction newAction;
  struct sigaction oldAction;

  memset(&newAction, 0, sizeof(newAction));
  sigemptyset(&newAction.sa_mask);
  newAction.sa_handler = newHandler;

  if (sigaction(number, &newAction, &oldAction) != -1) {
    *oldHandler = oldAction.sa_handler;
    return 1;
  }

  logSystemError("sigaction");
#else /* HAVE_SIGACTION */
  if ((*oldHandler = signal(number, newHandler)) != SIG_ERR) return 1;
  logSystemError("signal");
#endif /* HAVE_SIGACTION */

  return 0;
}

static void
deallocateHandlerEntry (void *item, void *data) {
  HandlerEntry *hnd = item;

  free(hnd);
}

static void
deallocateSignalEntry (void *item, void *data) {
  SignalEntry *sig = item;

  deallocateQueue(sig->handlers);
  free(sig);
}

static Queue *
getSignalQueue (int create) {
  AsyncThreadSpecificData *tsd = asyncGetThreadSpecificData();
  if (!tsd) return NULL;

  if (!tsd->signalQueue && create) {
    tsd->signalQueue = newQueue(deallocateSignalEntry, NULL);
  }

  return tsd->signalQueue;
}

static int
testSignalEntry (const void *item, const void *data) {
  const SignalEntry *sig = item;
  const SignalKey *key = data;
  return sig->number == key->signalNumber;
}

static Element *
getSignalElement (int signalNumber, int create) {
  Queue *signals = getSignalQueue(create);

  if (signals) {
    {
      const SignalKey key = {
        .signalNumber = signalNumber
      };

      {
        Element *element = findElement(signals, testSignalEntry, &key);
        if (element) return element;
      }
    }

    if (create) {
      SignalEntry *sig;

      if ((sig = malloc(sizeof(*sig)))) {
        sig->number = signalNumber;

        if ((sig->handlers = newQueue(deallocateHandlerEntry, NULL))) {
          {
            Element *element = enqueueItem(signals, sig);
            if (element) return element;
          }

          deallocateQueue(sig->handlers);
        }

        free(sig);
      } else {
        logMallocError();
      }
    }
  }

  return NULL;
}

static void
handleSignal (int signalNumber) {
  Element *signalElement = getSignalElement(signalNumber, 0);

  if (signalElement) {
    SignalEntry *sig = getElementItem(signalElement);
    Element *handlerElement = getQueueHead(sig->handlers);

    if (handlerElement) {
      HandlerEntry *hnd = getElementItem(handlerElement);

      const AsyncSignalHandlerParameters parameters = {
        .signalNumber = signalNumber,
        .data = hnd->handlerData
      };

      if (!hnd->handlerFunction(&parameters)) {
        deleteElement(handlerElement);

        if (getQueueSize(sig->handlers) == 0) {
          sighandler_t oldHandler;

          setSignalHandler(signalNumber, sig->oldHandler, &oldHandler);
          setSignalBlocked(signalNumber, sig->wasBlocked);
          deleteElement(signalElement);
        }
      }
    }
  }
}

typedef struct {
  int number;
  AsyncSignalHandler *handler;
  void *data;
} HandlerElementParameters;

static Element *
newHandlerElement (const void *parameters) {
  const HandlerElementParameters *hep = parameters;
  Element *signalElement = getSignalElement(hep->number, 1);

  if (signalElement) {
    SignalEntry *sig = getElementItem(signalElement);
    int newSignal = getQueueSize(sig->handlers) == 0;
    HandlerEntry *hnd;

    if ((hnd = malloc(sizeof(*hnd)))) {
      memset(hnd, 0, sizeof(*hnd));
      hnd->signalEntry = sig;
      hnd->handlerFunction = hep->handler;
      hnd->handlerData = hep->data;

      {
        Element *handlerElement = enqueueItem(sig->handlers, hnd);

        if (handlerElement) {
          if (!newSignal) return handlerElement;

          if (setSignalHandler(hep->number, handleSignal, &sig->oldHandler)) {
            sig->wasBlocked = isSignalBlocked(hep->number);
            return handlerElement;
          }
        }

        deleteElement(handlerElement);
      }

      free(hnd);
    } else {
      logMallocError();
    }

    if (newSignal) deleteElement(signalElement);
  }

  return NULL;
}

int
asyncHandleSignal (
  AsyncHandle *handle, int number,
  AsyncSignalHandler *handler, void *data
) {
  const HandlerElementParameters hep = {
    .number = number,
    .handler = handler,
    .data = data
  };

  return asyncMakeHandle(handle, newHandlerElement, &hep);
}
#endif /* ASYNC_CAN_HANDLE_SIGNALS */
