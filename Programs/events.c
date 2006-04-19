/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Team. All rights reserved.
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

#include <string.h>
#include <errno.h>
#include <sys/time.h>

#ifdef WINDOWS
typedef HANDLE InputOutputMonitor;
#else /* WINDOWS */
#include <sys/poll.h>
typedef struct pollfd InputOutputMonitor;
#endif /* WINDOWS */

#include "misc.h"
#include "queue.h"
#include "events.h"

typedef struct InputOutputEntryStruct InputOutputEntry;

typedef struct {
  InputOutputEntry *io;

  InputOutputListener respond;
  void *data;

  size_t error;
  size_t count;

  size_t size;
  unsigned char buffer[1];
} InputOutputOperation;

typedef struct {
  void (*prepareEntry) (InputOutputEntry *io);

  void (*startOperation) (InputOutputOperation *op);
  void (*finishOperation) (InputOutputOperation *op);

  void (*initializeMonitor) (InputOutputMonitor *monitor, const InputOutputEntry *io, const InputOutputOperation *op);
  int (*testMonitor) (const InputOutputMonitor *monitor);
} InputOutputMethods;

struct InputOutputEntryStruct {
  int fileDescriptor;
  const InputOutputMethods *methods;
  Queue *operations;

#ifdef WINDOWS
  OVERLAPPED ol;
#else /* WINDOWS */
  short events;
#endif /* WINDOWS */
};

typedef struct {
  int fileDescriptor;
  const InputOutputMethods *methods;
} InputOutputKey;

#ifdef WINDOWS
static int
monitorEvents (InputOutputMonitor *monitorArray, int monitorCount, int timeout) {
  return 0;
}
#else /* WINDOWS */
static void
prepareUnixInputEntry (InputOutputEntry *io) {
  io->events = POLLIN;
}

static void
prepareUnixOutputEntry (InputOutputEntry *io) {
  io->events = POLLOUT;
}

static void
finishUnixRead (InputOutputOperation *op) {
  int result = read(op->io->fileDescriptor, op->buffer, op->size);
  if (result == -1) {
    op->error = errno;
  } else {
    op->count = result;
  }
}

static void
finishUnixWrite (InputOutputOperation *op) {
  int result = write(op->io->fileDescriptor, op->buffer, op->size);
  if (result == -1) {
    op->error = errno;
  } else {
    op->count = result;
  }
}

static void
initializeUnixInputOutputMonitor (InputOutputMonitor *monitor, const InputOutputEntry *io, const InputOutputOperation *op) {
  monitor->fd = io->fileDescriptor;
  monitor->events = io->events;
  monitor->revents = 0;
}

static int
testUnixInputOutputMonitor (const InputOutputMonitor *monitor) {
  return monitor->revents != 0;
}

static int
monitorEvents (InputOutputMonitor *monitorArray, int monitorCount, int timeout) {
  int result = poll(monitorArray, monitorCount, timeout);
  if (result > 0) return 1;

  if (result == -1) {
    LogError("poll");
  }

  return 0;
}
#endif /* WINDOWS */

static void
deallocateInputOutputEntry (void *item, void *data) {
  InputOutputEntry *io = item;
  if (io->operations) deallocateQueue(io->operations);
  free(io);
}

static int
testInputOutputEntry (void *item, void *data) {
  const InputOutputEntry *io = item;
  const InputOutputKey *key = data;
  return (io->fileDescriptor == key->fileDescriptor) &&
         (io->methods == key->methods);
}

static void
deallocateInputOutputOperation (void *item, void *data) {
  InputOutputOperation *op = item;
  free(op);
}

static Queue *
getInputOutputEntries (int create) {
  static Queue *entries = NULL;

  if (!entries) {
    if (create) {
      if ((entries = newQueue(deallocateInputOutputEntry, NULL))) {
      }
    }
  }

  return entries;
}

static Element *
getInputOutputElement (int fileDescriptor, const InputOutputMethods *methods, int create) {
  Queue *entries = getInputOutputEntries(create);
  if (entries) {
    {
      InputOutputKey key;
      key.fileDescriptor = fileDescriptor;
      key.methods = methods;

      {
        Element *element = processQueue(entries, testInputOutputEntry, &key);
        if (element) return element;
      }
    }

    if (create) {
      InputOutputEntry *io;

      if ((io = malloc(sizeof(*io)))) {
        io->fileDescriptor = fileDescriptor;
        io->methods = methods;

        if ((io->operations = newQueue(deallocateInputOutputOperation, NULL))) {
          if (methods->prepareEntry) methods->prepareEntry(io);

          {
            Element *element = enqueueItem(entries, io);
            if (element) return element;
          }

          deallocateQueue(io->operations);
        }

        free(io);
      }
    }
  }

  return NULL;
}

