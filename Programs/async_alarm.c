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
#include "async_alarm.h"
#include "async_internal.h"
#include "timing.h"

typedef struct {
  TimeValue time;
  AsyncAlarmCallback *callback;
  void *data;
} AlarmEntry;

static void
deallocateAlarmEntry (void *item, void *data) {
  AlarmEntry *alarm = item;

  free(alarm);
}

static int
compareAlarmEntries (const void *item1, const void *item2, void *data) {
  const AlarmEntry *alarm1 = item1;
  const AlarmEntry *alarm2 = item2;
  return compareTimeValues(&alarm1->time, &alarm2->time) < 0;
}

static Queue *
getAlarmQueue (int create) {
  AsyncThreadSpecificData *tsd = asyncGetThreadSpecificData();
  if (!tsd) return NULL;

  if (!tsd->alarmQueue && create) {
    tsd->alarmQueue = newQueue(deallocateAlarmEntry, compareAlarmEntries);
  }

  return tsd->alarmQueue;
}

typedef struct {
  const TimeValue *time;
  AsyncAlarmCallback *callback;
  void *data;
} AlarmElementParameters;

static Element *
newAlarmElement (const void *parameters) {
  const AlarmElementParameters *aep = parameters;
  Queue *alarms = getAlarmQueue(1);

  if (alarms) {
    AlarmEntry *alarm;

    if ((alarm = malloc(sizeof(*alarm)))) {
      alarm->time = *aep->time;
      alarm->callback = aep->callback;
      alarm->data = aep->data;

      {
        Element *element = enqueueItem(alarms, alarm);

        if (element) return element;
      }

      free(alarm);
    } else {
      logMallocError();
    }
  }

  return NULL;
}

int
asyncSetAlarmTo (
  AsyncHandle *handle,
  const TimeValue *time,
  AsyncAlarmCallback *callback,
  void *data
) {
  const AlarmElementParameters aep = {
    .time = time,
    .callback = callback,
    .data = data
  };

  return asyncMakeHandle(handle, newAlarmElement, &aep);
}

int
asyncSetAlarmIn (
  AsyncHandle *handle,
  int interval,
  AsyncAlarmCallback *callback,
  void *data
) {
  TimeValue time;
  getRelativeTime(&time, interval);
  return asyncSetAlarmTo(handle, &time, callback, data);
}

static int
checkAlarmHandle (AsyncHandle handle) {
  Queue *alarms = getAlarmQueue(0);

  if (!alarms) return 0;
  return asyncCheckHandle(handle, alarms);
}

int
asyncResetAlarmTo (AsyncHandle handle, const TimeValue *time) {
  if (checkAlarmHandle(handle)) {
    Element *element = handle->element;
    AlarmEntry *alarm = getElementItem(element);

    alarm->time = *time;
    requeueElement(element);
    return 1;
  }

  return 0;
}

int
asyncResetAlarmIn (AsyncHandle handle, int interval) {
  TimeValue time;
  getRelativeTime(&time, interval);
  return asyncResetAlarmTo(handle, &time);
}

int
asyncPerformAlarm (AsyncThreadSpecificData *tsd, long int *timeout) {
  Queue *alarms = tsd->alarmQueue;

  if (alarms) {
    Element *element = getQueueHead(alarms);

    if (element) {
      AlarmEntry *alarm = getElementItem(element);
      TimeValue now;
      long int milliseconds;

      getCurrentTime(&now);
      milliseconds = millisecondsBetween(&now, &alarm->time);

      if (milliseconds <= 0) {
        AsyncAlarmCallback *callback = alarm->callback;
        const AsyncAlarmResult result = {
          .data = alarm->data
        };

        deleteElement(element);
        if (callback) callback(&result);
        return 1;
      }

      if (milliseconds < *timeout) *timeout = milliseconds;
    }
  }

  return 0;
}
