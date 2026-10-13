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

/* Debug helper: render a raw byte stream through the real brltty-pty terminal
 * emulator and print the resulting screen grid (as UTF-8) plus the cursor
 * position. Useful for reproducing and diagnosing rendering problems.
 *
 * Configuration is via the environment so it does not fight the command-line
 * parser:
 *   PTYDUMP_INPUT  absolute path to a file whose bytes are fed to the emulator
 *   PTYDUMP_ROWS   screen height (default 24)
 *   PTYDUMP_COLS   screen width  (default 80)
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
#include "utf8.h"

#define SKIP_STATUS 77

BEGIN_COMMAND_LINE_OPTIONS(programOptions)
END_COMMAND_LINE_OPTIONS(programOptions)

BEGIN_COMMAND_LINE_PARAMETERS(programParameters)
END_COMMAND_LINE_PARAMETERS(programParameters)

BEGIN_COMMAND_LINE_NOTES(programNotes)
END_COMMAND_LINE_NOTES

BEGIN_COMMAND_LINE_DESCRIPTOR(programDescriptor)
  .name = "ptydump",
  .purpose = strtext("Render a byte stream through brltty-pty and print the screen grid."),

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

static void
dumpScreen (ScreenSegmentHeader *segment) {
  unsigned int rows = segment->screenHeight;
  unsigned int cols = segment->screenWidth;

  fprintf(stderr, "screen %ux%u cursor=(row %u, col %u)\n",
          cols, rows, segment->cursorRow, segment->cursorColumn);

  fprintf(stderr, "    +");
  for (unsigned int c=0; c<cols; c+=1) fprintf(stderr, "-");
  fprintf(stderr, "+\n");

  for (unsigned int r=0; r<rows; r+=1) {
    fprintf(stderr, "%3u |", r);

    for (unsigned int c=0; c<cols; c+=1) {
      const ScreenSegmentCharacter *cell = getScreenCharacter(segment, r, c, NULL);
      uint32_t text = cell? cell->text: ' ';
      if ((text == 0) || (text == ' ')) {
        fputc(' ', stderr);
      } else {
        Utf8Buffer utf8;
        size_t length = convertCodepointToUtf8(text, utf8);
        fwrite(utf8, 1, length, stderr);
      }
    }

    fprintf(stderr, "|%s\n", (r == segment->cursorRow)? " <-- cursor row": "");
  }

  fprintf(stderr, "    +");
  for (unsigned int c=0; c<cols; c+=1) fprintf(stderr, "-");
  fprintf(stderr, "+\n");
}

int
main (int argc, char *argv[]) {
  PROCESS_COMMAND_LINE(programDescriptor, argc, argv);

  const char *inputPath = getenv("PTYDUMP_INPUT");
  if (!inputPath || !*inputPath) {
    fprintf(stderr, "ptydump: set PTYDUMP_INPUT to a file of bytes to render\n");
    return PROG_EXIT_SYNTAX;
  }

  unsigned short rows = 24;
  unsigned short cols = 80;
  { const char *v = getenv("PTYDUMP_ROWS"); if (v && atoi(v) > 0) rows = atoi(v); }
  { const char *v = getenv("PTYDUMP_COLS"); if (v && atoi(v) > 0) cols = atoi(v); }

  int master = posix_openpt(O_RDWR | O_NOCTTY);
  if (master == -1) { fprintf(stderr, "posix_openpt: %s\n", strerror(errno)); return SKIP_STATUS; }
  if ((grantpt(master) == -1) || (unlockpt(master) == -1)) return SKIP_STATUS;

  char slavePath[0X100];
  { const char *n = ptsname(master); if (!n) return SKIP_STATUS; snprintf(slavePath, sizeof(slavePath), "%s", n); }

  /* On macOS a pty's window size is only retained while a slave fd is open,
   * and setting it on the master before any slave exists is silently ignored.
   * Open a slave here, set the size on it, and keep it open for the lifetime
   * of the test so brltty-pty sees a properly sized controlling terminal. */
  int sizedSlave = open(slavePath, O_RDWR);
  if (sizedSlave != -1) {
    struct winsize ws = { .ws_row = rows, .ws_col = cols };
    ioctl(sizedSlave, TIOCSWINSZ, &ws);
  }

  int errorPipe[2];
  if (pipe(errorPipe) == -1) return SKIP_STATUS;

  char command[0X400];
  snprintf(command, sizeof(command), "cat '%s'; sleep 30", inputPath);

  pid_t child = fork();
  if (child == -1) return SKIP_STATUS;

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
    close(master); close(errorPipe[0]); close(errorPipe[1]);
    if (slave > STDERR_FILENO) close(slave);
    setenv("TERM", "xterm-256color", 1);
    execl("./brltty-pty", "brltty-pty", "--show-path", "sh", "-c", command, (char *)NULL);
    _exit(SKIP_STATUS);
  }

  close(errorPipe[1]);
  fcntl(master, F_SETFL, O_NONBLOCK);
  fcntl(errorPipe[0], F_SETFL, O_NONBLOCK);

  int exitStatus = SKIP_STATUS;
  ScreenSegmentHeader *segment = NULL;
  char ptyPath[0X100] = {0};
  char errorText[0X1000];
  size_t errorLength = 0;
  const long deadline = nowMilliseconds() + 12000;

  while (!*ptyPath && (nowMilliseconds() < deadline)) {
    drain(master);
    struct pollfd pfd = { .fd = errorPipe[0], .events = POLLIN };
    poll(&pfd, 1, 50);
    ssize_t count = read(errorPipe[0], errorText + errorLength, sizeof(errorText) - errorLength - 1);
    if (count > 0) {
      errorLength += count;
      errorText[errorLength] = 0;
      char *line = errorText, *nl;
      while ((nl = strchr(line, '\n'))) {
        *nl = 0;
        if (line[0] == '/') { snprintf(ptyPath, sizeof(ptyPath), "%s", line); break; }
        line = nl + 1;
      }
    }
  }

  if (!*ptyPath) { fprintf(stderr, "SKIP: brltty-pty did not start\n"); goto cleanup; }

  while (!segment && (nowMilliseconds() < deadline)) {
    drain(master);
    segment = getScreenSegmentForPath(ptyPath);
    if (!segment) usleep(50000);
  }
  if (!segment) { fprintf(stderr, "SKIP: segment unavailable\n"); goto cleanup; }

  /* Give the emulator time to consume all of the input. */
  for (int i=0; i<10; i+=1) { drain(master); usleep(50000); }

  dumpScreen(segment);
  exitStatus = PROG_EXIT_SUCCESS;

cleanup:
  if (sizedSlave != -1) close(sizedSlave);
  if (segment) detachScreenSegment(segment);
  if (child > 0) {
    kill(child, SIGTERM);
    int reaped = 0;
    for (int i=0; i<20; i+=1) { drain(master); if (waitpid(child, NULL, WNOHANG) == child) { reaped = 1; break; } usleep(50000); }
    if (!reaped) { kill(child, SIGKILL); waitpid(child, NULL, 0); }
  }
  if (*ptyPath) {
    key_t key = ftok(ptyPath, 't');
    if (key != -1) { int id = shmget(key, 0, 0); if (id != -1) shmctl(id, IPC_RMID, NULL); }
  }
  close(master);
  close(errorPipe[0]);
  return exitStatus;
}
