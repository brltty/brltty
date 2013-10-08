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
#include "program.h"
#include "brltty.h"

#ifdef ENABLE_SPEECH_SUPPORT
static char *speechInputPath;
static FileDescriptor speechInputPipe;
static AsyncHandle speechInputMonitor;

static size_t
monitorSpeechInput (const AsyncInputResult *result) {
  sayCharacters(&spk, result->buffer, result->length, 0);
  return result->length;
}

static int
startSpeechInputMonitor (void) {
  if (!speechInputMonitor) {
    if (!asyncReadFile(&speechInputMonitor, speechInputPipe, 0x1000, monitorSpeechInput, NULL)) {
      return 0;
    }
  }

  return 1;
}

static void
stopSpeechInputMonitor (void) {
  if (speechInputMonitor) {
    asyncCancelRequest(speechInputMonitor);
    speechInputMonitor = NULL;
  }
}

#if defined(__MINGW32__)
static HANDLE speechInputHandle = INVALID_HANDLE_VALUE;
static int speechInputConnected;

static int
makeSpeechInputPipe (void) {
  if ((speechInputHandle = CreateNamedPipe(speechInputPath, PIPE_ACCESS_INBOUND,
                                           PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_NOWAIT,
                                           1, 0, 0, 0, NULL)) != INVALID_HANDLE_VALUE) {
    logMessage(LOG_DEBUG, "speech input pipe created: %s: handle=%u",
               speechInputPath, (unsigned)speechInputHandle);
    return 1;
  } else {
    logWindowsSystemError("CreateNamedPipe");
  }

  return 0;
}

#elif defined(S_ISFIFO)
static int
makeSpeechInputPipe (void) {
  int result = mkfifo(speechInputPath, 0);

  if ((result == -1) && (errno == EEXIST)) {
    struct stat fifo;

    if (lstat(speechInputPath, &fifo) == -1) {
      logMessage(LOG_ERR, "cannot stat speech input FIFO: %s: %s",
                 speechInputPath, strerror(errno));
    } else if (S_ISFIFO(fifo.st_mode)) {
      result = 0;
    }
  }

  if (result != -1) {
    if (chmod(speechInputPath, S_IRUSR|S_IWUSR|S_IWGRP|S_IWOTH) != -1) {
      if ((speechInputPipe = open(speechInputPath, O_RDONLY|O_NONBLOCK)) != -1) {
        if (startSpeechInputMonitor()) {
          logMessage(LOG_DEBUG, "speech input FIFO created: %s: fd=%d",
                     speechInputPath, speechInputPipe);

          return 1;
        }

        close(speechInputPipe);
        speechInputPipe = INVALID_FILE_DESCRIPTOR;
      } else {
        logMessage(LOG_ERR, "cannot open speech input FIFO: %s: %s",
                   speechInputPath, strerror(errno));
      }
    } else {
      logMessage(LOG_ERR, "cannot set speech input FIFO permissions: %s: %s",
                 speechInputPath, strerror(errno));
    }

    unlink(speechInputPath);
  } else {
    logMessage(LOG_ERR, "cannot create speech input FIFO: %s: %s",
               speechInputPath, strerror(errno));
  }

  return 0;
}

#else /* speech input functions */
#warning speech input not supported on this platform
#endif /* speech input functions */

static void
exitSpeechInput (void *data) {
  stopSpeechInputMonitor();

  if (speechInputPipe != INVALID_FILE_DESCRIPTOR) {
    closeFileDescriptor(speechInputPipe);
    speechInputPipe = INVALID_FILE_DESCRIPTOR;
  }

  if (speechInputPath) {
    unlink(speechInputPath);
    free(speechInputPath);
    speechInputPath = NULL;
  }
}

int
startSpeechInput (const char *name) {
  const char *directory = getNamedPipeDirectory();

  speechInputPath = NULL;
  speechInputPipe = INVALID_FILE_DESCRIPTOR;
  speechInputMonitor = NULL;
  onProgramExit("speech-input", exitSpeechInput, NULL);

  if ((speechInputPath = makePath(directory, name))) {
    if (makeSpeechInputPipe()) {
      return 1;
    }

    free(speechInputPath);
    speechInputPath = NULL;
  }

  return 0;
}
#endif /* ENABLE_SPEECH_SUPPORT */
