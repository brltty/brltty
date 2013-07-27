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
#include <errno.h>

#ifdef __MSDOS__
#include "sys_msdos.h"
#endif /* __MSDOS__ */

#undef ASYNC_CAN_MONITOR_IO
#if defined(__MINGW32__)
#define ASYNC_CAN_MONITOR_IO

typedef HANDLE MonitorEntry;

#elif defined(HAVE_SYS_POLL_H)
#define ASYNC_CAN_MONITOR_IO

#include <sys/poll.h>
typedef struct pollfd MonitorEntry;

#elif defined(HAVE_SELECT)
#define ASYNC_CAN_MONITOR_IO

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else /* HAVE_SYS_SELECT_H */
#include <sys/time.h>
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

#include "log.h"
#include "timing.h"
#include "queue.h"
#include "async.h"

struct AsyncHandleStruct {
  Element *element;
  int identifier;
};

typedef struct {
  void (*cancel) (Element *element);
} QueueMethods;

typedef struct FunctionEntryStruct FunctionEntry;

typedef struct {
  AsyncMonitorCallback callback;
} MonitorExtension;

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

  MonitorEntry *monitor;
  int error;

  unsigned active:1;
  unsigned cancel:1;
  unsigned finished:1;
} OperationEntry;

typedef struct {
  const char *functionName;

  void (*beginFunction) (FunctionEntry *function);
  void (*endFunction) (FunctionEntry *function);

  void (*startOperation) (OperationEntry *operation);
  void (*finishOperation) (OperationEntry *operation);
  void (*cancelOperation) (OperationEntry *operation);

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
#elif defined(HAVE_SELECT)
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
      logWindowsSystemError("WaitForMultipleObjects");
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
testMonitor (const MonitorEntry *monitor, const FunctionEntry *function) {
  DWORD result = WaitForSingleObject(*monitor, 0);
  if (result == WAIT_OBJECT_0) return 1;

  if (result == WAIT_FAILED) {
    logWindowsSystemError("WaitForSingleObject");
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
    if (error == ERROR_IO_INCOMPLETE) return;

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

static void
cancelWindowsTransferOperation (OperationEntry *operation) {
  FunctionEntry *function = operation->function;
  DWORD count;

  if (CancelIoEx(function->fileDescriptor, &function->ol)) {
    GetOverlappedResult(function->fileDescriptor, &function->ol, &count, TRUE);
  }
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
    if (errno != EINTR) logSystemError("poll");
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
testMonitor (const MonitorEntry *monitor, const FunctionEntry *function) {
  return (monitor->revents & function->pollEvents) != 0;
}

static void
beginUnixInputFunction (FunctionEntry *function) {
  function->pollEvents = POLLIN;
}

static void
beginUnixOutputFunction (FunctionEntry *function) {
  function->pollEvents = POLLOUT;
}

#elif defined(HAVE_SELECT)

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
      if (errno != EINTR) logSystemError("select");
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
testMonitor (const MonitorEntry *monitor, const FunctionEntry *function) {
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

#endif /* Unix I/O monitoring capabilities */

#ifdef ASYNC_CAN_MONITOR_IO
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
#endif /* ASYNC_CAN_MONITOR_IO */
#endif /* __MINGW32__ */

#ifdef ASYNC_CAN_MONITOR_IO
static void
deallocateFunctionEntry (void *item, void *data) {
  FunctionEntry *function = item;
  if (function->operations) deallocateQueue(function->operations);
  if (function->methods->endFunction) function->methods->endFunction(function);
  free(function);
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

static OperationEntry *
getFirstOperation (const FunctionEntry *function) {
  Element *element = getQueueHead(function->operations);

  if (element) return getElementItem(element);
  return NULL;
}

static void
startOperation (OperationEntry *operation) {
  if (operation->function->methods->startOperation) {
    operation->function->methods->startOperation(operation);
  }
}

static void
finishOperation (OperationEntry *operation) {
  if (operation->function->methods->finishOperation) {
    operation->function->methods->finishOperation(operation);
  }
}

typedef struct {
  MonitorEntry *monitor;
} AddMonitorData;

static int
addMonitor (void *item, void *data) {
  const FunctionEntry *function = item;
  AddMonitorData *add = data;
  OperationEntry *operation = getFirstOperation(function);

  if (operation) {
    operation->monitor = NULL;

    if (!operation->active) {
      if (operation->finished) {
        return 1;
      }

      operation->monitor = add->monitor++;
      initializeMonitor(operation->monitor, function, operation);
    }
  }

  return 0;
}

static int
findMonitor (void *item, void *data) {
  FunctionEntry *function = item;
  OperationEntry *operation = getFirstOperation(function);

  if (operation) {
    if (operation->monitor) {
      if (testMonitor(operation->monitor, function)) {
        return 1;
      }
    }
  }

  return 0;
}

static int
invokeMonitorCallback (OperationEntry *operation) {
  MonitorExtension *extension = operation->extension;
  AsyncMonitorCallback callback = extension->callback;

  if (callback) {
    const AsyncMonitorResult result = {
      .data = operation->data,
      .fileDescriptor = operation->function->fileDescriptor
    };

    if (callback(&result)) return 1;
  }

  return 0;
}

static int
invokeInputCallback (OperationEntry *operation) {
  TransferExtension *extension = operation->extension;
  size_t count;

  if (!extension->direction.input.callback) return 0;

  {
    const AsyncInputResult result = {
      .data = operation->data,
      .buffer = extension->buffer,
      .size = extension->size,
      .length = extension->length,
      .error = operation->error,
      .end = extension->direction.input.end
    };

    count = extension->direction.input.callback(&result);
  }

  if (operation->error) return 0;
  if (extension->direction.input.end) return 0;

  operation->finished = 0;
  if (count) {
    memmove(extension->buffer, &extension->buffer[count],
            extension->length -= count);
    if (extension->length > 0) operation->finished = 1;
  }

  return 1;
}

static int
invokeOutputCallback (OperationEntry *operation) {
  TransferExtension *extension = operation->extension;

  if (!operation->error && (extension->length < extension->size)) {
    operation->finished = 0;
    return 1;
  }

  if (extension->direction.output.callback) {
    const AsyncOutputResult result = {
      .data = operation->data,
      .buffer = extension->buffer,
      .size = extension->size,
      .error = operation->error
    };

    extension->direction.output.callback(&result);
  }

  return 0;
}

static void
deallocateOperationEntry (void *item, void *data) {
  OperationEntry *operation = item;
  if (operation->extension) free(operation->extension);
  free(operation);
}

static void
cancelOperation (Element *operationElement) {
  OperationEntry *operation = getElementItem(operationElement);

  if (operation->active) {
    operation->cancel = 1;
  } else {
    FunctionEntry *function = operation->function;
    int isFirstOperation = operationElement == getQueueHead(function->operations);

    if (isFirstOperation) {
      if (!operation->finished) {
        if (operation->function->methods->cancelOperation) {
          operation->function->methods->cancelOperation(operation);
        }
      }
    }

    if (getQueueSize(function->operations) == 1) {
      Queue *functionQueue = getFunctionQueue(0);
      Element *functionElement = findElementWithItem(functionQueue, function);

      deleteElement(functionElement);
    } else {
      deleteElement(operationElement);

      if (isFirstOperation) {
        operationElement = getQueueHead(function->operations);
        operation = getElementItem(operationElement);

        if (!operation->finished) startOperation(operation);
      }
    }
  }
}

static int
testFunctionEntry (const void *item, const void *data) {
  const FunctionEntry *function = item;
  const FunctionKey *key = data;
  return (function->fileDescriptor == key->fileDescriptor) &&
         (function->methods == key->methods);
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
        Element *element = findElement(functions, testFunctionEntry, &key);
        if (element) return element;
      }
    }

    if (create) {
      FunctionEntry *function;

      if ((function = malloc(sizeof(*function)))) {
        function->fileDescriptor = fileDescriptor;
        function->methods = methods;

        if ((function->operations = newQueue(deallocateOperationEntry, NULL))) {
          {
            static QueueMethods methods = {
              .cancel = cancelOperation
            };

            setQueueData(function->operations, &methods);
          }

          if (methods->beginFunction) methods->beginFunction(function);

          {
            Element *element = enqueueItem(functions, function);
            if (element) return element;
          }

          deallocateQueue(function->operations);
        }

        free(function);
      } else {
        logMallocError();
      }
    }
  }

  return NULL;
}

static Element *
newOperation (
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
      int isFirstOperation = !getQueueSize(function->operations);
      Element *operationElement = enqueueItem(function->operations, operation);

      if (operationElement) {
        operation->function = function;
        operation->extension = extension;
        operation->data = data;

        operation->monitor = NULL;
        operation->error = 0;

        operation->active = 0;
        operation->cancel = 0;
        operation->finished = 0;

        if (isFirstOperation) startOperation(operation);
        return operationElement;
      }

      if (isFirstOperation) deleteElement(functionElement);
    }

    free(operation);
  } else {
    logMallocError();
  }

  return NULL;
}

typedef struct {
  FileDescriptor fileDescriptor;
  const FunctionMethods *methods;
  AsyncMonitorCallback callback;
  void *data;
} MonitorOperationParameters;

static Element *
newMonitorOperation (const void *parameters) {
  const MonitorOperationParameters *mop = parameters;
  MonitorExtension *extension;

  if ((extension = malloc(sizeof(*extension)))) {
    extension->callback = mop->callback;

    {
      Element *element = newOperation(mop->fileDescriptor, mop->methods, extension, mop->data);

      if (element) return element;
    }

    free(extension);
  } else {
    logMallocError();
  }

  return NULL;
}

static Element *
newTransferOperation (
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

    {
      Element *element = newOperation(fileDescriptor, methods, extension, data);

      if (element) return element;
    }

    free(extension);
  } else {
    logMallocError();
  }

  return NULL;
}

