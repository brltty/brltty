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
#include "thread.h"
#include "async_signal.h"

#undef HAVE_THREAD_NAMES

#ifdef GOT_PTHREADS
typedef struct {
  ThreadFunction *function;
  void *argument;
  char name[0];
} RunThreadArgument;

static void *
runThread (void *argument) {
  RunThreadArgument *run = argument;
  void *result;

  setThreadName(run->name);
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
  ThreadFunction *const function;
  void *const argument;

  int error;
} CreateThreadParameters;

static int
createActualThread (void *parameters) {
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
ASYNC_WITH_SIGNALS_BLOCKED_FUNCTION(createSignalSafeThread) {
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

  createActualThread(create);
}
#endif /* ASYNC_CAN_BLOCK_SIGNALS */

int
createThread (
  const char *name,
  pthread_t *thread, const pthread_attr_t *attributes,
  ThreadFunction *function, void *argument
) {
  CreateThreadParameters create = {
    .name = name,
    .thread = thread,
    .attributes = attributes,
    .function = function,
    .argument = argument
  };

#ifdef ASYNC_CAN_BLOCK_SIGNALS
  asyncCallWithObtainableSignalsBlocked(createSignalSafeThread, &create);
#else /* ASYNC_CAN_BLOCK_SIGNALS */
  createActualThread(&create);
#endif /* ASYNC_CAN_BLOCK_SIGNALS */

  return create.error;
}

int
lockMutex (pthread_mutex_t *mutex) {
  int result = pthread_mutex_lock(mutex);

  logSymbol(LOG_CATEGORY(ASYNC_EVENTS), mutex, "mutex lock");
  return result;
}

int
unlockMutex (pthread_mutex_t *mutex) {
  logSymbol(LOG_CATEGORY(ASYNC_EVENTS), mutex, "mutex unlock");
  return pthread_mutex_unlock(mutex);
}

#if defined(HAVE_PTHREAD_GETNAME_NP) && defined(__GLIBC__)
#define HAVE_THREAD_NAMES

size_t
formatThreadName (char *buffer, size_t size) {
  int error = pthread_getname_np(pthread_self(), buffer, size);

  return error? 0: strlen(buffer);
}

void
setThreadName (const char *name) {
  pthread_setname_np(pthread_self(), name);
}

#elif defined(HAVE_PTHREAD_GETNAME_NP) && defined(__APPLE__)
#define HAVE_THREAD_NAMES

size_t
formatThreadName (char *buffer, size_t size) {
  {
    int error = pthread_getname_np(pthread_self(), buffer, size);

    if (error) return 0;
    if (*buffer) return strlen(buffer);
  }

  if (pthread_main_np()) {
    size_t length;

    STR_BEGIN(buffer, size);
    STR_PRINTF("main");
    length = STR_LENGTH;
    STR_END;

    return length;
  }

  return 0;
}

void
setThreadName (const char *name) {
  pthread_setname_np(name);
}

#endif /* thread names */
#endif /* GOT_PTHREADS */

#ifndef HAVE_THREAD_NAMES
size_t
formatThreadName (char *buffer, size_t size) {
  return 0;
}

void
setThreadName (const char *name) {
}
#endif /* HAVE_THREAD_NAMES */
