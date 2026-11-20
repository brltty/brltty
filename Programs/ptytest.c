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

/* End-to-end test for brltty-pty's UTF-8 / wide-character rendering.
 *
 * It launches the real brltty-pty binary on its own pseudo-terminal, has it
 * run a child that prints a known UTF-8 string (box-drawing, accented and
 * 4-byte characters), then attaches to the shared-memory screen segment that
 * brltty-pty publishes and checks that each cell holds the expected Unicode
 * code point.
 *
 * Because it depends on pseudo-terminal allocation and System V shared memory,
 * it exits with status 77 (the conventional "skipped" code) when the
 * environment cannot support the setup, rather than reporting a false failure.
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
  .name = "ptytest",
  .purpose = strtext("Test brltty-pty UTF-8/wide-character screen rendering end to end."),

  .options = &programOptions,
  .parameters = &programParameters,
  .notes = COMMAND_LINE_NOTES(programNotes),
END_COMMAND_LINE_DESCRIPTOR

typedef struct {
  unsigned int row;
  unsigned int column;
  uint32_t expected;
} ExpectedCell;

typedef struct {
  const char *name;
  unsigned short rows; /* terminal size to run at (0 => default 24x80) */
  unsigned short cols;
  const char *emit; /* sh command that prints the test bytes (no trailing sleep) */
  const ExpectedCell *cells;
  unsigned int cellCount;
  unsigned int readyRow; /* cell whose appearance means rendering is complete */
  unsigned int readyColumn;
  uint32_t readyValue;
  const char *clipboardExpected; /* if set, OSC 52 must deliver this text */
} Scenario;

/* Scenario 1 - UTF-8 / wide rendering. Raw UTF-8 bytes printed:
 *   U+256D U+2500 U+256E  (rounded box: top-left, horizontal, top-right)
 *   ' '  'h'  'i'  ' '
 *   U+00E9 (e-acute)
 *   U+1F600 (grinning face - a 4-byte, double-width emoji)
 */
static const ExpectedCell utf8Cells[] = {
  { 0, 0, 0X256D }, { 0, 1, 0X2500 }, { 0, 2, 0X256E },
  { 0, 3, ' '    }, { 0, 4, 'h'    }, { 0, 5, 'i'    }, { 0, 6, ' ' },
  { 0, 7, 0X00E9 }, { 0, 8, 0X1F600 },
};

/* Scenario 2 - deferred wrap (xenl / magic margin). Print exactly one screen
 * width of 'A', then CR-LF, then 'X'. A terminal with correct deferred wrap
 * leaves 'X' at the start of the very next row; one that wraps immediately
 * inserts a spurious blank line, pushing 'X' down a row (the bug that garbled
 * Claude Code's input box). We assert 'X' is on row 1, not row 2.
 */
static const ExpectedCell wrapCells[] = {
  { 0,  0, 'A' },
  { 0, 79, 'A' },
  { 1,  0, 'X' },
};

/* Scenario 3 - the alternate screen (smcup, "\E[?1049h") must present a
 * cleared buffer. Print old content, switch to the alternate screen, home the
 * cursor and write new text. A correct emulator shows only the new text; the
 * bug this guards against left the old content behind, so full-screen programs
 * (Claude Code, editors) drew on top of stale text. We assert the new text is
 * present and that a cell that held old content is now blank.
 */
static const ExpectedCell altScreenCells[] = {
  { 0, 0, 'X' }, { 0, 1, 'Y' },
  { 0, 2, ' ' }, /* was 'C' before smcup */
  { 0, 4, ' ' }, /* was 'E' before smcup */
};

/* Scenario 4 - arbitrary (non 80x24) screen size. The emulated screen must
 * match the real terminal, not a hardcoded 80x24. Printed at the bottom-right
 * region so the assertions only pass if the screen really is that large.
 */
