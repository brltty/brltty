/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <hurd/console.h>

#include "misc.h"
#include "brldefs.h"
#include "charset.h"

#include "scr_driver.h"
#include "screen.h"

static char *
vtPath (const char *base, unsigned char vt) {
  size_t length = strlen(base);
  char buffer[length+1];
  snprintf(buffer, length, base, vt);
  buffer[length] = 0;
  return strdup(buffer);
}

static int
currentVt(void) {
  char link[]=HURD_VCSDIR "/00";
  char *c,*d;
  int size,ret;
  if ((size = readlink(HURD_CURVCSPATH,link,sizeof(link))) == -1) {
    /* console may not be started yet or in X mode, don't flood */
    if (errno != ENOENT && errno != ENOTDIR && errno != ENODEV)
      LogError("reading " HURD_CURVCSPATH " link");
    return 1;
  }
  link[size]='\0';
  if (!(c = strrchr(link,'/')))
    c = link;
  else
    c++;
  if (!*c)
    /* bad number */
    return 1;
  ret = strtol(c,&d,10);
  if (*d)
    /* bad number */
    return 1;
  return ret;
}

static int
openDevice (const char *path, const char *description, int flags) {
  int file;
  LogPrint(LOG_DEBUG, "Opening %s device: %s", description, path);
  if ((file = open(path, flags)) == -1) {
    LogPrint(LOG_ERR, "Cannot open %s device: %s: %s",
             description, path, strerror(errno));
  }
  return file;
}

static const char *const consolePath = HURD_INPUTPATH;
static int consoleDescriptor;

static void
closeConsole (void) {
  if (consoleDescriptor != -1) {
    if (close(consoleDescriptor) == -1) {
      LogError("Console close");
    }
    LogPrint(LOG_DEBUG, "Console closed: fd=%d", consoleDescriptor);
    consoleDescriptor = -1;
  }
}

static int
openConsole (unsigned char vt) {
  char *path = vtPath(consolePath, vt?vt:currentVt());
  if (path) {
    int console = openDevice(path, "console", O_RDWR|O_NOCTTY);
    if (console != -1) {
      closeConsole();
      consoleDescriptor = console;
      LogPrint(LOG_DEBUG, "Console opened: %s: fd=%d", path, consoleDescriptor);
      free(path);
      return 1;
    }
    LogError("Console open");
    free(path);
  }
  return 0;
}

static const char *const screenPath = HURD_DISPLAYPATH;
static int screenDescriptor;
static const struct cons_display *screenMap;
static size_t screenMapSize;
#define screenDisplay ((conchar_t *)((wchar_t *) screenMap + screenMap->screen.matrix))
static unsigned char virtualTerminal; /* currently shown (0 means system's) */
static unsigned char lastReadVt; /* last shown */

static void
closeScreen (void) {
  if (screenDescriptor != -1) {
    if (munmap((void *) screenMap, screenMapSize) == -1) {
      LogError("Screen unmap");
    }
    if (close(screenDescriptor) == -1) {
      LogError("Screen close");
    }
    LogPrint(LOG_DEBUG, "Screen closed: fd=%d mmap=%p", screenDescriptor, screenMap);
    screenDescriptor = -1;
    screenMap = NULL;
  }
}

static int
openScreen (unsigned char vt) {
  char *path = vtPath(screenPath, vt?vt:currentVt());
  if (path) {
    int screen = openDevice(path, "screen", O_RDONLY);
    if (screen != -1) {
      struct stat st;
      if (fstat(screen, &st) != -1) {
        const struct cons_display *map = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, screen, 0);
        if (map != MAP_FAILED) {
          if (map->magic == CONS_MAGIC && map->version >> CONS_VERSION_MAJ_SHIFT ==0 && openConsole(vt)) {
            closeScreen();
            screenDescriptor = screen;
            screenMap = map;
            screenMapSize = st.st_size;
            LogPrint(LOG_DEBUG, "Screen opened: %s: fd=%d map=%p", path, screenDescriptor, screenMap);
            free(path);
            return 1;
          }
          munmap((void *) map, st.st_size);
        } else LogError("Screen map");
      } else LogError("Getting size of Screen");
      close(screen);
    }
    free(path);
  }
  return 0;
}

static int
processParameters_HurdScreen (char **parameters) {
  return 1;
}

static int
construct_HurdScreen (void) {
  screenDescriptor = -1;
  consoleDescriptor = -1;
  return openScreen(0);
}

