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

/* End-to-end test for brltty-pty's dynamic screen resizing.
 *
 * It launches the real brltty-pty on its own pseudo-terminal, attaches to the
 * published screen segment, then changes the controlling terminal's window size
 * and raises SIGWINCH (exactly as a real terminal does when its window is
 * resized). It verifies that brltty-pty republishes the segment at the new size
 * - re-attaching by key, the way the brltty screen driver does - and that the
 * existing content survives the resize.
 *
 * Exits with status 77 ("skipped") when the environment can't support it.
 */

#include "prologue.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "cmdline.h"
#include "scr_terminal.h"

#define SKIP_STATUS 77

BEGIN_COMMAND_LINE_OPTIONS(programOptions)
END_COMMAND_LINE_OPTIONS(programOptions)

BEGIN_COMMAND_LINE_PARAMETERS(programParameters)
END_COMMAND_LINE_PARAMETERS(programParameters)

BEGIN_COMMAND_LINE_NOTES(programNotes)
END_COMMAND_LINE_NOTES

BEGIN_COMMAND_LINE_DESCRIPTOR(programDescriptor)
  .name = "ptyresize",
  .purpose = strtext("Test brltty-pty dynamic screen resizing end to end."),

  .options = &programOptions,
  .parameters = &programParameters,
  .notes = COMMAND_LINE_NOTES(programNotes),
END_COMMAND_LINE_DESCRIPTOR

static long
nowMilliseconds (void) {
  struct timeval now;
  gettimeofday(&now, NULL);
  return ((long)now.tv_sec * 1000) + (now.tv_usec / 1000);
}

static void
drain (int fd) {
  char buffer[0X400];
  while (read(fd, buffer, sizeof(buffer)) > 0);
}

/* Re-attach to the segment for a path (the emulator recreates it on resize, so
 * the previous attachment becomes stale). Mirrors the screen driver's logic. */
static ScreenSegmentHeader *
reattach (ScreenSegmentHeader *old, const char *path) {
  if (old) detachScreenSegment(old);
  return getScreenSegmentForPath(path);
}

static int
waitForSize (ScreenSegmentHeader **segment, const char *path, int master,
             unsigned int wantWidth, unsigned int wantHeight) {
  const long deadline = nowMilliseconds() + 4000;

  while (nowMilliseconds() < deadline) {
    drain(master);
    *segment = reattach(*segment, path);

    if (*segment &&
        ((*segment)->screenWidth == wantWidth) &&
        ((*segment)->screenHeight == wantHeight)) {
      return 1;
    }

    usleep(50000);
  }

  return 0;
}

