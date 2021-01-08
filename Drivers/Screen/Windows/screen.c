/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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

#include "log.h"
#include "parse.h"
#include "brl_cmds.h"
#include "system_windows.h"
#include "kbd_keycodes.h"
#include "unicode.h"

typedef enum {
  PARM_ROOT,
  PARM_FOLLOWFOCUS
} ScreenParameters;
#define SCRPARMS "root", "followfocus"
static unsigned int root, followFocus;

#include "scr_driver.h"

static HANDLE consoleOutput = INVALID_HANDLE_VALUE;
static HANDLE consoleInput = INVALID_HANDLE_VALUE;

static int
processParameters_WindowsScreen (char **parameters) {
  root = 0;
  if (*parameters[PARM_ROOT])
    if (!validateYesNo(&root, parameters[PARM_ROOT]))
      logMessage(LOG_WARNING, "%s: %s", "invalid root setting", parameters[PARM_ROOT]);
  if (root && AttachConsoleProc)
    logMessage(LOG_WARNING, "No need for root BRLTTY on newer (XP or later) systems");

  followFocus = 1;
  if (*parameters[PARM_FOLLOWFOCUS])
    if (!validateYesNo(&followFocus, parameters[PARM_FOLLOWFOCUS]))
      logMessage(LOG_WARNING, "%s: %s", "invalid follow focus setting", parameters[PARM_FOLLOWFOCUS]);
  if (followFocus && !AttachConsoleProc)
    logMessage(LOG_WARNING, "Cannot follow focus on older (pre-XP) systems");
  return 1;
}

static int
openStdHandles (void) {
  if ((consoleOutput == INVALID_HANDLE_VALUE &&
    (consoleOutput = CreateFile("CONOUT$",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL)) == INVALID_HANDLE_VALUE)
    ||(consoleInput == INVALID_HANDLE_VALUE &&
    (consoleInput = CreateFile("CONIN$",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL)) == INVALID_HANDLE_VALUE)) {
    logWindowsSystemError("GetStdHandle");
    return 0;
  }
  return 1;
}

static void
closeStdHandles (void) {
  CloseHandle(consoleInput);
  consoleInput = INVALID_HANDLE_VALUE;
  CloseHandle(consoleOutput);
  consoleOutput = INVALID_HANDLE_VALUE;
}