static void
startInputOutputOperation (InputOutputOperation *op) {
  if (op->io->methods->startOperation) op->io->methods->startOperation(op);
}

static void
finishInputOutputOperation (InputOutputOperation *op) {
  if (op->io->methods->finishOperation) op->io->methods->finishOperation(op);
}

static int
createInputOutputOperation (
  int fileDescriptor,
  const InputOutputMethods *methods,
  InputOutputListener respond, void *data,
  size_t size, const void *buffer
) {
  InputOutputOperation *op;

  if ((op = malloc(sizeof(*op) - sizeof(op->buffer) + size))) {
    Element *ioElement;

    if ((ioElement = getInputOutputElement(fileDescriptor, methods, 1))) {
      InputOutputEntry *io = getElementItem(ioElement);
      int new = !getQueueSize(io->operations);
      Element *opElement;

      if ((opElement = enqueueItem(io->operations, op))) {
        op->io = io;
        op->respond = respond;
        op->data = data;
        op->error = 0;
        op->count = 0;
        op->size = size;
        if (buffer) memcpy(op->buffer, buffer, size);
        if (new) startInputOutputOperation(op);
        return 1;
      }

      if (new) deleteElement(ioElement);
    }

    free(op);
  }

  return 0;
}

static InputOutputOperation *
getInputOutputOperation (const InputOutputEntry *io) {
  return getElementItem(getQueueHead(io->operations));
}

int
asyncRead (
  int fileDescriptor, size_t size,
  InputOutputListener respond, void *data
) {
  static const InputOutputMethods methods = {
#ifdef WINDOWS
#else /* WINDOWS */
    .prepareEntry = prepareUnixInputEntry,
    .finishOperation = finishUnixRead,
    .initializeMonitor = initializeUnixInputOutputMonitor,
    .testMonitor = testUnixInputOutputMonitor
#endif /* WINDOWS */
  };
  return createInputOutputOperation(fileDescriptor, &methods, respond, data, size, NULL);
}

int
asyncWrite (
  int fileDescriptor, const void *buffer, size_t size,
  InputOutputListener respond, void *data
) {
  static const InputOutputMethods methods = {
#ifdef WINDOWS
#else /* WINDOWS */
    .prepareEntry = prepareUnixOutputEntry,
    .finishOperation = finishUnixWrite,
    .initializeMonitor = initializeUnixInputOutputMonitor,
    .testMonitor = testUnixInputOutputMonitor
#endif /* WINDOWS */
  };
  return createInputOutputOperation(fileDescriptor, &methods, respond, data, size, buffer);
}

typedef struct {
  InputOutputMonitor *monitor;
} AddInputOutputMonitorData;

static int
addInputOutputMonitor (void *item, void *data) {
  const InputOutputEntry *io = item;
  AddInputOutputMonitorData *add = data;

  io->methods->initializeMonitor(add->monitor++, io, getInputOutputOperation(io));
  return 0;
}

typedef struct {
  const InputOutputMonitor *monitor;
  InputOutputEntry *io;
} FindInputOutputMonitorData;

static int
findInputOutputMonitor (void *item, void *data) {
  InputOutputEntry *io = item;
  FindInputOutputMonitorData *find = data;
  int posted = io->methods->testMonitor(find->monitor);

  if (posted) {
    find->io = io;
    return 1;
  }

  find->monitor++;
  return 0;
}

void
processEvents (int timeout) {
  struct timeval start;
  gettimeofday(&start, NULL);

  do {
    Queue *ioEntries = getInputOutputEntries(0);
    int monitorCount = ioEntries? getQueueSize(ioEntries): 0;
    InputOutputMonitor *monitorArray = NULL;

    if (monitorCount) {
      if ((monitorArray = malloc(ARRAY_SIZE(monitorArray, monitorCount)))) {
        AddInputOutputMonitorData add;
        add.monitor = monitorArray;
        processQueue(ioEntries, addInputOutputMonitor, &add);
        monitorCount = add.monitor - monitorArray;
      } else {
        monitorCount = 0;
      }
    }

    if (monitorEvents(monitorArray, monitorCount, timeout)) {
      FindInputOutputMonitorData find;
      find.monitor = monitorArray;
      find.io = NULL;

      {
        Element *ioElement = processQueue(ioEntries, findInputOutputMonitor, &find);
        if (ioElement) {
          InputOutputEntry *io = find.io;
          Element *opElement = getQueueHead(io->operations);

          if (opElement) {
            InputOutputOperation *op = getElementItem(opElement);
            finishInputOutputOperation(op);
          }

          if (getQueueSize(io->operations)) {
            requeueElement(ioElement);
          } else {
            deleteElement(ioElement);
          }
        }
      }
    }

    if (monitorArray) free(monitorArray);
  } while ((timeout > 0) &&
           (millisecondsSince(&start) < timeout));
}
