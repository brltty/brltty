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

#include "prologue.h"

#undef ASYNC_CAN_HANDLE_THREADS

#if defined(__MINGW32__)
#define ASYNC_CAN_HANDLE_THREADS
#include "win_pthread.h"

#elif defined(HAVE_POSIX_THREADS)
#define ASYNC_CAN_HANDLE_THREADS
#include <pthread.h>

#endif /* posix thread definitions */

#include "async.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef ASYNC_CAN_HANDLE_THREADS
#define ASYNC_THREAD_FUNCTION(name) void *name (void *argument)
typedef ASYNC_THREAD_FUNCTION(AsyncThreadFunction);

extern int asyncCreateThread (
  const char *name,
  pthread_t *thread, const pthread_attr_t *attributes,
  AsyncThreadFunction *function, void *argument
);
#endif /* ASYNC_CAN_HANDLE_THREADS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_ASYNC_THREAD */