static int
tryToAttach (HWND win) {
#define CONSOLEWINDOW "ConsoleWindowClass"
  static char class[] = CONSOLEWINDOW;
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
construct_WindowsScreen (void) {
  if (followFocus && (AttachConsoleProc || root)) {
    /* disable ^C */
    SetConsoleCtrlHandler(NULL,TRUE);
    if (!FreeConsole() && GetLastError() != ERROR_INVALID_PARAMETER)
      logWindowsSystemError("FreeConsole");
    return 1;
  }
  return openStdHandles();
}

static void
destruct_WindowsScreen (void) {
  closeStdHandles();
}

static ALTTABINFO altTabInfo;
static HWND altTab;
static char altTabName[128];
static BOOL CALLBACK
findAltTab (HWND win, LPARAM lparam) {
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

  unreadable = NULL;
  altTab = NULL;

  if (followFocus && (AttachConsoleProc || root) && GetAltTabInfoAProc) {
    altTabInfo.cbSize = sizeof(altTabInfo);
    EnumWindows(findAltTab, 0);

    if (altTab) {
      if (!(GetAltTabInfoAProc(altTab,
	      altTabInfo.iColFocus + altTabInfo.iRowFocus * altTabInfo.cColumns,
	      &altTabInfo, altTabName, sizeof(altTabName)))) {
	altTab = NULL;
      } else {
	return 0;
      }
    }
  }

  if (!(win = GetForegroundWindow())) win = INVALID_HANDLE_VALUE;

  if (!AttachConsoleProc && root) {
    unreadable = "root BRLTTY";
  } else if (win == INVALID_HANDLE_VALUE) {
    unreadable = "no foreground window";
  } else if (followFocus && AttachConsoleProc && !tryToAttach(win)) {
    unreadable = "no attachable console";
  } else if (consoleOutput == INVALID_HANDLE_VALUE) {
    unreadable = "can't open console output";
  } else if (!(GetConsoleScreenBufferInfo(consoleOutput, &info))) {
    logWindowsSystemError("GetConsoleScreenBufferInfo");
    unreadable = "can't read console information";

    CloseHandle(consoleOutput);
    consoleOutput = INVALID_HANDLE_VALUE;
  }

  return (LONG_PTR)win;
}

static void
describe_WindowsScreen (ScreenDescription *description) {
  description->number = (int) currentVirtualTerminal_WindowsScreen();
  description->unreadable = unreadable;
  if (unreadable) {
    description->rows = 1;
    description->cols = strlen(unreadable);
    description->posx = 0;
    description->posy = 0;
    description->hasCursor = 0;
  } else if (altTab) {
    description->rows = 1;
    description->cols = strlen(altTabName);
    description->posx = 0;
    description->posy = 0;
    description->hasCursor = 0;
  } else {
    description->cols = info.srWindow.Right + 1 - info.srWindow.Left;
    description->rows = info.srWindow.Bottom + 1 - info.srWindow.Top;
    description->posx = info.dwCursorPosition.X - info.srWindow.Left;
    description->posy = info.dwCursorPosition.Y - info.srWindow.Top; 
    description->hasCursor = 1;
  }
  cols = description->cols;
  rows = description->rows;
}

static int
readCharacters_WindowsScreen (const ScreenBox *box, ScreenCharacter *buffer) {
  int x, xx, y;
  static int wide;
  COORD coord;

  BOOL WINAPI (*fun) (HANDLE, void*, DWORD, COORD, LPDWORD);
  const char *name;
  size_t size;
  void *buf;
  WORD *bufAttr;

  if (!validateScreenBox(box, cols, rows)) return 0;

  if (unreadable) {
    setScreenMessage(box, buffer, unreadable);
    return 1;
  }

  if (altTab) {
    setScreenMessage(box, buffer, altTabName);
    return 1;
  }

  coord.X = box->left + info.srWindow.Left;
  coord.Y = box->top + info.srWindow.Top;

  if (!wide) {
    wchar_t buf;
    DWORD read;
    if (ReadConsoleOutputCharacterW(consoleOutput, &buf, 1, coord, &read))
      wide = 1;
    else {
      if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
	wide = -1;
      else {
	logWindowsSystemError("ReadConsoleOutputCharacterW");
	return 0;
      }
    }
  }
#define USE(f, t) (fun = (typeof(fun))f, name = #f, size = sizeof(t))
  if (wide > 0)
    USE(ReadConsoleOutputCharacterW, wchar_t);
  else
    USE(ReadConsoleOutputCharacterA, char);
#undef USE

  if (!(buf = malloc(box->width*size))) {
    logSystemError("malloc for Windows console reading");
    return 0;
  }

  if (!(bufAttr = malloc(box->width*sizeof(WORD)))) {
    logSystemError("malloc for Windows console reading");
    free(buf);
    return 0;
  }

  for (y=0; y<box->height; y++, coord.Y++) {
    DWORD read;

    if (!fun(consoleOutput, buf, box->width, coord, &read)) {
      logWindowsSystemError(name);
      break;
    }

    if (wide > 0) {
      for (x=0, xx = 0; x<box->width && xx < read; x++, xx++) {
	wchar_t wc = ((wchar_t*)buf)[xx];
	int i;

	buffer[y*box->width+x].text = wc;
	for (i = 1; i < getCharacterWidth(wc); i++) {
	  x++;
	  buffer[y*box->width+x].text = UNICODE_ZERO_WIDTH_SPACE;
	}
      }
      for ( ; x<box->width; x++) {
	buffer[y*box->width+x].text = L' ';
      }
    } else {
      for (x=0; x<read; x++) {
	/* TODO: GetConsoleCP and convert */
	buffer[y*box->width+x].text = ((char*)buf)[x];
      }
      for ( ; x<box->width; x++) {
	buffer[y*box->width+x].text = ' ';
      }
    }

    if (!ReadConsoleOutputAttribute(consoleOutput, bufAttr, box->width, coord, &read)) {
      logWindowsSystemError(name);
      break;
    }

    for (x=0, xx=0; x<box->width && xx < read; x++, xx++) {
      int i;
      buffer[y*box->width+x].attributes = bufAttr[xx];
      for (i = 1; i < getCharacterWidth(buffer[y*box->width+x].text); i++) {
	x++;
        buffer[y*box->width+x].attributes = bufAttr[xx];
      }
    }
    for ( ; x < box->width; x++)
      buffer[y*box->width+x].attributes = 0;
  }

  free(buf);
  free(bufAttr);

  return (y == box->height);
}

static int 
doInsertWriteConsoleInput (BOOL down, WCHAR wchar, WORD vk, WORD scancode, DWORD controlKeyState) {
  DWORD num;
  INPUT_RECORD buf;
  KEY_EVENT_RECORD *keyE = &buf.Event.KeyEvent;

  buf.EventType = KEY_EVENT;
  memset(keyE, 0, sizeof(*keyE));
  keyE->bKeyDown = down;
  keyE->wRepeatCount = 1;
  keyE->wVirtualKeyCode = vk;
  keyE->wVirtualScanCode = scancode;
  keyE->uChar.UnicodeChar = wchar;
  keyE->dwControlKeyState = controlKeyState;
  if (WriteConsoleInputW(consoleInput, &buf, 1, &num)) {
    if (num == 1) {
      return 1;
    } else {
      logMessage(LOG_ERR, "inserted %d keys, expected 1", (int)num);
    }
  } else {
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
      keyE->uChar.AsciiChar = wchar;
      if (WriteConsoleInputA(consoleInput, &buf, 1, &num)) {
	if (num == 1) {
	  return 1;
	} else {
	  logMessage(LOG_ERR, "inserted %d keys, expected 1", (int)num);
	}
      }
    }
    logWindowsSystemError("WriteConsoleInput");
    CloseHandle(consoleInput);
    consoleInput = INVALID_HANDLE_VALUE;
  }
  return 0;
}

