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

#ifndef BRLTTY_INCLUDED_ASYNC_INTERNAL
#define BRLTTY_INCLUDED_ASYNC_INTERNAL

#include "async.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct SignalDataStruct SignalData;
extern void asyncDeallocateSignalData (SignalData *sd);
extern int asyncPerformSignal (SignalData *sd);

typedef struct AlarmDataStruct AlarmData;
extern void asyncDeallocateAlarmData (AlarmData *alarmData);
extern int asyncPerformAlarm (AlarmData *ad, long int *timeout);

typedef struct TaskDataStruct TaskData;
extern void asyncDeallocateTaskData (TaskData *taskData);
extern int asyncPerformTask (TaskData *td);

typedef struct InputOutputDataStruct InputOutputData;
extern void asyncDeallocateInputOutputData (InputOutputData *ioData);
extern int asyncPerformOperation (InputOutputData *iod, long int timeout);

typedef struct {
  SignalData *signalData;
  AlarmData *alarmData;
  TaskData *taskData;
  InputOutputData *ioData;
  unsigned int waitDepth;
} AsyncThreadSpecificData;

extern AsyncThreadSpecificData *asyncGetThreadSpecificData (void);

extern int asyncMakeHandle (
  AsyncHandle *handle,
  Element *(*newElement) (const void *parameters),
  const void *parameters
);

extern int asyncMakeElementHandle (AsyncHandle *handle, Element *element);

#define ASYNC_ANY_QUEUE ((const Queue *)1)
extern Element *asyncGetHandleElement (AsyncHandle handle, const Queue *queue);

typedef struct {
  void (*cancelRequest) (Element *element);
} AsyncQueueMethods;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_ASYNC_INTERNAL */
