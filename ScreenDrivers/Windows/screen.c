/*
 * BRLTTY - A background process providing access to window's console
 *          for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "Programs/misc.h"
#include "Programs/brldefs.h"

#include "Programs/scr_driver.h"

static HANDLE consoleOutput = INVALID_HANDLE_VALUE;

static int
prepare_WindowsScreen (char **parameters) {
  return 1;
}

static int
open_WindowsScreen (void) {
  if (consoleOutput == INVALID_HANDLE_VALUE &&
    (consoleOutput = CreateFile("CONOUT$",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL)) == INVALID_HANDLE_VALUE) {
    LogWindowsError("GetStdHandle");
    return 0;
  }
  return 1;
}

static int
setup_WindowsScreen (void) {
  return 1;
}

static void
close_WindowsScreen (void) {
}

static void
describe_WindowsScreen (ScreenDescription *description) {
  CONSOLE_SCREEN_BUFFER_INFO info;
  if (!(GetConsoleScreenBufferInfo(consoleOutput, &info)))
    return;
  description->cols = info.srWindow.Right + 1 - info.srWindow.Left;
  description->rows = info.srWindow.Bottom + 1 - info.srWindow.Top;
  description->posx = info.dwCursorPosition.X - info.srWindow.Left;
  description->posy = info.dwCursorPosition.Y - info.srWindow.Top; 
  description->no = 0;
}

static unsigned char *
read_WindowsScreen (ScreenBox box, unsigned char *buffer, ScreenMode mode) {
  int x,y;
  int text = mode == SCR_TEXT;
  unsigned long c;
  COORD coord;
  DWORD read;
  void *buf;
  BOOL (*fun) (HANDLE, void*, DWORD, COORD, LPDWORD);
  size_t size;
  CONSOLE_SCREEN_BUFFER_INFO info;

  if (consoleOutput == INVALID_HANDLE_VALUE)
    return NULL;

  if (!(GetConsoleScreenBufferInfo(consoleOutput, &info)))
    return NULL;

  coord.X = box.left + info.srWindow.Left;
  coord.Y = box.top + info.srWindow.Top;

  if (text) {
    fun = (typeof(fun)) ReadConsoleOutputCharacterW;
    size = sizeof(wchar_t);
  } else {
    fun = (typeof(fun)) ReadConsoleOutputAttribute;
    size = sizeof(WORD);
  }

  if (!(buf = malloc(box.width*size)))
    return NULL;

  for (y = 0; y < box.height; y++, coord.Y++) {
    if (!fun(consoleOutput, buf, box.width, coord, &read) || read != box.width) {
      LogWindowsError("ReadConsoleOutput");
      exit(0);
    }
    if (text)
      for (x = 0; x < box.width; x++) {
	c = ((wchar_t *)buf)[x];
	if (c<0x100)
	  buffer[y*box.width+x] = c;
	else
	  buffer[y*box.width+x] = '?';
      }
    else
      for (x = 0; x < box.width; x++)
	buffer[y*box.width+x] = ((WORD *)buf)[x];
  }
  free(buf);
  return buffer;
}

static int
insert_WindowsScreen (ScreenKey key) {
  return 0;
}

static int
selectvt_WindowsScreen (int vt) {
  return 0;
}

static int
switchvt_WindowsScreen (int vt) {
  return 0;
}

static int
currentvt_WindowsScreen (void) {
  ScreenDescription description;
  describe_WindowsScreen(&description);
  return description.no;
}

static int
execute_WindowsScreen (int command) {
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
  main->base.describe = describe_WindowsScreen;
  main->base.read = read_WindowsScreen;
  main->base.insert = insert_WindowsScreen;
  main->base.selectvt = selectvt_WindowsScreen;
  main->base.switchvt = switchvt_WindowsScreen;
  main->base.currentvt = currentvt_WindowsScreen;
  main->base.execute = execute_WindowsScreen;
  main->prepare = prepare_WindowsScreen;
  main->open = open_WindowsScreen;
  main->setup = setup_WindowsScreen;
  main->close = close_WindowsScreen;
}
