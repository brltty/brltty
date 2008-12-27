/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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
#include <errno.h>
#include <sys/time.h>

#ifdef __MSDOS__
#include "sys_msdos.h"
#endif /* __MSDOS__ */

#if defined(__MINGW32__)

typedef HANDLE MonitorEntry;

#elif defined(HAVE_SYS_POLL_H)

#include <sys/poll.h>
typedef struct pollfd MonitorEntry;

#else /* monitor definitions */

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */

typedef struct {
  int size;
  fd_set set;
} SelectDescriptor;

static SelectDescriptor selectDescriptor_read;
static SelectDescriptor selectDescriptor_write;

typedef struct {
  fd_set *selectSet;
  FileDescriptor fileDescriptor;
} MonitorEntry;

#endif /* monitor definitions */

#include "misc.h"
#include "queue.h"
#include "async.h"

typedef struct FunctionEntryStruct FunctionEntry;

typedef union {
  struct {
    AsyncInputCallback callback;
    unsigned end:1;
  } input;

  struct {
    AsyncOutputCallback callback;
  } output;
} TransferDirectionUnion;

typedef struct {
  TransferDirectionUnion direction;
  size_t size;
  size_t length;
  unsigned char buffer[];
} TransferExtension;

typedef struct {
  FunctionEntry *function;
  void *extension;
  void *data;
  unsigned finished:1;
  int error;
} OperationEntry;

typedef struct {
  void (*beginFunction) (FunctionEntry *function);
  void (*endFunction) (FunctionEntry *function);
  void (*startOperation) (OperationEntry *operation);
  void (*finishOperation) (OperationEntry *operation);
  int (*invokeCallback) (OperationEntry *operation);
} FunctionMethods;

struct FunctionEntryStruct {
  FileDescriptor fileDescriptor;
  const FunctionMethods *methods;
  Queue *operations;

#if defined(__MINGW32__)
  OVERLAPPED ol;
#elif defined(HAVE_SYS_POLL_H)
  short pollEvents;
#else /* monitor definitions */
  SelectDescriptor *selectDescriptor;
#endif /* monitor definitions */
};

typedef struct {
  FileDescriptor fileDescriptor;
  const FunctionMethods *methods;
} FunctionKey;

#ifdef __MINGW32__
static void
prepareMonitors (void) {
}

static int
awaitOperation (MonitorEntry *monitors, int count, int timeout) {
  if (count) {
    DWORD result = WaitForMultipleObjects(count, monitors, FALSE, timeout);
    if ((result >= WAIT_OBJECT_0) && (result < (WAIT_OBJECT_0 + count))) return 1;

    if (result == WAIT_FAILED) {
      LogWindowsError("WaitForMultipleObjects");
    }
  } else {
    approximateDelay(timeout);
  }

  return 0;
}

static void
initializeMonitor (MonitorEntry *monitor, const FunctionEntry *function, const OperationEntry *operation) {
  *monitor = function->ol.hEvent;
}

static int
testMonitor (const MonitorEntry *monitor) {
  DWORD result = WaitForSingleObject(*monitor, 0);
  if (result == WAIT_OBJECT_0) return 1;

  if (result == WAIT_FAILED) {
    LogWindowsError("WaitForSingleObject");
  }

  return 0;
}

static int
allocateWindowsEvent (HANDLE *event) {
  if (*event == INVALID_HANDLE_VALUE) {
    HANDLE handle = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!handle) return 0;
    *event = handle;
  }

  return ResetEvent(*event);
}

static void
deallocateWindowsEvent (HANDLE *event) {
  if (*event != INVALID_HANDLE_VALUE) {
    CloseHandle(*event);
    *event = INVALID_HANDLE_VALUE;
  }
}

static int
allocateWindowsResources (OperationEntry *operation) {
  FunctionEntry *function = operation->function;

  if (allocateWindowsEvent(&function->ol.hEvent)) {
    return 1;
  }

  operation->finished = 1;
  operation->error = GetLastError();
  return 0;
}

static void
setWindowsTransferResult (OperationEntry *operation, DWORD success, DWORD count) {
  TransferExtension *extension = operation->extension;

  if (success) {
    extension->length += count;
  } else {
    DWORD error = GetLastError();
    if (error == ERROR_IO_PENDING) return;

    if ((error == ERROR_HANDLE_EOF) || (error == ERROR_BROKEN_PIPE)) {
      extension->direction.input.end = 1;
    } else {
      operation->error = error;
    }
  }

  operation->finished = 1;
}

