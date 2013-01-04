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
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "embed.h"
#include "brltty.h"
#include "log.h"

static ProgramExitStatus
brlttyRun (void) {
  while (brlttyUpdate());
  return PROG_EXIT_SUCCESS;
}

#ifdef __MINGW32__
static SERVICE_STATUS_HANDLE serviceStatusHandle;
static DWORD serviceState;
static ProgramExitStatus serviceExitStatus;

static BOOL
setServiceState (DWORD state, ProgramExitStatus exitStatus, const char *name) {
  SERVICE_STATUS status = {
    .dwServiceType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
    .dwCurrentState = state,
    .dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE,
    .dwWin32ExitCode = (exitStatus == PROG_EXIT_SUCCESS)? ERROR_SERVICE_SPECIFIC_ERROR: NO_ERROR,
    .dwServiceSpecificExitCode = exitStatus,
    .dwCheckPoint = 0,
    .dwWaitHint = 10000, /* milliseconds */
  };

  serviceState = state;
  if (SetServiceStatus(serviceStatusHandle, &status)) return 1;

  logWindowsSystemError(name);
  return 0;
}

static void WINAPI
serviceHandler (DWORD code) {
  switch (code) {
    case SERVICE_CONTROL_STOP:
      setServiceState(SERVICE_STOP_PENDING, PROG_EXIT_SUCCESS, "SERVICE_STOP_PENDING");
      raise(SIGTERM);
      break;

    case SERVICE_CONTROL_PAUSE:
      setServiceState(SERVICE_PAUSE_PENDING, PROG_EXIT_SUCCESS, "SERVICE_PAUSE_PENDING");
      /* TODO: suspend */
      break;

    case SERVICE_CONTROL_CONTINUE:
      setServiceState(SERVICE_CONTINUE_PENDING, PROG_EXIT_SUCCESS, "SERVICE_CONTINUE_PENDING");
      /* TODO: resume */
      break;

    default:
      setServiceState(serviceState, PROG_EXIT_SUCCESS, "SetServiceStatus");
      break;
  }
}

static void
exitService (void) {
  setServiceState(SERVICE_STOPPED, PROG_EXIT_SUCCESS, "SERVICE_STOPPED");
}

static void WINAPI
serviceMain (DWORD argc, LPSTR *argv) {
  atexit(exitService);

  if ((serviceStatusHandle = RegisterServiceCtrlHandler("", &serviceHandler))) {
    if ((setServiceState(SERVICE_START_PENDING, PROG_EXIT_SUCCESS, "SERVICE_START_PENDING"))) {
      if ((serviceExitStatus = brlttyConstruct(argc, argv)) == PROG_EXIT_SUCCESS) {
        if ((setServiceState(SERVICE_RUNNING, PROG_EXIT_SUCCESS, "SERVICE_RUNNING"))) {
          serviceExitStatus = brlttyRun();
        } else {
          serviceExitStatus = PROG_EXIT_FATAL;
        }
      } else if (serviceExitStatus == PROG_EXIT_FORCE) {
        serviceExitStatus = PROG_EXIT_SUCCESS;
      }

      setServiceState(SERVICE_STOPPED, serviceExitStatus, "SERVICE_STOPPED");
    }
  } else {
    logWindowsSystemError("RegisterServiceCtrlHandler");
  }
}
#endif /* __MINGW32__ */

int
main (int argc, char *argv[]) {
#ifdef __MINGW32__
  {
    static SERVICE_TABLE_ENTRY serviceTable[] = {
      { .lpServiceName="", .lpServiceProc=serviceMain },
      {}
    };

    isWindowsService = 1;
    if (StartServiceCtrlDispatcher(serviceTable)) return serviceExitStatus;
    isWindowsService = 0;

    if (GetLastError() != ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
      logWindowsSystemError("StartServiceCtrlDispatcher");
      return 20;
    }
  }
#endif /* __MINGW32__ */

#ifdef INIT_PATH
#define INIT_NAME "init"

  if ((getpid() == 1) || strstr(argv[0], "linuxrc")) {
    fprintf(stderr, gettext("\"%s\" started as \"%s\"\n"), PACKAGE_TITLE, argv[0]);
    fflush(stderr);

    switch (fork()) {
      case -1: /* failed */
        fprintf(stderr, gettext("fork of \"%s\" failed: %s\n"),
                PACKAGE_TITLE, strerror(errno));
        fflush(stderr);

      default: /* parent */
        fprintf(stderr, gettext("executing \"%s\" (from \"%s\")\n"), INIT_NAME, INIT_PATH);
        fflush(stderr);

      executeInit:
        execv(INIT_PATH, argv);
        /* execv() shouldn't return */

        fprintf(stderr, gettext("execution of \"%s\" failed: %s\n"), INIT_NAME, strerror(errno));
        fflush(stderr);
        exit(1);

      case 0: { /* child */
        static char *arguments[] = {"brltty", "-E", "-n", "-e", "-linfo", NULL};
        argv = arguments;
        argc = ARRAY_COUNT(arguments) - 1;
        break;
      }
    }
  } else if (!strstr(argv[0], "brltty")) {
    /* 
     * If we are substituting the real init binary, then we may consider
     * when someone might want to call that binary even when pid != 1.
     * One example is /sbin/telinit which is a symlink to /sbin/init.
     */
    goto executeInit;
  }
#endif /* INIT_PATH */

#ifdef STDERR_PATH
  freopen(STDERR_PATH, "a", stderr);
#endif /* STDERR_PATH */

  {
    ProgramExitStatus exitStatus = brlttyConstruct(argc, argv);

    if (exitStatus == PROG_EXIT_SUCCESS) {
      exitStatus = brlttyRun();
    } else if (exitStatus == PROG_EXIT_FORCE) {
      exitStatus = PROG_EXIT_SUCCESS;
    }

    return exitStatus;
  }
}
