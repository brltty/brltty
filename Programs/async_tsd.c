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

#include "log.h"
#include "async_tsd.h"

#if defined(__MINGW32__)
#include "win_pthread.h"

#elif defined(__MSDOS__)

#elif defined(GRUB_RUNTIME)

#else /* posix thread definitions */
#include <pthread.h>
#endif /* posix thread definitions */

#ifdef PTHREAD_ONCE_INIT
static pthread_once_t tsdOnce = PTHREAD_ONCE_INIT;
static pthread_key_t tsdKey;

static void
tsdDestroyData (void *data) {
  AsyncThreadSpecificData *tsd = data;

  if (tsd) {
    if (tsd->functionQueue) deallocateQueue(tsd->functionQueue);
    if (tsd->alarmQueue) deallocateQueue(tsd->alarmQueue);
    free(tsd);
  }
}

static void
tsdCreateKey (void) {
  int error = pthread_key_create(&tsdKey, tsdDestroyData);
  if (error) logActionError(error, "pthread_key_create");
}

AsyncThreadSpecificData *
asyncGetThreadSpecificData (void) {
  int error = pthread_once(&tsdOnce, tsdCreateKey);

  if (!error) {
    AsyncThreadSpecificData *tsd = pthread_getspecific(tsdKey);
    if (tsd) return tsd;

    if ((tsd = malloc(sizeof(*tsd)))) {
      tsd->functionQueue = NULL;
      tsd->alarmQueue = NULL;

      if (!(error = pthread_setspecific(tsdKey, tsd))) {
        return tsd;
      } else {
        logActionError(error, "pthread_setspecific");
      }
    } else {
      logMallocError();
    }
  } else {
    logActionError(error, "pthread_once");
  }

  return NULL;
}

#else /* PTHREAD_ONCE_INIT */
static AsyncThreadSpecificData tsd = {
  .functionQueue = NULL,
  .alarmQueue = NULL
};

AsyncThreadSpecificData *
asyncGetThreadSpecificData (void) {
  return &tsd;
}
#endif /* PTHREAD_ONCE_INIT */
