/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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
#include "Programs/sys_windows.h"

typedef enum {
  PARM_ROOT
} ScreenParameters;
#define SCRPARMS "root"
static unsigned int root;

#include "Programs/scr_driver.h"

static HANDLE consoleOutput = INVALID_HANDLE_VALUE;
static HANDLE consoleInput = INVALID_HANDLE_VALUE;

static int
prepare_WindowsScreen (char **parameters) {
  if (*parameters[PARM_ROOT]) {
    if (AttachConsoleProc)
      LogError("No need for root BRLTTY on systems that support AttachConsole()");
    if (!validateYesNo(&root, parameters[PARM_ROOT]))
      LogPrint(LOG_WARNING, "%s: %s", "invalid root setting", parameters[PARM_ROOT]);
  }
  return 1;
}

static int openStdHandles(void) {
  if ((consoleOutput == INVALID_HANDLE_VALUE &&
    (consoleOutput = CreateFile("CONOUT$",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL)) == INVALID_HANDLE_VALUE)
    ||(consoleInput == INVALID_HANDLE_VALUE &&
    (consoleInput = CreateFile("CONIN$",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL)) == INVALID_HANDLE_VALUE)) {
    LogWindowsError("GetStdHandle");
    return 0;
  }
  return 1;
}

static void closeStdHandles(void) {
  CloseHandle(consoleInput);
  consoleInput = INVALID_HANDLE_VALUE;
  CloseHandle(consoleOutput);
  consoleOutput = INVALID_HANDLE_VALUE;
}

static int tryToAttach(HWND win) {
#define CONSOLEWINDOW "ConsoleWindowClass"
  static char class[strlen(CONSOLEWINDOW)+1];
  DWORD process;
  if (GetClassName(win, class, sizeof(class)) != strlen(CONSOLEWINDOW)
      || memcmp(class,CONSOLEWINDOW,strlen(CONSOLEWINDOW)))
    return 0;
  if (!GetWindowThreadProcessId(win, &process))
    return 0;
  FreeConsole();
  if (!AttachConsoleProc(process))
    return 0;
  closeStdHandles();
  return openStdHandles();
}

static int
open_WindowsScreen (void) {
  if (AttachConsoleProc || root) {
    /* disable ^C */
    SetConsoleCtrlHandler(NULL,TRUE);
    if (!FreeConsole() && GetLastError() != ERROR_INVALID_PARAMETER)
      LogWindowsError("FreeConsole");
    return 1;
  }
  return openStdHandles();
}

static int
setup_WindowsScreen (void) {
  return 1;
}

static void
close_WindowsScreen (void) {
  closeStdHandles();
}

static int
selectVirtualTerminal_WindowsScreen (int vt) {
  return 0;
}

static int
switchVirtualTerminal_WindowsScreen (int vt) {
  return 0;
}

static ALTTABINFO altTabInfo;
static HWND altTab;
static char altTabName[128];
static BOOL CALLBACK findAltTab(HWND win, LPARAM lparam) {
  if (GetAltTabInfoAProc(win, -1, &altTabInfo, NULL, 0)) {
    altTab = win;
    return FALSE;
  }
  return TRUE;
}

static CONSOLE_SCREEN_BUFFER_INFO info;
static const char *unreadable;
static int cols;
static int rows;

static int
currentVirtualTerminal_WindowsScreen (void) {
  HWND win;
  altTab = NULL;
  if ((AttachConsoleProc || root) && GetAltTabInfoAProc) {
    altTabInfo.cbSize = sizeof(altTabInfo);
    EnumWindows(findAltTab, 0);
    if (altTab) {
      if (!(GetAltTabInfoAProc(altTab,
	      altTabInfo.iColFocus + altTabInfo.iRowFocus * altTabInfo.cColumns,
	      &altTabInfo, altTabName, sizeof(altTabName))))
	altTab = NULL;
      else
	return (int)altTab;
    }
  }
  win = GetForegroundWindow();
  unreadable = NULL;
  if (!AttachConsoleProc && root) {
    unreadable = "root BRLTTY";
    goto error;
  }
  if (AttachConsoleProc && !tryToAttach(win)) {
    unreadable = "no terminal to read";
    goto error;
  }
  if (consoleOutput == INVALID_HANDLE_VALUE) {
    unreadable = "can't open terminal output";
    goto error;
  }
  if (!(GetConsoleScreenBufferInfo(consoleOutput, &info))) {
    LogWindowsError("GetConsoleScreenBufferInfo");
    CloseHandle(consoleOutput);
    consoleOutput = INVALID_HANDLE_VALUE;
    unreadable = "can't read terminal information";
    goto error;
  }
error:
  return (int)win;
}