static void
beginWindowsFunction (FunctionEntry *function) {
  ZeroMemory(&function->ol, sizeof(function->ol));
  function->ol.hEvent = INVALID_HANDLE_VALUE;
}

static void
endWindowsFunction (FunctionEntry *function) {
  deallocateWindowsEvent(&function->ol.hEvent);
}

static void
startWindowsRead (OperationEntry *operation) {
  FunctionEntry *function = operation->function;
  TransferExtension *extension = operation->extension;

  if (allocateWindowsResources(operation)) {
    DWORD count;
    DWORD success = ReadFile(function->fileDescriptor,
                             &extension->buffer[extension->length],
                             extension->size - extension->length,
                             &count, &function->ol);
    setWindowsTransferResult(operation, success, count);
  }
}

static void
startWindowsWrite (OperationEntry *operation) {
  FunctionEntry *function = operation->function;
  TransferExtension *extension = operation->extension;

  if (allocateWindowsResources(operation)) {
    DWORD count;
    DWORD success = WriteFile(function->fileDescriptor,
                              &extension->buffer[extension->length],
                              extension->size - extension->length,
                              &count, &function->ol);
    setWindowsTransferResult(operation, success, count);
  }
}

static void
finishWindowsTransferOperation (OperationEntry *operation) {
  FunctionEntry *function = operation->function;
  DWORD count;
  DWORD success = GetOverlappedResult(function->fileDescriptor, &function->ol, &count, FALSE);
  setWindowsTransferResult(operation, success, count);
}
#else /* __MINGW32__ */
#ifdef HAVE_SYS_POLL_H
static void
prepareMonitors (void) {
}

static int
awaitOperation (MonitorEntry *monitors, int count, int timeout) {
  int result = poll(monitors, count, timeout);
  if (result > 0) return 1;

  if (result == -1) {
    if (errno != EINTR) LogError("poll");
  }

  return 0;
}

static void
initializeMonitor (MonitorEntry *monitor, const FunctionEntry *function, const OperationEntry *operation) {
  monitor->fd = function->fileDescriptor;
  monitor->events = function->pollEvents;
  monitor->revents = 0;
}

static int
testMonitor (const MonitorEntry *monitor) {
  return monitor->revents != 0;
}

static void
beginUnixInputFunction (FunctionEntry *function) {
  function->pollEvents = POLLIN;
}

static void
beginUnixOutputFunction (FunctionEntry *function) {
  function->pollEvents = POLLOUT;
}
#else /* HAVE_SYS_POLL_H */
static void
prepareSelectDescriptor (SelectDescriptor *descriptor) {
  FD_ZERO(&descriptor->set);
  descriptor->size = 0;
}

static void
prepareMonitors (void) {
  prepareSelectDescriptor(&selectDescriptor_read);
  prepareSelectDescriptor(&selectDescriptor_write);
}

static fd_set *
getSelectSet (SelectDescriptor *descriptor) {
  return descriptor->size? &descriptor->set: NULL;
}

static int
doSelect (int setSize, fd_set *readSet, fd_set *writeSet, int timeout) {
  struct timeval time;

  time.tv_sec = timeout / 1000;
  time.tv_usec = timeout % 1000 * 1000;

  {
    int result = select(setSize, readSet, writeSet, NULL, &time);
    if (result > 0) return 1;

    if (result == -1) {
      if (errno != EINTR) LogError("select");
    }

    return 0;
  }
}

static int
awaitOperation (MonitorEntry *monitors, int count, int timeout) {
  int setSize = MAX(selectDescriptor_read.size, selectDescriptor_write.size);
  fd_set *readSet = getSelectSet(&selectDescriptor_read);
  fd_set *writeSet = getSelectSet(&selectDescriptor_write);

#ifdef __MSDOS__
  int elapsed = 0;

  do {
    fd_set readSet1, writeSet1;

    if (readSet) readSet1 = *readSet;
    if (writeSet) writeSet1 = *writeSet;

    if (doSelect(setSize, (readSet? &readSet1: NULL), (writeSet? &writeSet1: NULL), 0)) {
      if (readSet) *readSet = readSet1;
      if (writeSet) *writeSet = writeSet1;
      return 1;
    }
  } while ((elapsed += tsr_usleep(1000)) < timeout);
#else /* __MSDOS__ */
  if (doSelect(setSize, readSet, writeSet, timeout)) return 1;
#endif /* __MSDOS__ */

  return 0;
}