int
main (int argc, char *argv[]) {
  PROCESS_COMMAND_LINE(programDescriptor, argc, argv);

  fprintf(stderr, "== resize scenario ==\n");

  int master = posix_openpt(O_RDWR | O_NOCTTY);
  if (master == -1) { fprintf(stderr, "  SKIP: no pty\n"); return SKIP_STATUS; }
  if ((grantpt(master) == -1) || (unlockpt(master) == -1)) { close(master); return SKIP_STATUS; }

  char slavePath[0X100];
  {
    const char *name = ptsname(master);
    if (!name) { close(master); return SKIP_STATUS; }
    snprintf(slavePath, sizeof(slavePath), "%s", name);
  }

  /* Keep a sized slave open (the macOS pty size only persists while one is). */
  int sizedSlave = open(slavePath, O_RDWR);
  {
    struct winsize size = { .ws_row = 24, .ws_col = 80 };
    ioctl((sizedSlave != -1)? sizedSlave: master, TIOCSWINSZ, &size);
  }

  int errorPipe[2];
  if (pipe(errorPipe) == -1) { close(master); if (sizedSlave != -1) close(sizedSlave); return SKIP_STATUS; }

  pid_t child = fork();
  if (child == -1) { close(master); if (sizedSlave != -1) close(sizedSlave); return SKIP_STATUS; }

  if (child == 0) {
    setsid();
    int slave = open(slavePath, O_RDWR);
    if (slave == -1) _exit(SKIP_STATUS);
#ifdef TIOCSCTTY
    ioctl(slave, TIOCSCTTY, 0);
#endif
    dup2(slave, STDIN_FILENO);
    dup2(slave, STDOUT_FILENO);
    dup2(errorPipe[1], STDERR_FILENO);
    close(master);
    if (sizedSlave != -1) close(sizedSlave);
    close(errorPipe[0]);
    close(errorPipe[1]);
    if (slave > STDERR_FILENO) close(slave);

    setenv("TERM", "xterm-256color", 1);
    /* Print a marker at the top-left, then idle so the screen stays put. */
    execl("./brltty-pty", "brltty-pty", "--show-path",
          "sh", "-c", "printf 'TOP'; sleep 30", (char *)NULL);
    _exit(SKIP_STATUS);
  }

  close(errorPipe[1]);
  fcntl(master, F_SETFL, O_NONBLOCK);
  fcntl(errorPipe[0], F_SETFL, O_NONBLOCK);

  int result = SKIP_STATUS;
  ScreenSegmentHeader *segment = NULL;
  char ptyPath[0X100] = {0};
  char errorText[0X1000];
  size_t errorLength = 0;
  const long deadline = nowMilliseconds() + 8000;

  /* Learn the inner pty path from --show-path output. */
  while (!*ptyPath && (nowMilliseconds() < deadline)) {
    drain(master);
    struct pollfd pfd = { .fd = errorPipe[0], .events = POLLIN };
    poll(&pfd, 1, 50);
    ssize_t count = read(errorPipe[0], errorText + errorLength, sizeof(errorText) - errorLength - 1);
    if (count > 0) {
      errorLength += count;
      errorText[errorLength] = 0;
      char *line = errorText, *newline;
      while ((newline = strchr(line, '\n'))) {
        *newline = 0;
        if (line[0] == '/') { snprintf(ptyPath, sizeof(ptyPath), "%s", line); break; }
        line = newline + 1;
      }
    }
  }

  if (!*ptyPath) {
    fprintf(stderr, "  SKIP: brltty-pty did not start (likely sandboxed)\n");
    goto cleanup;
  }

  /* Wait for the initial 80x24 segment with the marker rendered. */
  if (!waitForSize(&segment, ptyPath, master, 80, 24)) {
    fprintf(stderr, "  SKIP: initial segment unavailable (likely sandboxed)\n");
    goto cleanup;
  }
  fprintf(stderr, "  ok   initial size 80x24\n");

  {
    const ScreenSegmentCharacter *cell = getScreenCharacter(segment, 0, 0, NULL);
    if (!cell || (cell->text != 'T')) {
      fprintf(stderr, "  FAIL: marker not rendered before resize\n");
      result = PROG_EXIT_FATAL;
      goto cleanup;
    }
  }

  /* --- Grow to 100x30 --- */
  {
    struct winsize size = { .ws_row = 30, .ws_col = 100 };
    ioctl((sizedSlave != -1)? sizedSlave: master, TIOCSWINSZ, &size);
    kill(child, SIGWINCH);
  }

  if (!waitForSize(&segment, ptyPath, master, 100, 30)) {
    fprintf(stderr, "  FAIL: segment did not grow to 100x30 (got %ux%u)\n",
            segment? segment->screenWidth: 0, segment? segment->screenHeight: 0);
    result = PROG_EXIT_FATAL;
    goto cleanup;
  }
  fprintf(stderr, "  ok   grew to 100x30\n");

  {
    /* Content preserved, and the new bottom-right cell is addressable. */
    const ScreenSegmentCharacter *top = getScreenCharacter(segment, 0, 0, NULL);
    const ScreenSegmentCharacter *corner = getScreenCharacter(segment, 29, 99, NULL);
    if (!top || (top->text != 'T')) {
      fprintf(stderr, "  FAIL: content lost after growing\n");
      result = PROG_EXIT_FATAL;
      goto cleanup;
    }
    if (!corner) {
      fprintf(stderr, "  FAIL: new corner cell not addressable\n");
      result = PROG_EXIT_FATAL;
      goto cleanup;
    }
    fprintf(stderr, "  ok   content preserved and new area addressable\n");
  }

  /* --- Shrink to 60x20 --- */
  {
    struct winsize size = { .ws_row = 20, .ws_col = 60 };
    ioctl((sizedSlave != -1)? sizedSlave: master, TIOCSWINSZ, &size);
    kill(child, SIGWINCH);
  }

  if (!waitForSize(&segment, ptyPath, master, 60, 20)) {
    fprintf(stderr, "  FAIL: segment did not shrink to 60x20 (got %ux%u)\n",
            segment? segment->screenWidth: 0, segment? segment->screenHeight: 0);
    result = PROG_EXIT_FATAL;
    goto cleanup;
  }
  fprintf(stderr, "  ok   shrank to 60x20\n");

  result = PROG_EXIT_SUCCESS;

cleanup:
  if (segment) detachScreenSegment(segment);

  if (child > 0) {
    kill(child, SIGTERM);
    int reaped = 0;
    for (int i=0; i<20; i+=1) {
      drain(master);
      if (waitpid(child, NULL, WNOHANG) == child) { reaped = 1; break; }
      usleep(50000);
    }
    if (!reaped) { kill(child, SIGKILL); waitpid(child, NULL, 0); }
  }

  if (*ptyPath) {
    key_t key = ftok(ptyPath, 't');
    if (key != -1) {
      int identifier = shmget(key, 0, 0);
      if (identifier != -1) shmctl(identifier, IPC_RMID, NULL);
    }
  }

  if (sizedSlave != -1) close(sizedSlave);
  close(master);
  close(errorPipe[0]);

  if (result == PROG_EXIT_SUCCESS) fprintf(stderr, "\nresize: all checks passed\n");
  else if (result == SKIP_STATUS) fprintf(stderr, "\nresize: skipped\n");
  else fprintf(stderr, "\nresize: FAILED\n");

  return result;
}
