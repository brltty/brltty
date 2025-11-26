/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2025 by The BRLTTY Developers.
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

#include <string.h>
#include <fcntl.h>

#include "log.h"
#include "pty_object.h"
#include "scr_types.h"
#include "get_term.h"

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

int
ptyWriteInputCharacter (PtyObject *pty, wchar_t character, int kxMode) {
  if (!isSpecialKey(character)) {
    char buffer[MB_CUR_MAX];
    int count = wctomb(buffer, character);
    if (count == -1) return 0;
    return ptyWriteInputData(pty, buffer, count);
  }

  const char *capability = NULL;
  const char *sequence = NULL;
  char buffer[0X20];

  #define KEY(key, cap) case SCR_KEY_##key: capability = #cap; break;
  switch (character) {
      KEY(ENTER       , kent)
      KEY(BACKSPACE   , kbs)

      KEY(CURSOR_LEFT , kcub1)
      KEY(CURSOR_RIGHT, kcuf1)
      KEY(CURSOR_UP   , kcuu1)
      KEY(CURSOR_DOWN , kcud1)

      KEY(PAGE_UP     , kpp)
      KEY(PAGE_DOWN   , knp)
      KEY(HOME        , khome)
      KEY(END         , kend)
      KEY(INSERT      , kich1)
      KEY(DELETE      , kdch1)

      KEY(F1          , kf1)
      KEY(F2          , kf2)
      KEY(F3          , kf3)
      KEY(F4          , kf4)
      KEY(F5          , kf5)
      KEY(F6          , kf6)
      KEY(F7          , kf7)
      KEY(F8          , kf8)
      KEY(F9          , kf9)
      KEY(F10         , kf10)
      KEY(F11         , kf11)
      KEY(F12         , kf12)
      KEY(F13         , kf13)
      KEY(F14         , kf14)
      KEY(F15         , kf15)
      KEY(F16         , kf16)
      KEY(F17         , kf17)
      KEY(F18         , kf18)
      KEY(F19         , kf19)
      KEY(F20         , kf20)
      KEY(F21         , kf21)
      KEY(F22         , kf22)
      KEY(F23         , kf23)
      KEY(F24         , kf24)
      KEY(F25         , kf25)
      KEY(F26         , kf26)
      KEY(F27         , kf27)
      KEY(F28         , kf28)
      KEY(F29         , kf29)
      KEY(F30         , kf30)
      KEY(F31         , kf31)
      KEY(F32         , kf32)
      KEY(F33         , kf33)
      KEY(F34         , kf34)
      KEY(F35         , kf35)
      KEY(F36         , kf36)
      KEY(F37         , kf37)
      KEY(F38         , kf38)
      KEY(F39         , kf39)
      KEY(F40         , kf40)
      KEY(F41         , kf41)
      KEY(F42         , kf42)
      KEY(F43         , kf43)
      KEY(F44         , kf44)
      KEY(F45         , kf45)
      KEY(F46         , kf46)
      KEY(F47         , kf47)
      KEY(F48         , kf48)
      KEY(F49         , kf49)
      KEY(F50         , kf50)
      KEY(F51         , kf51)
      KEY(F52         , kf52)
      KEY(F53         , kf53)
      KEY(F54         , kf54)
      KEY(F55         , kf55)
      KEY(F56         , kf56)
      KEY(F57         , kf57)
      KEY(F58         , kf58)
      KEY(F59         , kf59)
      KEY(F60         , kf60)
      KEY(F61         , kf61)
      KEY(F62         , kf62)
      KEY(F63         , kf63)
  }
  #undef KEY

  if (capability) {
    sequence = tigetstr(capability);
    intptr_t result = (intptr_t)sequence;

    switch (result) {
      case -1:
        logMessage(LOG_WARNING, "not a terminfo string capability: %s", capability);
        goto NO_SEQUENCE;

      case 0:
        logMessage(LOG_WARNING, "unrecognized terminfo capability: %s", capability);
        goto NO_SEQUENCE;

      NO_SEQUENCE:
        sequence = NULL;
      default:
        break;
    }
  }

  if (!sequence) {
    #define KEY(key, seq) case SCR_KEY_##key: sequence = seq; break;
    switch (character) {
      KEY(ENTER       , "\r")
      KEY(TAB         , "\t")
      KEY(BACKSPACE   , "\x7F")
      KEY(ESCAPE      , "\x1B")

      KEY(CURSOR_UP   , "\x1BOA")
      KEY(CURSOR_DOWN , "\x1BOB")
      KEY(CURSOR_RIGHT, "\x1BOC")
      KEY(CURSOR_LEFT , "\x1BOD")

      KEY(HOME        , "\x1B[1~")
      KEY(INSERT      , "\x1B[2~")
      KEY(DELETE      , "\x1B[3~")
      KEY(END         , "\x1B[4~")
      KEY(PAGE_UP     , "\x1B[5~")
      KEY(PAGE_DOWN   , "\x1B[6~")

      KEY(F1          , "\x1BOP")
      KEY(F2          , "\x1BOQ")
      KEY(F3          , "\x1BOR")
      KEY(F4          , "\x1BOS")
      KEY(F5          , "\x1B[15~")
      KEY(F6          , "\x1B[17~")
      KEY(F7          , "\x1B[18~")
      KEY(F8          , "\x1B[19~")
      KEY(F9          , "\x1B[20~")
      KEY(F10         , "\x1B[21~")
      KEY(F11         , "\x1B[23~")
      KEY(F12         , "\x1B[24~")
    }
    #undef KEY
  }

  if (sequence) {
    switch (character) {
      case SCR_KEY_CURSOR_LEFT:
      case SCR_KEY_CURSOR_RIGHT:
      case SCR_KEY_CURSOR_UP:
      case SCR_KEY_CURSOR_DOWN:
        strcpy(buffer, sequence);
        buffer[1] = kxMode? 'O': '[';
        sequence = buffer;
        break;
    }

    if (!ptyWriteInputData(pty, sequence, strlen(sequence))) return 0;
  } else {
    logMessage(LOG_WARNING, "unsupported pty screen key: %04X", character);
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