typedef struct {
  FileDescriptor fileDescriptor;
  size_t size;
  AsyncInputCallback callback;
  void *data;
} InputOperationParameters;

static Element *
newInputOperation (const void *parameters) {
  const InputOperationParameters *iop = parameters;

  TransferDirectionUnion direction = {
    .input = {
      .callback = iop->callback,
      .end = 0
    }
  };

  static const FunctionMethods methods = {
    .functionName = "transferInput",

#ifdef __MINGW32__
    .beginFunction = beginWindowsFunction,
    .endFunction = endWindowsFunction,
    .startOperation = startWindowsRead,
    .finishOperation = finishWindowsTransferOperation,
    .cancelOperation = cancelWindowsTransferOperation,
#else /* __MINGW32__ */
    .beginFunction = beginUnixInputFunction,
    .finishOperation = finishUnixRead,
#endif /* __MINGW32__ */

    .invokeCallback = invokeInputCallback
  };

  return newTransferOperation(iop->fileDescriptor, &methods, &direction, iop->size, NULL, iop->data);
}

typedef struct {
  FileDescriptor fileDescriptor;
  size_t size;
  const void *buffer;
  AsyncOutputCallback callback;
  void *data;
} OutputOperationParameters;

static Element *
newOutputOperation (const void *parameters) {
  const OutputOperationParameters *oop = parameters;

  TransferDirectionUnion direction = {
    .output = {
      .callback = oop->callback
    }
  };

  static const FunctionMethods methods = {
    .functionName = "transferOutput",

#ifdef __MINGW32__
    .beginFunction = beginWindowsFunction,
    .endFunction = endWindowsFunction,
    .startOperation = startWindowsWrite,
    .finishOperation = finishWindowsTransferOperation,
    .cancelOperation = cancelWindowsTransferOperation,
#else /* __MINGW32__ */
    .beginFunction = beginUnixOutputFunction,
    .finishOperation = finishUnixWrite,
#endif /* __MINGW32__ */

    .invokeCallback = invokeOutputCallback
  };

  return newTransferOperation(oop->fileDescriptor, &methods, &direction, oop->size, oop->buffer, oop->data);
}
#endif /* ASYNC_CAN_MONITOR_IO */

