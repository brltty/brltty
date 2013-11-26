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
#include <sys/stat.h>
#include <fcntl.h>

#include "log.h"
#include "spk.h"
#include "spk_input.h"
#include "async_io.h"
#include "file.h"
#include "brltty.h"

#ifdef ENABLE_SPEECH_SUPPORT
struct SpeechInputObjectStruct {
  int (*createPipe) (SpeechInputObject *obj);
  int (*monitorPipe) (SpeechInputObject *obj);
  void (*resetPipe) (SpeechInputObject *obj);
  void (*releaseResources) (SpeechInputObject *obj);

  char *pipePath;
  FileDescriptor pipeDescriptor;
  AsyncHandle inputMonitor;

#if defined(__MINGW32__)
  struct {
    AsyncHandle connectMonitor;
    HANDLE connectEvent;
    OVERLAPPED connectOverlapped;
  } windows;

#endif /*  */
};

static void
initializePipePath (SpeechInputObject *obj) {
  obj->pipePath = NULL;
}

static void
deallocatePipePath (SpeechInputObject *obj) {
  if (obj->pipePath) free(obj->pipePath);
  initializePipePath(obj);
}

static void
removePipe (SpeechInputObject *obj) {
  if (obj->pipePath) unlink(obj->pipePath);
}

static void
initializePipeDescriptor (SpeechInputObject *obj) {
  obj->pipeDescriptor = INVALID_FILE_DESCRIPTOR;
}

static void
closePipeDescriptor (SpeechInputObject *obj) {
  if (obj->pipeDescriptor != INVALID_FILE_DESCRIPTOR) closeFileDescriptor(obj->pipeDescriptor);
  initializePipeDescriptor(obj);
}

static void
initializeInputMonitor (SpeechInputObject *obj) {
  obj->inputMonitor = NULL;
}

static void
stopInputMonitor (SpeechInputObject *obj) {
  if (obj->inputMonitor) asyncCancelRequest(obj->inputMonitor);
  initializeInputMonitor(obj);
}

static size_t
handleInput (const AsyncInputCallbackParameters *parameters) {
  SpeechInputObject *obj = parameters->data;

  if (parameters->error) {
  } else if (parameters->end) {
  } else {
    const char *buffer = parameters->buffer;
    size_t length = parameters->length;
    char string[length + 1];

    memcpy(string, buffer, length);
    string[length] = 0;
    sayString(&spk, string, 0);
    return length;
  }

  asyncDiscardHandle(obj->inputMonitor);
  initializeInputMonitor(obj);

  if (obj->resetPipe) obj->resetPipe(obj);
  obj->monitorPipe(obj);

  return 0;
}

static int
monitorInput (SpeechInputObject *obj) {
  if (!obj->inputMonitor) {
    if (!asyncReadFile(&obj->inputMonitor, obj->pipeDescriptor, 0x1000, handleInput, obj)) {
      return 0;
    }
  }

  return 1;
}

#if defined(__MINGW32__)
static int
createWindowsPipe (SpeechInputObject *obj) {
  obj->windows.connectMonitor = NULL;

  if ((obj->windows.connectEvent = CreateEvent(NULL, TRUE, FALSE, NULL))) {
    if ((obj->pipeDescriptor = CreateNamedPipe(obj->pipePath,
                                               PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                                               PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
                                               1, 0, 0, 0, NULL)) != INVALID_HANDLE_VALUE) {
      logMessage(LOG_DEBUG, "speech input pipe created: %s: handle=%u",
                 obj->pipePath, (unsigned int)obj->pipeDescriptor);

      return 1;
    } else {
      logWindowsSystemError("CreateNamedPipe");
    }

    CloseHandle(obj->windows.connectEvent);
  } else {
    logWindowsSystemError("CreateEvent");
  }

  return 0;
}

static int
doWindowsConnected (SpeechInputObject *obj) {
  return monitorInput(obj);
}

static int
handleWindowsConnect (const AsyncMonitorCallbackParameters *parameters) {
  SpeechInputObject *obj = parameters->data;

  asyncDiscardHandle(obj->windows.connectMonitor);
  obj->windows.connectMonitor = NULL;

  doWindowsConnected(obj);
  return 0;
}

