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
#include <errno.h>

#include "log.h"
#include "async_signal.h"
#include "async_thread.h"

#ifdef ASYNC_CAN_HANDLE_THREADS
typedef struct {
  AsyncThreadFunction *function;
  void *argument;
  char name[0];
} RunThreadArgument;

static void *
runThread (void *argument) {
  RunThreadArgument *run = argument;
  void *result;

  logMessage(LOG_CATEGORY(ASYNC_EVENTS), "thread starting: %s", run->name);
  result = run->function(run->argument);
  logMessage(LOG_CATEGORY(ASYNC_EVENTS), "thread finished: %s", run->name);

  free(run);
  return result;
}

typedef struct {
  const char *const name;
  pthread_t *const thread;
  const pthread_attr_t *const attributes;
  AsyncThreadFunction *const function;
  void *const argument;

  int error;
} CreateThreadParameters;

static int
createThread (void *parameters) {
  CreateThreadParameters *create = parameters;
  RunThreadArgument *run;

  if ((run = malloc(sizeof(*run) + strlen(create->name) + 1))) {
    memset(run, 0, sizeof(*run));
    run->function = create->function;
    run->argument = create->argument;
    strcpy(run->name, create->name);

    logMessage(LOG_CATEGORY(ASYNC_EVENTS), "creating thread: %s", create->name);
    create->error = pthread_create(create->thread, create->attributes, runThread, run);
    if (!create->error) return 1;
    logMessage(LOG_CATEGORY(ASYNC_EVENTS), "thread not created: %s: %s", create->name, strerror(create->error));

    free(run);
  } else {
    create->error = errno;
    logMallocError();
  }

  return 0;
}

#ifdef ASYNC_CAN_BLOCK_SIGNALS
ASYNC_WITH_SIGNALS_BLOCKED_FUNCTION(asyncCreateSignalSafeThread) {
  static const int signals[] = {
#ifdef SIGINT
    SIGINT,
#endif /* SIGINT */

#ifdef SIGTERM
    SIGTERM,
#endif /* SIGTERM */

#ifdef SIGCHLD
    SIGCHLD,
#endif /* SIGCHLD */

    0
  };

  CreateThreadParameters *create = data;
  const int *signal = signals;

  while (*signal) {
    asyncSetSignalBlocked(*signal, 1);
    signal += 1;
  }

  createThread(create);
}
#endif /* ASYNC_CAN_BLOCK_SIGNALS */

int
asyncCreateThread (
  const char *name,
  pthread_t *thread, const pthread_attr_t *attributes,
  AsyncThreadFunction *function, void *argument
) {
  CreateThreadParameters create = {
    .name = name,
    .thread = thread,
    .attributes = attributes,
    .function = function,
    .argument = argument
  };

#ifdef ASYNC_CAN_BLOCK_SIGNALS
  asyncCallWithObtainableSignalsBlocked(asyncCreateSignalSafeThread, &create);
#else /* ASYNC_CAN_BLOCK_SIGNALS */
  createThread(&create);
#endif /* ASYNC_CAN_BLOCK_SIGNALS */

  return create.error;
}

int
asyncLockMutex (pthread_mutex_t *mutex) {
  int result = pthread_mutex_lock(mutex);

  logSymbol(LOG_CATEGORY(ASYNC_EVENTS), mutex, "mutex lock");
  return result;
}

int
asyncUnlockMutex (pthread_mutex_t *mutex) {
  logSymbol(LOG_CATEGORY(ASYNC_EVENTS), mutex, "mutex unlock");
  return pthread_mutex_unlock(mutex);
}
#endif /* ASYNC_CAN_HANDLE_THREADS */
