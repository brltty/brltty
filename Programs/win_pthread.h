/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* This is a minimal pthread implementation based on windows functions.
 * It is *not* intended to be complete - just complete enough to get
 * BRLTTY running.
 */

#ifndef BRLTTY_INCLUDED_WIN_PTHREAD
#define BRLTTY_INCLUDED_WIN_PTHREAD

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <windows.h>
#include <stdio.h>
#include <errno.h>
#include "misc.h"
#define winPthreadAssertWindows(expr) do { if (!(expr)) { setErrno(); return -1; } } while (0)
#define winPthreadAssert(expr) do { if (!(expr)) return -1; } while (0)

/***********
 * threads *
 ***********/

typedef DWORD pthread_attr_t;
typedef HANDLE pthread_t;

static inline int pthread_attr_init (pthread_attr_t *attr) {
  *attr = 0;
  return 0;
}

#define PTHREAD_CREATE_DETACHED 1
static inline int pthread_attr_setdetachstate (pthread_attr_t *attr, int yes) {
  /* not supported, ignore */
  return 0;
}

static inline int pthread_attr_setstacksize (pthread_attr_t *attr, size_t stacksize) {
  /* not supported, ignore */
  return 0;
}

static inline int pthread_attr_destroy (pthread_attr_t *attr) {
  return 0;
}

/* "real" cleanup handling not yet implemented */
typedef struct {
  void (*routine) (void *);
  void *arg;
} __pthread_cleanup_handler;

void pthread_cleanup_push (void (*routine) (void *), void *arg);
#define pthread_cleanup_push(routine, arg) do { \
  __pthread_cleanup_handler __cleanup_handler = {routine, arg};

void pthread_cleanup_pop (int execute);
#define pthread_cleanup_pop(execute) \
  if (execute) __cleanup_handler.routine(__cleanup_handler.arg); \
} while (0);

static inline int pthread_create (
  pthread_t *thread, const pthread_attr_t *attr,
  void * (*fun) (void *), void *arg
) {
  if (attr && *attr) {
    errno = EINVAL;
    return -1;
  }
  winPthreadAssertWindows(*thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) fun, arg, 0, NULL));
  return 0;
}

static inline int pthread_setcancelstate (int state, int *oldstate) {
  /* not yet implemented :( */
  return 0;
}

static inline int pthread_cancel (pthread_t thread) {
  /* This is quite harsh :( */
  winPthreadAssertWindows(TerminateThread(thread, 0));
  return 0;
}

static inline void pthread_exit (void *res) {
  ExitThread((DWORD) res);
}

static inline int pthread_join (pthread_t thread, void **res) {
  DWORD _res;

  while (1) {
    if (GetExitCodeThread(thread, &_res)) {
      if (res) *res = (void *)_res;
      return 0;
    }
    winPthreadAssertWindows(GetLastError() == STILL_ACTIVE);
    approximateDelay(1);
  }
}

/***********
 * mutexes *
 ***********/

#define PTHREAD_MUTEX_INITIALIZER NULL
typedef HANDLE pthread_mutex_t;
#define PTHREAD_MUTEX_RECURSIVE 1
typedef int pthread_mutexattr_t;

static inline int pthread_mutexattr_init(pthread_mutexattr_t *attr) {
  *attr = PTHREAD_MUTEX_RECURSIVE;
  return 0;
}

static inline int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type) {
  if (type != PTHREAD_MUTEX_RECURSIVE)
    return EINVAL;
  *attr = type;
  return 0;
}

static inline int pthread_mutex_init (pthread_mutex_t *mutex, pthread_mutexattr_t *attr) {
  if (attr && *attr!=PTHREAD_MUTEX_RECURSIVE) {
    errno = EINVAL;
    return -1;
  }
  winPthreadAssertWindows(*mutex = CreateMutex(NULL, FALSE, NULL));
  return 0;
}

static inline int pthread_mutex_unlock (pthread_mutex_t *mutex) {
  winPthreadAssertWindows(ReleaseMutex(*mutex));
  return 0;
}