static int
makeHandle (
  AsyncHandle *handle,
  Element *(*newElement) (const void *parameters),
  const void *parameters
) {
  if (handle) {
    if (!(*handle = malloc(sizeof(**handle)))) {
      logMallocError();
      return 0;
    }
  }

  {
    Element *element = newElement(parameters);

    if (element) {
      if (handle) {
        memset(*handle, 0, sizeof(**handle));
        (*handle)->element = element;
        (*handle)->identifier = getElementIdentifier(element);
      }

      return 1;
    }

    if (handle) free(*handle);
    return 0;
  }
}

static int
checkHandleValidity (AsyncHandle handle) {
  if (handle) {
    if (handle->element) {
      return 1;
    }
  }

  return 0;
}

static int
checkHandleIdentifier (AsyncHandle handle) {
  return handle->identifier == getElementIdentifier(handle->element);
}

static int
checkHandle (AsyncHandle handle, Queue *queue) {
  if (checkHandleValidity(handle)) {
    if (checkHandleIdentifier(handle)) {
      if (getElementQueue(handle->element) == queue) {
        return 1;
      }
    }
  }

  return 0;
}

void
asyncCancel (AsyncHandle handle) {
  if (checkHandleValidity(handle)) {
    if (checkHandleIdentifier(handle)) {
      Element *element = handle->element;
      int cancelled = 0;

      {
        Queue *queue = getElementQueue(element);
        const QueueMethods *methods = getQueueData(queue);

        if (methods) {
          if (methods->cancel) {
            methods->cancel(element);
            cancelled = 1;
          }
        }
      }

      if (!cancelled) deleteElement(element);
    }

    free(handle);
  }
}

