/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#include "prologue.h"

#include <errno.h>

#ifdef __MINGW32__
#include "win_pthread.h"

#elif defined(__MSDOS__)

#else /* posix threads */
#include <pthread.h>
#endif /* posix threads */

#include "misc.h"
#include "lock.h"

#ifdef __MSDOS__
LockDescriptor *
newLockDescriptor (void) {
  return NULL;
}

void
freeLockDescriptor (LockDescriptor *lock) {
}

int
obtainLock (LockDescriptor *lock, LockOptions options) {
  return 1;
}

void
releaseLock (LockDescriptor *lock) {
}

LockDescriptor *
getLockDescriptor (LockDescriptor **lock) {
  *lock = NULL;
  return NULL;
}
#else /* __MSDOS__ */
#ifdef PTHREAD_RWLOCK_INITIALIZER
struct LockDescriptorStruct {
  pthread_rwlock_t lock;
};

LockDescriptor *
newLockDescriptor (void) {
  LockDescriptor *lock;
  int result;

  if ((lock = malloc(sizeof(*lock)))) {
    if (!(result = pthread_rwlock_init(&lock->lock, NULL))) {
      return lock;
    }
    free(lock);
  }

  return NULL;
}

void
freeLockDescriptor (LockDescriptor *lock) {
  pthread_rwlock_destroy(&lock->lock);
  free(lock);
}

int
obtainLock (LockDescriptor *lock, LockOptions options) {
  if (options & LOCK_Exclusive) {
    if (options & LOCK_NoWait) return !pthread_rwlock_trywrlock(&lock->lock);
    pthread_rwlock_wrlock(&lock->lock);
  } else {
    if (options & LOCK_NoWait) return !pthread_rwlock_tryrdlock(&lock->lock);
    pthread_rwlock_rdlock(&lock->lock);
  }
  return 1;
}

void
releaseLock (LockDescriptor *lock) {
  pthread_rwlock_unlock(&lock->lock);
}
#else /* PTHREAD_RWLOCK_INITIALIZER */
struct LockDescriptorStruct {
  pthread_mutex_t mutex;
  pthread_cond_t read;
  pthread_cond_t write;
  int count;
  unsigned int writers;
};

LockDescriptor *
newLockDescriptor (void) {
  LockDescriptor *lock;
  int result;

  if ((lock = malloc(sizeof(*lock)))) {
    if (!(result = pthread_cond_init(&lock->read, NULL))) {
      if (!(result = pthread_cond_init(&lock->write, NULL))) {
        if (!(result = pthread_mutex_init(&lock->mutex, NULL))) {
          lock->count = 0;
          lock->writers = 0;
          return lock;
        }

        pthread_cond_destroy(&lock->write);
      }

      pthread_cond_destroy(&lock->read);
    }

    free(lock);
  }

  return NULL;
}

void
freeLockDescriptor (LockDescriptor *lock) {
  pthread_mutex_destroy(&lock->mutex);
  pthread_cond_destroy(&lock->read);
  pthread_cond_destroy(&lock->write);
  free(lock);
}

int
obtainLock (LockDescriptor *lock, LockOptions options) {
  int locked = 0;
  pthread_mutex_lock(&lock->mutex);

  if (options & LOCK_Exclusive) {
    while (lock->count) {
      if (options & LOCK_NoWait) goto done;
      ++lock->writers;
      pthread_cond_wait(&lock->write, &lock->mutex);
      --lock->writers;
    }
    lock->count = -1;
  } else {
    while (lock->count < 0) {
      if (options & LOCK_NoWait) goto done;
      pthread_cond_wait(&lock->read, &lock->mutex);
    }
    ++lock->count;
  }
  locked = 1;

done:
  pthread_mutex_unlock(&lock->mutex);
  return locked;
}

void
releaseLock (LockDescriptor *lock) {
  pthread_mutex_lock(&lock->mutex);

  if (lock->count < 0) {
    lock->count = 0;
  } else if (--lock->count) {
    goto done;
  }

  if (lock->writers) {
    pthread_cond_signal(&lock->write);
  } else {
    pthread_cond_broadcast(&lock->read);
  }

done:
  pthread_mutex_unlock(&lock->mutex);
}
#endif /* PTHREAD_RWLOCK_INITIALIZER */

LockDescriptor *
getLockDescriptor (LockDescriptor **lock) {
  if (!*lock) {
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&mutex);
    if (!*lock) *lock = newLockDescriptor();
    pthread_mutex_unlock(&mutex);
  }
  return *lock;
}
#endif /* __MSDOS__ */