static void
destruct_HurdScreen (void) {
  closeConsole();
  closeScreen();
}

static void
getScreenDescription (ScreenDescription *description) {
  description->rows = screenMap->screen.height;
  description->cols = screenMap->screen.width;
  description->posx = screenMap->cursor.col;
  description->posy = screenMap->cursor.row;
}

static void
getConsoleDescription (ScreenDescription *description) {
  description->number = virtualTerminal ? virtualTerminal : currentVt();
}

static void
describe_HurdScreen (ScreenDescription *description) {
  getScreenDescription(description);
  getConsoleDescription(description);
}

#ifndef offsetof
#define offsetof(type,field) ((size_t) &((type *) 0)->field)
#endif
static int
readCharacters_HurdScreen (const ScreenBox *box, ScreenCharacter *buffer) {
  uint32_t lines, start, row, col;
  ScreenDescription description;
  describe_HurdScreen(&description);
  if (lastReadVt != description.number) {
    openScreen(description.number);
    lastReadVt = description.number;
  }
  if (!validateScreenBox(box, description.cols, description.rows)) return 0;

  lines = screenMap->screen.lines;
  start = screenMap->screen.cur_line;
  for (row=start+box->top; row<start+box->top+box->height; ++row)
    for (col=box->left; col<box->left+box->width; ++col) {
      conchar_attr_t attr;
      buffer->text = screenDisplay[(row%lines)*description.cols+col].chr;
      attr = screenDisplay[(row%lines)*description.cols+col].attr;
      buffer->attributes = attr.fgcol | (attr.bgcol << 4)
	| (attr.intensity == CONS_ATTR_INTENSITY_BOLD?SCR_ATTR_FG_BRIGHT:0)
	| (attr.blinking?SCR_ATTR_BLINK:0);
      buffer++;
    }
  return 1;
}

static int
insertByte (unsigned char byte) {
  if (write(consoleDescriptor, &byte, 1) != -1)
    return 1;
  LogError("Console write");
  return 0;
}

static int
insertMapped (ScreenKey key, int (*insertCharacter)(wchar_t character)) {
  wchar_t buffer[2];
  wchar_t *sequence;
  wchar_t *end;

  setKeyModifiers(&key, 0);

  if (isSpecialKey(key)) {
    switch (key & SCR_KEY_CHAR_MASK) {
      case SCR_KEY_ENTER:
        sequence = WS_C("\r");
        break;
      case SCR_KEY_TAB:
        sequence = WS_C("\t");
        break;
      case SCR_KEY_BACKSPACE:
        sequence = WS_C("\x7f");
        break;
      case SCR_KEY_ESCAPE:
        sequence = WS_C("\x1b");
        break;
      case SCR_KEY_CURSOR_LEFT:
        sequence = WS_C("\x1b[D");
        break;
      case SCR_KEY_CURSOR_RIGHT:
        sequence = WS_C("\x1b[C");
        break;
      case SCR_KEY_CURSOR_UP:
        sequence = WS_C("\x1b[A");
        break;
      case SCR_KEY_CURSOR_DOWN:
        sequence = WS_C("\x1b[B");
        break;
      case SCR_KEY_PAGE_UP:
        sequence = WS_C("\x1b[5~");
        break;
      case SCR_KEY_PAGE_DOWN:
        sequence = WS_C("\x1b[6~");
        break;
      case SCR_KEY_HOME:
        sequence = WS_C("\x1b[1~");
        break;
      case SCR_KEY_END:
        sequence = WS_C("\x1b[4~");
        break;
      case SCR_KEY_INSERT:
        sequence = WS_C("\x1b[2~");
        break;
      case SCR_KEY_DELETE:
        sequence = WS_C("\x1b[3~");
        break;
      case SCR_KEY_FUNCTION + 0:
        sequence = WS_C("\x1bOP");
        break;
      case SCR_KEY_FUNCTION + 1:
        sequence = WS_C("\x1bOQ");
        break;
      case SCR_KEY_FUNCTION + 2:
        sequence = WS_C("\x1bOR");
        break;
      case SCR_KEY_FUNCTION + 3:
        sequence = WS_C("\x1bOS");
        break;
      case SCR_KEY_FUNCTION + 4:
        sequence = WS_C("\x1b[15~");
        break;
      case SCR_KEY_FUNCTION + 5:
        sequence = WS_C("\x1b[17~");
        break;
      case SCR_KEY_FUNCTION + 6:
        sequence = WS_C("\x1b[18~");
        break;
      case SCR_KEY_FUNCTION + 7:
        sequence = WS_C("\x1b[19~");
        break;
      case SCR_KEY_FUNCTION + 8:
        sequence = WS_C("\x1b[20~");
        break;
      case SCR_KEY_FUNCTION + 9:
        sequence = WS_C("\x1b[21~");
        break;
      case SCR_KEY_FUNCTION + 10:
        sequence = WS_C("\x1b[23~");
        break;
      case SCR_KEY_FUNCTION + 11:
        sequence = WS_C("\x1b[24~");
        break;
      case SCR_KEY_FUNCTION + 12:
        sequence = WS_C("\x1b[25~");
        break;
      case SCR_KEY_FUNCTION + 13:
        sequence = WS_C("\x1b[26~");
        break;
      case SCR_KEY_FUNCTION + 14:
        sequence = WS_C("\x1b[28~");
        break;
      case SCR_KEY_FUNCTION + 15:
        sequence = WS_C("\x1b[29~");
        break;
      case SCR_KEY_FUNCTION + 16:
        sequence = WS_C("\x1b[31~");
        break;
      case SCR_KEY_FUNCTION + 17:
        sequence = WS_C("\x1b[32~");
        break;
      case SCR_KEY_FUNCTION + 18:
        sequence = WS_C("\x1b[33~");
        break;
      case SCR_KEY_FUNCTION + 19:
        sequence = WS_C("\x1b[34~");
        break;
      default:
        LogPrint(LOG_WARNING, "Key %04X not suported in ANSI mode.", key);
        return 0;
    }
    end = sequence + wcslen(sequence);
  } else {
    sequence = end = buffer + ARRAY_COUNT(buffer);
    *--sequence = key & SCR_KEY_CHAR_MASK;

    if (key & SCR_KEY_ALT_LEFT)
      *--sequence = 0X1B;
  }

  while (sequence != end)
    if (!insertCharacter(*sequence++))
      return 0;
  return 1;
}

