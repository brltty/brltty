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

#include <stdio.h>

#include "log.h"

#include "hostcmd_unix.h"
#include "hostcmd_internal.h"

int
isHostCommand (const char *path) {
  return access(path, X_OK) != -1;
}

void
subconstructHostCommandStream (HostCommandStream *hcs) {
  hcs->package.pipe[0] = hcs->package.pipe[1] = -1;
}

void
subdestructHostCommandStream (HostCommandStream *hcs) {
  {
    int *descriptor = hcs->package.pipe;
    const int *end = descriptor + 2;

    while (descriptor < end) {
      if (*descriptor != -1) {
        close(*descriptor);
        *descriptor = -1;
      }

      descriptor += 1;
    }
  }
}

int
prepareHostCommandStream (HostCommandStream *hcs) {
  if (pipe(hcs->package.pipe) == -1) {
    logSystemError("pipe");
    return 0;
  }

  return 1;
}

static int
parentHostCommandStream (HostCommandStream *hcs) {
  int *local;
  int *remote;
  const char *mode;

  if (hcs->input) {
    local = &hcs->package.pipe[1];
    remote = &hcs->package.pipe[0];
    mode = "w";
  } else {
    local = &hcs->package.pipe[0];
    remote = &hcs->package.pipe[1];
    mode = "r";
  }

  close(*remote);
  *remote = -1;

  if (!(**hcs->file = fdopen(*local, mode))) {
    logSystemError("fdopen");
    return 0;
  }
  *local = -1;

  return 1;
}

static int
childHostCommandStream (HostCommandStream *hcs) {
  int *local;
  int *remote;

  if (hcs->input) {
    local = &hcs->package.pipe[0];
    remote = &hcs->package.pipe[1];
  } else {
    local = &hcs->package.pipe[1];
    remote = &hcs->package.pipe[0];
  }

  close(*remote);
  *remote = -1;

  if (close(hcs->descriptor) == -1) {
    logSystemError("close");
    return 0;
  }

  if (fcntl(*local, F_DUPFD, hcs->descriptor) == -1) {
    logSystemError("fcntl[F_DUPFD]");
    return 0;
  }

  close(*local);
  *local = -1;

  return 1;
}

int
runCommand (
  int *result,
  const char *const *command,
  HostCommandStream *streams,
  int asynchronous
) {
  int ok = 0;
  sigset_t newMask, oldMask;
  pid_t pid;

  sigemptyset(&newMask);
  sigaddset(&newMask, SIGCHLD);
  sigprocmask(SIG_BLOCK, &newMask, &oldMask);

  switch ((pid = fork())) {
    case -1: /* error */
      logSystemError("fork");
      break;

    case 0: /* child */
      sigprocmask(SIG_SETMASK, &oldMask, NULL);

      if (processHostCommandStreams(childHostCommandStream, streams)) {
        execvp(command[0], (char *const*)command);
        logSystemError("execvp");
      }

      _exit(1);

    default: /* parent */
      if (processHostCommandStreams(parentHostCommandStream, streams)) {
        ok = 1;

        if (asynchronous) {
          *result = 0;
        } else {
          int status;

          if (waitpid(pid, &status, 0) == -1) {
            logSystemError("waitpid");
          } else if (WIFEXITED(status)) {
            *result = WEXITSTATUS(status);
            logMessage(LOG_DEBUG, "host command exit status: %d: %s",
                       *result, command[0]);
          } else if (WIFSIGNALED(status)) {
            *result = WTERMSIG(status);
            logMessage(LOG_DEBUG, "host command termination signal: %d: %s",
                       *result, command[0]);
            *result += 0X80;
          } else if (WIFSTOPPED(status)) {
            *result = WSTOPSIG(status);
            logMessage(LOG_DEBUG, "host command stop signal: %d: %s",
                       *result, command[0]);
            *result += 0X80;
          } else {
            logMessage(LOG_DEBUG, "unknown host command status: 0X%X: %s",
                       status, command[0]);
          }
        }
      }

      break;
  }

  sigprocmask(SIG_SETMASK, &oldMask, NULL);
  return ok;
}
