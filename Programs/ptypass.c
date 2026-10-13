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

/* End-to-end test for brltty-pty's display passthrough (BRLTTY_PTY_PASSTHROUGH).
 *
 * It launches the real brltty-pty with passthrough enabled, running a child that
 * emits a known mix of output, and stands in for the outer terminal (it owns the
 * master pty). It then checks what reached that terminal:
 *   - plain text and a 24-bit (true-colour) SGR sequence appear verbatim -
 *     proving the terminal renders the child's real colours, not a 16-colour
 *     reduction;
 *   - a cursor-position-report query (CSI 6 n) does NOT appear - we answer it
 *     ourselves, so it must not also reach the terminal;
 *   - an OSC 52 clipboard write does NOT appear - we consume and publish it
 *     (here, to the file override) instead of letting the terminal see it.
 *
 * Exits 77 ("skipped") when the environment can't support the setup.
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

/* The child emits: a true-colour run "HELLO", a CPR query, and an OSC 52 write
 * of "PASS" (base64 UEFTUw==). printf(1) interprets the backslash escapes. */
#define TRUECOLOR_SGR "\033[38;2;10;200;30m"
#define CHILD_COMMAND \
  "printf '\\033[38;2;10;200;30mHELLO\\033[0m\\n\\033[6n'; " \
  "printf '\\033]52;c;UEFTUw==\\007'; " \
  "sleep 5"
#define CLIPBOARD_EXPECTED "PASS"

BEGIN_COMMAND_LINE_OPTIONS(programOptions)
END_COMMAND_LINE_OPTIONS(programOptions)

BEGIN_COMMAND_LINE_PARAMETERS(programParameters)
END_COMMAND_LINE_PARAMETERS(programParameters)

BEGIN_COMMAND_LINE_NOTES(programNotes)
END_COMMAND_LINE_NOTES

BEGIN_COMMAND_LINE_DESCRIPTOR(programDescriptor)
  .name = "ptypass",
  .purpose = strtext("Test brltty-pty's display passthrough end to end."),

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

/* Search a (possibly binary) buffer for a NUL-terminated needle. */
static int
bufferContains (const char *haystack, size_t length, const char *needle) {
  size_t needleLength = strlen(needle);
  if (needleLength == 0 || length < needleLength) return 0;

  for (size_t i=0; i+needleLength<=length; i+=1) {
    if (memcmp(haystack + i, needle, needleLength) == 0) return 1;
  }

  return 0;
}

static int
fileContains (const char *path, const char *expected) {
  FILE *file = fopen(path, "r");
  if (!file) return 0;

  char buffer[0X100] = {0};
  size_t length = fread(buffer, 1, sizeof(buffer) - 1, file);
  fclose(file);

  return bufferContains(buffer, length, expected);
}

static int
check (const char *label, int condition, int *failures) {
  if (condition) {
    fprintf(stderr, "  ok   %s\n", label);
  } else {
    fprintf(stderr, "  FAIL %s\n", label);
    *failures += 1;
  }

  return condition;
}

