/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2026 by The BRLTTY Developers.
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

/*
 * brltty-handoff: suspend or resume BRLTTY's braille driver via BrlAPI.
 *
 * "suspend" forks a background worker that holds the BrlAPI connection open
 * (keeping the driver suspended) and writes its PID to a state file.
 * "resume" sends SIGUSR1 to that worker, which resumes the driver and exits.
 *
 * Design note: brlapi_suspendDriver was originally intended for a second
 * BrlAPI-aware screen reader (such as Orca) to take over the braille display
 * through BrlAPI itself.  This program uses it differently: to release the
 * USB device so that an application that accesses braille hardware directly
 * — bypassing BrlAPI entirely — can claim it.
 *
 * The immediate use case is VoiceOver on macOS.  macOS enforces exclusive USB
 * device access via IOKit (USBDeviceOpenSeize); whichever application opens
 * the device first blocks all others.  VoiceOver does not speak BrlAPI, so
 * the normal BrlAPI sharing mechanism does not apply.  Suspending the driver
 * causes BRLTTY to close its USB handle, after which VoiceOver can seize the
 * device.  On resume, VoiceOver releases its seize and BRLTTY reclaims it.
 *
 * A similar workaround for JAWS on Windows would require a separate
 * Windows-specific implementation; this code relies on POSIX fork/signal.
 *
 * Usage: brltty-handoff suspend
 *        brltty-handoff resume
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/time.h>
#include "brlapi.h"

#define PID_FILE "/tmp/brltty-handoff.pid"
#define SUSPEND_TIMEOUT_SECS 5

static volatile int doResume = 0;

static void
handleSIGUSR1 (int sig) {
  doResume = 1;
}

static int
cmdSuspend (void) {
  brlapi_connectionSettings_t settings = BRLAPI_SETTINGS_INITIALIZER;

  if (brlapi_openConnection(&settings, NULL) < 0) {
    brlapi_perror("brlapi_openConnection");
    return 1;
  }

  char driver[BRLAPI_MAXNAMELENGTH + 1];
  if (brlapi_getDriverName(driver, sizeof(driver)) < 0) {
    brlapi_perror("brlapi_getDriverName");
    brlapi_closeConnection();
    return 1;
  }

  /*
   * Run brlapi_suspendDriver in a worker child with a timeout.
   * If the driver close blocks (e.g. Bluetooth teardown), the parent kills
   * the worker after SUSPEND_TIMEOUT_SECS and returns failure.
   */
  int pipefd[2];
  if (pipe(pipefd) < 0) {
    perror("pipe");
    brlapi_closeConnection();
    return 1;
  }

  pid_t worker = fork();
  if (worker < 0) {
    perror("fork");
    close(pipefd[0]); close(pipefd[1]);
    brlapi_closeConnection();
    return 1;
  }

  if (worker == 0) {
    /* Worker child: attempt suspend, signal result via pipe. */
    close(pipefd[0]);

    if (brlapi_suspendDriver(driver) < 0) {
      brlapi_perror("brlapi_suspendDriver");
      write(pipefd[1], "F", 1);
      close(pipefd[1]);
      _exit(1);
    }

    /* Install handler before writing 'S': avoids a race where the parent
     * writes the PID file and the caller sends SIGUSR1 before the handler
     * is in place. */
    signal(SIGUSR1, handleSIGUSR1);
    write(pipefd[1], "S", 1);
    close(pipefd[1]);

    /*
     * Redirect inherited stdio to /dev/null so that callers that capture
     * output (e.g. via a pipe) see EOF and do not block waiting for the
     * worker to exit.
     */
    {
      int devnull = open("/dev/null", O_RDWR);
      if (devnull >= 0) {
        dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        if (devnull > STDERR_FILENO) close(devnull);
      }
    }

    /* Hold connection open until SIGUSR1 signals resume. */
    while (!doResume) pause();

    if (brlapi_resumeDriver() < 0) brlapi_perror("brlapi_resumeDriver");

    brlapi_closeConnection();
    unlink(PID_FILE);
    _exit(0);
  }

  /* Parent: wait for worker to report suspend result, with a timeout. */
  close(pipefd[1]);

  struct timeval tv = { SUSPEND_TIMEOUT_SECS, 0 };
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(pipefd[0], &rfds);

  int ready = select(pipefd[0] + 1, &rfds, NULL, NULL, &tv);

  if (ready <= 0) {
    kill(worker, SIGKILL);
    waitpid(worker, NULL, 0);
    close(pipefd[0]);
    brlapi_closeConnection();
    fprintf(stderr, "brltty-handoff: suspendDriver timed out\n");
    return 1;
  }

  char status = 'F';
  if (read(pipefd[0], &status, 1) < 1) status = 'F';
  close(pipefd[0]);

  if (status != 'S') {
    waitpid(worker, NULL, 0);
    brlapi_closeConnection();
    return 1;
  }

  /* Worker succeeded: write its PID and exit (worker holds the connection). */
  FILE *f = fopen(PID_FILE, "w");
  if (f) { fprintf(f, "%d\n", worker); fclose(f); }
  _exit(0);
}

static int
cmdResume (void) {
  FILE *f = fopen(PID_FILE, "r");
  if (!f) {
    /* No prior suspend: brltty will connect to the device on its own. */
    return 0;
  }

  pid_t pid = 0;
  int count = fscanf(f, "%d", &pid);
  fclose(f);

  if (count != 1 || pid <= 0) {
    unlink(PID_FILE);
    return 0;
  }

  if (kill(pid, SIGUSR1) < 0) {
    /* Process is gone — stale PID file, treat as already resumed. */
    unlink(PID_FILE);
  }

  return 0;
}

int
main (int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s suspend | resume\n", argv[0]);
    return 1;
  }

  if (strcmp(argv[1], "suspend") == 0) return cmdSuspend();
  if (strcmp(argv[1], "resume") == 0)  return cmdResume();

  fprintf(stderr, "Unknown command: %s\n", argv[1]);
  return 1;
}
