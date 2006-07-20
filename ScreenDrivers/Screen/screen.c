/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
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

#include "Programs/misc.h"
#include "Programs/system.h"

#include "Programs/scr_driver.h"
#include "screen.h"

static char *shmAddress = NULL;
static const mode_t shmMode = S_IRWXU;
static const int shmSize = 4 + ((66 * 132) * 2);

static int
doScreenCommand (const char *command, ...) {
  va_list args;
  int count = 0;

  va_start(args, command);
  while (va_arg(args, char *)) ++count;
  va_end(args);

  {
    const char *argv[count + 4];
    const char **arg = argv;

    *arg++ = "screen";
    *arg++ = "-X";
    *arg++ = command;

    va_start(args, command);
    while ((*arg++ = va_arg(args, char *)));
    va_end(args);

    {
      int result = executeHostCommand(argv);
      if (result == 0) return 1;
      LogPrint(LOG_ERR, "screen error: %d", result);
    }
  }

  return 0;
}

static int
open_ScreenScreen (void) {
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
      LogPrint(LOG_DEBUG, "Shared memory file system object: %s", path);
      if ((keys[keyCount] = ftok(path, project)) != -1) {
        keyCount++;
      } else {
        LogPrint(LOG_WARNING, "Per user shared memory key not generated: %s",
                 strerror(errno));
      }
    }

    while (keyCount > 0) {
      shmKey = keys[--keyCount];
      LogPrint(LOG_DEBUG, "Trying shared memory key: 0X%" PRIX_KEY_T, shmKey);
      if ((shmIdentifier = shmget(shmKey, shmSize, shmMode)) != -1) {
        if ((shmAddress = shmat(shmIdentifier, NULL, 0)) != (char *)-1) {
          LogPrint(LOG_INFO, "Screen image shared memory key: 0X%" PRIX_KEY_T, shmKey);
          return 1;
        } else {
          LogPrint(LOG_WARNING, "Cannot attach shared memory segment 0X%" PRIX_KEY_T ": %s",
                   shmKey, strerror(errno));
        }
      } else {
        LogPrint(LOG_WARNING, "Cannot access shared memory segment 0X%" PRIX_KEY_T ": %s",
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
        LogError("mmap");
      }

      close(shmFileDescriptor);
      shmFileDescriptor = -1;
    } else {
      LogError("shm_open");
    }
  }
#endif /* HAVE_SHM_OPEN */

  return 0;
}

static int
currentvt_ScreenScreen (void) {
  return 0;
}

static int
uservt_ScreenScreen (int number) {
  return 1 + number;
}

static void
describe_ScreenScreen (ScreenDescription *description) {
  description->cols = shmAddress[0];
  description->rows = shmAddress[1];
  description->posx = shmAddress[2];
  description->posy = shmAddress[3];
  description->number = currentvt_ScreenScreen();
}

static int
read_ScreenScreen (ScreenBox box, unsigned char *buffer, ScreenMode mode) {
  ScreenDescription description;                 /* screen statistics */
  describe_ScreenScreen(&description);
  if (validateScreenBox(&box, description.cols, description.rows)) {
    off_t start = 4 + (((mode == SCR_TEXT)? 0: 1) * description.cols * description.rows) + (box.top * description.cols) + box.left;
    int row;
    for (row=0; row<box.height; row++) {
      memcpy(buffer + (row * box.width),
             shmAddress + start + (row * description.cols),
             box.width);
    }
    return 1;
  }
  return 0;
}

static int
insert_ScreenScreen (ScreenKey key) {
  char buffer[3];
  char *sequence;

  LogPrint(LOG_DEBUG, "insert key: %4.4X", key);
  if (key < SCR_KEY_ENTER) {
    sequence = buffer + sizeof(buffer);
    *--sequence = 0;
    *--sequence = key & 0XFF;
    if (key & SCR_KEY_MOD_META) *--sequence = 0X1B;
  } else {
#define KEY(key,string) case (key): sequence = (string); break
    switch (key) {
      KEY(SCR_KEY_ENTER, "\r");
      KEY(SCR_KEY_TAB, "\t");
      KEY(SCR_KEY_BACKSPACE, "\x7f");
      KEY(SCR_KEY_ESCAPE, "\x1b");
      KEY(SCR_KEY_CURSOR_LEFT, "\x1b[D");
      KEY(SCR_KEY_CURSOR_RIGHT, "\x1b[C");
      KEY(SCR_KEY_CURSOR_UP, "\x1b[A");
      KEY(SCR_KEY_CURSOR_DOWN, "\x1b[B");
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
        LogPrint(LOG_WARNING, "unsuported key: %4.4X", key);
        return 0;
    }
#undef KEY
  }

  {
    size_t count = strlen(sequence);
    char buffer[(count * 4) + 3];
    char *byte = buffer;
    const char quote = '"';
    const char escape = '\\';

    *byte++ = quote;
    {
      size_t index;
      for (index=0; index<count; ++index) {
        char character = sequence[index];
        if ((character == quote) || (character == escape)) {
          *byte++ = escape;
          *byte++ = character;
        } else if (isascii(character) && isprint(character)) {
          *byte++ = character;
        } else {
          *byte++ = escape;
          sprintf(byte, "%.3o", character&0XFF);
          byte += 3;
        }
      }
    }
    *byte++ = quote;
    *byte++ = 0;

    return doScreenCommand("stuff", buffer, NULL);
  }
}

static void
close_ScreenScreen (void) {
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
  main->base.currentvt = currentvt_ScreenScreen;
  main->base.describe = describe_ScreenScreen;
  main->base.read = read_ScreenScreen;
  main->base.insert = insert_ScreenScreen;
  main->open = open_ScreenScreen;
  main->close = close_ScreenScreen;
  main->uservt = uservt_ScreenScreen;
}
