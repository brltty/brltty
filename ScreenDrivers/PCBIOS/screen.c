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

#include <string.h>

#include <pc.h>
#include <go32.h>
#include <dpmi.h>
#include <sys/farptr.h>

#include "Programs/misc.h"
#include "Programs/brldefs.h"

#include "Programs/scr_driver.h"

static int
prepare_PcbiosScreen (char **parameters) {
  return 1;
}

static int
open_PcbiosScreen (void) {
  return 1;
}

static int
setup_PcbiosScreen (void) {
  return 1;
}

static void
close_PcbiosScreen (void) {
}

static void
describe_PcbiosScreen (ScreenDescription *description) {
  int row, column;
  description->rows = ScreenRows();
  description->cols = ScreenCols();
  ScreenGetCursor(&row, &column);
  description->posx = column;
  description->posy = row;
  description->number = 1;
}

static int
read_PcbiosScreen (ScreenBox box, unsigned char *buffer, ScreenMode mode) {
  unsigned offset = ScreenPrimary;
  ScreenDescription description;
  describe_PcbiosScreen(&description);
  if (validateScreenBox(&box, description.cols, description.rows)) {
    int row, col;
    _farsetsel(_go32_conventional_mem_selector());
    if (mode == SCR_ATTRIB) offset++;
    for (row=box.top; row<box.top+box.height; ++row)
      for (col=box.left; col<box.left+box.width; ++col)
	*buffer++ = _farnspeekb(offset + row*description.cols*2 + col*2);
    return 1;
  } else {
    LogPrint(LOG_ERR, "Invalid screen area: cols=%d left=%d width=%d rows=%d top=%d height=%d",
             description.cols, box.left, box.width,
             description.rows, box.top, box.height);
  }
  return 0;
}

static int
simulateKey (unsigned char scanCode, unsigned char asciiCode) {
  __dpmi_regs r;
  r.h.ah = 0x05;
  r.h.ch = scanCode;
  r.h.cl = asciiCode;
  __dpmi_int(0x16, &r);
  return !r.h.ah;
}

static int
insertMapped (ScreenKey key) {
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
        sequence = "\x08";
        break;
      case SCR_KEY_ESCAPE:
        sequence = "\x1b";
        break;
      case SCR_KEY_CURSOR_LEFT:
	return(simulateKey(105, 0));
      case SCR_KEY_CURSOR_RIGHT:
        return(simulateKey(106, 0));
      case SCR_KEY_CURSOR_UP:
        return(simulateKey(103, 0));
      case SCR_KEY_CURSOR_DOWN:
        return(simulateKey(108, 0));
      case SCR_KEY_PAGE_UP:
        return(simulateKey(104, 0));
      case SCR_KEY_PAGE_DOWN:
        return(simulateKey(109, 0));
      case SCR_KEY_HOME:
        return(simulateKey(102, 0));
      case SCR_KEY_END:
        return(simulateKey(107, 0));
      case SCR_KEY_INSERT:
        return(simulateKey(110, 0));
      case SCR_KEY_DELETE:
        return(simulateKey(111, 0));
      case SCR_KEY_FUNCTION + 0:
      case SCR_KEY_FUNCTION + 1:
      case SCR_KEY_FUNCTION + 2:
      case SCR_KEY_FUNCTION + 3:
      case SCR_KEY_FUNCTION + 4:
      case SCR_KEY_FUNCTION + 5:
      case SCR_KEY_FUNCTION + 6:
      case SCR_KEY_FUNCTION + 7:
      case SCR_KEY_FUNCTION + 8:
      case SCR_KEY_FUNCTION + 9:
      case SCR_KEY_FUNCTION + 10:
        return(simulateKey(key - SCR_KEY_FUNCTION + 59, 0));
      case SCR_KEY_FUNCTION + 11:
        return(simulateKey(87, 0));
      case SCR_KEY_FUNCTION + 12:
        return(simulateKey(88, 0));
      default:
        LogPrint(LOG_WARNING, "Key %4.4X not suported.", key);
        return 0;
    }
    end = sequence + strlen(sequence);
  }

  while (sequence != end)
    if (simulateKey(0, *sequence++))
      return 0;
  return 1;
}

static int
insert_PcbiosScreen (ScreenKey key) {
  LogPrint(LOG_DEBUG, "Insert key: %4.4X", key);
  return insertMapped(key); 
}

static int
selectVirtualTerminal_PcbiosScreen (int vt) {
  return vt == 1;
}

static int
switchVirtualTerminal_PcbiosScreen (int vt) {
  return vt == 1;
}

static int
currentVirtualTerminal_PcbiosScreen (void) {
  return 1;
}

static int
executeCommand_PcbiosScreen (int command) {
  if ((command & BRL_MSK_BLK) == BRL_BLK_PASSAT2)
    return simulateKey(command & BRL_MSK_ARG, 0);
  return 0;
}

static void
scr_initialize (MainScreen *main) {
  initializeRealScreen(main);
  main->base.describe = describe_PcbiosScreen;
  main->base.read = read_PcbiosScreen;
  main->base.insert = insert_PcbiosScreen;
  main->base.selectVirtualTerminal = selectVirtualTerminal_PcbiosScreen;
  main->base.switchVirtualTerminal = switchVirtualTerminal_PcbiosScreen;
  main->base.currentVirtualTerminal = currentVirtualTerminal_PcbiosScreen;
  main->base.executeCommand = executeCommand_PcbiosScreen;
  main->prepare = prepare_PcbiosScreen;
  main->open = open_PcbiosScreen;
  main->setup = setup_PcbiosScreen;
  main->close = close_PcbiosScreen;
}
