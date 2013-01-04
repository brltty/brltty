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
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef HAVE_SHMGET
#include <sys/ipc.h>
#include <sys/shm.h>
static key_t shmKey;
static int shmIdentifier;
#endif /* HAVE_SHMGET */

#ifdef HAVE_SHM_OPEN
#include <sys/mman.h>
static const char *shmPath = "/screen";
static int shmFileDescriptor = -1;
#endif /* HAVE_SHM_OPEN */

#include "log.h"
#include "system.h"
#include "charset.h"

#include "scr_driver.h"
#include "screen.h"

static unsigned char *shmAddress = NULL;
static const mode_t shmMode = S_IRWXU;
static const int shmSize = 4 + ((66 * 132) * 2);

static int
construct_ScreenScreen (void) {
#ifdef HAVE_SHMGET
  {
    key_t keys[2];
    int keyCount = 0;

    /* The original, static key. */
    keys[keyCount++] = 0xBACD072F;

    /* The new, dynamically generated, per user key. */
    {
      int project = 'b';
      const char *path = getenv("HOME");
      if (!path || !*path) path = "/";
      logMessage(LOG_DEBUG, "Shared memory file system object: %s", path);
      if ((keys[keyCount] = ftok(path, project)) != -1) {
        keyCount++;
      } else {
        logMessage(LOG_WARNING, "Per user shared memory key not generated: %s",
                   strerror(errno));
      }
    }

    while (keyCount > 0) {
      shmKey = keys[--keyCount];
      logMessage(LOG_DEBUG, "Trying shared memory key: 0X%" PRIX_KEY_T, shmKey);
      if ((shmIdentifier = shmget(shmKey, shmSize, shmMode)) != -1) {
        if ((shmAddress = shmat(shmIdentifier, NULL, 0)) != (unsigned char *)-1) {
          logMessage(LOG_INFO, "Screen image shared memory key: 0X%" PRIX_KEY_T, shmKey);
          return 1;
        } else {
          logMessage(LOG_WARNING, "Cannot attach shared memory segment 0X%" PRIX_KEY_T ": %s",
                     shmKey, strerror(errno));
        }
      } else {
        logMessage(LOG_WARNING, "Cannot access shared memory segment 0X%" PRIX_KEY_T ": %s",
                   shmKey, strerror(errno));
      }
    }
    shmIdentifier = -1;
  }
#endif /* HAVE_SHMGET */

#ifdef HAVE_SHM_OPEN
  {
    if ((shmFileDescriptor = shm_open(shmPath, O_RDONLY, shmMode)) != -1) {
      if ((shmAddress = mmap(0, shmSize, PROT_READ, MAP_SHARED, shmFileDescriptor, 0)) != MAP_FAILED) {
        return 1;
      } else {
        logSystemError("mmap");
      }

      close(shmFileDescriptor);
      shmFileDescriptor = -1;
    } else {
      logSystemError("shm_open");
    }
  }
#endif /* HAVE_SHM_OPEN */

  return 0;
}

static const unsigned char *
getAuxiliaryData (void) {
  return &shmAddress[4 + (shmAddress[0] * shmAddress[1] * 2)];
}

static int
currentVirtualTerminal_ScreenScreen (void) {
  return getAuxiliaryData()[0];
}

static int
doScreenCommand (const char *command, ...) {
  va_list args;
  int count = 0;

  va_start(args, command);
  while (va_arg(args, char *)) ++count;
  va_end(args);

  {
    char window[0X10];
    const char *argv[count + 6];
    const char **arg = argv;

    *arg++ = "screen";

    *arg++ = "-p";
    snprintf(window, sizeof(window), "%d", currentVirtualTerminal_ScreenScreen());
    *arg++ = window;

    *arg++ = "-X";
    *arg++ = command;

    va_start(args, command);
    while ((*arg++ = va_arg(args, char *)));
    va_end(args);

    {
      int result = executeHostCommand(argv);
      if (result == 0) return 1;
      logMessage(LOG_ERR, "screen error: %d", result);
    }
  }

  return 0;
}

static int
userVirtualTerminal_ScreenScreen (int number) {
  return 1 + number;
}

static void
describe_ScreenScreen (ScreenDescription *description) {
  description->cols = shmAddress[0];
  description->rows = shmAddress[1];
  description->posx = shmAddress[2];
  description->posy = shmAddress[3];
  description->number = currentVirtualTerminal_ScreenScreen();
}

static int
readCharacters_ScreenScreen (const ScreenBox *box, ScreenCharacter *buffer) {
  ScreenDescription description;                 /* screen statistics */
  describe_ScreenScreen(&description);
  if (validateScreenBox(box, description.cols, description.rows)) {
    ScreenCharacter *character = buffer;
    unsigned char *text = shmAddress + 4 + (box->top * description.cols) + box->left;
    unsigned char *attributes = text + (description.cols * description.rows);
    size_t increment = description.cols - box->width;
    int row;
    for (row=0; row<box->height; row++) {
      int column;
      for (column=0; column<box->width; column++) {
        wint_t wc = convertCharToWchar(*text++);
        if (wc == WEOF) wc = L'?';
        character->text = wc;
        character->attributes = *attributes++;
        character++;
      }
      text += increment;
      attributes += increment;
    }
    return 1;
  }
  return 0;
}

