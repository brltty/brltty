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

#include <stdio.h>

#include "misc.h"
#include "scr.h"
#include "scr_main.h"

static int
processParameters_MainScreen (char **parameters) {
  return 1;
}

static int
open_MainScreen (void) {
  return 1;
}

static void
close_MainScreen (void) {
}

static int
userVirtualTerminal_MainScreen (int number) {
  return 0 + number;
}

void
initializeMainScreen (MainScreen *main) {
  initializeBaseScreen(&main->base);
  main->processParameters = processParameters_MainScreen;
  main->open = open_MainScreen;
  main->close = close_MainScreen;
  main->userVirtualTerminal = userVirtualTerminal_MainScreen;
}
