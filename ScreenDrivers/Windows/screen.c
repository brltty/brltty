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

#include "prologue.h"

#include "Programs/misc.h"
#include "Programs/brldefs.h"

#include "Programs/scr_driver.h"

static HANDLE consoleOutput = INVALID_HANDLE_VALUE;
static HANDLE consoleInput = INVALID_HANDLE_VALUE;

static int
prepare_WindowsScreen (char **parameters) {
  return 1;
}

static int
open_WindowsScreen (void) {
  if ((consoleOutput == INVALID_HANDLE_VALUE &&
    (consoleOutput = CreateFile("CONOUT$",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL)) == INVALID_HANDLE_VALUE)
    ||(consoleInput == INVALID_HANDLE_VALUE &&
    (consoleInput = CreateFile("CONIN$",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL)) == INVALID_HANDLE_VALUE)) {
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

static int
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
    return 0;

  if (!(GetConsoleScreenBufferInfo(consoleOutput, &info)))
    return 0;

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
    return 0;

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
  return 1;
}

static int 
doinsert(INPUT_RECORD *buf) {
  DWORD num;
  if (!(WriteConsoleInputW(consoleInput, buf, 1, &num))) {
    LogWindowsError("WriteConsoleInput");
    return 0;
  }
  if (num!=1) {
    LogPrint(LOG_ERR, "inserted only %ld keys, expected 1", num);
    return 0;
  }
  return 1;
}

static int
insert_WindowsScreen (ScreenKey key) {
  INPUT_RECORD buf;
  KEY_EVENT_RECORD *keyE = &buf.Event.KeyEvent;

  if (consoleInput == INVALID_HANDLE_VALUE)
    return 0;

  LogPrint(LOG_DEBUG, "Insert key: %4.4X",key);
  buf.EventType = KEY_EVENT;
  memset(keyE, 0, sizeof(*keyE));
  if (key < SCR_KEY_ENTER) {
    if (key & SCR_KEY_MOD_META) {
      keyE->dwControlKeyState |= LEFT_ALT_PRESSED;
      key &= ~ SCR_KEY_MOD_META;
    }
    if (!(key & 0xE0)) {
      keyE->dwControlKeyState |= LEFT_CTRL_PRESSED;
      key |= 0x40;
    }
    keyE->uChar.UnicodeChar = key;
  } else {
    switch (key) {
      case SCR_KEY_ENTER:         keyE->wVirtualKeyCode = VK_RETURN; break;
      case SCR_KEY_TAB:           keyE->wVirtualKeyCode = VK_TAB;    break;
      case SCR_KEY_BACKSPACE:     keyE->wVirtualKeyCode = VK_BACK;   break;
      case SCR_KEY_ESCAPE:        keyE->wVirtualKeyCode = VK_ESCAPE; break;
      case SCR_KEY_CURSOR_LEFT:   keyE->wVirtualKeyCode = VK_LEFT;   break;
      case SCR_KEY_CURSOR_RIGHT:  keyE->wVirtualKeyCode = VK_RIGHT;  break;
      case SCR_KEY_CURSOR_UP:     keyE->wVirtualKeyCode = VK_UP;     break;
      case SCR_KEY_CURSOR_DOWN:   keyE->wVirtualKeyCode = VK_DOWN;   break;
      case SCR_KEY_PAGE_UP:       keyE->wVirtualKeyCode = VK_PRIOR;  break;
      case SCR_KEY_PAGE_DOWN:     keyE->wVirtualKeyCode = VK_NEXT;   break;
      case SCR_KEY_HOME:          keyE->wVirtualKeyCode = VK_HOME;   break;
      case SCR_KEY_END:           keyE->wVirtualKeyCode = VK_END;    break;
      case SCR_KEY_INSERT:        keyE->wVirtualKeyCode = VK_INSERT; break;
      case SCR_KEY_DELETE:        keyE->wVirtualKeyCode = VK_DELETE; break;
      case SCR_KEY_FUNCTION + 0:  keyE->wVirtualKeyCode = VK_F1;     break;
      case SCR_KEY_FUNCTION + 1:  keyE->wVirtualKeyCode = VK_F2;     break;
      case SCR_KEY_FUNCTION + 2:  keyE->wVirtualKeyCode = VK_F3;     break;
      case SCR_KEY_FUNCTION + 3:  keyE->wVirtualKeyCode = VK_F4;     break;
      case SCR_KEY_FUNCTION + 4:  keyE->wVirtualKeyCode = VK_F5;     break;
      case SCR_KEY_FUNCTION + 5:  keyE->wVirtualKeyCode = VK_F6;     break;
      case SCR_KEY_FUNCTION + 6:  keyE->wVirtualKeyCode = VK_F7;     break;
      case SCR_KEY_FUNCTION + 7:  keyE->wVirtualKeyCode = VK_F8;     break;
      case SCR_KEY_FUNCTION + 8:  keyE->wVirtualKeyCode = VK_F9;     break;
      case SCR_KEY_FUNCTION + 9:  keyE->wVirtualKeyCode = VK_F10;    break;
      case SCR_KEY_FUNCTION + 10: keyE->wVirtualKeyCode = VK_F11;    break;
      case SCR_KEY_FUNCTION + 11: keyE->wVirtualKeyCode = VK_F12;    break;
      case SCR_KEY_FUNCTION + 12: keyE->wVirtualKeyCode = VK_F13;    break;
      case SCR_KEY_FUNCTION + 13: keyE->wVirtualKeyCode = VK_F14;    break;
      case SCR_KEY_FUNCTION + 14: keyE->wVirtualKeyCode = VK_F15;    break;
      case SCR_KEY_FUNCTION + 15: keyE->wVirtualKeyCode = VK_F16;    break;
      case SCR_KEY_FUNCTION + 16: keyE->wVirtualKeyCode = VK_F17;    break;
      case SCR_KEY_FUNCTION + 17: keyE->wVirtualKeyCode = VK_F18;    break;
      case SCR_KEY_FUNCTION + 18: keyE->wVirtualKeyCode = VK_F19;    break;
      case SCR_KEY_FUNCTION + 19: keyE->wVirtualKeyCode = VK_F20;    break;
      case SCR_KEY_FUNCTION + 20: keyE->wVirtualKeyCode = VK_F21;    break;
      case SCR_KEY_FUNCTION + 21: keyE->wVirtualKeyCode = VK_F22;    break;
      case SCR_KEY_FUNCTION + 22: keyE->wVirtualKeyCode = VK_F23;    break;
      case SCR_KEY_FUNCTION + 23: keyE->wVirtualKeyCode = VK_F24;    break;
      default: LogPrint(LOG_WARNING, "Key %4.4X not suported.", key);
               return 0;
    }
  }

  keyE->wRepeatCount = 1;
  keyE->bKeyDown = TRUE;
  if (!doinsert(&buf))
    return 0;
  keyE->bKeyDown = FALSE;
  if (!doinsert(&buf))
    return 0;
  return 1;
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
