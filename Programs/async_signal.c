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
  Queue *monitors;

  SignalHandler oldHandler;
  unsigned wasBlocked:1;
} SignalEntry;

typedef struct {
  SignalEntry *signal;
  AsyncSignalCallback *callback;
  void *data;
} MonitorEntry;

int
asyncHandleSignal (int signalNumber, SignalHandler newHandler, SignalHandler *oldHandler) {
#ifdef HAVE_SIGACTION
  struct sigaction newAction;
  struct sigaction oldAction;

  memset(&newAction, 0, sizeof(newAction));
  sigemptyset(&newAction.sa_mask);
  newAction.sa_handler = newHandler;

  if (sigaction(signalNumber, &newAction, &oldAction) != -1) {
    if (oldHandler) *oldHandler = oldAction.sa_handler;
    return 1;
  }

  logSystemError("sigaction");
#else /* HAVE_SIGACTION */
  SignalHandler result = signal(signalNumber, newHandler);

  if (result != SIG_ERR) {
    if (oldHandler) *oldHandler = result;
    return 1;
  }

  logSystemError("signal");
#endif /* HAVE_SIGACTION */

  return 0;
}

int
asyncIgnoreSignal (int signalNumber, SignalHandler *oldHandler) {
  return asyncHandleSignal(signalNumber, SIG_IGN, oldHandler);
}

int
asyncRevertSignal (int signalNumber, SignalHandler *oldHandler) {
  return asyncHandleSignal(signalNumber, SIG_DFL, oldHandler);
}

static int
setSignalMask (int how, const sigset_t *newMask, sigset_t *oldMask) {
#ifdef HAVE_POSIX_THREADS
  int error = pthread_sigmask(how, newMask, oldMask);

  if (!error) return 1;
  logActionError(error, "pthread_setmask");
#else /* HAVE_POSIX_THREADS */
  if (sigprocmask(how, newMask, oldMask) != -1) return 1;
  logSystemError("sigprocmask");
#endif /* HAVE_POSIX_THREADS */

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

int
asyncSetSignalBlocked (int signalNumber, int state) {
  sigset_t mask;

  if (makeSignalMask(&mask, signalNumber)) {
    if (setSignalMask((state? SIG_BLOCK: SIG_UNBLOCK), &mask, NULL)) {
      return 1;
    }
  }

  return 0;
}

static int
getSignalMask (sigset_t *mask) {
  return setSignalMask(SIG_SETMASK, NULL, mask);
}

int
asyncIsSignalBlocked (int signalNumber) {
  sigset_t signalMask;

  if (getSignalMask(&signalMask)) {
    int result = sigismember(&signalMask, signalNumber);

    if (result != -1) return result;
    logSystemError("sigismember");
  }

  return 0;
}

static void
deallocateMonitorEntry (void *item, void *data) {
  MonitorEntry *mon = item;

  free(mon);
}

static void
deallocateSignalEntry (void *item, void *data) {
  SignalEntry *sig = item;

  deallocateQueue(sig->monitors);
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

static void
cancelMonitor (Element *monitorElement) {
  MonitorEntry *mon = getElementItem(monitorElement);
  SignalEntry *sig = mon->signal;
  deleteElement(monitorElement);

  if (getQueueSize(sig->monitors) == 0) {
    asyncHandleSignal(sig->number, sig->oldHandler, NULL);
    asyncSetSignalBlocked(sig->number, sig->wasBlocked);

    {
      Queue *signals = getSignalQueue(0);
      Element *signalElement = findElementWithItem(signals, sig);
      deleteElement(signalElement);
    }
  }
}

typedef struct {
  int signalNumber;
} SignalKey;

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
        memset(sig, 0, sizeof(*sig));
        sig->number = signalNumber;

        if ((sig->monitors = newQueue(deallocateMonitorEntry, NULL))) {
          {
            static AsyncQueueMethods methods = {
              .cancelRequest = cancelMonitor
            };

            setQueueData(sig->monitors, &methods);
          }

          {
            Element *element = enqueueItem(signals, sig);

            if (element) return element;
          }

          deallocateQueue(sig->monitors);
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
handleMonitoredSignal (int signalNumber) {
  Element *signalElement = getSignalElement(signalNumber, 0);

  if (signalElement) {
    SignalEntry *sig = getElementItem(signalElement);
    Element *monitorElement = getQueueHead(sig->monitors);

    if (monitorElement) {
      MonitorEntry *mon = getElementItem(monitorElement);

      const AsyncSignalCallbackParameters parameters = {
        .signal = signalNumber,
        .data = mon->data
      };

      mon->callback(&parameters);
    }
  }
}

typedef struct {
  int signal;
  AsyncSignalCallback *callback;
  void *data;
} MonitorElementParameters;

static Element *
newMonitorElement (const void *parameters) {
  const MonitorElementParameters *mep = parameters;
  Element *signalElement = getSignalElement(mep->signal, 1);

  if (signalElement) {
    SignalEntry *sig = getElementItem(signalElement);
    int newSignal = getQueueSize(sig->monitors) == 0;
    MonitorEntry *mon;

    if ((mon = malloc(sizeof(*mon)))) {
      memset(mon, 0, sizeof(*mon));
      mon->signal = sig;
      mon->callback = mep->callback;
      mon->data = mep->data;

      {
        Element *monitorElement = enqueueItem(sig->monitors, mon);

        if (monitorElement) {
          if (!newSignal) return monitorElement;

          if (asyncHandleSignal(mep->signal, handleMonitoredSignal, &sig->oldHandler)) {
            sig->wasBlocked = asyncIsSignalBlocked(mep->signal);
            return monitorElement;
          }

          deleteElement(monitorElement);
        }
      }

      free(mon);
    } else {
      logMallocError();
    }

    if (newSignal) deleteElement(signalElement);
  }

  return NULL;
}

int
asyncMonitorSignal (
  AsyncHandle *handle, int signal,
  AsyncSignalCallback *callback, void *data
) {
  const MonitorElementParameters mep = {
    .signal = signal,
    .callback = callback,
    .data = data
  };

  return asyncMakeHandle(handle, newMonitorElement, &mep);
}
#endif /* ASYNC_CAN_HANDLE_SIGNALS */
