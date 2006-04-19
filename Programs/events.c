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
  InputOutputListener respond;
  void *data;
  size_t size;
  size_t offset;

#ifdef WINDOWS
  OVERLAPPED ol;
#else /* WINDOWS */
  short events;
#endif /* WINDOWS */

  unsigned char buffer[1];
} InputOutputOperation;

typedef struct {
  int (*prepareOperation) (InputOutputOperation *op);
  ssize_t (*startOperation) (int fileDescriptor, void *buffer, size_t size);
  ssize_t (*finishOperation) (int fileDescriptor, void *buffer, size_t size);

  void (*initializeMonitor) (InputOutputMonitor *monitor, const InputOutputEntry *io, const InputOutputOperation *op);
  int (*testMonitor) (const InputOutputMonitor *monitor);
} InputOutputMethods;

struct InputOutputEntryStruct {
  int fileDescriptor;
  const InputOutputMethods *methods;
  Queue *operations;
};

typedef struct {
  int fileDescriptor;
  const InputOutputMethods *methods;
} InputOutputKey;

#ifdef WINDOWS
#else /* WINDOWS */
static int
prepareUnixInputOperation (InputOutputOperation *op) {
  op->events = POLLIN;
  return 1;
}

static int
prepareUnixOutputOperation (InputOutputOperation *op) {
  op->events = POLLOUT;
  return 1;
}

static ssize_t
finishUnixRead (int fileDescriptor, void *buffer, size_t size) {
  return read(fileDescriptor, buffer, size);
}

static ssize_t
finishUnixWrite (int fileDescriptor, void *buffer, size_t size) {
  return write(fileDescriptor, buffer, size);
}

static void
initializeUnixInputOutputMonitor (InputOutputMonitor *monitor, const InputOutputEntry *io, const InputOutputOperation *op) {
  monitor->fd = io->fileDescriptor;
  monitor->events = op->events;
  monitor->revents = 0;
}

static int
testUnixInputOutputMonitor (const InputOutputMonitor *monitor) {
  return monitor->revents != 0;
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
          setQueueData(io->operations, io);

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
      Element *opElement;

      if ((opElement = enqueueItem(io->operations, op))) {
        op->respond = respond;
        op->data = data;
        op->size = size;
        op->offset = 0;
        if (buffer) memcpy(op->buffer, buffer, size);

        if (!methods->prepareOperation || methods->prepareOperation(op)) return 1;

        deleteElement(opElement);
        op = NULL;
      }

      if (!getQueueSize(io->operations)) deleteElement(ioElement);
    }

    if (op) free(op);
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
    .prepareOperation = prepareUnixInputOperation,
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
    .prepareOperation = prepareUnixOutputOperation,
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
processEvents (int milliseconds) {
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
      } else {
        monitorCount = 0;
      }
    }

    {
      int posted = 0;

#ifdef WINDOWS
#else /* WINDOWS */
      {
        int count = poll(monitorArray, monitorCount, milliseconds);
        if (count == -1) {
          LogError("poll");
          milliseconds = 0;
        } else if (count > 0) {
          posted = 1;
        }
      }
#endif /* WINDOWS */

      if (posted) {
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
              int respond = 0;
              InputOutputResponse response;

              response.error = 0;

              if (op->offset < op->size) {
                ssize_t opCount = io->methods->finishOperation(io->fileDescriptor,
                                                                  &op->buffer[op->offset],
                                                                  op->size - op->offset);

                if (opCount > 0) {
                  op->offset += opCount;
                } else {
                  if (opCount == -1) response.error = errno;
                  respond = 1;
                }
              }

              if (respond || (op->offset == op->size)) {
                response.data = op->data;
                response.buffer = (response.size = op->size)? op->buffer: NULL;
                response.count = op->offset;

                deleteElement(opElement);
                op->respond(&response);
              }
            }

            if (getQueueSize(io->operations)) {
              requeueElement(ioElement);
            } else {
              deleteElement(ioElement);
            }
          }
        }
      }
    }

    if (monitorArray) free(monitorArray);
  } while ((milliseconds > 0) &&
           (millisecondsSince(&start) < milliseconds));
}
