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

/* End-to-end test for brltty-pty's clipboard bridge (emulator side).
 *
 * Each scenario launches the real brltty-pty (with --driver-directives, so the
 * terminal message queue is active) and stands in for both the outer terminal
 * (it owns the master pty) and the screen driver (it attaches to the message
 * queue). It drives BRLTTY's clipboard out to the host via
 * TERM_MSG_CLIPBOARD_TO_HOST and checks the two publish modes:
 *   - native:      BRLTTY_PTY_CLIPBOARD_FILE intercepts the write; expect the
 *                  file to hold the text. (No real pasteboard is touched.)
 *   - passthrough: BRLTTY_PTY_CLIPBOARD_MODE=passthrough; expect an OSC 52
 *                  sequence carrying the base64 text on the outer pty.
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
#include <sys/msg.h>

#include "cmdline.h"
#include "scr_terminal.h"
#include "msg_queue.h"

#define SKIP_STATUS 77
#define CLIPBOARD_TEXT "from-brltty-clipboard"

BEGIN_COMMAND_LINE_OPTIONS(programOptions)
END_COMMAND_LINE_OPTIONS(programOptions)

BEGIN_COMMAND_LINE_PARAMETERS(programParameters)
END_COMMAND_LINE_PARAMETERS(programParameters)

BEGIN_COMMAND_LINE_NOTES(programNotes)
END_COMMAND_LINE_NOTES

BEGIN_COMMAND_LINE_DESCRIPTOR(programDescriptor)
  .name = "ptyclip",
  .purpose = strtext("Test brltty-pty's clipboard bridge end to end."),

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
readFileEquals (const char *path, const char *expected) {
  FILE *file = fopen(path, "r");
  if (!file) return 0;

  char buffer[0X100] = {0};
  size_t length = fread(buffer, 1, sizeof(buffer) - 1, file);
  fclose(file);
  buffer[length] = 0;

  return strcmp(buffer, expected) == 0;
}

static const char base64Alphabet[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Encode for the passthrough expectation; mirrors encodeBase64 in pty_clipboard.c. */
static void
encodeBase64 (const char *text, char *out) {
  const unsigned char *data = (const unsigned char *)text;
  size_t length = strlen(text);
  size_t in = 0, off = 0;

  while ((length - in) >= 3) {
    unsigned int triple = (data[in] << 16) | (data[in+1] << 8) | data[in+2];
    out[off++] = base64Alphabet[(triple >> 18) & 0X3F];
    out[off++] = base64Alphabet[(triple >> 12) & 0X3F];
    out[off++] = base64Alphabet[(triple >> 6) & 0X3F];
    out[off++] = base64Alphabet[triple & 0X3F];
    in += 3;
  }

  size_t remaining = length - in;
  if (remaining == 1) {
    unsigned int triple = data[in] << 16;
    out[off++] = base64Alphabet[(triple >> 18) & 0X3F];
    out[off++] = base64Alphabet[(triple >> 12) & 0X3F];
    out[off++] = '='; out[off++] = '=';
  } else if (remaining == 2) {
    unsigned int triple = (data[in] << 16) | (data[in+1] << 8);
    out[off++] = base64Alphabet[(triple >> 18) & 0X3F];
    out[off++] = base64Alphabet[(triple >> 12) & 0X3F];
    out[off++] = base64Alphabet[(triple >> 6) & 0X3F];
    out[off++] = '=';
  }

  out[off] = 0;
}

/* Run one publish scenario. passthrough selects the mode; returns PROG_EXIT_*
 * or SKIP_STATUS. */
