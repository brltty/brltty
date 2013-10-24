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
static char *pipePath;
static FileDescriptor pipeDescriptor;
static AsyncHandle inputMonitor;

static void
initializePipePath (void) {
  pipePath = NULL;
}

static void
deallocatePipePath (void) {
  if (pipePath) free(pipePath);
  initializePipePath();
}

static void
removePipe (void) {
  if (pipePath) unlink(pipePath);
}

static void
initializePipeDescriptor (void) {
  pipeDescriptor = INVALID_FILE_DESCRIPTOR;
}

static void
closePipeDescriptor (void) {
  if (pipeDescriptor != INVALID_FILE_DESCRIPTOR) closeFileDescriptor(pipeDescriptor);
  initializePipeDescriptor();
}

static void
initializeInputMonitor (void) {
  inputMonitor = NULL;
}

static void
stopInputMonitor (void) {
  if (inputMonitor) asyncCancelRequest(inputMonitor);
  initializeInputMonitor();
}

static size_t
monitorInput (const AsyncInputCallbackParameters *parameters) {
  const char *buffer = parameters->buffer;
  size_t length = parameters->length;

  sayCharacters(&spk, buffer, length, 0);
  return length;
}

static int
startInputMonitor (void) {
  if (!inputMonitor) {
    if (!asyncReadFile(&inputMonitor, pipeDescriptor, 0x1000, monitorInput, NULL)) {
      return 0;
    }
  }

  return 1;
}

#if defined(__MINGW32__)
#define startPipeMonitor startConnectionMonitor

static int speechInputConnected;

static int
createPipe (void) {
  if ((pipeDescriptor = CreateNamedPipe(pipePath,
                                        PIPE_ACCESS_INBOUND | PIPE_FLAG_OVERLAPPED,
                                        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
                                        1, 0, 0, 0, NULL)) != INVALID_HANDLE_VALUE) {
    logMessage(LOG_DEBUG, "speech input pipe created: %s: handle=%u",
               pipePath, (unsigned)pipeDescriptor);

    return 1;
  } else {
    logWindowsSystemError("CreateNamedPipe");
  }

  return 0;
}

static int
startConnectionMonitor (void) {
  return 0;
}

#elif defined(S_ISFIFO)
static int
createPipe (void) {
  int result = mkfifo(pipePath, 0);

  if ((result == -1) && (errno == EEXIST)) {
    struct stat fifo;

    if (lstat(pipePath, &fifo) == -1) {
      logMessage(LOG_ERR, "cannot stat speech input FIFO: %s: %s",
                 pipePath, strerror(errno));
    } else if (S_ISFIFO(fifo.st_mode)) {
      result = 0;
    }
  }

  if (result != -1) {
    if (chmod(pipePath, S_IRUSR|S_IWUSR|S_IWGRP|S_IWOTH) != -1) {
      if ((pipeDescriptor = open(pipePath, O_RDONLY|O_NONBLOCK)) != -1) {
        logMessage(LOG_DEBUG, "speech input FIFO created: %s: fd=%d",
                   pipePath, pipeDescriptor);

        return 1;
      } else {
        logMessage(LOG_ERR, "cannot open speech input FIFO: %s: %s",
                   pipePath, strerror(errno));
      }
    } else {
      logMessage(LOG_ERR, "cannot set speech input FIFO permissions: %s: %s",
                 pipePath, strerror(errno));
    }

    removePipe();
  } else {
    logMessage(LOG_ERR, "cannot create speech input FIFO: %s: %s",
               pipePath, strerror(errno));
  }

  return 0;
}

#else /* speech input functions */
#warning speech input not supported on this platform

static int
createPipe (void) {
  logUnsupportedFeature("speech input");
  return 0;
}
#endif /* speech input functions */

#ifndef startPipeMonitor
#define startPipeMonitor startInputMonitor
#endif /* startPipeMonitor */

int
enableSpeechInput (const char *name) {
  const char *directory = getNamedPipeDirectory();

  initializePipePath();
  initializePipeDescriptor();
  initializeInputMonitor();

  if ((pipePath = makePath(directory, name))) {
    if (createPipe()) {
      if (startPipeMonitor()) {
        return 1;
      }

      closePipeDescriptor();
      removePipe();
    }

    deallocatePipePath();
  }

  return 0;
}

void
disableSpeechInput (void) {
  stopInputMonitor();
  closePipeDescriptor();
  removePipe();
  deallocatePipePath();
}
#endif /* ENABLE_SPEECH_SUPPORT */
