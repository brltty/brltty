/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_THREAD
#define BRLTTY_INCLUDED_THREAD

#include "get_pthreads.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef GOT_PTHREADS
#define THREAD_FUNCTION(name) void *name (void *argument)
typedef THREAD_FUNCTION(ThreadFunction);

extern int createThread (
  const char *name,
  pthread_t *thread, const pthread_attr_t *attributes,
  ThreadFunction *function, void *argument
);

extern int callThreadFunction (
  const char *name, ThreadFunction *function,
  void *argument, void **result
);

extern int lockMutex (pthread_mutex_t *mutex);
extern int unlockMutex (pthread_mutex_t *mutex);
#endif /* GOT_PTHREADS */

extern size_t formatThreadName (char *buffer, size_t size);
extern void setThreadName (const char *name);

#define THREAD_SPECIFIC_DATA_NEW(name) void *name##_new (void)
typedef THREAD_SPECIFIC_DATA_NEW(ThreadSpecificData);

#define THREAD_SPECIFIC_DATA_DESTROY(name) void name##_destroy (void *data)
typedef THREAD_SPECIFIC_DATA_DESTROY(ThreadSpecificData);

typedef struct {
  ThreadSpecificData_new *new;
  ThreadSpecificData_destroy *destroy;

#ifdef PTHREAD_MUTEX_INITIALIZER
  struct {
    pthread_key_t value;
    unsigned created;
  } key;

  pthread_mutex_t mutex;

#define THREAD_SPECIFIC_DATA_INITIALIZER() \
  .key = { .created = 0 }, .mutex = PTHREAD_MUTEX_INITIALIZER
#else /* PTHREAD_MUTEX_INITIALIZER */
  void *data;

#define THREAD_SPECIFIC_DATA_INITIALIZER() \
  .data = NULL
#endif /* PTHREAD_MUTEX_INITIALIZER */
} ThreadSpecificDataControl;

#define THREAD_SPECIFIC_DATA_CONTROL(name) \
  static ThreadSpecificDataControl name = { \
    .new = name ## _new, \
    .destroy = name ## _destroy, \
    \
    THREAD_SPECIFIC_DATA_INITIALIZER() \
  }

extern void *getThreadSpecificData (ThreadSpecificDataControl *ctl);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_THREAD */
