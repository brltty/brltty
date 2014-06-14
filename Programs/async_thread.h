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

#ifndef BRLTTY_INCLUDED_ASYNC_THREAD
#define BRLTTY_INCLUDED_ASYNC_THREAD

#include "async.h"
#include "get_pthreads.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef GOT_PTHREADS
#define ASYNC_THREAD_FUNCTION(name) void *name (void *argument)
typedef ASYNC_THREAD_FUNCTION(AsyncThreadFunction);

extern int asyncCreateThread (
  const char *name,
  pthread_t *thread, const pthread_attr_t *attributes,
  AsyncThreadFunction *function, void *argument
);

extern int asyncLockMutex (pthread_mutex_t *mutex);
extern int asyncUnlockMutex (pthread_mutex_t *mutex);
#endif /* GOT_PTHREADS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_ASYNC_THREAD */
