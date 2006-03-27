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
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <hurd/console.h>

#include "Programs/misc.h"
#include "Programs/brldefs.h"

#include "Programs/scr_driver.h"
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
prepare_HurdScreen (char **parameters) {
  return 1;
}

static int
open_HurdScreen (void) {
  screenDescriptor = -1;
  consoleDescriptor = -1;
  return openScreen(0);
}

static int
setup_HurdScreen (void) {
  return 1;
}

static void
close_HurdScreen (void) {
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
read_HurdScreen (ScreenBox box, unsigned char *buffer, ScreenMode mode) {
  ScreenDescription description;
  describe_HurdScreen(&description);
  if (lastReadVt != description.number) {
    openScreen(description.number);
    lastReadVt = description.number;
  }
  if (validateScreenBox(&box, description.cols, description.rows)) {
    uint32_t lines, start, row, col;
    const int which = mode == SCR_TEXT ? offsetof(conchar_t,chr):offsetof(conchar_t,attr);
    lines = screenMap->screen.lines;
    start = screenMap->screen.cur_line;
    for (row=start+box.top; row<start+box.top+box.height; ++row)
      for (col=box.left; col<box.left+box.width; ++col)
	      /* TODO: correctly translate uint32_t into char */
	*buffer++ = *(uint32_t *)(((unsigned char *) &screenDisplay[(row%lines)*description.cols+col])+which);
    return 1;
  } else {
    LogPrint(LOG_ERR, "Invalid screen area: cols=%d left=%d width=%d rows=%d top=%d height=%d",
             description.cols, box.left, box.width,
             description.rows, box.top, box.height);
  }
  return 0;
}

static int
insertByte (unsigned char byte) {
  if (write(consoleDescriptor, &byte, 1) != -1)
    return 1;
  LogError("Console write");
  return 0;
}

static int
insertMapped (ScreenKey key, int (*byteInserter)(unsigned char byte)) {
  char buffer[2];
  char *sequence;
  char *end;

  if (key < SCR_KEY_ENTER) {
    sequence = end = buffer + sizeof(buffer);
    *--sequence = key & 0XFF;

    if (key & SCR_KEY_MOD_META)
      *--sequence = 0X1B;
  } else {
    switch (key) {
      case SCR_KEY_ENTER:
        sequence = "\r";
        break;
      case SCR_KEY_TAB:
        sequence = "\t";
        break;
      case SCR_KEY_BACKSPACE:
        sequence = "\x7f";
        break;
      case SCR_KEY_ESCAPE:
        sequence = "\x1b";
        break;
      case SCR_KEY_CURSOR_LEFT:
        sequence = "\x1b[D";
        break;
      case SCR_KEY_CURSOR_RIGHT:
        sequence = "\x1b[C";
        break;
      case SCR_KEY_CURSOR_UP:
        sequence = "\x1b[A";
        break;
      case SCR_KEY_CURSOR_DOWN:
        sequence = "\x1b[B";
        break;
      case SCR_KEY_PAGE_UP:
        sequence = "\x1b[5~";
        break;
      case SCR_KEY_PAGE_DOWN:
        sequence = "\x1b[6~";
        break;
      case SCR_KEY_HOME:
        sequence = "\x1b[1~";
        break;
      case SCR_KEY_END:
        sequence = "\x1b[4~";
        break;
      case SCR_KEY_INSERT:
        sequence = "\x1b[2~";
        break;
      case SCR_KEY_DELETE:
        sequence = "\x1b[3~";
        break;
      case SCR_KEY_FUNCTION + 0:
        sequence = "\x1bOP";
        break;
      case SCR_KEY_FUNCTION + 1:
        sequence = "\x1bOQ";
        break;
      case SCR_KEY_FUNCTION + 2:
        sequence = "\x1bOR";
        break;
      case SCR_KEY_FUNCTION + 3:
        sequence = "\x1bOS";
        break;
      case SCR_KEY_FUNCTION + 4:
        sequence = "\x1b[15~";
        break;
      case SCR_KEY_FUNCTION + 5:
        sequence = "\x1b[17~";
        break;
      case SCR_KEY_FUNCTION + 6:
        sequence = "\x1b[18~";
        break;
      case SCR_KEY_FUNCTION + 7:
        sequence = "\x1b[19~";
        break;
      case SCR_KEY_FUNCTION + 8:
        sequence = "\x1b[20~";
        break;
      case SCR_KEY_FUNCTION + 9:
        sequence = "\x1b[21~";
        break;
      case SCR_KEY_FUNCTION + 10:
        sequence = "\x1b[23~";
        break;
      case SCR_KEY_FUNCTION + 11:
        sequence = "\x1b[24~";
        break;
      case SCR_KEY_FUNCTION + 12:
        sequence = "\x1b[25~";
        break;
      case SCR_KEY_FUNCTION + 13:
        sequence = "\x1b[26~";
        break;
      case SCR_KEY_FUNCTION + 14:
        sequence = "\x1b[28~";
        break;
      case SCR_KEY_FUNCTION + 15:
        sequence = "\x1b[29~";
        break;
      case SCR_KEY_FUNCTION + 16:
        sequence = "\x1b[31~";
        break;
      case SCR_KEY_FUNCTION + 17:
        sequence = "\x1b[32~";
        break;
      case SCR_KEY_FUNCTION + 18:
        sequence = "\x1b[33~";
        break;
      case SCR_KEY_FUNCTION + 19:
        sequence = "\x1b[34~";
        break;
      default:
        LogPrint(LOG_WARNING, "Key %4.4X not suported in ANSI mode.", key);
        return 0;
    }
    end = sequence + strlen(sequence);
  }

  while (sequence != end)
    if (!byteInserter(*sequence++))
      return 0;
  return 1;
}

static int
insertUtf8 (unsigned char byte) {
  if (byte & 0X80) {
    if (!insertByte(0XC0 | (byte >> 6))) return 0;
    byte &= 0XBF;
  }
  if (!insertByte(byte)) return 0;
  return 1;
}

static int
insert_HurdScreen (ScreenKey key) {
  LogPrint(LOG_DEBUG, "Insert key: %4.4X", key);
  return insertMapped(key, &insertUtf8); 
}

static int
validateVt (int vt) {
  if ((vt >= 1) && (vt <= 99)) return 1;
  LogPrint(LOG_DEBUG, "Virtual terminal %d is out of range.", vt);
  return 0;
}

static int
selectvt_HurdScreen (int vt) {
  if (vt == virtualTerminal) return 1;
  if (vt && !validateVt(vt)) return 0;
  return openScreen(vt);
}

static int
switchvt_HurdScreen (int vt) {
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
currentvt_HurdScreen (void) {
  ScreenDescription description;
  getConsoleDescription(&description);
  return description.number;
}

static int
execute_HurdScreen (int command) {
  int blk = command & BRL_MSK_BLK;
  int arg
#ifdef HAVE_ATTRIBUTE_UNUSED
      __attribute__((unused))
#endif /* HAVE_ATTRIBUTE_UNUSED */
      = command & BRL_MSK_ARG;

  switch (blk) {
    case BRL_BLK_PASSAT2:
      break;
  }
  return 0;
}

static void
scr_initialize (MainScreen *main) {
  initializeRealScreen(main);
  main->base.describe = describe_HurdScreen;
  main->base.read = read_HurdScreen;
  main->base.insert = insert_HurdScreen;
  main->base.selectvt = selectvt_HurdScreen;
  main->base.switchvt = switchvt_HurdScreen;
  main->base.currentvt = currentvt_HurdScreen;
  main->base.execute = execute_HurdScreen;
  main->prepare = prepare_HurdScreen;
  main->open = open_HurdScreen;
  main->setup = setup_HurdScreen;
  main->close = close_HurdScreen;
}
