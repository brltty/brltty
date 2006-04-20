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
  unsigned finished:1;

  int error;
  size_t count;

  InputOutputCallback callback;
  void *data;

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
  FileDescriptor fileDescriptor;
  const InputOutputMethods *methods;
  Queue *operations;

#ifdef WINDOWS
  OVERLAPPED ol;
#else /* WINDOWS */
  short events;
#endif /* WINDOWS */
};

typedef struct {
  FileDescriptor fileDescriptor;
  const InputOutputMethods *methods;
} InputOutputKey;

#ifdef WINDOWS
static void
prepareWindowsInputOutputEntry (InputOutputEntry *io) {
  ZeroMemory(&io->ol, sizeof(io->ol));
  io->ol.hEvent = INVALID_HANDLE_VALUE;
}

static int
allocateWindowsInputOutputResources (InputOutputEntry *io) {
  {
    HANDLE *event = &io->ol.hEvent;

    if (*event == INVALID_HANDLE_VALUE) {
      HANDLE handle;

      if (!(handle = CreateEvent(NULL, TRUE, FALSE, NULL))) {
        LogWindowsError("CreateEvent");
        return 0;
      }

      *event = handle;
    }

    if (!ResetEvent(*event)) {
      LogWindowsError("ResetEvent");
      return 0;
    }
  }

  return 1;
}

static int
setWindowsInputOutputResult (InputOutputOperation *op, DWORD success, DWORD count) {
  if (success) {
    op->count = count;
  } else {
    DWORD error = GetLastError();
    if (error == ERROR_IO_PENDING) return 0;

    if ((error == ERROR_HANDLE_EOF) || (error == ERROR_BROKEN_PIPE)) {
      op->count = 0;
    } else {
      op->error = error;
    }
  }

  op->finished = 1;
  return 1;
}

static void
startWindowsRead (InputOutputOperation *op) {
  InputOutputEntry *io = op->io;

  if (allocateWindowsInputOutputResources(io)) {
    DWORD count;
    DWORD success = ReadFile(io->fileDescriptor, op->buffer, op->size, &count, &io->ol);
    setWindowsInputOutputResult(op, success, count);
  }
}

static void
startWindowsWrite (InputOutputOperation *op) {
  InputOutputEntry *io = op->io;
  if (allocateWindowsInputOutputResources(io)) {
    DWORD count;
    DWORD success = WriteFile(io->fileDescriptor, op->buffer, op->size, &count, &io->ol);
    setWindowsInputOutputResult(op, success, count);
  }
}

static void
finishWindowsInputOutputOperation (InputOutputOperation *op) {
  InputOutputEntry *io = op->io;
  DWORD count;
  DWORD success = GetOverlappedResult(io->fileDescriptor, &io->ol, &count, FALSE);
  setWindowsInputOutputResult(op, success, count);
}

static void
initializeWindowsInputOutputMonitor (InputOutputMonitor *monitor, const InputOutputEntry *io, const InputOutputOperation *op) {
  *monitor = io->ol.hEvent;
}

static int
testWindowsInputOutputMonitor (const InputOutputMonitor *monitor) {
  DWORD result = WaitForSingleObject(*monitor, 0);
  if (result == WAIT_OBJECT_0) return 1;

  if (result == WAIT_FAILED) {
    LogWindowsError("WaitForSingleObject");
  }

  return 0;
}