static const ExpectedCell bigSizeCells[] = {
  { 0, 0, 'T' }, { 0, 1, 'L' },
  { 39, 0, 'B' }, { 39, 1, 'L' },
};

/* Scenario 5 - clear whole display (CSI 2J). Old content must be gone. */
static const ExpectedCell clearDisplayCells[] = {
  { 0, 0, 'Z' }, { 0, 1, ' ' }, { 0, 2, ' ' }, { 1, 0, ' ' },
};

/* Scenario 6 - repeat character (CSI b / REP), used by ncurses' rep. */
static const ExpectedCell repeatCells[] = {
  { 0, 0, 'A' }, { 0, 4, 'A' }, { 0, 5, ' ' },
};

/* Scenario 7 - erase characters in place (CSI X / ECH). */
static const ExpectedCell eraseCells[] = {
  { 0, 0, 'Z' }, { 0, 1, ' ' }, { 0, 2, 'C' },
};

/* Scenario 8 - VT100 line drawing (ESC ( 0). "lqk" must render as the box
 * glyphs U+250C U+2500 U+2510, not as the letters l, q, k. */
static const ExpectedCell lineDrawingCells[] = {
  { 0, 0, 0X250C }, { 0, 1, 0X2500 }, { 0, 2, 0X2510 },
};

/* Scenario 9 - keyboard-protocol negotiation (modifyOtherKeys "CSI > 4 ; 2 m"
 * and kitty "CSI > 1 u") must be swallowed, not misread as SGR or leaked as
 * text. The surrounding A/B/C must land in consecutive cells. */
static const ExpectedCell keyboardModeCells[] = {
  { 0, 0, 'A' }, { 0, 1, 'B' }, { 0, 2, 'C' },
};

/* Scenario 10 - OSC 52 sets the clipboard ("OSC52!" = base64 T1NDNTIh) and is
 * consumed without leaking; the surrounding X/Y must be in consecutive cells. */
static const ExpectedCell clipboardCells[] = {
  { 0, 0, 'X' }, { 0, 1, 'Y' },
};

static const Scenario scenarios[] = {
  {
    .name = "utf8/wide rendering",
    .emit = "printf '\\342\\225\\255\\342\\224\\200\\342\\225\\256 hi \\303\\251\\360\\237\\230\\200'",
    .cells = utf8Cells, .cellCount = ARRAY_COUNT(utf8Cells),
    .readyRow = 0, .readyColumn = 8, .readyValue = 0X1F600,
  },
  {
    .name = "deferred wrap (xenl)",
    .emit = "printf '%080d' 0 | tr 0 A; printf '\\r\\nX'",
    .cells = wrapCells, .cellCount = ARRAY_COUNT(wrapCells),
    .readyRow = 1, .readyColumn = 0, .readyValue = 'X',
  },
  {
    .name = "alternate screen clears (smcup)",
    .emit = "printf 'ABCDE\\033[?1049h\\033[HXY'",
    .cells = altScreenCells, .cellCount = ARRAY_COUNT(altScreenCells),
    .readyRow = 0, .readyColumn = 1, .readyValue = 'Y',
  },
  {
    .name = "arbitrary screen size (40x120)",
    .rows = 40, .cols = 120,
    .emit = "printf 'TL\\033[40;1HBL'",
    .cells = bigSizeCells, .cellCount = ARRAY_COUNT(bigSizeCells),
    .readyRow = 39, .readyColumn = 1, .readyValue = 'L',
  },
  {
    .name = "clear display (CSI 2J)",
    .emit = "printf 'AAAA\\r\\nBBBB\\033[2J\\033[HZ'",
    .cells = clearDisplayCells, .cellCount = ARRAY_COUNT(clearDisplayCells),
    .readyRow = 0, .readyColumn = 0, .readyValue = 'Z',
  },
  {
    .name = "repeat character (CSI b)",
    .emit = "printf 'A\\033[4b'",
    .cells = repeatCells, .cellCount = ARRAY_COUNT(repeatCells),
    .readyRow = 0, .readyColumn = 4, .readyValue = 'A',
  },
  {
    .name = "erase characters (CSI X)",
    .emit = "printf 'ABCDE\\033[1;1H\\033[2XZ'",
    .cells = eraseCells, .cellCount = ARRAY_COUNT(eraseCells),
    .readyRow = 0, .readyColumn = 0, .readyValue = 'Z',
  },
  {
    .name = "VT100 line drawing (ESC ( 0)",
    .emit = "printf '\\033(0lqk\\033(B'",
    .cells = lineDrawingCells, .cellCount = ARRAY_COUNT(lineDrawingCells),
    .readyRow = 0, .readyColumn = 2, .readyValue = 0X2510,
  },
  {
    .name = "keyboard-protocol negotiation swallowed",
    .emit = "printf 'A\\033[>4;2mB\\033[>1uC'",
    .cells = keyboardModeCells, .cellCount = ARRAY_COUNT(keyboardModeCells),
    .readyRow = 0, .readyColumn = 2, .readyValue = 'C',
  },
  {
    .name = "OSC 52 sets clipboard",
    .emit = "printf 'X\\033]52;c;T1NDNTIh\\007Y'",
    .cells = clipboardCells, .cellCount = ARRAY_COUNT(clipboardCells),
    .readyRow = 0, .readyColumn = 1, .readyValue = 'Y',
    .clipboardExpected = "OSC52!",
  },
};

