/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "scr.h"
#include "scr_base.h"

static const char *text_BaseScreen = "no screen";

static int
selectvt_BaseScreen (int vt) {
  return 0;
}

static int
switchvt_BaseScreen (int vt) {
  return 0;
}

static int
currentvt_BaseScreen (void) {
  return -1;
}

static void
describe_BaseScreen (ScreenDescription *description) {
  description->rows = 1;
  description->cols = strlen(text_BaseScreen);
  description->posx = 0;
  description->posy = 0;
  description->number = currentvt_BaseScreen();
}

static int
read_BaseScreen (ScreenBox box, unsigned char *buffer, ScreenMode mode) {
  ScreenDescription description;
  describe_BaseScreen(&description);
  if (!validateScreenBox(&box, description.cols, description.rows)) return 0;
  if (mode == SCR_TEXT) {
    memcpy(buffer, text_BaseScreen+box.left, box.width);
  } else {
    memset(buffer, 0X07, box.width);
  }
  return 1;
}

static int
insert_BaseScreen (ScreenKey key) {
  return 0;
}

static int
route_BaseScreen (int column, int row, int screen) {
  return 0;
}

static int
point_BaseScreen (int column, int row) {
  return 0;
}

static int
pointer_BaseScreen (int *column, int *row) {
  return 0;
}

static int
execute_BaseScreen (int cmd) {
  return 0;
}

void
initializeBaseScreen (BaseScreen *base) {
  base->selectvt = selectvt_BaseScreen;
  base->switchvt = switchvt_BaseScreen;
  base->currentvt = currentvt_BaseScreen;
  base->describe = describe_BaseScreen;
  base->read = read_BaseScreen;
  base->insert = insert_BaseScreen;
  base->route = route_BaseScreen;
  base->point = point_BaseScreen;
  base->pointer = pointer_BaseScreen;
  base->execute = execute_BaseScreen;
}
