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

#include "log.h"
#include "spk.h"
#include "spk_input.h"
#include "file.h"
#include "program.h"

static char *speechInputPath = NULL;

#if defined(__MINGW32__)
static HANDLE speechInputHandle = INVALID_HANDLE_VALUE;
static int speechInputConnected;
#elif defined(S_ISFIFO)
static int speechInputDescriptor = -1;
#endif /* speech input definitions */

static void
exitSpeechInput (void *data) {
  if (speechInputPath) {
#if defined(__MINGW32__)
#elif defined(S_ISFIFO)
    unlink(speechInputPath);
#endif /* cleanup speech input */

    free(speechInputPath);
    speechInputPath = NULL;
  }

#if defined(__MINGW32__)
  if (speechInputHandle != INVALID_HANDLE_VALUE) {
    if (speechInputConnected) {
      DisconnectNamedPipe(speechInputHandle);
      speechInputConnected = 0;
    }

    CloseHandle(speechInputHandle);
    speechInputHandle = INVALID_HANDLE_VALUE;
  }
#elif defined(S_ISFIFO)
  if (speechInputDescriptor != -1) {
    close(speechInputDescriptor);
    speechInputDescriptor = -1;
  }
#endif /* disable speech input */
}

int
enableSpeechInput (const char *name) {
  const char *directory;
  onProgramExit("speech-input", exitSpeechInput, NULL);

#if defined(__MINGW32__)
  directory = "//./pipe";
#else /* set speech input directory */
  directory = ".";
#endif /* set speech input directory */

  if ((speechInputPath = makePath(directory, name))) {
#if defined(__MINGW32__)
    if ((speechInputHandle = CreateNamedPipe(speechInputPath, PIPE_ACCESS_INBOUND,
                                             PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_NOWAIT,
                                             1, 0, 0, 0, NULL)) != INVALID_HANDLE_VALUE) {
      logMessage(LOG_DEBUG, "speech input pipe created: %s: handle=%u",
                 speechInputPath, (unsigned)speechInputHandle);
      return 1;
    } else {
      logWindowsSystemError("CreateNamedPipe");
    }
#elif defined(S_ISFIFO)
    int result = mkfifo(speechInputPath, 0);

    if ((result == -1) && (errno == EEXIST)) {
      struct stat fifo;

      if ((lstat(speechInputPath, &fifo) != -1) && S_ISFIFO(fifo.st_mode)) result = 0;
    }

    if (result != -1) {
      chmod(speechInputPath, S_IRUSR|S_IWUSR|S_IWGRP|S_IWOTH);

      if ((speechInputDescriptor = open(speechInputPath, O_RDONLY|O_NONBLOCK)) != -1) {
        logMessage(LOG_DEBUG, "speech input FIFO created: %s: fd=%d",
                   speechInputPath, speechInputDescriptor);
        return 1;
      } else {
        logMessage(LOG_ERR, "cannot open speech input FIFO: %s: %s",
                   speechInputPath, strerror(errno));
      }

      unlink(speechInputPath);
    } else {
      logMessage(LOG_ERR, "cannot create speech input FIFO: %s: %s",
                 speechInputPath, strerror(errno));
    }
#endif /* enable speech input */

    free(speechInputPath);
    speechInputPath = NULL;
  }

  return 0;
}

void
processSpeechInput (SpeechSynthesizer *spk) {
#if defined(__MINGW32__)
  if (speechInputHandle != INVALID_HANDLE_VALUE) {
    if (speechInputConnected ||
        (speechInputConnected = ConnectNamedPipe(speechInputHandle, NULL)) ||
        (speechInputConnected = (GetLastError() == ERROR_PIPE_CONNECTED))) {
      char buffer[0X1000];
      DWORD count;

      if (ReadFile(speechInputHandle, buffer, sizeof(buffer), &count, NULL)) {
        if (count) sayCharacters(spk, buffer, count, 0);
      } else {
        DWORD error = GetLastError();

        if (error != ERROR_NO_DATA) {
          speechInputConnected = 0;
          DisconnectNamedPipe(speechInputHandle);

          if (error != ERROR_BROKEN_PIPE)
            logWindowsSystemError("speech input FIFO read");
        }
      }
    }
  }
#elif defined(S_ISFIFO)
  if (speechInputDescriptor != -1) {
    char buffer[0X1000];
    int count = read(speechInputDescriptor, buffer, sizeof(buffer));
    if (count > 0) sayCharacters(spk, buffer, count, 0);
  }
#endif /* process speech input */
}