static int
monitorEvents (InputOutputMonitor *monitors, int count, int timeout) {
  DWORD result = WaitForMultipleObjects(count, monitors, FALSE, timeout);
  if ((result >= WAIT_OBJECT_0) && (result < (WAIT_OBJECT_0 + count))) return 1;

  if (result == WAIT_FAILED) {
    LogWindowsError("WaitForMultipleObjects");
  }

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

static int
setUnixInputOutputResult (InputOutputOperation *op, ssize_t count) {
  if (count != -1) {
    op->count = count;
  } else {
    op->error = errno;
  }

  op->finished = 1;
  return 1;
}

static void
finishUnixRead (InputOutputOperation *op) {
  int result = read(op->io->fileDescriptor, op->buffer, op->size);
  setUnixInputOutputResult(op, result);
}

static void
finishUnixWrite (InputOutputOperation *op) {
  int result = write(op->io->fileDescriptor, op->buffer, op->size);
  setUnixInputOutputResult(op, result);
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
monitorEvents (InputOutputMonitor *monitors, int count, int timeout) {
  int result = poll(monitors, count, timeout);
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
getInputOutputElement (FileDescriptor fileDescriptor, const InputOutputMethods *methods, int create) {
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
  FileDescriptor fileDescriptor,
  const InputOutputMethods *methods,
  InputOutputCallback callback, void *data,
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
        op->finished = 0;

        op->error = 0;
        op->count = 0;

        op->callback = callback;
        op->data = data;

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
  FileDescriptor fileDescriptor,
  size_t size,
  InputOutputCallback callback, void *data
) {
  static const InputOutputMethods methods = {
#ifdef WINDOWS
    .prepareEntry = prepareWindowsInputOutputEntry,
    .startOperation = startWindowsRead,
    .finishOperation = finishWindowsInputOutputOperation,
    .initializeMonitor = initializeWindowsInputOutputMonitor,
    .testMonitor = testWindowsInputOutputMonitor
#else /* WINDOWS */
    .prepareEntry = prepareUnixInputEntry,
    .finishOperation = finishUnixRead,
    .initializeMonitor = initializeUnixInputOutputMonitor,
    .testMonitor = testUnixInputOutputMonitor
#endif /* WINDOWS */
  };
  return createInputOutputOperation(fileDescriptor, &methods, callback, data, size, NULL);
}

int
asyncWrite (
  FileDescriptor fileDescriptor,
  const void *buffer, size_t size,
  InputOutputCallback callback, void *data
) {
  static const InputOutputMethods methods = {
#ifdef WINDOWS
    .prepareEntry = prepareWindowsInputOutputEntry,
    .startOperation = startWindowsWrite,
    .finishOperation = finishWindowsInputOutputOperation,
    .initializeMonitor = initializeWindowsInputOutputMonitor,
    .testMonitor = testWindowsInputOutputMonitor
#else /* WINDOWS */
    .prepareEntry = prepareUnixOutputEntry,
    .finishOperation = finishUnixWrite,
    .initializeMonitor = initializeUnixInputOutputMonitor,
    .testMonitor = testUnixInputOutputMonitor
#endif /* WINDOWS */
  };
  return createInputOutputOperation(fileDescriptor, &methods, callback, data, size, buffer);
}

typedef struct {
  InputOutputMonitor *monitor;
} AddInputOutputMonitorData;

static int
addInputOutputMonitor (void *item, void *data) {
  const InputOutputEntry *io = item;
  AddInputOutputMonitorData *add = data;
  InputOutputOperation *op = getInputOutputOperation(io);
  if (op->finished) return 1;

  io->methods->initializeMonitor(add->monitor++, io, op);
  return 0;
}

typedef struct {
  const InputOutputMonitor *monitor;
} FindInputOutputMonitorData;

static int
findInputOutputMonitor (void *item, void *data) {
  InputOutputEntry *io = item;
  FindInputOutputMonitorData *find = data;
  if (io->methods->testMonitor(find->monitor)) return 1;

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
    Element *ioElement = NULL;

    if (monitorCount) {
      if ((monitorArray = malloc(ARRAY_SIZE(monitorArray, monitorCount)))) {
        AddInputOutputMonitorData add;
        add.monitor = monitorArray;
        ioElement = processQueue(ioEntries, addInputOutputMonitor, &add);
        if (!(monitorCount = add.monitor - monitorArray)) {
          free(monitorArray);
          monitorArray = NULL;
        }
      } else {
        monitorCount = 0;
      }
    }

    if (!ioElement) {
      if (monitorEvents(monitorArray, monitorCount, timeout)) {
        FindInputOutputMonitorData find;
        find.monitor = monitorArray;
        ioElement = processQueue(ioEntries, findInputOutputMonitor, &find);
      }
    }

    if (ioElement) {
      InputOutputEntry *io = getElementItem(ioElement);
      Element *opElement = getQueueHead(io->operations);
      InputOutputOperation *op = getElementItem(opElement);

      if (!op->finished) finishInputOutputOperation(op);
      {
        InputOutputResult result;
        result.data = op->data;
        result.buffer = op->buffer;
        result.size = op->size;
        result.error = op->error;
        result.count = op->count;
        op->callback(&result);
      }
      deleteElement(opElement);

      if ((opElement = getQueueHead(io->operations))) {
        op = getElementItem(opElement);
        startInputOutputOperation(op);
        requeueElement(ioElement);
      } else {
        deleteElement(ioElement);
      }
    }

    if (monitorArray) free(monitorArray);
  } while ((timeout > 0) &&
           (millisecondsSince(&start) < timeout));
}