static int
insertKey_ScreenScreen (ScreenKey key) {
  const unsigned char flags = getAuxiliaryData()[1];
  wchar_t character = key & SCR_KEY_CHAR_MASK;
  char buffer[3];
  char *sequence;

  logMessage(LOG_DEBUG, "insert key: %04X", key);
  setKeyModifiers(&key, 0);

  if (isSpecialKey(key)) {
#define KEY(key,string) case (key): sequence = (string); break
#define CURSOR_KEY(key,string1,string2) KEY((key), ((flags & 0X01)? (string1): (string2)))
    switch (character) {
      KEY(SCR_KEY_ENTER, "\r");
      KEY(SCR_KEY_TAB, "\t");
      KEY(SCR_KEY_BACKSPACE, "\x7f");
      KEY(SCR_KEY_ESCAPE, "\x1b");

      CURSOR_KEY(SCR_KEY_CURSOR_LEFT , "\x1bOD", "\x1b[D");
      CURSOR_KEY(SCR_KEY_CURSOR_RIGHT, "\x1bOC", "\x1b[C");
      CURSOR_KEY(SCR_KEY_CURSOR_UP   , "\x1bOA", "\x1b[A");
      CURSOR_KEY(SCR_KEY_CURSOR_DOWN , "\x1bOB", "\x1b[B");

      KEY(SCR_KEY_PAGE_UP, "\x1b[5~");
      KEY(SCR_KEY_PAGE_DOWN, "\x1b[6~");
      KEY(SCR_KEY_HOME, "\x1b[1~");
      KEY(SCR_KEY_END, "\x1b[4~");
      KEY(SCR_KEY_INSERT, "\x1b[2~");
      KEY(SCR_KEY_DELETE, "\x1b[3~");
      KEY(SCR_KEY_FUNCTION+0, "\x1bOP");
      KEY(SCR_KEY_FUNCTION+1, "\x1bOQ");
      KEY(SCR_KEY_FUNCTION+2, "\x1bOR");
      KEY(SCR_KEY_FUNCTION+3, "\x1bOS");
      KEY(SCR_KEY_FUNCTION+4, "\x1b[15~");
      KEY(SCR_KEY_FUNCTION+5, "\x1b[17~");
      KEY(SCR_KEY_FUNCTION+6, "\x1b[18~");
      KEY(SCR_KEY_FUNCTION+7, "\x1b[19~");
      KEY(SCR_KEY_FUNCTION+8, "\x1b[20~");
      KEY(SCR_KEY_FUNCTION+9, "\x1b[21~");
      KEY(SCR_KEY_FUNCTION+10, "\x1b[23~");
      KEY(SCR_KEY_FUNCTION+11, "\x1b[24~");
      KEY(SCR_KEY_FUNCTION+12, "\x1b[25~");
      KEY(SCR_KEY_FUNCTION+13, "\x1b[26~");
      KEY(SCR_KEY_FUNCTION+14, "\x1b[28~");
      KEY(SCR_KEY_FUNCTION+15, "\x1b[29~");
      KEY(SCR_KEY_FUNCTION+16, "\x1b[31~");
      KEY(SCR_KEY_FUNCTION+17, "\x1b[32~");
      KEY(SCR_KEY_FUNCTION+18, "\x1b[33~");
      KEY(SCR_KEY_FUNCTION+19, "\x1b[34~");

      default:
        logMessage(LOG_WARNING, "unsuported key: %04X", key);
        return 0;
    }
#undef CURSOR_KEY
#undef KEY
  } else {
    int byte = convertWcharToChar(character);

    if (byte == EOF) {
      logMessage(LOG_WARNING, "character not supported in local character set: 0X%04X", key);
    }

    sequence = buffer + sizeof(buffer);
    *--sequence = 0;
    *--sequence = byte;
    if (key & SCR_KEY_ALT_LEFT) *--sequence = 0X1B;
  }

  return doScreenCommand("stuff", sequence, NULL);
}

static void
destruct_ScreenScreen (void) {
#ifdef HAVE_SHMGET
  if (shmIdentifier != -1) {
    shmdt(shmAddress);
  }
#endif /* HAVE_SHMGET */

#ifdef HAVE_SHM_OPEN
  if (shmFileDescriptor != -1) {
    munmap(shmAddress, shmSize);
    close(shmFileDescriptor);
    shmFileDescriptor = -1;
  }
#endif /* HAVE_SHM_OPEN */

  shmAddress = NULL;
}

static void
scr_initialize (MainScreen *main) {
  initializeRealScreen(main);
  main->base.currentVirtualTerminal = currentVirtualTerminal_ScreenScreen;
  main->base.describe = describe_ScreenScreen;
  main->base.readCharacters = readCharacters_ScreenScreen;
  main->base.insertKey = insertKey_ScreenScreen;
  main->construct = construct_ScreenScreen;
  main->destruct = destruct_ScreenScreen;
  main->userVirtualTerminal = userVirtualTerminal_ScreenScreen;
}