static int 
doInsertSendInput (BOOL down, WCHAR wchar, WORD vk, WORD scancode, DWORD flags) {
  if (SendInputProc) {
    UINT num;
    INPUT input;
    KEYBDINPUT *ki = &input.ki;

    input.type = INPUT_KEYBOARD;
    memset(ki, 0, sizeof(*ki));
    ki->dwFlags = flags;
    if (wchar) {
      ki->wVk = 0;
      ki->wScan = wchar;
      ki->dwFlags |= KEYEVENTF_UNICODE;
    } else if (vk) {
      ki->wVk = vk;
      ki->wScan = scancode;
    } else {
      ki->wVk = 0;
      ki->wScan = scancode;
      ki->dwFlags |= KEYEVENTF_SCANCODE;
    }

    if (!down)
      ki->dwFlags |= KEYEVENTF_KEYUP;

    num = SendInput(1, &input, sizeof(INPUT));
    switch (num) {
      case 1:  return 1;
      case 0:  logWindowsSystemError("SendInput"); break;
      default: logMessage(LOG_ERR, "inserted %d keys, expected 1", num); break;
    }
    return 0;
  } else {
    keybd_event(vk, scancode, flags | (down ? 0 : KEYEVENTF_KEYUP), 0);
    return 1;
  }
}

