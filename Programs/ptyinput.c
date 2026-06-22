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

/* End-to-end test for brltty-pty's keyboard input translation.
 *
 * It launches the real brltty-pty on its own pseudo-terminal, has it run a
 * child (this same binary, in "reader" mode) that puts its terminal into raw
 * mode and copies every byte it receives to a file. The test then writes a
 * key sequence (as a host terminal would) to brltty-pty's controlling terminal
 * and checks that the child received the bytes a screen/tmux program expects.
 *
 * This is how we verify, for example, that Home arriving as the application
 * sequence "ESC O H" is delivered to the child as "ESC [ 1 ~" (and not left as
 * "ESC O H", which vim would read as "open line and insert").
 *
 * Because it depends on pseudo-terminal allocation, it exits with status 77
 * (the conventional "skipped" code) when the environment cannot support it.
 */

#include "prologue.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "cmdline.h"

#define SKIP_STATUS 77

BEGIN_COMMAND_LINE_OPTIONS(programOptions)
END_COMMAND_LINE_OPTIONS(programOptions)

BEGIN_COMMAND_LINE_PARAMETERS(programParameters)
END_COMMAND_LINE_PARAMETERS(programParameters)

BEGIN_COMMAND_LINE_NOTES(programNotes)
END_COMMAND_LINE_NOTES

BEGIN_COMMAND_LINE_DESCRIPTOR(programDescriptor)
  .name = "ptyinput",
  .purpose = strtext("Test brltty-pty keyboard input translation end to end."),

  .options = &programOptions,
  .parameters = &programParameters,
  .notes = COMMAND_LINE_NOTES(programNotes),
END_COMMAND_LINE_DESCRIPTOR

/* The environment variables that put this binary into "reader" (child) mode. */
#define ENV_OUTPUT "PTYINPUT_OUTPUT"   /* file to append received bytes to     */
#define ENV_INIT   "PTYINPUT_INIT"     /* bytes to emit to stdout at startup    */

/* Reader mode: become the program running inside brltty-pty. Optionally emit an
 * initialization sequence to the emulator (e.g. smkx to turn on application
 * cursor keys), then copy raw stdin to the output file a byte at a time. */
