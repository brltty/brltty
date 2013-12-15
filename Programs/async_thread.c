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

#include "async_thread.h"
#include "async_signal.h"

#ifdef ASYNC_CAN_HANDLE_THREADS
#ifdef ASYNC_CAN_HANDLE_SIGNALS
typedef struct {
  pthread_t *const thread;
  const pthread_attr_t *const attributes;
  AsyncThreadFunction *const function;
  void *const argument;

  int result;
} CreateThreadData;

static void
createThread (void *data) {
  CreateThreadData *ctd = data;

  ctd->result = pthread_create(ctd->thread, ctd->attributes, ctd->function, ctd->argument);
}
#endif /* ASYNC_CAN_HANDLE_SIGNALS */

int
asyncCreateThread (
  pthread_t *thread, const pthread_attr_t *attributes,
  AsyncThreadFunction *function, void *argument
) {
#ifdef ASYNC_CAN_HANDLE_SIGNALS
  CreateThreadData ctd = {
    .thread = thread,
    .attributes = attributes,
    .function = function,
    .argument = argument
  };

  asyncCallWithObtainableSignalsBlocked(createThread, &ctd);
  return ctd.result;
#else /* ASYNC_CAN_HANDLE_SIGNALS */
  return pthread_create(thread, attributes, function, argument);
#endif /* ASYNC_CAN_HANDLE_SIGNALS */
}
#endif /* ASYNC_CAN_HANDLE_THREADS */
