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
#include "async_wait.h"
#include "async_internal.h"
#include "timing.h"

struct AsyncWaitDataStruct {
  AsyncThreadSpecificData *tsd;
  unsigned int waitDepth;
};

void
asyncDeallocateWaitData (AsyncWaitData *wd) {
  if (wd) {
    free(wd);
  }
}

static AsyncWaitData *
getWaitData (void) {
  AsyncThreadSpecificData *tsd = asyncGetThreadSpecificData();
  if (!tsd) return NULL;

  if (!tsd->waitData) {
    AsyncWaitData *wd;

    if (!(wd = malloc(sizeof(*wd)))) {
      logMallocError();
      return NULL;
    }

    memset(wd, 0, sizeof(*wd));
    wd->tsd = tsd;
    wd->waitDepth = 0;
    tsd->waitData = wd;
  }

  return tsd->waitData;
}

static void
asyncAwaitAction (long int timeout) {
  AsyncWaitData *wd = getWaitData();

  if (wd) {
    AsyncThreadSpecificData *tsd = wd->tsd;
    const char *action;

    wd->waitDepth += 1;
    logMessage(LOG_CATEGORY(ASYNC_EVENTS),
               "begin: level %u: timeout %ld",
               wd->waitDepth, timeout);

    if (asyncPerformSignal(tsd->signalData)) {
      action = "signal handled";
    } else if (asyncPerformAlarm(tsd->alarmData, &timeout)) {
      action = "alarm handled";
    } else if ((wd->waitDepth == 1) && asyncPerformTask(tsd->taskData)) {
      action = "task handled";
    } else if (asyncPerformOperation(tsd->ioData, timeout)) {
      action = "I/O operation handled";
    } else {
      action = "wait timed out";
    }

    logMessage(LOG_CATEGORY(ASYNC_EVENTS),
               "end: level %u: %s",
               wd->waitDepth, action);

    wd->waitDepth -= 1;
  } else {
    logMessage(LOG_CATEGORY(ASYNC_EVENTS), "waiting: %ld", timeout);
    approximateDelay(timeout);
  }
}

int
asyncAwaitCondition (int timeout, AsyncConditionTester *testCondition, void *data) {
  TimePeriod period;
  startTimePeriod(&period, timeout);

  while (!(testCondition && testCondition(data))) {
    long int elapsed;

    if (afterTimePeriod(&period, &elapsed)) return 0;
    asyncAwaitAction(timeout - elapsed);
  }

  return 1;
}

void
asyncWait (int duration) {
  asyncAwaitCondition(duration, NULL, NULL);
}
