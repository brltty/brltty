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

#ifndef BRLTTY_INCLUDED_GET_PTHREADS
#define BRLTTY_INCLUDED_GET_PTHREADS

#include "prologue.h"
#undef GOT_PTHREADS
#undef GOT_PTHREADS_NAME

#if defined(__MINGW32__)
#define GOT_PTHREADS
#include "win_pthread.h"

#elif defined(HAVE_POSIX_THREADS)
#define GOT_PTHREADS
#include <pthread.h>

#endif /* posix thread definitions */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef GOT_PTHREADS
#if defined(HAVE_PTHREAD_GETNAME_NP) && defined(__GLIBC__)
#define GOT_PTHREADS_NAME

static inline size_t formatThreadName (char *buffer, size_t size) {
  int error = pthread_getname_np(pthread_self(), buffer, size);

  return error? 0: strlen(buffer);
}

static inline void setThreadName (const char *name) {
  pthread_setname_np(pthread_self(), name);
}

#else /* get/set thread name */
static inline size_t formatThreadName (char *buffer, size_t size) {
  return 0;
}

static inline void setThreadName (const char *name) {
}
#endif /* format/set thread name */
#endif /* GOT_PTHREADS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_GET_PTHREADS */
