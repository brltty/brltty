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

/* not done yet:
 * parent: terminal type list
 * screen: resize
 */

#include "prologue.h"

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <sys/wait.h>

#include "log.h"
#include "options.h"
#include "pty_object.h"
#include "pty_terminal.h"
#include "file.h"
#include "async_handle.h"
#include "async_wait.h"
#include "async_io.h"
#include "async_signal.h"

static int opt_driverDirectives;
static int opt_ttyPath;

static int opt_logInput;
static int opt_logOutput;
static int opt_logSequences;
static int opt_logUnexpected;

BEGIN_OPTION_TABLE(programOptions)
  { .word = "driver-directives",
    .letter = 'd',
    .setting.flag = &opt_driverDirectives,
    .description = strtext("write driver directives to standard error")
  },

  { .word = "tty-path",
    .letter = 't',
    .setting.flag = &opt_ttyPath,
    .description = strtext("show the absolute path to the pty slave")
  },

  { .word = "log-input",
    .letter = 'I',
    .flags = OPT_Hidden,
    .setting.flag = &opt_logInput,
    .description = strtext("log input written to the pty slave")
  },

  { .word = "log-output",
    .letter = 'O',
    .flags = OPT_Hidden,
    .setting.flag = &opt_logOutput,
    .description = strtext("log output received from the pty slave that isn't an escape sequence or a special character")
  },

  { .word = "log-sequences",
    .letter = 'S',
    .flags = OPT_Hidden,
    .setting.flag = &opt_logSequences,
    .description = strtext("log escape sequences and special characters received from the pty slave")
  },

  { .word = "log-unexpected",
    .letter = 'U',
    .flags = OPT_Hidden,
    .setting.flag = &opt_logUnexpected,
    .description = strtext("log unexpected input/output")
  },
END_OPTION_TABLE

static void writeDriverDirective (const char *format, ...) PRINTF(1, 2);

static void
writeDriverDirective (const char *format, ...) {
  if (opt_driverDirectives) {
    va_list args;
    va_start(args, format);

    {
      FILE *stream = stderr;
      vfprintf(stream, format, args);
      fputc('\n', stream);
      fflush(stream);
    }

    va_end(args);
  }
}

static int
setEnvironmentString (const char *variable, const char *string) {
  int result = setenv(variable, string, 1);
  if (result != -1) return 1;

  logSystemError("setenv");
  return 0;
}

static int
setEnvironmentInteger (const char *variable, int integer) {
  char string[0X10];
  snprintf(string, sizeof(string), "%d", integer);
  return setEnvironmentString(variable, string);
}

static int
setEnvironmentVariables (void) {
  {
    size_t width, height;

    if (getConsoleSize(&width, &height)) {
      if (!setEnvironmentInteger("COLUMNS", width)) return 0;
      if (!setEnvironmentInteger("LINES", height)) return 0;
    }
  }

  return setEnvironmentString("TERM", ptyGetTerminalType());
}

static int
prepareChild (PtyObject *pty) {
  setsid();
  ptyCloseMaster(pty);

  if (setEnvironmentVariables()) {
    int tty;
    if (!ptyOpenSlave(pty, &tty)) return 0;
    int keep = 0;

    for (int fd=0; fd<=2; fd+=1) {
      if (fd == tty) {
        keep = 1;
      } else {
        int result = dup2(tty, fd);

        if (result == -1) {
          logSystemError("dup2");
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
          logMessage(LOG_ERR, "%s: %s", gettext("command not found"), *command);
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

static unsigned char parentHasDied;
static unsigned char childHasTerminated;
static unsigned char slaveHasBeenClosed;

static
ASYNC_CONDITION_TESTER(parentTerminationTester) {
  if (parentHasDied) return 1;
  return childHasTerminated && slaveHasBeenClosed;
}

static void
parentDeathHandler (int signalNumber) {
  parentHasDied = 1;
}

static void
childTerminationHandler (int signalNumber) {
  childHasTerminated = 1;
}

static int
installSignalHandlers (void) {
  if (!asyncHandleSignal(SIGTERM, parentDeathHandler, NULL)) return 0;
  if (!asyncHandleSignal(SIGINT, parentDeathHandler, NULL)) return 0;
  if (!asyncHandleSignal(SIGQUIT, parentDeathHandler, NULL)) return 0;
  return asyncHandleSignal(SIGCHLD, childTerminationHandler, NULL);
}

static
ASYNC_MONITOR_CALLBACK(standardInputMonitor) {
  PtyObject *pty = parameters->data;
  ptyProcessTerminalInput(pty);
  return 1;
}

static
ASYNC_INPUT_CALLBACK(ptyInputHandler) {
  if (!(parameters->error || parameters->end)) {
    size_t length = parameters->length;

    if (ptyParseOutputBytes(parameters->buffer, length)) {
      ptySynchronizeTerminal();
    }

    return length;
  }

  slaveHasBeenClosed = 1;
  return 0;
}

static int
reapExitStatus (pid_t pid) {
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
  AsyncHandle ptyInputHandle;

  parentHasDied = 0;
  childHasTerminated = 0;
  slaveHasBeenClosed = 0;

  if (asyncReadFile(&ptyInputHandle, ptyGetMaster(pty), 1, ptyInputHandler, NULL)) {
    AsyncHandle standardInputHandle;

    if (asyncMonitorFileInput(&standardInputHandle, STDIN_FILENO, standardInputMonitor, pty)) {
      if (installSignalHandlers()) {
        if (!isatty(2)) ptySetTerminalLogLevel(LOG_NOTICE);

        if (ptyBeginTerminal(pty)) {
          writeDriverDirective("path %s", ptyGetPath(pty));

          asyncAwaitCondition(INT_MAX, parentTerminationTester, NULL);
          if (!parentHasDied) exitStatus = reapExitStatus(child);

          ptyEndTerminal();
        }
      }

      asyncCancelRequest(standardInputHandle);
    }

    asyncCancelRequest(ptyInputHandle);
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
      .argumentsSummary = "[command [arg ...]]"
    };

    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  ptySetLogInput(opt_logInput);
  ptySetLogOutput(opt_logOutput);
  ptySetLogSequences(opt_logSequences);
  ptySetLogUnexpected(opt_logUnexpected);

  if (!isatty(STDIN_FILENO)) {
    logMessage(LOG_ERR, "%s", gettext("standard input isn't a terminal"));
    return PROG_EXIT_SEMANTIC;
  }

  if (!isatty(STDOUT_FILENO)) {
    logMessage(LOG_ERR, "%s", gettext("standard output isn't a terminal"));
    return PROG_EXIT_SEMANTIC;
  }

  if ((pty = ptyNewObject())) {
    const char *ttyPath = ptyGetPath(pty);

    if (opt_ttyPath) {
      FILE *stream = stderr;
      fprintf(stream, "%s\n", ttyPath);
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