static void
initializeMonitor (MonitorEntry *monitor, const FunctionEntry *function, const OperationEntry *operation) {
  monitor->selectSet = &function->selectDescriptor->set;
  monitor->fileDescriptor = function->fileDescriptor;

  FD_SET(function->fileDescriptor, &function->selectDescriptor->set);
  if (function->fileDescriptor >= function->selectDescriptor->size) function->selectDescriptor->size = function->fileDescriptor + 1;
}

static int
testMonitor (const MonitorEntry *monitor) {
  return FD_ISSET(monitor->fileDescriptor, monitor->selectSet);
}

static void
beginUnixInputFunction (FunctionEntry *function) {
  function->selectDescriptor = &selectDescriptor_read;
}

static void
beginUnixOutputFunction (FunctionEntry *function) {
  function->selectDescriptor = &selectDescriptor_write;
}
#endif /* HAVE_SYS_POLL_H */

static void
setUnixTransferResult (OperationEntry *operation, ssize_t count) {
  TransferExtension *extension = operation->extension;

  if (count == -1) {
    operation->error = errno;
  } else if (count == 0) {
    extension->direction.input.end = 1;
  } else {
    extension->length += count;
  }

  operation->finished = 1;
}

static void
finishUnixRead (OperationEntry *operation) {
  FunctionEntry *function = operation->function;
  TransferExtension *extension = operation->extension;
  int result = read(function->fileDescriptor,
                    &extension->buffer[extension->length],
                    extension->size - extension->length);
  setUnixTransferResult(operation, result);
}

static void
finishUnixWrite (OperationEntry *operation) {
  FunctionEntry *function = operation->function;
  TransferExtension *extension = operation->extension;
  int result = write(function->fileDescriptor,
                     &extension->buffer[extension->length],
                     extension->size - extension->length);
  setUnixTransferResult(operation, result);
}
#endif /* __MINGW32__ */

static int
invokeInputCallback (OperationEntry *operation) {
  TransferExtension *extension = operation->extension;
  size_t count;

  if (!extension->direction.input.callback) return 0;

  {
    AsyncInputResult result;
    result.data = operation->data;
    result.buffer = extension->buffer;
    result.size = extension->size;
    result.length = extension->length;
    result.error = operation->error;
    result.end = extension->direction.input.end;
    count = extension->direction.input.callback(&result);
  }

  if (operation->error) return 0;
  if (extension->direction.input.end) return 0;

  if (count)
    memmove(extension->buffer, &extension->buffer[count],
            extension->length -= count);
  return 1;
}

static int
invokeOutputCallback (OperationEntry *operation) {
  TransferExtension *extension = operation->extension;

  if (!operation->error && (extension->length < extension->size)) return 1;

  if (extension->direction.output.callback) {
    AsyncOutputResult result;
    result.data = operation->data;
    result.buffer = extension->buffer;
    result.size = extension->size;
    result.error = operation->error;
    extension->direction.output.callback(&result);
  }

  return 0;
}

static void
deallocateFunctionEntry (void *item, void *data) {
  FunctionEntry *function = item;
  if (function->operations) deallocateQueue(function->operations);
  if (function->methods->endFunction) function->methods->endFunction(function);
  free(function);
}

static int
testFunctionEntry (void *item, void *data) {
  const FunctionEntry *function = item;
  const FunctionKey *key = data;
  return (function->fileDescriptor == key->fileDescriptor) &&
         (function->methods == key->methods);
}

static void
deallocateOperationEntry (void *item, void *data) {
  OperationEntry *operation = item;
  if (operation->extension) free(operation->extension);
  free(operation);
}

static Queue *
getFunctionQueue (int create) {
  static Queue *functions = NULL;

  if (!functions) {
    if (create) {
      if ((functions = newQueue(deallocateFunctionEntry, NULL))) {
      }
    }
  }

  return functions;
}

static Element *
getFunctionElement (FileDescriptor fileDescriptor, const FunctionMethods *methods, int create) {
  Queue *functions = getFunctionQueue(create);
  if (functions) {
    {
      FunctionKey key;
      key.fileDescriptor = fileDescriptor;
      key.methods = methods;

      {
        Element *element = processQueue(functions, testFunctionEntry, &key);
        if (element) return element;
      }
    }

    if (create) {
      FunctionEntry *function;

      if ((function = malloc(sizeof(*function)))) {
        function->fileDescriptor = fileDescriptor;
        function->methods = methods;

        if ((function->operations = newQueue(deallocateOperationEntry, NULL))) {
          if (methods->beginFunction) methods->beginFunction(function);

          {
            Element *element = enqueueItem(functions, function);
            if (element) return element;
          }

          deallocateQueue(function->operations);
        }

        free(function);
      }
    }
  }

  return NULL;
}

