/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2022 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <sys/wait.h>

#include "log.h"
#include "options.h"
#include "pty_object.h"
#include "async_handle.h"
#include "async_wait.h"
#include "async_io.h"
#include "async_signal.h"

static int opt_showPath;

BEGIN_OPTION_TABLE(programOptions)
  { .word = "show-path",
    .letter = 'p',
    .setting.flag = &opt_showPath,
    .description = strtext("show the path of the slave pty device")
  },
END_OPTION_TABLE

static int
prepareChild (PtyObject *pty) {
  setsid();
  ptyCloseMaster(pty);

  if (setenv("TERM", "dumb", 1) == -1) {
    logSystemError("setenv");
    return 0;
  }

  {
    int tty;
    if (!ptyOpenSlave(pty, &tty)) return 0;
    int keep = 0;

    for (int fd=0; fd<=2; fd+=1) {
      if (fd == tty) {
        keep = 1;
      } else {
        int result = dup2(tty, fd);

        if (result == -1) {
          if (fd != 2) logSystemError("dup2");
          return 0;
        }
      }
    }

    if (!keep) close(tty);
  }

  return 1;
}

static int
runChild (PtyObject *pty, char **command) {
  char *defaultCommand[2];

  if (!(command && *command)) {
    char *shell = getenv("SHELL");
    if (!(shell && *shell)) shell = "/bin/sh";

    defaultCommand[0] = shell;
    defaultCommand[1] = NULL;
    command = defaultCommand;
  }

  if (prepareChild(pty)) {
    int result = execvp(*command, command);

    if (result == -1) {
      switch (errno) {
        case ENOENT:
          logMessage(LOG_ERR, "command not found: %s", *command);
          return PROG_EXIT_SEMANTIC;

        default:
          logSystemError("execvp");
          break;
      }
    } else {
      logMessage(LOG_ERR, "unexpected return from execvp");
    }
  }

  return PROG_EXIT_FATAL;
}

static unsigned char childHasTerminated;
static unsigned char slaveIsClosed;

static
ASYNC_CONDITION_TESTER(childTerminationTester) {
  return childHasTerminated && slaveIsClosed;
}

static
ASYNC_INPUT_CALLBACK(ptyInputHandler) {
  if (!(parameters->error || parameters->end)) {
    size_t length = parameters->length;
    write(1, parameters->buffer, length);
    return length;
  }

  slaveIsClosed = 1;
  return 0;
}

static void
childTerminationHandler (int signalNumber) {
  childHasTerminated = 1;
}

static int
getExitStatus (pid_t pid) {
  while (1) {
    int status;
    pid_t result = waitpid(pid, &status, 0);

    if (result == -1) {
      if (errno == EINTR) continue;
      logSystemError("waitpid");
      break;
    }

    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 0X80 | WTERMSIG(status);

    #ifdef WCOREDUMP
    if (WCOREDUMP(status)) return 0X80 | WTERMSIG(status);
    #endif /* WCOREDUMP */

    #ifdef WIFSTOPPED
    if (WIFSTOPPED(status)) continue;
    #endif /* WIFSTOPPED */

    #ifdef WIFCONTINUED
    if (WIFCONTINUED(status)) continue;
    #endif /* WIFCONTINUED */
  }

  return PROG_EXIT_FATAL;
}

static int
runParent (PtyObject *pty, pid_t child) {
  int exitStatus = PROG_EXIT_FATAL;
  AsyncHandle inputHandle;

  childHasTerminated = 0;
  slaveIsClosed = 0;

  if (asyncReadFile(&inputHandle, ptyGetMaster(pty), 1, ptyInputHandler, NULL)) {
    if (asyncHandleSignal(SIGCHLD, childTerminationHandler, NULL)) {
      asyncAwaitCondition(INT_MAX, childTerminationTester, NULL);
      exitStatus = getExitStatus(child);
    }

    asyncCancelRequest(inputHandle);
  }

  return exitStatus;
}

int
main (int argc, char *argv[]) {
  int exitStatus = PROG_EXIT_FATAL;
  PtyObject *pty;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "brltty-pty",
      .argumentsSummary = "[command [arg ...a]]"
    };

    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  if ((pty = ptyNewObject())) {
    if (opt_showPath) {
      FILE *stream = stdout;
      fprintf(stream, "%s\n", ptyGetPath(pty));
      fflush(stream);
    }

    pid_t child = fork();
    switch (child) {
      case -1:
        logSystemError("fork");
        break;

      case 0:
        _exit(runChild(pty, argv));

      default:
        exitStatus = runParent(pty, child);
        break;
    }

    ptyDestroyObject(pty);
  }

  return exitStatus;
}
