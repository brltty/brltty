/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>

#include <pc.h>
#include <go32.h>
#include <dpmi.h>
#include <sys/farptr.h>

#include "log.h"
#include "brl_cmds.h"
#include "charset.h"
#include "kbd_keycodes.h"

#include "scr_driver.h"

static int
processParameters_PcBiosScreen (char **parameters) {
  return 1;
}

static int
construct_PcBiosScreen (void) {
  return 1;
}

static void
destruct_PcBiosScreen (void) {
}

static void
describe_PcBiosScreen (ScreenDescription *description) {
  int row, column;
  description->rows = ScreenRows();
  description->cols = ScreenCols();
  ScreenGetCursor(&row, &column);
  description->posx = column;
  description->posy = row;
  description->number = 1;
}

static int
readCharacters_PcBiosScreen (const ScreenBox *box, ScreenCharacter *buffer) {
  unsigned offset = ScreenPrimary;
  ScreenDescription description;
  int row, col;
  describe_PcBiosScreen(&description);
  if (!validateScreenBox(box, description.cols, description.rows)) return 0;
  _farsetsel(_go32_conventional_mem_selector());
  for (row=box->top; row<box->top+box->height; ++row)
    for (col=box->left; col<box->left+box->width; ++col) {
      buffer->text = _farnspeekb(offset + row*description.cols*2 + col*2);
      buffer->attributes = _farnspeekb(offset + row*description.cols*2 + col*2 + 1);
      buffer++;
    }
  return 1;
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

  if (isSpecialKey(key)) {
    switch (key & SCR_KEY_CHAR_MASK) {
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
        logMessage(LOG_WARNING, "Key %4.4X not suported.", key);
        return 0;
    }
    end = sequence + strlen(sequence);
  } else {
    int character = key & SCR_KEY_CHAR_MASK;
    int c = convertWcharToChar(character);

    if (c== EOF) {
      logMessage(LOG_WARNING, "Character %d not supported", character);
      return 0;
    }

    sequence = end = buffer + ARRAY_COUNT(buffer);
    *--sequence = key & SCR_KEY_CHAR_MASK;

    if (key & SCR_KEY_ALT_LEFT)
      *--sequence = 0X1B;
  }

  while (sequence != end)
    if (simulateKey(0, *sequence++))
      return 0;
  return 1;
}

static int
insertKey_PcBiosScreen (ScreenKey key) {
  logMessage(LOG_DEBUG, "Insert key: %4.4X", key);
  return insertMapped(key); 
}

static int
selectVirtualTerminal_PcBiosScreen (int vt) {
  return vt == 1;
}

static int
switchVirtualTerminal_PcBiosScreen (int vt) {
  return vt == 1;
}

static int
currentVirtualTerminal_PcBiosScreen (void) {
  return 1;
}

static int
handleCommand_PcBiosScreen (int command) {
  int blk = command & BRL_MSK_BLK;
  int arg = command & BRL_MSK_ARG;
  int press = 0;

  switch (blk) {
    case BRL_CMD_BLK(PASSXT):
      press = !(arg & 0X80);
      arg &= 0X7F;
      /* fallthrough */
    case BRL_CMD_BLK(PASSAT): {
      press |= !(command & BRL_FLG_KBD_RELEASE);

      if (arg >= 0X80)
	return 0;

      if (command & BRL_FLG_KBD_EMUL0) {
	if (!simulateKey(0XE0, 0))
	  return 0;
      } else if (command & BRL_FLG_KBD_EMUL1) {
	if (!simulateKey(0XE1, 0))
	  return 0;
      }

      if (blk == BRL_CMD_BLK(PASSAT))
	arg = AT2XT[arg];

      if (!press)
	arg |= 0x80;

      return simulateKey(arg, 0);
    }
  }

  return 0;
}

static void
scr_initialize (MainScreen *main) {
  initializeRealScreen(main);
  main->base.describe = describe_PcBiosScreen;
  main->base.readCharacters = readCharacters_PcBiosScreen;
  main->base.insertKey = insertKey_PcBiosScreen;
  main->base.selectVirtualTerminal = selectVirtualTerminal_PcBiosScreen;
  main->base.switchVirtualTerminal = switchVirtualTerminal_PcBiosScreen;
  main->base.currentVirtualTerminal = currentVirtualTerminal_PcBiosScreen;
  main->base.handleCommand = handleCommand_PcBiosScreen;
  main->processParameters = processParameters_PcBiosScreen;
  main->construct = construct_PcBiosScreen;
  main->destruct = destruct_PcBiosScreen;
}