static int
runReader (void) {
  const char *outputPath = getenv(ENV_OUTPUT);
  if (!outputPath) return SKIP_STATUS;

  {
    struct termios attributes;
    if (tcgetattr(STDIN_FILENO, &attributes) != -1) {
      cfmakeraw(&attributes);
      tcsetattr(STDIN_FILENO, TCSANOW, &attributes);
    }
  }

  {
    const char *init = getenv(ENV_INIT);
    if (init && *init) {
      write(STDOUT_FILENO, init, strlen(init));
    }
  }

  int output = open(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (output == -1) return SKIP_STATUS;

  while (1) {
    char byte;
    ssize_t count = read(STDIN_FILENO, &byte, 1);
    if (count <= 0) break;
    write(output, &byte, 1);
    fsync(output);
  }

  close(output);
  return PROG_EXIT_SUCCESS;
}

typedef struct {
  const char *name;
  const char *init;          /* bytes the child emits first (NULL for none) */
  const unsigned char *input;
  size_t inputLength;
  const unsigned char *expected;
  size_t expectedLength;
} Scenario;

#define BYTES(...) (const unsigned char []){__VA_ARGS__}, sizeof((const unsigned char []){__VA_ARGS__})

static const Scenario scenarios[] = {
  { .name = "plain text passes through",
    .input = BYTES('h','i'), .expected = BYTES('h','i') },

  { .name = "UTF-8 passes through",                       /* e-acute */
    .input = BYTES(0xC3,0xA9), .expected = BYTES(0xC3,0xA9) },

  { .name = "Alt+a stays ESC a",
    .input = BYTES(0x1B,'a'), .expected = BYTES(0x1B,'a') },

  { .name = "Home (SS3 ESC O H) -> ESC [ 1 ~",
    .input = BYTES(0x1B,'O','H'), .expected = BYTES(0x1B,'[','1','~') },

  { .name = "End (SS3 ESC O F) -> ESC [ 4 ~",
    .input = BYTES(0x1B,'O','F'), .expected = BYTES(0x1B,'[','4','~') },

  { .name = "Home (tilde) stays ESC [ 1 ~",
    .input = BYTES(0x1B,'[','1','~'), .expected = BYTES(0x1B,'[','1','~') },

  { .name = "Up arrow, normal mode -> ESC [ A",
    .input = BYTES(0x1B,'[','A'), .expected = BYTES(0x1B,'[','A') },

  { .name = "Up arrow, application mode -> ESC O A",
    .init = "\x1B[?1h",
    .input = BYTES(0x1B,'[','A'), .expected = BYTES(0x1B,'O','A') },

  { .name = "Ctrl+Left passes through ESC [ 1 ; 5 D",
    .input = BYTES(0x1B,'[','1',';','5','D'), .expected = BYTES(0x1B,'[','1',';','5','D') },

  { .name = "lone ESC becomes Escape after timeout",
    .input = BYTES(0x1B), .expected = BYTES(0x1B) },

  /* Device queries: the child (init) emits the query to the emulator, which
   * must reply on the child's input. No host input is sent. */
  { .name = "primary device attributes (CSI c) answered",
    .init = "\x1B[c",
    .expected = BYTES(0x1B,'[','?','1',';','2','c') },

  { .name = "cursor position report (CSI 6n) answered",
    .init = "\x1B[6n",
    .expected = BYTES(0x1B,'[','1',';','1','R') },
};

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
describeBytes (const char *label, const unsigned char *bytes, size_t length) {
  fprintf(stderr, "    %s:", label);
  for (size_t i=0; i<length; i+=1) fprintf(stderr, " %02X", bytes[i]);
  fprintf(stderr, "\n");
}

static int
runScenario (const Scenario *scenario, const char *selfPath) {
  fprintf(stderr, "\n== input scenario: %s ==\n", scenario->name);

  char outputPath[0X100];
  snprintf(outputPath, sizeof(outputPath), "/tmp/ptyinput.%d.bin", (int)getpid());
  unlink(outputPath);

  int master = posix_openpt(O_RDWR | O_NOCTTY);
  if (master == -1) return SKIP_STATUS;
  if ((grantpt(master) == -1) || (unlockpt(master) == -1)) { close(master); return SKIP_STATUS; }

  char slavePath[0X100];
  {
    const char *name = ptsname(master);
    if (!name) { close(master); return SKIP_STATUS; }
    snprintf(slavePath, sizeof(slavePath), "%s", name);
  }

  /* Keep a sized slave open: on macOS a pty's size is only retained while a
   * slave fd is open. */
  int sizedSlave = open(slavePath, O_RDWR);
  {
    struct winsize size = { .ws_row = 24, .ws_col = 80 };
    ioctl((sizedSlave != -1)? sizedSlave: master, TIOCSWINSZ, &size);
  }

  /* brltty-pty prints its inner pty path on stderr (--show-path); capture it so
   * we can remove the published shared-memory segment if brltty-pty is killed
   * before it can clean up itself. */
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
    setenv(ENV_OUTPUT, outputPath, 1);
    if (scenario->init) setenv(ENV_INIT, scenario->init, 1);
    else unsetenv(ENV_INIT);

    execl("./brltty-pty", "brltty-pty", "--show-path", selfPath, (char *)NULL);
    _exit(SKIP_STATUS);
  }

  close(errorPipe[1]);
  fcntl(master, F_SETFL, O_NONBLOCK);
  fcntl(errorPipe[0], F_SETFL, O_NONBLOCK);

  int result = SKIP_STATUS;
  char ptyPath[0X100] = {0};
  char errorText[0X400];
  size_t errorLength = 0;
  const long deadline = nowMilliseconds() + 8000;

  /* Wait for the child reader to create (and, with an init, write to) its
   * output file, which means brltty-pty is up and the child is running. */
  while (nowMilliseconds() < deadline) {
    drain(master);

    if (!*ptyPath) {
      ssize_t count = read(errorPipe[0], errorText + errorLength,
                           sizeof(errorText) - errorLength - 1);
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

    if (access(outputPath, F_OK) == 0) break;
    usleep(50000);
  }

  if (access(outputPath, F_OK) != 0) {
    fprintf(stderr, "  SKIP: brltty-pty did not start (likely sandboxed)\n");
    goto cleanup;
  }

  /* Let any initialization sequence reach and be processed by the emulator. */
  if (scenario->init) {
    for (int i=0; i<6; i+=1) { drain(master); usleep(50000); }
  }

  /* Send the key sequence as a host terminal would. */
  {
    size_t offset = 0;
    while (offset < scenario->inputLength) {
      ssize_t written = write(master, scenario->input + offset, scenario->inputLength - offset);
      if (written > 0) offset += written;
      else if ((errno == EAGAIN) || (errno == EINTR)) { drain(master); usleep(10000); }
      else break;
    }
  }

  /* Collect what the child received. Wait long enough to cover the lone-ESC
   * timeout (100ms) plus scheduling slack. */
  unsigned char got[0X40];
  size_t gotLength = 0;
  const long settle = nowMilliseconds() + 1500;
  while (nowMilliseconds() < settle) {
    drain(master);
    int fd = open(outputPath, O_RDONLY);
    if (fd != -1) {
      gotLength = 0;
      ssize_t n;
      while ((gotLength < sizeof(got)) &&
             ((n = read(fd, got + gotLength, sizeof(got) - gotLength)) > 0)) {
        gotLength += n;
      }
      close(fd);
    }
    if (gotLength >= scenario->expectedLength) break;
    usleep(30000);
  }

  if ((gotLength == scenario->expectedLength) &&
      (memcmp(got, scenario->expected, gotLength) == 0)) {
    fprintf(stderr, "  ok\n");
    result = PROG_EXIT_SUCCESS;
  } else {
    fprintf(stderr, "  FAIL\n");
    describeBytes("sent    ", scenario->input, scenario->inputLength);
    describeBytes("expected", scenario->expected, scenario->expectedLength);
    describeBytes("got     ", got, gotLength);
    result = PROG_EXIT_FATAL;
  }

cleanup:
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

  /* Remove the shared-memory segment in case brltty-pty was killed before it
   * could clean up itself, so repeated runs never attach to a stale segment. */
  if (*ptyPath) {
    key_t key = ftok(ptyPath, 't');
    if (key != -1) {
      int identifier = shmget(key, 0, 0);
      if (identifier != -1) shmctl(identifier, IPC_RMID, NULL);
    }
  }

  unlink(outputPath);
  if (sizedSlave != -1) close(sizedSlave);
  close(master);
  close(errorPipe[0]);
  return result;
}

int
main (int argc, char *argv[]) {
  /* When brltty-pty runs us as its child, act as the raw-input reader. */
  if (getenv(ENV_OUTPUT)) return runReader();

  /* Resolve our own path before PROCESS_COMMAND_LINE, which rewrites argv. */
  char selfPath[PATH_MAX];
  if (!realpath(argv[0], selfPath)) {
    snprintf(selfPath, sizeof(selfPath), "%s", argv[0]);
  }

  PROCESS_COMMAND_LINE(programDescriptor, argc, argv);

  unsigned int failed = 0;
  unsigned int skipped = 0;

  for (unsigned int i=0; i<ARRAY_COUNT(scenarios); i+=1) {
    int result = runScenario(&scenarios[i], selfPath);
    if (result == PROG_EXIT_FATAL) failed += 1;
    else if (result == SKIP_STATUS) skipped += 1;
  }

  fprintf(stderr, "\n%u input scenario(s): %u failed, %u skipped\n",
          (unsigned int)ARRAY_COUNT(scenarios), failed, skipped);

  if (failed) return PROG_EXIT_FATAL;
  if (skipped == ARRAY_COUNT(scenarios)) return SKIP_STATUS;
  return PROG_EXIT_SUCCESS;
}
