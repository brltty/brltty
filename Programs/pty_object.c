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

#include "prologue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "log.h"
#include "pty_object.h"
#include "scr_types.h"

struct PtyObjectStruct {
  char *path;
  int master;

  unsigned char logLevel;
  unsigned char logInput:1;
};

PtyObject *
ptyNewObject (void) {
  PtyObject *pty;

  if ((pty = malloc(sizeof(*pty)))) {
    memset(pty, 0, sizeof(*pty));

    pty->path = NULL;
    pty->master = INVALID_FILE_DESCRIPTOR;

    pty->logLevel = LOG_DEBUG;
    pty->logInput = 0;

    if ((pty->master = posix_openpt(O_RDWR)) != -1) {
      if ((pty->path = ptsname(pty->master))) {
        if ((pty->path = strdup(pty->path))) {
          if (grantpt(pty->master) != -1) {
            if (unlockpt(pty->master) != -1) {
              return pty;
            } else {
              logSystemError("unlockpt");
            }
          } else {
            logSystemError("grantpt");
          }

          free(pty->path);
        } else {
          logMallocError();
        }
      } else {
        logSystemError("ptsname");
      }

      close(pty->master);
    } else {
      logSystemError("posix_openpt");
    }

    free(pty);
  } else {
    logMallocError();
  }

  return NULL;
}

const char *
ptyGetPath (const PtyObject *pty) {
  return pty->path;
}

int
ptyGetMaster (const PtyObject *pty) {
  return pty->master;
}

void
ptySetLogLevel (PtyObject *pty, unsigned char level) {
  pty->logLevel = level;
}

void
ptySetLogInput (PtyObject *pty, int yes) {
  pty->logInput = yes;
}

int
ptyWriteInputData (PtyObject *pty, const void *data, size_t length) {
  if (pty->logInput) {
    logBytes(pty->logLevel, "pty input", data, length);
  }

  if (write(pty->master, data, length) != -1) return 1;
  logSystemError("pty write input");
  return 0;
}

/* All of the ScreenKey modifier flags, so the base key can be isolated without
 * truncating astral-plane code points (which the SCR_KEY_CHAR_MASK would). */
#define PTY_KEY_MODIFIER_MASK ( \
  SCR_KEY_SHIFT | SCR_KEY_UPPER | SCR_KEY_CONTROL | \
  SCR_KEY_ALT_LEFT | SCR_KEY_ALT_RIGHT | SCR_KEY_GUI | SCR_KEY_CAPSLOCK \
)

/* Encode a key for the child and write it to the pty.
 *
 * The byte sequences are taken from tmux's input-keys.c so that they match the
 * terminfo we advertise to the child (the screen/tmux family). We deliberately
 * do NOT consult the host's terminfo: that describes the *outer* terminal and
 * may encode a key differently (famously Home as "ESC O H" rather than the
 * "ESC [ 1 ~" a screen/tmux program expects), which would make the child
 * mis-read the key - e.g. vim treating the "O" as "open line and insert".
 *
 * Modifiers carried in the ScreenKey (Shift/Alt/Control) are emitted using the
 * standard xterm scheme: a parameter of 1 + (shift?1) + (alt?2) + (ctrl?4),
 * which every screen/tmux-aware program understands. kxMode selects the
 * application-cursor-keys form (DECCKM) for the arrow keys.
 */
