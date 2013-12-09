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

#include "log.h"
#include "async_internal.h"

#if defined(__MINGW32__)
#include "win_pthread.h"

#elif defined(__MSDOS__)

#elif defined(GRUB_RUNTIME)

#else /* posix thread definitions */
#include <pthread.h>
#endif /* posix thread definitions */

static AsyncThreadSpecificData *
newThreadSpecificData (void) {
  AsyncThreadSpecificData *tsd;

  if ((tsd = malloc(sizeof(*tsd)))) {
    memset(tsd, 0, sizeof(*tsd));

    tsd->alarmData = NULL;
    tsd->taskData = NULL;
    tsd->ioData = NULL;
    tsd->signalData = NULL;
    tsd->waitData = NULL;

    return tsd;
  } else {
    logMallocError();
  }

  return NULL;
}

static void
destroyThreadSpecificData (AsyncThreadSpecificData *tsd) {
  if (tsd) {
    asyncDeallocateAlarmData(tsd->alarmData);
    asyncDeallocateTaskData(tsd->taskData);
    asyncDeallocateIoData(tsd->ioData);
    asyncDeallocateSignalData(tsd->signalData);
    asyncDeallocateWaitData(tsd->waitData);
    free(tsd);
  }
}

#ifdef PTHREAD_ONCE_INIT
static pthread_once_t tsdOnce = PTHREAD_ONCE_INIT;
static pthread_key_t tsdKey;
static unsigned char tsdKeyCreated = 0;

static void
tsdDestroyData (void *data) {
  AsyncThreadSpecificData *tsd = data;

  destroyThreadSpecificData(tsd);
}

static void
tsdCreateKey (void) {
  int error = pthread_key_create(&tsdKey, tsdDestroyData);

  if (!error) {
    tsdKeyCreated = 1;
  } else {
    logActionError(error, "pthread_key_create");
  }
}

AsyncThreadSpecificData *
asyncGetThreadSpecificData (void) {
  int error;

  if (!(error = pthread_once(&tsdOnce, tsdCreateKey))) {
    if (tsdKeyCreated) {
      AsyncThreadSpecificData *tsd = pthread_getspecific(tsdKey);
      if (tsd) return tsd;

      if ((tsd = newThreadSpecificData())) {
        if (!(error = pthread_setspecific(tsdKey, tsd))) {
          return tsd;
        } else {
          logActionError(error, "pthread_setspecific");
        }

        destroyThreadSpecificData(tsd);
      }
    }
  } else {
    logActionError(error, "pthread_once");
  }

  return NULL;
}

#else /* PTHREAD_ONCE_INIT */
#include "program.h"

static AsyncThreadSpecificData *threadSpecificData = NULL;

static void
exitThreadSpecificData (void *data) {
  if (threadSpecificData) {
    destroyThreadSpecificData(threadSpecificData);
    threadSpecificData = NULL;
  }
}

AsyncThreadSpecificData *
asyncGetThreadSpecificData (void) {
  if (!threadSpecificData) {
    if ((threadSpecificData = newThreadSpecificData())) {
      onProgramExit("async-data", exitThreadSpecificData, NULL);
    }
  }

  return threadSpecificData;
}
#endif /* PTHREAD_ONCE_INIT */
