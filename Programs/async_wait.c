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
#include "async_wait.h"
#include "async_internal.h"
#include "timing.h"

static void
asyncAwaitAction (long int timeout) {
  AsyncThreadSpecificData *tsd = asyncGetThreadSpecificData();

  if (tsd) {
    const char *action;

    tsd->waitDepth += 1;
    logMessage(LOG_CATEGORY(ASYNC_EVENTS),
               "begin: level %u: timeout %ld",
               tsd->waitDepth, timeout);

    if (asyncPerformSignal(tsd->signalData)) {
      action = "signal handled";
    } else if (asyncPerformAlarm(tsd->alarmData, &timeout)) {
      action = "alarm handled";
    } else if ((tsd->waitDepth == 1) && asyncPerformTask(tsd->taskData)) {
      action = "task handled";
    } else if (asyncPerformOperation(tsd->ioData, timeout)) {
      action = "I/O operation handled";
    } else {
      action = "wait timed out";
    }

    logMessage(LOG_CATEGORY(ASYNC_EVENTS),
               "end: level %u: %s",
               tsd->waitDepth, action);

    tsd->waitDepth -= 1;
  } else {
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
