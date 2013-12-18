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

#ifndef BRLTTY_INCLUDED_ASYNC_SIGNAL
#define BRLTTY_INCLUDED_ASYNC_SIGNAL

#include "prologue.h"

#undef ASYNC_CAN_HANDLE_SIGNALS

#ifdef HAVE_SIGNAL_H
#define ASYNC_CAN_HANDLE_SIGNALS
#include <signal.h>
#endif /* HAVE_SIGNAL_H */

#include "async.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef ASYNC_CAN_HANDLE_SIGNALS
typedef struct {
  int signal;
  void *data;
} AsyncSignalCallbackParameters;

#define ASYNC_SIGNAL_CALLBACK(name) int name (const AsyncSignalCallbackParameters *parameters)
typedef ASYNC_SIGNAL_CALLBACK(AsyncSignalCallback);

extern int asyncMonitorSignal (
  AsyncHandle *handle, int signal,
  AsyncSignalCallback *callback, void *data
);


/* Type sighandler_t isn't defined on all platforms. */
#define ASYNC_SIGNAL_HANDLER(name) void name (int signalNumber)
typedef ASYNC_SIGNAL_HANDLER(AsyncSignalHandler);

extern int asyncHandleSignal (int signalNumber, AsyncSignalHandler *newHandler, AsyncSignalHandler **oldHandler);
extern int asyncIgnoreSignal (int signalNumber, AsyncSignalHandler **oldHandler);
extern int asyncRevertSignal (int signalNumber, AsyncSignalHandler **oldHandler);
extern ASYNC_SIGNAL_HANDLER(asyncEmptySignalHandler);

extern int asyncSetSignalBlocked (int signalNumber, int state);
extern int asyncIsSignalBlocked (int signalNumber);


#define ASYNC_WITH_BLOCKED_SIGNALS_FUNCTION(name) void name (void *data)
typedef ASYNC_WITH_BLOCKED_SIGNALS_FUNCTION(AsyncWithBlockedSignalsFunction);

extern int asyncCallWithSignalsBlocked (
  const sigset_t *mask,
  AsyncWithBlockedSignalsFunction *function,
  void *data
);

extern int asyncCallWithSignalBlocked (
  int number,
  AsyncWithBlockedSignalsFunction *function,
  void *data
);

extern int asyncCallWithAllSignalsBlocked (
  AsyncWithBlockedSignalsFunction *function,
  void *data
);

extern int asyncCallWithObtainableSignalsBlocked (
  AsyncWithBlockedSignalsFunction *function,
  void *data
);


extern int asyncClaimSignalNumber (int signal);
extern int asyncReleaseSignalNumber (int signal);

extern int asyncObtainSignalNumber (void);
extern int asyncRelinquishSignalNumber (int signal);
extern int asyncBlockObtainableSignals (void);
#endif /* ASYNC_CAN_HANDLE_SIGNALS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_ASYNC_SIGNAL */