static void
describe_WindowsScreen (ScreenDescription *description) {
  description->number = (int) currentVirtualTerminal_WindowsScreen();
  description->unreadable = unreadable;
  if (altTab) {
    description->rows = 1;
    description->cols = strlen(altTabName);
    description->posx = 0;
    description->posy = 0;
  } else if (unreadable) {
    description->rows = 1;
    description->cols = strlen(unreadable);
    description->posx = 0;
    description->posy = 0;
  } else {
    description->cols = cols = info.srWindow.Right + 1 - info.srWindow.Left;
    description->rows = rows = info.srWindow.Bottom + 1 - info.srWindow.Top;
    description->posx = info.dwCursorPosition.X - info.srWindow.Left;
    description->posy = info.dwCursorPosition.Y - info.srWindow.Top; 
  }
}

static int
read_WindowsScreen (ScreenBox box, unsigned char *buffer, ScreenMode mode) {
  /* TODO: GetConsoleCP */
  int text = mode == SCR_TEXT;
  int x, y;
  static int wide;
  COORD coord;

  BOOL WINAPI (*fun) (HANDLE, void*, DWORD, COORD, LPDWORD);
  const char *name;
  size_t size;
  void *buf;

  if (altTab) {
    setScreenMessage(&box, buffer, mode, altTabName);
    return 1;
  }
  if (consoleOutput == INVALID_HANDLE_VALUE) return 0;
  if (unreadable) {
    setScreenMessage(&box, buffer, mode, unreadable);
    return 1;
  }
  if (!validateScreenBox(&box, cols, rows)) return 0;

  coord.X = box.left + info.srWindow.Left;
  coord.Y = box.top + info.srWindow.Top;

  if (!wide) {
    wchar_t buf;
    DWORD read;
    if (ReadConsoleOutputCharacterW(consoleOutput, &buf, 1, coord, &read))
      wide = 1;
    else {
      if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
	wide = -1;
      else {
	LogWindowsError("ReadConsoleOutputCharacterW");
	return 0;
      }
    }
  }
#define USE(f, t) (fun = (typeof(fun))f, name = #f, size = sizeof(t))
  if (text) {
    if (wide > 0)
      USE(ReadConsoleOutputCharacterW, wchar_t);
    else
      USE(ReadConsoleOutputCharacterA, char);
  } else {
    USE(ReadConsoleOutputAttribute, WORD);
  }
#undef USE

  if (text && wide < 0) {
    buf = buffer;
  } else {
    if (!(buf = malloc(box.width*size))) {
      LogError("malloc for Windows console reading");
      return 0;
    }
  }

  for (y=0; y<box.height; y++, coord.Y++) {
    DWORD read;

    if (!fun(consoleOutput, buf, box.width, coord, &read)) {
      LogWindowsError(name);
      break;
    }

    if (read != box.width) {
      LogPrint(LOG_ERR, "wrong number of items read: %s: %ld != %d",
               name, read, box.width);
      break;
    }

    if (text) {
      if (wide > 0) {
	for (x=0; x<box.width; x++) {
	  wchar_t c = ((wchar_t *)buf)[x];
	  if (c >= 0X100) c = '?';
	  buffer[y*box.width+x] = c;
	}
      } else {
	buf += box.width;
      }
    } else {
      for (x=0; x<box.width; x++) {
	buffer[y*box.width+x] = ((WORD *)buf)[x];
      }
    }
  }

  if (!text || wide > 0) {
    free(buf);
  }

  return (y == box.height);
}

static int 
doinsert(INPUT_RECORD *buf) {
  DWORD num;
  if (!(WriteConsoleInputA(consoleInput, buf, 1, &num))) {
    LogWindowsError("WriteConsoleInput");
    CloseHandle(consoleInput);
    consoleInput = INVALID_HANDLE_VALUE;
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

  if (consoleInput == INVALID_HANDLE_VALUE) return 0;

  LogPrint(LOG_DEBUG, "Insert key: %4.4X",key);
  buf.EventType = KEY_EVENT;
  memset(keyE, 0, sizeof(*keyE));
  if (key < SCR_KEY_ENTER) {
    if (key & SCR_KEY_MOD_META) {
      keyE->dwControlKeyState |= LEFT_ALT_PRESSED;
      key &= ~ SCR_KEY_MOD_META;
    }
    keyE->uChar.AsciiChar = key;
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
executeCommand_WindowsScreen (int command) {
  int blk = command & BRL_MSK_BLK;
  int arg UNUSED = command & BRL_MSK_ARG;

  switch (blk) {
    case BRL_BLK_PASSAT2:
      break;
  }
  return 0;
}

static void
scr_initialize (MainScreen *main) {
  initializeRealScreen(main);
  main->base.selectVirtualTerminal = selectVirtualTerminal_WindowsScreen;
  main->base.switchVirtualTerminal = switchVirtualTerminal_WindowsScreen;
  main->base.currentVirtualTerminal = currentVirtualTerminal_WindowsScreen;
  main->base.describe = describe_WindowsScreen;
  main->base.read = read_WindowsScreen;
  main->base.insert = insert_WindowsScreen;
  main->base.executeCommand = executeCommand_WindowsScreen;
  main->prepare = prepare_WindowsScreen;
  main->open = open_WindowsScreen;
  main->setup = setup_WindowsScreen;
  main->close = close_WindowsScreen;
}