static void
startOperation (OperationEntry *operation) {
  if (operation->function->methods->startOperation) operation->function->methods->startOperation(operation);
}

static void
finishOperation (OperationEntry *operation) {
  if (operation->function->methods->finishOperation) operation->function->methods->finishOperation(operation);
}

static int
createOperation (
  FileDescriptor fileDescriptor,
  const FunctionMethods *methods,
  void *extension,
  void *data
) {
  OperationEntry *operation;

  if ((operation = malloc(sizeof(*operation)))) {
    Element *functionElement;

    if ((functionElement = getFunctionElement(fileDescriptor, methods, 1))) {
      FunctionEntry *function = getElementItem(functionElement);
      int new = !getQueueSize(function->operations);
      Element *operationElement;

      if ((operationElement = enqueueItem(function->operations, operation))) {
        operation->function = function;
        operation->extension = extension;
        operation->data = data;
        operation->finished = 0;
        operation->error = 0;

        if (new) startOperation(operation);
        return 1;
      }

      if (new) deleteElement(functionElement);
    }

    free(operation);
  }

  return 0;
}

static int
createTransferOperation (
  FileDescriptor fileDescriptor,
  const FunctionMethods *methods,
  const TransferDirectionUnion *direction,
  size_t size, const void *buffer,
  void *data
) {
  TransferExtension *extension;

  if ((extension = malloc(sizeof(*extension) + size))) {
    extension->direction = *direction;
    extension->size = size;
    extension->length = 0;
    if (buffer) memcpy(extension->buffer, buffer, size);

    if (createOperation(fileDescriptor, methods, extension, data)) return 1;

    free(extension);
  }

  return 0;
}

static int
createInputOperation (
  FileDescriptor fileDescriptor,
  const FunctionMethods *methods,
  AsyncInputCallback callback,
  size_t size,
  void *data
) {
  TransferDirectionUnion direction;
  direction.input.callback = callback;
  direction.input.end = 0;
  return createTransferOperation(fileDescriptor, methods, &direction, size, NULL, data);
}

static int
createOutputOperation (
  FileDescriptor fileDescriptor,
  const FunctionMethods *methods,
  AsyncOutputCallback callback,
  size_t size, const void *buffer,
  void *data
) {
  TransferDirectionUnion direction;
  direction.output.callback = callback;
  return createTransferOperation(fileDescriptor, methods, &direction, size, buffer, data);
}

static OperationEntry *
getFirstOperation (const FunctionEntry *function) {
  return getElementItem(getQueueHead(function->operations));
}

int
asyncRead (
  FileDescriptor fileDescriptor,
  size_t size,
  AsyncInputCallback callback, void *data
) {
  static const FunctionMethods methods = {
#ifdef __MINGW32__
    .beginFunction = beginWindowsFunction,
    .endFunction = endWindowsFunction,
    .startOperation = startWindowsRead,
    .finishOperation = finishWindowsTransferOperation,
#else /* __MINGW32__ */
    .beginFunction = beginUnixInputFunction,
    .finishOperation = finishUnixRead,
#endif /* __MINGW32__ */
    .invokeCallback = invokeInputCallback
  };

  return createInputOperation(fileDescriptor, &methods, callback, size, data);
}

int
asyncWrite (
  FileDescriptor fileDescriptor,
  const void *buffer, size_t size,
  AsyncOutputCallback callback, void *data
) {
  static const FunctionMethods methods = {
#ifdef __MINGW32__
    .beginFunction = beginWindowsFunction,
    .endFunction = endWindowsFunction,
    .startOperation = startWindowsWrite,
    .finishOperation = finishWindowsTransferOperation,
#else /* __MINGW32__ */
    .beginFunction = beginUnixOutputFunction,
    .finishOperation = finishUnixWrite,
#endif /* __MINGW32__ */
    .invokeCallback = invokeOutputCallback
  };

  return createOutputOperation(fileDescriptor, &methods, callback, size, buffer, data);
}

typedef struct {
  struct timeval time;
  AsyncAlarmCallback callback;
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
  if (alarm2->time.tv_sec < alarm1->time.tv_sec) return 0;
  if (alarm2->time.tv_sec > alarm1->time.tv_sec) return 1;
  return alarm2->time.tv_usec > alarm1->time.tv_usec;
}

static Queue *
getAlarmQueue (int create) {
  static Queue *alarms = NULL;

  if (!alarms) {
    if (create) {
      if ((alarms = newQueue(deallocateAlarmEntry, compareAlarmEntries))) {
      }
    }
  }

  return alarms;
}

