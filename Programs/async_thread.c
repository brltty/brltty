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
#include <errno.h>

#include "log.h"
#include "async_thread.h"
#include "async_signal.h"

#ifdef ASYNC_CAN_HANDLE_THREADS
#ifdef ASYNC_CAN_HANDLE_SIGNALS
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

static void
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
    if (!create->error) return;
    logMessage(LOG_CATEGORY(ASYNC_EVENTS), "thread not created: %s: %s", create->name, strerror(create->error));

    errno = create->error;
    free(run);
  } else {
    logMallocError();
  }
}
#endif /* ASYNC_CAN_HANDLE_SIGNALS */

int
asyncCreateThread (
  const char *name,
  pthread_t *thread, const pthread_attr_t *attributes,
  AsyncThreadFunction *function, void *argument
) {
#ifdef ASYNC_CAN_HANDLE_SIGNALS
  CreateThreadParameters create = {
    .name = name,
    .thread = thread,
    .attributes = attributes,
    .function = function,
    .argument = argument
  };

  asyncCallWithObtainableSignalsBlocked(createThread, &create);
  return create.error;
#else /* ASYNC_CAN_HANDLE_SIGNALS */
  return pthread_create(thread, attributes, function, argument);
#endif /* ASYNC_CAN_HANDLE_SIGNALS */
}
#endif /* ASYNC_CAN_HANDLE_THREADS */