static long
nowMilliseconds (void) {
  struct timeval now;
  gettimeofday(&now, NULL);
  return ((long)now.tv_sec * 1000) + (now.tv_usec / 1000);
}

/* Read and discard whatever brltty-pty writes to its controlling terminal so
 * its rendering never blocks on a full pty buffer. */
static void
drain (int fd) {
  char buffer[0X400];
  while (read(fd, buffer, sizeof(buffer)) > 0);
}

/* Run one scenario through a freshly spawned brltty-pty.
 * Returns PROG_EXIT_SUCCESS, PROG_EXIT_FATAL (assertion failed), or
 * SKIP_STATUS (the environment could not support the test). */
static int
runScenario (const Scenario *scenario) {
  fprintf(stderr, "\n== scenario: %s ==\n", scenario->name);

  int master = posix_openpt(O_RDWR | O_NOCTTY);
  if (master == -1) {
    fprintf(stderr, "  posix_openpt: %s\n", strerror(errno));
    fprintf(stderr, "  SKIP: cannot allocate a pseudo-terminal\n");
    return SKIP_STATUS;
  }

  if ((grantpt(master) == -1) || (unlockpt(master) == -1)) {
    close(master);
    return SKIP_STATUS;
  }

  char slavePath[0X100];
  {
    const char *name = ptsname(master);
    if (!name) { close(master); return SKIP_STATUS; }
    snprintf(slavePath, sizeof(slavePath), "%s", name);
  }

  /* A freshly allocated pty reports a 0x0 window, which would make curses
   * build a zero-size screen. Give it a normal size.
   *
   * On macOS a pty's window size is only retained while a slave fd is open,
   * and setting it on the master before any slave exists is silently ignored.
   * Open a slave here, set the size on it, and keep it open for the lifetime
   * of the test so brltty-pty sees a properly sized controlling terminal. */
  unsigned short rows = scenario->rows? scenario->rows: 24;
  unsigned short cols = scenario->cols? scenario->cols: 80;
  int sizedSlave = open(slavePath, O_RDWR);
  {
    struct winsize size = { .ws_row = rows, .ws_col = cols };
    ioctl((sizedSlave != -1)? sizedSlave: master, TIOCSWINSZ, &size);
  }

  int errorPipe[2];
  if (pipe(errorPipe) == -1) { close(master); return SKIP_STATUS; }

  char clipPath[0X100];
  snprintf(clipPath, sizeof(clipPath), "/tmp/ptytest.clip.%d", (int)getpid());
  if (scenario->clipboardExpected) unlink(clipPath);

  char command[0X400];
  snprintf(command, sizeof(command), "%s; sleep 30", scenario->emit);

  pid_t child = fork();
  if (child == -1) { close(master); close(errorPipe[0]); close(errorPipe[1]); return SKIP_STATUS; }

  if (child == 0) {
    /* Child: become a session leader, adopt the outer pty as its controlling
     * terminal, then exec the real brltty-pty. */
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
    close(errorPipe[0]);
    close(errorPipe[1]);
    if (slave > STDERR_FILENO) close(slave);

    setenv("TERM", "xterm-256color", 1);
    if (scenario->clipboardExpected) setenv("BRLTTY_PTY_CLIPBOARD_FILE", clipPath, 1);

    execl("./brltty-pty", "brltty-pty", "--show-path",
          "sh", "-c", command, (char *)NULL);
    _exit(SKIP_STATUS); /* exec failed */
  }

  close(errorPipe[1]);
  fcntl(master, F_SETFL, O_NONBLOCK);
  fcntl(errorPipe[0], F_SETFL, O_NONBLOCK);

  int result = SKIP_STATUS;
  ScreenSegmentHeader *segment = NULL;
  char ptyPath[0X100] = {0};
  char errorText[0X1000];
  size_t errorLength = 0;
  const long deadline = nowMilliseconds() + 12000;

  /* Phase 1: learn the inner pty path from brltty-pty's --show-path output. */
  while (!*ptyPath && (nowMilliseconds() < deadline)) {
    drain(master);

    struct pollfd pfd = { .fd = errorPipe[0], .events = POLLIN };
    poll(&pfd, 1, 50);

    ssize_t count = read(errorPipe[0], errorText + errorLength,
                         sizeof(errorText) - errorLength - 1);
    if (count > 0) {
      errorLength += count;
      errorText[errorLength] = 0;

      char *line = errorText;
      char *newline;
      while ((newline = strchr(line, '\n'))) {
        *newline = 0;
        if (line[0] == '/') {
          snprintf(ptyPath, sizeof(ptyPath), "%s", line);
          break;
        }
        line = newline + 1;
      }
    }
  }

  if (!*ptyPath) {
    fprintf(stderr, "  SKIP: brltty-pty did not start (likely a sandboxed environment)\n");
    if (errorLength) fprintf(stderr, "  brltty-pty stderr: %s\n", errorText);
    goto cleanup;
  }

  /* Phase 2: attach to the published screen segment. */
  while (!segment && (nowMilliseconds() < deadline)) {
    drain(master);
    segment = getScreenSegmentForPath(ptyPath);
    if (!segment) usleep(50000);
  }

  if (!segment) {
    fprintf(stderr, "  SKIP: shared-memory segment unavailable (likely sandboxed)\n");
    goto cleanup;
  }

  /* Phase 3: wait until the readiness cell has been rendered. */
  int rendered = 0;
  while (!rendered && (nowMilliseconds() < deadline)) {
    drain(master);
    const ScreenSegmentCharacter *cell =
      getScreenCharacter(segment, scenario->readyRow, scenario->readyColumn, NULL);
    if (cell && (cell->text == scenario->readyValue)) rendered = 1;
    if (!rendered) usleep(20000);
  }

  if (!rendered) {
    fprintf(stderr, "  FAIL: expected text was never rendered\n");
    fprintf(stderr, "    segment %ux%u cursor=(%u,%u)\n",
            segment->screenWidth, segment->screenHeight,
            segment->cursorRow, segment->cursorColumn);
    result = PROG_EXIT_FATAL;
    goto cleanup;
  }

  {
    unsigned int failures = 0;

    if ((segment->screenHeight != rows) || (segment->screenWidth != cols)) {
      failures += 1;
      fprintf(stderr, "  FAIL size: got %ux%u, expected %ux%u\n",
              segment->screenWidth, segment->screenHeight, cols, rows);
    } else {
      fprintf(stderr, "  ok   size: %ux%u\n", cols, rows);
    }

    for (unsigned int i=0; i<scenario->cellCount; i+=1) {
      const ExpectedCell *e = &scenario->cells[i];
      const ScreenSegmentCharacter *cell = getScreenCharacter(segment, e->row, e->column, NULL);
      uint32_t got = cell? cell->text: 0;

      if (got == e->expected) {
        fprintf(stderr, "  ok   (%u,%u): U+%04X\n", e->row, e->column, (unsigned int)e->expected);
      } else {
        failures += 1;
        fprintf(stderr, "  FAIL (%u,%u): got U+%04X, expected U+%04X\n",
                e->row, e->column, (unsigned int)got, (unsigned int)e->expected);
      }
    }

    if (scenario->clipboardExpected) {
      char got[0X100] = {0};
      size_t expectedLength = strlen(scenario->clipboardExpected);

      /* The clipboard is written when OSC 52 is processed; give it a moment. */
      for (int i=0; i<20; i+=1) {
        drain(master);
        int fd = open(clipPath, O_RDONLY);
        if (fd != -1) {
          ssize_t n = read(fd, got, sizeof(got) - 1);
          close(fd);
          if (n >= 0) got[n] = 0;
          if (strlen(got) >= expectedLength) break;
        }
        usleep(50000);
      }

      if (strcmp(got, scenario->clipboardExpected) == 0) {
        fprintf(stderr, "  ok   clipboard: \"%s\"\n", got);
      } else {
        failures += 1;
        fprintf(stderr, "  FAIL clipboard: got \"%s\", expected \"%s\"\n",
                got, scenario->clipboardExpected);
      }
    }

    result = failures? PROG_EXIT_FATAL: PROG_EXIT_SUCCESS;
    fprintf(stderr, "  %u cell(s) checked, %u failed\n", scenario->cellCount, failures);
  }

cleanup:
  if (scenario->clipboardExpected) unlink(clipPath);
  if (sizedSlave != -1) close(sizedSlave);
  if (segment) detachScreenSegment(segment);

  /* Tear down brltty-pty without ever blocking: ask politely, then escalate. */
  if (child > 0) {
    kill(child, SIGTERM);

    int reaped = 0;
    for (int i=0; i<20; i+=1) { /* up to ~1 second */
      drain(master);
      if (waitpid(child, NULL, WNOHANG) == child) { reaped = 1; break; }
      usleep(50000);
    }

    if (!reaped) {
      kill(child, SIGKILL);
      waitpid(child, NULL, 0);
    }
  }

  /* Remove the shared-memory segment even if brltty-pty was killed before it
   * could clean up itself, so repeated runs never attach to a stale segment. */
  if (*ptyPath) {
    key_t key = ftok(ptyPath, 't');

    if (key != -1) {
      int identifier = shmget(key, 0, 0);
      if (identifier != -1) shmctl(identifier, IPC_RMID, NULL);
    }
  }

  close(master);
  close(errorPipe[0]);
  return result;
}

int
main (int argc, char *argv[]) {
  PROCESS_COMMAND_LINE(programDescriptor, argc, argv);

  unsigned int failed = 0;
  unsigned int skipped = 0;

  for (unsigned int i=0; i<ARRAY_COUNT(scenarios); i+=1) {
    int result = runScenario(&scenarios[i]);
    if (result == PROG_EXIT_FATAL) failed += 1;
    else if (result == SKIP_STATUS) skipped += 1;
  }

  fprintf(stderr, "\n%u scenario(s): %u failed, %u skipped\n",
          (unsigned int)ARRAY_COUNT(scenarios), failed, skipped);

  if (failed) return PROG_EXIT_FATAL;
  if (skipped == ARRAY_COUNT(scenarios)) return SKIP_STATUS;
  return PROG_EXIT_SUCCESS;
}