static int
insertUtf8 (wchar_t character) {
  Utf8Buffer utf8;
  size_t utfs = convertWcharToUtf8(character, utf8);
  int i;
  for (i=0; i<utfs; ++i)
    if (!insertByte(utf8[i]))
      return 0;
  return 1;
}

static int
insertKey_HurdScreen (ScreenKey key) {
  LogPrint(LOG_DEBUG, "Insert key: %4.4X", key);
  return insertMapped(key, insertUtf8); 
}

static int
validateVt (int vt) {
  if ((vt >= 1) && (vt <= 99)) return 1;
  LogPrint(LOG_DEBUG, "Virtual terminal %d is out of range.", vt);
  return 0;
}

static int
selectVirtualTerminal_HurdScreen (int vt) {
  if (vt == virtualTerminal) return 1;
  if (vt && !validateVt(vt)) return 0;
  return openScreen(vt);
}

static int
switchVirtualTerminal_HurdScreen (int vt) {
  if (validateVt(vt)) {
    char link[]=HURD_VCSDIR "/00";
    snprintf(link, sizeof(link), HURD_VCSDIR "/%u", vt);
    if (symlink(link, HURD_CURVCSPATH) != -1) {
      LogPrint(LOG_DEBUG, "Switched to virtual terminal %d.", vt);
      return 1;
    } else {
      LogError("symlinking to switch vt");
    }
  }
  return 0;
}

static int
currentVirtualTerminal_HurdScreen (void) {
  ScreenDescription description;
  getConsoleDescription(&description);
  return description.number;
}

static int
executeCommand_HurdScreen (int command) {
  int blk UNUSED = command & BRL_MSK_BLK;
  int arg UNUSED = command & BRL_MSK_ARG;

  return 0;
}

static void
scr_initialize (MainScreen *main) {
  initializeRealScreen(main);
  main->base.describe = describe_HurdScreen;
  main->base.readCharacters = readCharacters_HurdScreen;
  main->base.insertKey = insertKey_HurdScreen;
  main->base.selectVirtualTerminal = selectVirtualTerminal_HurdScreen;
  main->base.switchVirtualTerminal = switchVirtualTerminal_HurdScreen;
  main->base.currentVirtualTerminal = currentVirtualTerminal_HurdScreen;
  main->base.executeCommand = executeCommand_HurdScreen;
  main->processParameters = processParameters_HurdScreen;
  main->construct = construct_HurdScreen;
  main->destruct = destruct_HurdScreen;
}