static inline int pthread_mutex_lock (pthread_mutex_t *mutex) {
  volatile pthread_mutex_t *vmutex = mutex;
  if (!*vmutex) {
    HANDLE mutex_init_mutex;
    winPthreadAssertWindows((mutex_init_mutex = CreateMutex(NULL, FALSE, "BRLTTY mutex init")));
    winPthreadAssert(!pthread_mutex_lock(&mutex_init_mutex));
    if (!*vmutex)
      winPthreadAssert(!pthread_mutex_init((pthread_mutex_t *) vmutex,NULL));
    winPthreadAssert(!pthread_mutex_unlock(&mutex_init_mutex));
    winPthreadAssertWindows(CloseHandle(mutex_init_mutex));
  }
again:
  switch (WaitForSingleObject(*vmutex, INFINITE)) {
    default:
    case WAIT_FAILED:
      setErrno();
      return -1;
    case WAIT_ABANDONED:
    case WAIT_OBJECT_0:
      return 0;
    case WAIT_TIMEOUT:
      goto again;
  }
}

static inline int pthread_mutex_destroy (pthread_mutex_t *mutex) {
  winPthreadAssertWindows(CloseHandle(*mutex));
  return 0;
}

/**************
 * conditions *
 **************/

typedef struct {
  HANDLE sem;
  volatile unsigned nbwait;
} pthread_cond_t;

struct timespec {
  time_t  tv_sec;  /* Seconds */
  long    tv_nsec; /* Nanoseconds */
};

typedef unsigned pthread_condattr_t;

static inline int pthread_cond_init (pthread_cond_t *cond, const pthread_condattr_t *attr) {
  if (attr) {
    errno = EINVAL;
    return -1;
  }
  winPthreadAssertWindows(cond->sem = CreateSemaphore(NULL, 1, 1, NULL));
  cond->nbwait = 0;
  return 0;
}

static inline int pthread_cond_timedwait (pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *time) {
  cond->nbwait++;
  winPthreadAssert(pthread_mutex_unlock(mutex));
again:
  switch (WaitForSingleObject(cond->sem, time->tv_sec*1000+time->tv_nsec/1000)) {
    default:
    case WAIT_FAILED:
      setErrno();
      return -1;
    case WAIT_TIMEOUT:
      goto again;
    case WAIT_ABANDONED:
    case WAIT_OBJECT_0:
      break;
  }
  winPthreadAssert(pthread_mutex_lock(mutex));
  cond->nbwait--;
  return 0;
}

static inline int pthread_cond_wait (pthread_cond_t *cond, pthread_mutex_t *mutex) {
  cond->nbwait++;
  winPthreadAssert(pthread_mutex_unlock(mutex));
again:
  switch (WaitForSingleObject(cond->sem, INFINITE)) {
    case WAIT_FAILED:
      setErrno();
      return -1;
    case WAIT_TIMEOUT:
      goto again;
    case WAIT_ABANDONED:
    case WAIT_OBJECT_0:
      break;
  }
  winPthreadAssert(pthread_mutex_lock(mutex));
  cond->nbwait--;
  return 0;
}

static inline int pthread_cond_signal (pthread_cond_t *cond) {
  if (cond->nbwait)
    ReleaseSemaphore(cond->sem, 1, NULL);
  return 0;
}

static inline int pthread_cond_destroy (pthread_cond_t *cond) {
  winPthreadAssertWindows(CloseHandle(cond->sem));
  return 0;
}

/*******
 * TLS *
 *******/

typedef DWORD pthread_key_t;
#define PTHREAD_ONCE_INIT {PTHREAD_MUTEX_INITIALIZER, 0}
typedef struct {
  pthread_mutex_t mutex;
  unsigned done;
} pthread_once_t;

static inline int pthread_once (pthread_once_t *once, void (*oncefun)(void)) {
  pthread_mutex_lock(&once->mutex);
  if (!once->done) {
    oncefun();
    once->done = 1;
  }
  pthread_mutex_unlock(&once->mutex);
  return 0;
}

static inline int pthread_key_create (pthread_key_t *key, void (*freefun)(void *)) {
  DWORD res;
  winPthreadAssertWindows((res = TlsAlloc()) != 0xFFFFFFFF);
  *key = res;
  return 0;
}

static inline void *pthread_getspecific (pthread_key_t key) {
  void * res = TlsGetValue(key);
  if (!res)
    errno = EIO;
  return res;
}

static inline int pthread_setspecific (pthread_key_t key, const void *data) {
  winPthreadAssertWindows(TlsSetValue(key, (LPVOID) data));
  return 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_WIN_PTHREAD */
