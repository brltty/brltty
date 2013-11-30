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
#include "async_alarm.h"
#include "async_internal.h"
#include "timing.h"

typedef struct {
  TimeValue time;
  AsyncAlarmCallback *callback;
  void *data;
} AlarmEntry;

struct AsyncAlarmDataStruct {
  Queue *alarmQueue;
};

void
asyncDeallocateAlarmData (AsyncAlarmData *ad) {
  if (ad) {
    if (ad->alarmQueue) deallocateQueue(ad->alarmQueue);
    free(ad);
  }
}

static AsyncAlarmData *
getAlarmData (void) {
  AsyncThreadSpecificData *tsd = asyncGetThreadSpecificData();
  if (!tsd) return NULL;

  if (!tsd->alarmData) {
    AsyncAlarmData *ad;

    if (!(ad = malloc(sizeof(*ad)))) {
      logMallocError();
      return NULL;
    }

    memset(ad, 0, sizeof(*ad));
    ad->alarmQueue = NULL;
    tsd->alarmData = ad;
  }

  return tsd->alarmData;
}

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
  AsyncAlarmData *ad = getAlarmData();
  if (!ad) return NULL;

  if (!ad->alarmQueue && create) {
    ad->alarmQueue = newQueue(deallocateAlarmEntry, compareAlarmEntries);
  }

  return ad->alarmQueue;
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

  getMonotonicTime(&time);
  adjustTimeValue(&time, interval);
  return asyncSetAlarmTo(handle, &time, callback, data);
}

static Element *
getAlarmElement (AsyncHandle handle) {
  return asyncGetHandleElement(handle, getAlarmQueue(0));
}

int
asyncResetAlarmTo (AsyncHandle handle, const TimeValue *time) {
  Element *element = getAlarmElement(handle);

  if (element) {
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

  getMonotonicTime(&time);
  adjustTimeValue(&time, interval);
  return asyncResetAlarmTo(handle, &time);
}

int
asyncExecuteAlarmCallback (AsyncAlarmData *ad, long int *timeout) {
  if (ad) {
    Queue *alarms = ad->alarmQueue;

    if (alarms) {
      Element *element = getQueueHead(alarms);

      if (element) {
        AlarmEntry *alarm = getElementItem(element);
        TimeValue now;
        long int milliseconds;

        getMonotonicTime(&now);
        milliseconds = millisecondsBetween(&now, &alarm->time);

        if (milliseconds <= 0) {
          AsyncAlarmCallback *callback = alarm->callback;
          const AsyncAlarmCallbackParameters parameters = {
            .now = &now,
            .data = alarm->data
          };

          deleteElement(element);
          logSymbol(LOG_CATEGORY(ASYNC_EVENTS), "alarm", callback);
          if (callback) callback(&parameters);
          return 1;
        }

        if (milliseconds < *timeout) {
          *timeout = milliseconds;
          logMessage(LOG_CATEGORY(ASYNC_EVENTS), "next alarm: %ld", *timeout);
        }
      }
    }
  }

  return 0;
}