static int
monitorWindowsConnect (SpeechInputObject *obj) {
  if (ResetEvent(obj->windows.connectEvent)) {
    ZeroMemory(&obj->windows.connectOverlapped, sizeof(obj->windows.connectOverlapped));
    obj->windows.connectOverlapped.hEvent = obj->windows.connectEvent;

    if (ConnectNamedPipe(obj->pipeDescriptor, &obj->windows.connectOverlapped)) {
      if (doWindowsConnected(obj)) {
        return 1;
      }
    } else {
      DWORD error = GetLastError();

      if (error == ERROR_PIPE_CONNECTED) {
        if (doWindowsConnected(obj)) {
          return 1;
        }
      } else if (error == ERROR_IO_PENDING) {
        if (asyncMonitorFileInput(&obj->windows.connectMonitor, obj->windows.connectEvent, handleWindowsConnect, obj)) {
          return 1;
        }
      } else {
        logWindowsError(error, "ConnectNamedPipe");
      }
    }
  } else {
    logWindowsSystemError("ResetEvent");
  }

  return 0;
}

static void
disconnectWindowsPipe (SpeechInputObject *obj) {
  if (!DisconnectNamedPipe(obj->pipeDescriptor)) {
    logWindowsSystemError("DisconnectNamedPipe");
  }
}

static void
releaseWindowsResources (SpeechInputObject *obj) {
  if (obj->windows.connectMonitor) {
    asyncCancelRequest(obj->windows.connectMonitor);
    obj->windows.connectMonitor = NULL;
  }

  if (obj->windows.connectEvent) {
    CloseHandle(obj->windows.connectEvent);
    obj->windows.connectEvent = NULL;
  }
}

static void
setSpeechInputMethods (SpeechInputObject *obj) {
  obj->createPipe = createWindowsPipe;
  obj->monitorPipe = monitorWindowsConnect;
  obj->resetPipe = disconnectWindowsPipe;
  obj->releaseResources = releaseWindowsResources;
}

#elif defined(S_ISFIFO)
static int
createUnixPipe (SpeechInputObject *obj) {
  int result = mkfifo(obj->pipePath, 0);

  if ((result == -1) && (errno == EEXIST)) {
    struct stat fifo;

    if (lstat(obj->pipePath, &fifo) == -1) {
      logMessage(LOG_ERR, "cannot stat speech input FIFO: %s: %s",
                 obj->pipePath, strerror(errno));
    } else if (S_ISFIFO(fifo.st_mode)) {
      result = 0;
    }
  }

  if (result != -1) {
    if (chmod(obj->pipePath, S_IRUSR|S_IWUSR|S_IWGRP|S_IWOTH) != -1) {
      if ((obj->pipeDescriptor = open(obj->pipePath, O_RDONLY|O_NONBLOCK)) != -1) {
        logMessage(LOG_DEBUG, "speech input FIFO created: %s: fd=%d",
                   obj->pipePath, obj->pipeDescriptor);

        return 1;
      } else {
        logMessage(LOG_ERR, "cannot open speech input FIFO: %s: %s",
                   obj->pipePath, strerror(errno));
      }
    } else {
      logMessage(LOG_ERR, "cannot set speech input FIFO permissions: %s: %s",
                 obj->pipePath, strerror(errno));
    }

    removePipe(obj);
  } else {
    logMessage(LOG_ERR, "cannot create speech input FIFO: %s: %s",
               obj->pipePath, strerror(errno));
  }

  return 0;
}

static void
setSpeechInputMethods (SpeechInputObject *obj) {
  obj->createPipe = createUnixPipe;
}

#else /* speech input functions */
#warning speech input not supported on this platform

static void
setSpeechInputMethods (SpeechInputObject *obj) {
}
#endif /* speech input functions */

SpeechInputObject *
newSpeechInputObject (const char *name) {
  SpeechInputObject *obj;

  if ((obj = malloc(sizeof(*obj)))) {
    memset(obj, 0, sizeof(*obj));

    obj->createPipe = NULL;
    obj->monitorPipe = monitorInput;
    obj->resetPipe = NULL;
    obj->releaseResources = NULL;
    setSpeechInputMethods(obj);

    initializePipePath(obj);
    initializePipeDescriptor(obj);
    initializeInputMonitor(obj);

    if ((obj->pipePath = makePath(getNamedPipeDirectory(), name))) {
      if (obj->createPipe(obj)) {
        if (obj->monitorPipe(obj)) {
          return obj;
        }

        closePipeDescriptor(obj);
        removePipe(obj);
      }

      deallocatePipePath(obj);
    }

    free(obj);
  } else {
    logMallocError();
  }

  return NULL;
}

void
destroySpeechInputObject (SpeechInputObject *obj) {
  if (obj->releaseResources) obj->releaseResources(obj);
  stopInputMonitor(obj);
  closePipeDescriptor(obj);
  removePipe(obj);
  deallocatePipePath(obj);
  free(obj);
}
#endif /* ENABLE_SPEECH_SUPPORT */