static int
insertKey_WindowsScreen (ScreenKey key) {
  SHORT vk = 0;
  SHORT scancode = 0;
  DWORD controlKeyState = 0;
  WCHAR wchar = 0;

  logMessage(LOG_DEBUG, "Insert key: %4.4X",key);
  if (isSpecialKey(key)) {
    switch (key & SCR_KEY_CHAR_MASK) {
      case SCR_KEY_ENTER:         vk = VK_RETURN; wchar='\r'; break;
      case SCR_KEY_TAB:           vk = VK_TAB;    wchar='\t'; break;
      case SCR_KEY_BACKSPACE:     vk = VK_BACK;   wchar='\b'; break;
      case SCR_KEY_ESCAPE:        vk = VK_ESCAPE; wchar='\e'; break;
      case SCR_KEY_CURSOR_LEFT:   vk = VK_LEFT;   break;
      case SCR_KEY_CURSOR_RIGHT:  vk = VK_RIGHT;  break;
      case SCR_KEY_CURSOR_UP:     vk = VK_UP;     break;
      case SCR_KEY_CURSOR_DOWN:   vk = VK_DOWN;   break;
      case SCR_KEY_PAGE_UP:       vk = VK_PRIOR;  break;
      case SCR_KEY_PAGE_DOWN:     vk = VK_NEXT;   break;
      case SCR_KEY_HOME:          vk = VK_HOME;   break;
      case SCR_KEY_END:           vk = VK_END;    break;
      case SCR_KEY_INSERT:        vk = VK_INSERT; break;
      case SCR_KEY_DELETE:        vk = VK_DELETE; break;
      case SCR_KEY_FUNCTION + 0:  vk = VK_F1;     break;
      case SCR_KEY_FUNCTION + 1:  vk = VK_F2;     break;
      case SCR_KEY_FUNCTION + 2:  vk = VK_F3;     break;
      case SCR_KEY_FUNCTION + 3:  vk = VK_F4;     break;
      case SCR_KEY_FUNCTION + 4:  vk = VK_F5;     break;
      case SCR_KEY_FUNCTION + 5:  vk = VK_F6;     break;
      case SCR_KEY_FUNCTION + 6:  vk = VK_F7;     break;
      case SCR_KEY_FUNCTION + 7:  vk = VK_F8;     break;
      case SCR_KEY_FUNCTION + 8:  vk = VK_F9;     break;
      case SCR_KEY_FUNCTION + 9:  vk = VK_F10;    break;
      case SCR_KEY_FUNCTION + 10: vk = VK_F11;    break;
      case SCR_KEY_FUNCTION + 11: vk = VK_F12;    break;
      case SCR_KEY_FUNCTION + 12: vk = VK_F13;    break;
      case SCR_KEY_FUNCTION + 13: vk = VK_F14;    break;
      case SCR_KEY_FUNCTION + 14: vk = VK_F15;    break;
      case SCR_KEY_FUNCTION + 15: vk = VK_F16;    break;
      case SCR_KEY_FUNCTION + 16: vk = VK_F17;    break;
      case SCR_KEY_FUNCTION + 17: vk = VK_F18;    break;
      case SCR_KEY_FUNCTION + 18: vk = VK_F19;    break;
      case SCR_KEY_FUNCTION + 19: vk = VK_F20;    break;
      case SCR_KEY_FUNCTION + 20: vk = VK_F21;    break;
      case SCR_KEY_FUNCTION + 21: vk = VK_F22;    break;
      case SCR_KEY_FUNCTION + 22: vk = VK_F23;    break;
      case SCR_KEY_FUNCTION + 23: vk = VK_F24;    break;
      default: logMessage(LOG_WARNING, "Key %4.4X not suported.", key);
               return 0;
    }
  } else {
    if (key & SCR_KEY_ALT_LEFT) {
      controlKeyState |= LEFT_ALT_PRESSED;
      key &= ~ SCR_KEY_ALT_LEFT;
    }
    if (key & SCR_KEY_ALT_RIGHT) {
      controlKeyState |= RIGHT_ALT_PRESSED;
      key &= ~ SCR_KEY_ALT_RIGHT;
    }
    if (key & SCR_KEY_SHIFT) {
      controlKeyState |= SHIFT_PRESSED;
      key &= ~ SCR_KEY_SHIFT;
    }
    if (key & SCR_KEY_CONTROL) {
      controlKeyState |= LEFT_CTRL_PRESSED;
      key &= ~ SCR_KEY_CONTROL;
    }
    wchar = key & SCR_KEY_CHAR_MASK;
    vk = VkKeyScanW(wchar);
    if (vk == -1 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
      vk = VkKeyScan(wchar);
    if (vk != -1) {
      logMessage(LOG_DEBUG, "vk is %4.4X", vk);
      if (vk & 0x100) controlKeyState |= SHIFT_PRESSED;
      if ((vk & 0x600) == 0x600) {
	controlKeyState |= RIGHT_CTRL_PRESSED;
	controlKeyState |= RIGHT_ALT_PRESSED;
      } else {
        if (vk & 0x200) controlKeyState |= LEFT_CTRL_PRESSED;
        if (vk & 0x400) controlKeyState |= LEFT_ALT_PRESSED;
      }
      vk = vk & 0xff;
    } else vk = 0;
  }

  scancode = MapVirtualKey(vk, 0);

  logMessage(LOG_DEBUG,"wchar %x vk %x scancode %d ks %d", wchar, vk, scancode, (int)controlKeyState);
  if (consoleInput != INVALID_HANDLE_VALUE && !unreadable) {
    logMessage(LOG_DEBUG, "using WriteConsoleInput");
    if (!doInsertWriteConsoleInput(TRUE, wchar, vk, scancode, controlKeyState))
      return 0;
    if (!doInsertWriteConsoleInput(FALSE, wchar, vk, scancode, controlKeyState))
      return 0;
    return 1;
  } else {
    logMessage(LOG_DEBUG, "using SendInput");
    if (controlKeyState & LEFT_CTRL_PRESSED)
      if (!doInsertSendInput(TRUE, 0, VK_CONTROL, 0, 0))
        return 0;
    if (controlKeyState & SHIFT_PRESSED)
      if (!doInsertSendInput(TRUE, 0, VK_SHIFT, 0, 0))
        return 0;
    if (controlKeyState & LEFT_ALT_PRESSED)
      if (!doInsertSendInput(TRUE, 0, VK_MENU, 0, 0))
        return 0;
    if (controlKeyState & RIGHT_ALT_PRESSED)
      if (!doInsertSendInput(TRUE, 0, VK_RMENU, 0, 0))
        return 0;
    if (!doInsertSendInput(TRUE, wchar, vk, scancode, 0))
      return 0;
    if (!doInsertSendInput(FALSE, wchar, vk, scancode, 0))
      return 0;
    if (controlKeyState & RIGHT_ALT_PRESSED)
      if (!doInsertSendInput(FALSE, 0, VK_RMENU, 0, 0))
        return 0;
    if (controlKeyState & LEFT_ALT_PRESSED)
      if (!doInsertSendInput(FALSE, 0, VK_MENU, 0, 0))
        return 0;
    if (controlKeyState & SHIFT_PRESSED)
      if (!doInsertSendInput(FALSE, 0, VK_SHIFT, 0, 0))
        return 0;
    if (controlKeyState & LEFT_CTRL_PRESSED)
      if (!doInsertSendInput(FALSE, 0, VK_CONTROL, 0, 0))
        return 0;
    return 1;
  }
  return 0;
}

static int
handleCommand_WindowsScreen (int command) {
  int blk = command & BRL_MSK_BLK;
  int arg = command & BRL_MSK_ARG;
  int press = 0;

  switch (blk) {
    case BRL_CMD_BLK(PASSXT):
      press = !(arg & 0X80);
      arg &= 0X7F;
      /* fallthrough */
      goto do_send;
    case BRL_CMD_BLK(PASSAT): {
      press |= !(command & BRL_FLG_KBD_RELEASE);

do_send:
      if (arg >= 0X80)
	return 0;

      if (command & BRL_FLG_KBD_EMUL1)
	return 0;

      if (blk == BRL_CMD_BLK(PASSAT))
	arg = AT2XT[arg];

      return doInsertSendInput (press, 0, 0, arg,
	  command & BRL_FLG_KBD_EMUL0 ? KEYEVENTF_EXTENDEDKEY : 0);
    }
  }

  return 0;
}

static void
scr_initialize (MainScreen *main) {
  initializeRealScreen(main);
  main->base.currentVirtualTerminal = currentVirtualTerminal_WindowsScreen;
  main->base.describe = describe_WindowsScreen;
  main->base.readCharacters = readCharacters_WindowsScreen;
  main->base.insertKey = insertKey_WindowsScreen;
  main->base.handleCommand = handleCommand_WindowsScreen;
  main->processParameters = processParameters_WindowsScreen;
  main->construct = construct_WindowsScreen;
  main->destruct = destruct_WindowsScreen;
}