int
asyncMonitorInput (
  AsyncHandle *handle,
  FileDescriptor fileDescriptor,
  AsyncMonitorCallback callback, void *data
) {
#ifdef ASYNC_CAN_MONITOR_IO
  static const FunctionMethods methods = {
    .functionName = "monitorInput",

#ifdef __MINGW32__
    .beginFunction = beginWindowsFunction,
    .endFunction = endWindowsFunction,
#else /* __MINGW32__ */
    .beginFunction = beginUnixInputFunction,
#endif /* __MINGW32__ */

    .invokeCallback = invokeMonitorCallback
  };

  const MonitorOperationParameters mop = {
    .fileDescriptor = fileDescriptor,
    .methods = &methods,
    .callback = callback,
    .data = data
  };

  return makeHandle(handle, newMonitorOperation, &mop);
#else /* ASYNC_CAN_MONITOR_IO */
  errno = ENOSYS;
  logSystemError("asyncMonitorInput");
  return 0;
#endif /* ASYNC_CAN_MONITOR_IO */
}

int
asyncMonitorOutput (
  AsyncHandle *handle,
  FileDescriptor fileDescriptor,
  AsyncMonitorCallback callback, void *data
) {
#ifdef ASYNC_CAN_MONITOR_IO
  static const FunctionMethods methods = {
    .functionName = "monitorOutput",

#ifdef __MINGW32__
    .beginFunction = beginWindowsFunction,
    .endFunction = endWindowsFunction,
#else /* __MINGW32__ */
    .beginFunction = beginUnixOutputFunction,
#endif /* __MINGW32__ */

    .invokeCallback = invokeMonitorCallback
  };

  const MonitorOperationParameters mop = {
    .fileDescriptor = fileDescriptor,
    .methods = &methods,
    .callback = callback,
    .data = data
  };

  return makeHandle(handle, newMonitorOperation, &mop);
#else /* ASYNC_CAN_MONITOR_IO */
  errno = ENOSYS;
  logSystemError("asyncMonitorOutput");
  return 0;
#endif /* ASYNC_CAN_MONITOR_IO */
}

int
asyncRead (
  AsyncHandle *handle,
  FileDescriptor fileDescriptor,
  size_t size,
  AsyncInputCallback callback, void *data
) {
#ifdef ASYNC_CAN_MONITOR_IO
  const InputOperationParameters iop = {
    .fileDescriptor = fileDescriptor,
    .size = size,
    .callback = callback,
    .data = data
  };

  return makeHandle(handle, newInputOperation, &iop);
#else /* ASYNC_CAN_MONITOR_IO */
  errno = ENOSYS;
  logSystemError("asyncRead");
  return 0;
#endif /* ASYNC_CAN_MONITOR_IO */
}