static int
runScenario (int passthrough, const char *label) {
  char setPath[0X100];
  snprintf(setPath, sizeof(setPath), "/tmp/ptyclip.%d.%d", passthrough, (int)getpid());
  unlink(setPath);

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
    if (passthrough) {
      setenv("BRLTTY_PTY_CLIPBOARD_MODE", "passthrough", 1);
    } else {
      /* The write-file override intercepts before the mode, so this exercises
       * the publish plumbing without a real pasteboard regardless of mode. */
      setenv("BRLTTY_PTY_CLIPBOARD_FILE", setPath, 1);
    }

    /* --driver-directives enables the message queue and prints the inner pty
     * path to stderr. */
    execl("./brltty-pty", "brltty-pty", "--driver-directives",
          "sh", "-c", "sleep 30", (char *)NULL);
    _exit(SKIP_STATUS);
  }

  close(errorPipe[1]);
  fcntl(master, F_SETFL, O_NONBLOCK);
  fcntl(errorPipe[0], F_SETFL, O_NONBLOCK);

  int result = SKIP_STATUS;

  /* Accumulate everything the emulator writes to the outer pty, so a passthrough
   * OSC 52 can be found in it. */
  char outBuffer[0X4000];
  size_t outLength = 0;

  char ptyPath[0X100] = {0};
  char errorText[0X1000];
  size_t errorLength = 0;
  const long deadline = nowMilliseconds() + 8000;

  /* Learn the inner pty path from the "path ..." driver directive. */
  while (!*ptyPath && (nowMilliseconds() < deadline)) {
    if (outLength < sizeof(outBuffer) - 1) {
      ssize_t n = read(master, outBuffer + outLength, sizeof(outBuffer) - 1 - outLength);
      if (n > 0) outLength += n;
    }

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
  }

  if (!*ptyPath) {
    fprintf(stderr, "  SKIP %s: brltty-pty did not start (likely sandboxed)\n", label);
    goto cleanup;
  }

  key_t key;
  int queue;
  if (!makeTerminalKey(&key, ptyPath) || !getMessageQueue(&queue, key)) {
    fprintf(stderr, "  SKIP %s: message queue unavailable\n", label);
    goto cleanup;
  }

  result = PROG_EXIT_SUCCESS;

  /* Send BRLTTY's clipboard; the emulator must publish it to the host. */
  sendMessage(queue, TERM_MSG_CLIPBOARD_TO_HOST, CLIPBOARD_TEXT, strlen(CLIPBOARD_TEXT), 0);

  char expectedOsc[0X100];
  if (passthrough) {
    char encoded[0X80];
    encodeBase64(CLIPBOARD_TEXT, encoded);
    snprintf(expectedOsc, sizeof(expectedOsc), "\033]52;c;%s", encoded);
  }

  int ok = 0;
  const long settle = nowMilliseconds() + 3000;
  while (nowMilliseconds() < settle) {
    if (outLength < sizeof(outBuffer) - 1) {
      ssize_t n = read(master, outBuffer + outLength, sizeof(outBuffer) - 1 - outLength);
      if (n > 0) outLength += n;
    }

    if (passthrough) {
      if (bufferContains(outBuffer, outLength, expectedOsc)) { ok = 1; break; }
    } else if (readFileEquals(setPath, CLIPBOARD_TEXT)) {
      ok = 1; break;
    }

    usleep(50000);
  }

  if (ok) {
    fprintf(stderr, "  ok   %s: clipboard reached the host\n", label);
  } else {
    fprintf(stderr, "  FAIL %s: clipboard not published\n", label);
    result = PROG_EXIT_FATAL;
  }

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
    /* brltty-pty's shared-memory segment and its message queue share one key
     * (ftok(path,'t')); remove both so a SIGKILLed brltty-pty doesn't leak them. */
    key_t key2 = ftok(ptyPath, 't');
    if (key2 != -1) {
      int id = shmget(key2, 0, 0);
      if (id != -1) shmctl(id, IPC_RMID, NULL);

      int queue = msgget(key2, 0);
      if (queue != -1) msgctl(queue, IPC_RMID, NULL);
    }
  }

  unlink(setPath);
  if (sizedSlave != -1) close(sizedSlave);
  close(master);
  close(errorPipe[0]);
  return result;
}

int
main (int argc, char *argv[]) {
  PROCESS_COMMAND_LINE(programDescriptor, argc, argv);

  int failures = 0;
  int ran = 0;

  static const struct { int passthrough; const char *label; } scenarios[] = {
    { .passthrough = 0, .label = "native (file)" },
    { .passthrough = 1, .label = "passthrough (OSC 52)" },
  };

  for (unsigned int i=0; i<ARRAY_COUNT(scenarios); i+=1) {
    int status = runScenario(scenarios[i].passthrough, scenarios[i].label);
    if (status == PROG_EXIT_SUCCESS) ran += 1;
    else if (status != SKIP_STATUS) failures += 1;
  }

  int result = failures? PROG_EXIT_FATAL: ran? PROG_EXIT_SUCCESS: SKIP_STATUS;

  fprintf(stderr, "\nclipboard bridge: %s\n",
          (result == PROG_EXIT_SUCCESS)? "all checks passed":
          (result == SKIP_STATUS)? "skipped": "FAILED");
  return result;
}
