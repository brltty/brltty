/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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
#include "hostcmd.h"

#if defined(__MINGW32__)

#else /* Unix */
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#endif /* platform-dependent declarations */

typedef struct {
  FILE **const *const file;
  const int descriptor;
  const unsigned input:1;

#if defined(__MINGW32__)

#else /* Unix */
  int pipe[2];

#endif /* platform-dependent fields */
} HostCommandStream;

typedef int HostCommandStreamProcessor (HostCommandStream *hcs);

static int
processHostCommandStreams (HostCommandStreamProcessor *processor, HostCommandStream *hcs) {
  while (hcs->file) {
    if (*hcs->file) {
      if (!processor(hcs)) return 0;
    }

    hcs += 1;
  }

  return 1;
}

#if defined(__MINGW32__)
static void
subconstructHostCommandStream (HostCommandStream *hcs) {
}

static void
subdestructHostCommandStream (HostCommandStream *hcs) {
}

static int
runCommand (
  int *result,
  const char *const *command,
  HostCommandStream *streams,
  int asynchronous
) {
  return 0;
}

#else /* Unix */
static void
subconstructHostCommandStream (HostCommandStream *hcs) {
  hcs->pipe[0] = hcs->pipe[1] = -1;
}

static void
subdestructHostCommandStream (HostCommandStream *hcs) {
  {
    int *descriptor = hcs->pipe;
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

static int
prepareHostCommandStream (HostCommandStream *hcs) {
  if (pipe(hcs->pipe) == -1) {
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
    local = &hcs->pipe[1];
    remote = &hcs->pipe[0];
    mode = "w";
  } else {
    local = &hcs->pipe[0];
    remote = &hcs->pipe[1];
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
    local = &hcs->pipe[0];
    remote = &hcs->pipe[1];
  } else {
    local = &hcs->pipe[1];
    remote = &hcs->pipe[0];
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

static int
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

#endif /* platform-dependent functions */

void
initializeHostCommandOptions (HostCommandOptions *options) {
  options->asynchronous = 0;

  options->standardInput = NULL;
  options->standardOutput = NULL;
  options->standardError = NULL;
}

static int
constructHostCommandStream (HostCommandStream *hcs) {
  **hcs->file = NULL;
  subconstructHostCommandStream(hcs);
  return 1;
}

static int
destructHostCommandStream (HostCommandStream *hcs) {
  subdestructHostCommandStream(hcs);

  if (**hcs->file) {
    fclose(**hcs->file);
    **hcs->file = NULL;
  }

  return 1;
}

int
runHostCommand (
  const char *const *command,
  const HostCommandOptions *options
) {
  int result = 0XFF;
  HostCommandOptions defaults;

  if (!options) {
    initializeHostCommandOptions(&defaults);
    options = &defaults;
  }

  logMessage(LOG_DEBUG, "starting host command: %s", command[0]);

  {
    HostCommandStream streams[] = {
      { .file = &options->standardInput,
        .descriptor = 0,
        .input = 1
      },

      { .file = &options->standardOutput,
        .descriptor = 1,
        .input = 0
      },

      { .file = &options->standardError,
        .descriptor = 2,
        .input = 0
      },

      { .file = NULL }
    };

    if (processHostCommandStreams(constructHostCommandStream, streams)) {
      int ok = 0;

      if (processHostCommandStreams(prepareHostCommandStream, streams)) {
        if (runCommand(&result, command, streams, options->asynchronous)) ok = 1;
      }

      if (!ok) processHostCommandStreams(destructHostCommandStream, streams);
    }
  }

  return result;
}
