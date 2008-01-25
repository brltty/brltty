/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#include "misc.h"
#include "scr.h"
#include "scr_base.h"

static const char *text_BaseScreen = "no screen";

static int
selectVirtualTerminal_BaseScreen (int vt) {
  return 0;
}

static int
switchVirtualTerminal_BaseScreen (int vt) {
  return 0;
}

static int
currentVirtualTerminal_BaseScreen (void) {
  return -1;
}

static void
describe_BaseScreen (ScreenDescription *description) {
  description->rows = 1;
  description->cols = strlen(text_BaseScreen);
  description->posx = 0;
  description->posy = 0;
  description->number = currentVirtualTerminal_BaseScreen();
}

static int
read_BaseScreen (ScreenBox box, unsigned char *buffer, ScreenCharacterProperty property) {
  ScreenDescription description;
  describe_BaseScreen(&description);
  if (!validateScreenBox(&box, description.cols, description.rows)) return 0;
  if (property == SCR_TEXT) {
    memcpy(buffer, text_BaseScreen+box.left, box.width);
  } else {
    memset(buffer, 0X07, box.width);
  }
  return 1;
}

static int
insertKey_BaseScreen (ScreenKey key) {
  return 0;
}

static int
routeCursor_BaseScreen (int column, int row, int screen) {
  return 0;
}

static int
highlightRegion_BaseScreen (int left, int right, int top, int bottom) {
  return 0;
}

int
unhighlightRegion_BaseScreen (void) {
  return 0;
}

static int
getPointer_BaseScreen (int *column, int *row) {
  return 0;
}

static int
executeCommand_BaseScreen (int command) {
  return 0;
}

void
initializeBaseScreen (BaseScreen *base) {
  base->selectVirtualTerminal = selectVirtualTerminal_BaseScreen;
  base->switchVirtualTerminal = switchVirtualTerminal_BaseScreen;
  base->currentVirtualTerminal = currentVirtualTerminal_BaseScreen;
  base->describe = describe_BaseScreen;
  base->read = read_BaseScreen;
  base->insertKey = insertKey_BaseScreen;
  base->routeCursor = routeCursor_BaseScreen;
  base->highlightRegion = highlightRegion_BaseScreen;
  base->unhighlightRegion = unhighlightRegion_BaseScreen;
  base->getPointer = getPointer_BaseScreen;
  base->executeCommand = executeCommand_BaseScreen;
}

void
describeBaseScreen (BaseScreen *base, ScreenDescription *description) {
  description->cols = description->rows = 1;
  description->posx = description->posy = 0;
  description->number = 0;
  description->cursor = 1;
  description->unreadable = NULL;
  base->describe(description);

  if (description->unreadable) {
    description->cursor = 0;
  } else if (description->number == -1) {
    description->unreadable = "unreadable screen";
  }
}
