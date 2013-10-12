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
#include "async_task.h"
#include "async_internal.h"

typedef struct {
  AsyncTaskFunction *function;
  void *data;
} TaskDefinition;

static void
deallocateTaskDefinition (void *item, void *data) {
  TaskDefinition *task = item;

  free(task);
}

static Queue *
getTaskQueue (int create) {
  AsyncThreadSpecificData *tsd = asyncGetThreadSpecificData();
  if (!tsd) return NULL;

  if (!tsd->taskQueue && create) {
    tsd->taskQueue = newQueue(deallocateTaskDefinition, NULL);
  }

  return tsd->taskQueue;
}

static int
addTask (TaskDefinition *task) {
  Queue *queue = getTaskQueue(1);

  if (queue) {
    if (enqueueItem(queue, task)) {
      return 1;
    }
  }

  return 0;
}

int
asyncAddTask (AsyncEvent *event, AsyncTaskFunction *function, void *data) {
  TaskDefinition *task;

  if ((task = malloc(sizeof(*task)))) {
    memset(task, 0, sizeof(*task));
    task->function = function;
    task->data = data;

    if (event) {
      if (asyncSignalEvent(event, task)) return 1;
    } else if (addTask(task)) {
      return 1;
    }

    free(task);
  } else {
    logMallocError();
  }

  return 0;
}

static void
handleAddTaskEvent (void *eventData, void *signalData) {
  addTask(signalData);
}

AsyncEvent *
asyncNewAddTaskEvent (void) {
  return asyncNewEvent(handleAddTaskEvent, NULL);
}

int
asyncPerformTask (AsyncThreadSpecificData *tsd) {
  if (tsd->waitDepth == 1) {
    Queue *queue = tsd->taskQueue;

    if (queue) {
      TaskDefinition *task = dequeueItem(queue);

      if (task) {
        if (task->function) task->function(task->data);
        free(task);
        return 1;
      }
    }
  }

  return 0;
}
