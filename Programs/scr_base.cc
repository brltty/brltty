/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

/* scr_base.cc - screen drivers library
 *
 * Note: Although C++, this code requires no standard C++ library.
 * This is important as BRLTTY *must not* rely on too many
 * run-time shared libraries, nor be a huge executable.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "misc.h"
#include "scr.h"
#include "scr_base.h"

int
Screen::insert (unsigned short key) {
  return 0;
}


int
Screen::route (int column, int row, int screen) {
  return 0;
}


int
Screen::point (int column, int row) {
  return 0;
}

int
Screen::pointer (int &column, int &row) {
  return 0;
}


int
Screen::selectvt (int vt) {
  return 0;
}


int
Screen::switchvt (int vt) {
  return 0;
}


int
Screen::execute (int cmd) {
  return 0;
}