static void
normalizeTime (struct timeval *time) {
  time->tv_sec += time->tv_usec / 1000000;
  time->tv_usec = time->tv_usec % 1000000;
}

static void
adjustTime (struct timeval *time, int amount) {
  int quotient = amount / 1000;
  int remainder = amount % 1000;

  if (remainder < 0) remainder += 1000, --quotient;
  time->tv_sec += quotient;
  time->tv_usec += remainder * 1000;
  normalizeTime(time);
}

int
asyncAbsoluteAlarm (
  const struct timeval *time,
  AsyncAlarmCallback callback,
  void *data
) {
  Queue *alarms;

  if ((alarms = getAlarmQueue(1))) {
    AlarmEntry *alarm;

    if ((alarm = malloc(sizeof(*alarm)))) {
      alarm->time = *time;
      alarm->callback = callback;
      alarm->data = data;

      if (enqueueItem(alarms, alarm)) return 1;

      free(alarm);
    }
  }

  return 0;
}

int
asyncRelativeAlarm (
  int interval,
  AsyncAlarmCallback callback,
  void *data
) {
  struct timeval time;
  gettimeofday(&time, NULL);
  adjustTime(&time, interval);
  return asyncAbsoluteAlarm(&time, callback, data);
}

typedef struct {
  MonitorEntry *monitor;
} AddMonitorData;

static int
addMonitor (void *item, void *data) {
  const FunctionEntry *function = item;
  AddMonitorData *add = data;
  OperationEntry *operation = getFirstOperation(function);
  if (operation->finished) return 1;

  initializeMonitor(add->monitor++, function, operation);
  return 0;
}

typedef struct {
  const MonitorEntry *monitor;
} FindMonitorData;

static int
findMonitor (void *item, void *data) {
/*FunctionEntry *function = item;*/
  FindMonitorData *find = data;
  if (testMonitor(find->monitor)) return 1;

  find->monitor++;
  return 0;
}

void
asyncWait (int duration) {
  long int elapsed = -1;
  struct timeval start;

  do {
    long int timeout = duration;
    Queue *functions = getFunctionQueue(0);
    int monitorCount = functions? getQueueSize(functions): 0;
    MonitorEntry *monitorArray = NULL;
    Element *functionElement = NULL;

    if (elapsed < 0) {
      elapsed = 0;
      gettimeofday(&start, NULL);
    }

    {
      Queue *alarms = getAlarmQueue(0);
      if (alarms) {
        Element *element = getQueueHead(alarms);
        if (element) {
          AlarmEntry *alarm = getElementItem(element);
          long int milliseconds = millisecondsBetween(&start, &alarm->time);
          if (milliseconds <= elapsed) {
            AsyncAlarmCallback callback = alarm->callback;
            void *data = alarm->data;
            deleteElement(element);
            callback(data);
            continue;
          }

          if (milliseconds < timeout) timeout = milliseconds;
        }
      }
    }

    prepareMonitors();
    if (monitorCount) {
      if ((monitorArray = malloc(ARRAY_SIZE(monitorArray, monitorCount)))) {
        AddMonitorData add;
        add.monitor = monitorArray;
        functionElement = processQueue(functions, addMonitor, &add);

        if (!(monitorCount = add.monitor - monitorArray)) {
          free(monitorArray);
          monitorArray = NULL;
        }
      } else {
        monitorCount = 0;
      }
    }

    if (!functionElement) {
      if (awaitOperation(monitorArray, monitorCount, timeout-elapsed)) {
        FindMonitorData find;
        find.monitor = monitorArray;
        functionElement = processQueue(functions, findMonitor, &find);
      }
    }

    if (functionElement) {
      FunctionEntry *function = getElementItem(functionElement);
      Element *operationElement = getQueueHead(function->operations);
      OperationEntry *operation = getElementItem(operationElement);
      TransferExtension *extension = operation->extension;

      if (!operation->finished) finishOperation(operation);
      if (function->methods->invokeCallback(operation)) {
        operation->finished = extension->length > 0;
        operation->error = 0;
      } else {
        deleteElement(operationElement);
      }

      if ((operationElement = getQueueHead(function->operations))) {
        operation = getElementItem(operationElement);
        startOperation(operation);
        requeueElement(functionElement);
      } else {
        deleteElement(functionElement);
      }
    }

    if (monitorArray) free(monitorArray);
  } while ((elapsed = millisecondsSince(&start)) < duration);
}