int
main (int argc, char *argv[]) {
  PROCESS_COMMAND_LINE(programDescriptor, argc, argv);

  int result = SKIP_STATUS;
  int failures = 0;

  char clipPath[0X100];
  snprintf(clipPath, sizeof(clipPath), "/tmp/ptypass.clip.%d", (int)getpid());
  unlink(clipPath);

  int master = posix_openpt(O_RDWR | O_NOCTTY);
  if (master == -1) return SKIP_STATUS;
  if ((grantpt(master) == -1) || (unlockpt(master) == -1)) { close(master); return SKIP_STATUS; }

  char slavePath[0X100];
  {
    const char *name = ptsname(master);
    if (!name) { close(master); return SKIP_STATUS; }
    snprintf(slavePath, sizeof(slavePath), "%s", name);
  }

  int sizedSlave = open(slavePath, O_RDWR);
  {
    struct winsize size = { .ws_row = 24, .ws_col = 80 };
    ioctl((sizedSlave != -1)? sizedSlave: master, TIOCSWINSZ, &size);
  }

  int errorPipe[2];
  if (pipe(errorPipe) == -1) { close(master); if (sizedSlave != -1) close(sizedSlave); return SKIP_STATUS; }

  pid_t child = fork();
  if (child == -1) {
    close(master); if (sizedSlave != -1) close(sizedSlave);
    close(errorPipe[0]); close(errorPipe[1]);
    return SKIP_STATUS;
  }

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
    /* Route any clipboard write to a file so OSC 52 isn't re-emitted to the
     * outer pty by the clipboard layer (which would confuse the suppression
     * check), and so we can confirm the OSC 52 was consumed. */
    setenv("BRLTTY_PTY_CLIPBOARD_FILE", clipPath, 1);

    execl("./brltty-pty", "brltty-pty", "--driver-directives",
          "sh", "-c", CHILD_COMMAND, (char *)NULL);
    _exit(SKIP_STATUS);
  }

  close(errorPipe[1]);
  fcntl(master, F_SETFL, O_NONBLOCK);
  fcntl(errorPipe[0], F_SETFL, O_NONBLOCK);

  char outBuffer[0X4000];
  size_t outLength = 0;

  char ptyPath[0X100] = {0};
  char errorText[0X1000];
  size_t errorLength = 0;

  /* Collect the outer-pty output and learn the inner pty path from stderr. We
   * stop as soon as the POSITIVE markers have arrived - the passthrough text
   * ("HELLO") and the consumed clipboard - so the negative assertions below
   * ("CPR/OSC 52 not present") cannot pass spuriously just because output hadn't
   * arrived yet. The deadline is only a backstop. */
  const long deadline = nowMilliseconds() + 4000;
  while (nowMilliseconds() < deadline) {
    if (outLength < sizeof(outBuffer) - 1) {
      ssize_t n = read(master, outBuffer + outLength, sizeof(outBuffer) - 1 - outLength);
      if (n > 0) outLength += n;
    }

    if (!*ptyPath) {
      struct pollfd pfd = { .fd = errorPipe[0], .events = POLLIN };
      poll(&pfd, 1, 50);
      ssize_t count = read(errorPipe[0], errorText + errorLength, sizeof(errorText) - errorLength - 1);
      if (count > 0) {
        errorLength += count;
        errorText[errorLength] = 0;
        char *line = errorText, *newline;
        while ((newline = strchr(line, '\n'))) {
          *newline = 0;
          char *slash = strchr(line, '/');
          if (slash) { snprintf(ptyPath, sizeof(ptyPath), "%s", slash); break; }
          line = newline + 1;
        }
      }
    } else {
      /* Ready once both positive signals are in: text passed through and the
       * clipboard was consumed (written to the file override). */
      if (bufferContains(outBuffer, outLength, "HELLO") &&
          fileContains(clipPath, CLIPBOARD_EXPECTED)) {
        break;
      }
      usleep(50000);
    }
  }

  if (!*ptyPath) {
    fprintf(stderr, "  SKIP: brltty-pty did not start (likely sandboxed)\n");
    goto cleanup;
  }

  result = PROG_EXIT_SUCCESS;

  check("text passes through to the terminal",
        bufferContains(outBuffer, outLength, "HELLO"), &failures);
  check("true-colour SGR passes through verbatim",
        bufferContains(outBuffer, outLength, TRUECOLOR_SGR), &failures);
  check("CPR query is suppressed (we answer it)",
        !bufferContains(outBuffer, outLength, "\033[6n"), &failures);
  check("OSC 52 is suppressed (we publish it)",
        !bufferContains(outBuffer, outLength, "\033]52;c;"), &failures);
  check("OSC 52 was consumed by the clipboard layer",
        fileContains(clipPath, CLIPBOARD_EXPECTED), &failures);

  if (failures) result = PROG_EXIT_FATAL;

cleanup:
  if (child > 0) {
    kill(child, SIGTERM);
    int reaped = 0;
    for (int i=0; i<20; i+=1) {
      char scratch[0X400];
      while (read(master, scratch, sizeof(scratch)) > 0);
      if (waitpid(child, NULL, WNOHANG) == child) { reaped = 1; break; }
      usleep(50000);
    }
    if (!reaped) { kill(child, SIGKILL); waitpid(child, NULL, 0); }
  }

  if (*ptyPath) {
    key_t key = ftok(ptyPath, 't');
    if (key != -1) {
      int id = shmget(key, 0, 0);
      if (id != -1) shmctl(id, IPC_RMID, NULL);
    }
  }

  unlink(clipPath);
  if (sizedSlave != -1) close(sizedSlave);
  close(master);
  close(errorPipe[0]);

  fprintf(stderr, "\ndisplay passthrough: %s\n",
          (result == PROG_EXIT_SUCCESS)? "all checks passed":
          (result == SKIP_STATUS)? "skipped": "FAILED");
  return result;
}
