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
typedef HANDLE MonitorEntry;
#else /* WINDOWS */
#include <sys/poll.h>
typedef struct pollfd MonitorEntry;
#endif /* WINDOWS */

#include "misc.h"
#include "queue.h"
#include "async.h"

typedef struct FunctionEntryStruct FunctionEntry;

typedef struct {
  TransferCallback callback;
  size_t size;
  size_t count;
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

#ifdef WINDOWS
  OVERLAPPED ol;
#else /* WINDOWS */
  short events;
#endif /* WINDOWS */
};

typedef struct {
  FileDescriptor fileDescriptor;
  const FunctionMethods *methods;
} FunctionKey;

#ifdef WINDOWS
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

static int
setWindowsTransferResult (OperationEntry *operation, DWORD success, DWORD count) {
  TransferExtension *extension = operation->extension;

  if (success) {
    extension->count = count;
  } else {
    DWORD error = GetLastError();
    if (error == ERROR_IO_PENDING) return 0;

    if ((error == ERROR_HANDLE_EOF) || (error == ERROR_BROKEN_PIPE)) {
      extension->count = 0;
    } else {
      operation->error = error;
    }
  }

  operation->finished = 1;
  return 1;
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
                             &extension->buffer[extension->count],
                             extension->size - extension->count,
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
                              &extension->buffer[extension->count],
                              extension->size - extension->count,
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
#else /* WINDOWS */
static int
awaitOperation (MonitorEntry *monitors, int count, int timeout) {
  int result = poll(monitors, count, timeout);
  if (result > 0) return 1;

  if (result == -1) {
    LogError("poll");
  }

  return 0;
}

static void
initializeMonitor (MonitorEntry *monitor, const FunctionEntry *function, const OperationEntry *operation) {
  monitor->fd = function->fileDescriptor;
  monitor->events = function->events;
  monitor->revents = 0;
}

static int
testMonitor (const MonitorEntry *monitor) {
  return monitor->revents != 0;
}

static void
beginUnixInputFunction (FunctionEntry *function) {
  function->events = POLLIN;
}

static void
beginUnixOutputFunction (FunctionEntry *function) {
  function->events = POLLOUT;
}

static int
setUnixResult (OperationEntry *operation, ssize_t count) {
  TransferExtension *extension = operation->extension;

  if (count != -1) {
    extension->count = count;
  } else {
    operation->error = errno;
  }

  operation->finished = 1;
  return 1;
}

static void
finishUnixRead (OperationEntry *operation) {
  FunctionEntry *function = operation->function;
  TransferExtension *extension = operation->extension;
  int result = read(function->fileDescriptor,
                    &extension->buffer[extension->count],
                    extension->size - extension->count);
  setUnixResult(operation, result);
}

static void
finishUnixWrite (OperationEntry *operation) {
  FunctionEntry *function = operation->function;
  TransferExtension *extension = operation->extension;
  int result = write(function->fileDescriptor,
                     &extension->buffer[extension->count],
                     extension->size - extension->count);
  setUnixResult(operation, result);
}
#endif /* WINDOWS */

static int
invokeTransferCallback (OperationEntry *operation) {
  TransferExtension *extension = operation->extension;

  if (extension->callback) {
    TransferResult result;
    result.data = operation->data;
    result.buffer = extension->buffer;
    result.size = extension->size;
    result.error = operation->error;
    result.count = extension->count;

    {
      size_t count = extension->callback(&result);

      if (count < extension->count) {
        if (count) {
          memmove(extension->buffer, &extension->buffer[count], 
                  extension->count -= count);
        }

        return 1;
      }
    }
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
  TransferCallback callback, void *data,
  size_t size, const void *buffer
) {
  TransferExtension *extension;

  if ((extension = malloc(sizeof(*extension)))) {
    extension->callback = callback;
    extension->size = size;
    extension->count = 0;
    if (buffer) memcpy(extension->buffer, buffer, size);

    if (createOperation(fileDescriptor, methods, extension, data)) {
      return 1;
    }

    free(extension);
  }

  return 0;
}

static OperationEntry *
getFirstOperation (const FunctionEntry *function) {
  return getElementItem(getQueueHead(function->operations));
}

int
asyncRead (
  FileDescriptor fileDescriptor,
  size_t size,
  TransferCallback callback, void *data
) {
  static const FunctionMethods methods = {
#ifdef WINDOWS
    .beginFunction = beginWindowsFunction,
    .endFunction = endWindowsFunction,
    .startOperation = startWindowsRead,
    .finishOperation = finishWindowsTransferOperation,
#else /* WINDOWS */
    .beginFunction = beginUnixInputFunction,
    .finishOperation = finishUnixRead,
#endif /* WINDOWS */
    .invokeCallback = invokeTransferCallback
  };

  return createTransferOperation(fileDescriptor, &methods, callback, data, size, NULL);
}

int
asyncWrite (
  FileDescriptor fileDescriptor,
  const void *buffer, size_t size,
  TransferCallback callback, void *data
) {
  static const FunctionMethods methods = {
#ifdef WINDOWS
    .beginFunction = beginWindowsFunction,
    .endFunction = endWindowsFunction,
    .startOperation = startWindowsWrite,
    .finishOperation = finishWindowsTransferOperation,
#else /* WINDOWS */
    .beginFunction = beginUnixOutputFunction,
    .finishOperation = finishUnixWrite,
#endif /* WINDOWS */
    .invokeCallback = invokeTransferCallback
  };

  return createTransferOperation(fileDescriptor, &methods, callback, data, size, buffer);
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
asyncWait (int timeout) {
  long int elapsed = 0;
  struct timeval start;
  gettimeofday(&start, NULL);

  do {
    Queue *functions = getFunctionQueue(0);
    int monitorCount = functions? getQueueSize(functions): 0;
    MonitorEntry *monitorArray = NULL;
    Element *functionElement = NULL;

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

      if (!operation->finished) finishOperation(operation);
      if (function->methods->invokeCallback(operation)) {
        operation->finished = 0;
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
  } while ((elapsed = millisecondsSince(&start)) < timeout);
}