int
ptyWriteInputCharacter (PtyObject *pty, wchar_t character, int kxMode) {
  unsigned int modifiers = 0;
  if (character & SCR_KEY_SHIFT) modifiers |= 1;
  if (character & (SCR_KEY_ALT_LEFT | SCR_KEY_ALT_RIGHT)) modifiers |= 2;
  if (character & SCR_KEY_CONTROL) modifiers |= 4;

  ScreenKey key = character & ~PTY_KEY_MODIFIER_MASK;
  unsigned int modParam = modifiers? (modifiers + 1): 0;

  if (!isSpecialKey(key)) {
    wchar_t wc = key;

    /* Control folds a typed character down to its C0 control code. */
    if (modifiers & 4) {
      if ((wc >= 'a') && (wc <= 'z')) wc = wc - 'a' + 1;
      else if ((wc >= 'A') && (wc <= 'Z')) wc = wc - 'A' + 1;
      else if ((wc >= '@') && (wc <= '_')) wc = wc - '@';
      else if (wc == ' ') wc = 0;
      else if (wc == '?') wc = 0X7F;
    }

    char bytes[1 + MB_CUR_MAX];
    char *out = bytes;
    if (modifiers & 2) *out++ = 0X1B;   /* Alt/Meta is an ESC prefix. */

    int count = wctomb(out, wc);
    if (count == -1) return 0;
    return ptyWriteInputData(pty, bytes, (out - bytes) + count);
  }

  char buffer[0X20];
  const char *seq = NULL;

  char final = 0;       /* letter-form keys: arrows, Home, End, F1-F4 */
  int isCursor = 0;     /* arrows additionally honor kxMode (DECCKM)   */
  int tilde = 0;        /* "ESC [ <n> ~" form keys                     */

  switch (key) {
    case SCR_KEY_ENTER:     seq = "\r";   break;
    case SCR_KEY_TAB:       seq = "\t";   break;
    case SCR_KEY_BACKSPACE: seq = "\x7F"; break;
    case SCR_KEY_ESCAPE:    seq = "\x1B"; break;

    case SCR_KEY_CURSOR_UP:    final = 'A'; isCursor = 1; break;
    case SCR_KEY_CURSOR_DOWN:  final = 'B'; isCursor = 1; break;
    case SCR_KEY_CURSOR_RIGHT: final = 'C'; isCursor = 1; break;
    case SCR_KEY_CURSOR_LEFT:  final = 'D'; isCursor = 1; break;

    case SCR_KEY_HOME: final = 'H'; tilde = 1; break;  /* unmodified -> ESC[1~ */
    case SCR_KEY_END:  final = 'F'; tilde = 4; break;  /* unmodified -> ESC[4~ */

    case SCR_KEY_INSERT:    tilde = 2; break;
    case SCR_KEY_DELETE:    tilde = 3; break;
    case SCR_KEY_PAGE_UP:   tilde = 5; break;
    case SCR_KEY_PAGE_DOWN: tilde = 6; break;

    case SCR_KEY_F1: final = 'P'; break;
    case SCR_KEY_F2: final = 'Q'; break;
    case SCR_KEY_F3: final = 'R'; break;
    case SCR_KEY_F4: final = 'S'; break;

    default: {
      /* F5..F20 use the numbered "ESC [ <n> ~" form. */
      static const unsigned char fnTilde[] = {
        15, 17, 18, 19, 20, 21, 23, 24, 25, 26, 28, 29, 31, 32, 33, 34
      };

      if ((key >= SCR_KEY_F5) && (key <= SCR_KEY_F20)) {
        tilde = fnTilde[key - SCR_KEY_F5];
      }

      break;
    }
  }

  if (!seq) {
    if (final && !tilde) {
      /* arrows and F1-F4 */
      if (modParam) {
        snprintf(buffer, sizeof(buffer), "\x1B[1;%u%c", modParam, final);
      } else if (isCursor) {
        snprintf(buffer, sizeof(buffer), "\x1B%c%c", (kxMode? 'O': '['), final);
      } else {
        snprintf(buffer, sizeof(buffer), "\x1BO%c", final);
      }

      seq = buffer;
    } else if (tilde) {
      if (modParam && final) {
        /* Home/End, when modified, use the letter form (ESC[1;modH|F). */
        snprintf(buffer, sizeof(buffer), "\x1B[1;%u%c", modParam, final);
      } else if (modParam) {
        snprintf(buffer, sizeof(buffer), "\x1B[%d;%u~", tilde, modParam);
      } else {
        snprintf(buffer, sizeof(buffer), "\x1B[%d~", tilde);
      }

      seq = buffer;
    }
  }

  if (seq) {
    if (!ptyWriteInputData(pty, seq, strlen(seq))) return 0;
  } else {
    logMessage(LOG_WARNING, "unsupported pty screen key: %04X", (unsigned int)key);
  }

  return 1;
}

void
ptyCloseMaster (PtyObject *pty) {
  if (pty->master != INVALID_FILE_DESCRIPTOR) {
    close(pty->master);
    pty->master = INVALID_FILE_DESCRIPTOR;
  }
}

int
ptyOpenSlave (const PtyObject *pty, int *fileDescriptor) {
  int result = open(pty->path, O_RDWR);
  int opened = result != INVALID_FILE_DESCRIPTOR;

  if (opened) {
    *fileDescriptor = result;
  } else {
    logSystemError("pty slave open");
  }

  return opened;
}

void
ptyDestroyObject (PtyObject *pty) {
  ptyCloseMaster(pty);
  free(pty->path);
  free(pty);
}