int
asyncWrite (
  AsyncHandle *handle,
  FileDescriptor fileDescriptor,
  const void *buffer, size_t size,
  AsyncOutputCallback callback, void *data
) {
#ifdef ASYNC_CAN_MONITOR_IO
  const OutputOperationParameters oop = {
    .fileDescriptor = fileDescriptor,
    .size = size,
    .buffer = buffer,
    .callback = callback,
    .data = data
  };

  return makeHandle(handle, newOutputOperation, &oop);
#else /* ASYNC_CAN_MONITOR_IO */
  errno = ENOSYS;
  logSystemError("asyncWrite");
  return 0;
#endif /* ASYNC_CAN_MONITOR_IO */
}

typedef struct {
  TimeValue time;
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
  return compareTimeValues(&alarm1->time, &alarm2->time) < 0;
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

typedef struct {
  const TimeValue *time;
  AsyncAlarmCallback callback;
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
  AsyncAlarmCallback callback,
  void *data
) {
  const AlarmElementParameters aep = {
    .time = time,
    .callback = callback,
    .data = data
  };

  return makeHandle(handle, newAlarmElement, &aep);
}

int
asyncSetAlarmIn (
  AsyncHandle *handle,
  int interval,
  AsyncAlarmCallback callback,
  void *data
) {
  TimeValue time;
  getRelativeTime(&time, interval);
  return asyncSetAlarmTo(handle, &time, callback, data);
}

static int
checkAlarmHandle (AsyncHandle handle) {
  return checkHandle(handle, getAlarmQueue(0));
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
asyncAwait (int duration, AsyncAwaitCallback callback, void *data) {
  long int elapsed = 0;
  TimePeriod period;
  startTimePeriod(&period, duration);

  do {
    long int timeout = duration - elapsed;

    {
      Queue *alarms = getAlarmQueue(0);

      if (alarms) {
        Element *element = getQueueHead(alarms);

        if (element) {
          AlarmEntry *alarm = getElementItem(element);
          TimeValue now;
          long int milliseconds;

          getCurrentTime(&now);
          milliseconds = millisecondsBetween(&now, &alarm->time);

          if (milliseconds <= 0) {
            AsyncAlarmCallback callback = alarm->callback;
            const AsyncAlarmResult result = {
              .data = alarm->data
            };

            deleteElement(element);
            if (callback) callback(&result);
            goto done;
          }

          if (milliseconds < timeout) timeout = milliseconds;
        }
      }
    }

#ifdef ASYNC_CAN_MONITOR_IO
    {
      Queue *functions = getFunctionQueue(0);
      Element *functionElement = NULL;

      int monitorCount = functions? getQueueSize(functions): 0;
      MonitorEntry *monitorArray = NULL;

      prepareMonitors();

      if (monitorCount) {
        if ((monitorArray = malloc(ARRAY_SIZE(monitorArray, monitorCount)))) {
          AddMonitorData add = {
            .monitor = monitorArray
          };

          functionElement = processQueue(functions, addMonitor, &add);

          if (!(monitorCount = add.monitor - monitorArray)) {
            free(monitorArray);
            monitorArray = NULL;
          }
        } else {
          logMallocError();
          monitorCount = 0;
        }
      }

      if (!functionElement) {
        if (awaitOperation(monitorArray, monitorCount, timeout)) {
          functionElement = processQueue(functions, findMonitor, NULL);
        }
      }

      if (functionElement) {
        FunctionEntry *function = getElementItem(functionElement);
        Element *operationElement = getQueueHead(function->operations);
        OperationEntry *operation = getElementItem(operationElement);

        if (!operation->finished) finishOperation(operation);

        operation->active = 1;
        if (!function->methods->invokeCallback(operation)) operation->cancel = 1;
        operation->active = 0;

        if (operation->cancel) {
          deleteElement(operationElement);
        } else {
          operation->error = 0;
        }

        if ((operationElement = getQueueHead(function->operations))) {
          operation = getElementItem(operationElement);
          if (!operation->finished) startOperation(operation);
          requeueElement(functionElement);
        } else {
          deleteElement(functionElement);
        }
      }

      if (monitorArray) free(monitorArray);
    }
#else /* ASYNC_CAN_MONITOR_IO */
    approximateDelay(timeout);
#endif /* ASYNC_CAN_MONITOR_IO */

  done:
    if (callback) {
      if (callback(data)) {
        return 1;
      }
    }
  } while (!afterTimePeriod(&period, &elapsed));

  return 0;
}

void
asyncWait (int duration) {
  asyncAwait(duration, NULL, NULL);
}
