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
#include "brl.h"
#include "touch.h"
#include "brltty.h"

static int touchLeft;
static int touchRight;
static int touchTop;
static int touchBottom;

int
touchGetRegion (int *left, int *right, int *top, int *bottom) {
  if ((touchLeft > touchRight) || (touchTop > touchBottom)) return 0;

  *left = touchLeft;
  *right = touchRight;
  *top = touchTop;
  *bottom = touchBottom;
  return 1;
}

static inline int
touchCheckColumn (int column) {
  int row;
  for (row=touchTop; row<=touchBottom; ++row) {
    if (brl.touchPressure[(row * brl.x) + column]) return 1;
  }
  return 0;
}

static inline int
touchCheckRow (int row) {
  int column;
  for (column=touchLeft; column<=touchRight; ++column) {
    if (brl.touchPressure[(row * brl.x) + column]) return 1;
  }
  return 0;
}

static inline int
touchCropLeft (void) {
  while (touchLeft <= touchRight) {
    if (touchCheckColumn(touchLeft)) return 1;
    ++touchLeft;
  }
  return 0;
}

static inline int
touchCropRight (void) {
  while (touchRight >= touchLeft) {
    if (touchCheckColumn(touchRight)) return 1;
    --touchRight;
  }
  return 0;
}

static inline int
touchCropTop (void) {
  while (touchTop <= touchBottom) {
    if (touchCheckRow(touchTop)) return 1;
    ++touchTop;
  }
  return 0;
}

static inline int
touchCropBottom (void) {
  while (touchBottom >= touchTop) {
    if (touchCheckRow(touchBottom)) return 1;
    --touchBottom;
  }
  return 0;
}

static inline int
touchCropWindow (void) {
  if (!touchCropRight()) {
    touchBottom = touchTop - 1;
    return 0;
  }

  touchCropLeft();
  touchCropTop();
  touchCropBottom();
  return 1;
}

static inline void
touchUncropWindow (void) {
  touchLeft = 0;
  touchRight = brl.x;
  touchTop = 0;
  touchBottom = brl.y;
}

static inline int
touchRecropWindow (void) {
  touchUncropWindow();
  return touchCropWindow();
}

int
touchAnalyzePressure (void) {
  LogBytes(LOG_DEBUG, "Touch Pressure", brl.touchPressure, brl.x*brl.y);
  touchRecropWindow();
  highlightWindow();
  return EOF;
}
