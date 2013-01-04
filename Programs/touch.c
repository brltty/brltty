/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>

#include "log.h"
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
touchCheckColumn (BrailleDisplay *brl, const unsigned char *pressure, int column) {
  int row;
  for (row=touchTop; row<=touchBottom; ++row) {
    if (pressure[(row * brl->textColumns) + column]) return 1;
  }
  return 0;
}

static inline int
touchCheckRow (BrailleDisplay *brl, const unsigned char *pressure, int row) {
  int column;
  for (column=touchLeft; column<=touchRight; ++column) {
    if (pressure[(row * brl->textColumns) + column]) return 1;
  }
  return 0;
}

static inline int
touchCropLeft (BrailleDisplay *brl, const unsigned char *pressure) {
  while (touchLeft <= touchRight) {
    if (touchCheckColumn(brl, pressure, touchLeft)) return 1;
    ++touchLeft;
  }
  return 0;
}

static inline int
touchCropRight (BrailleDisplay *brl, const unsigned char *pressure) {
  while (touchRight >= touchLeft) {
    if (touchCheckColumn(brl, pressure, touchRight)) return 1;
    --touchRight;
  }
  return 0;
}

static inline int
touchCropTop (BrailleDisplay *brl, const unsigned char *pressure) {
  while (touchTop <= touchBottom) {
    if (touchCheckRow(brl, pressure, touchTop)) return 1;
    ++touchTop;
  }
  return 0;
}

static inline int
touchCropBottom (BrailleDisplay *brl, const unsigned char *pressure) {
  while (touchBottom >= touchTop) {
    if (touchCheckRow(brl, pressure, touchBottom)) return 1;
    --touchBottom;
  }
  return 0;
}

static inline int
touchCropWindow (BrailleDisplay *brl, const unsigned char *pressure) {
  if (pressure) {
    if (touchCropRight(brl, pressure)) {
      touchCropLeft(brl, pressure);
      touchCropTop(brl, pressure);
      touchCropBottom(brl, pressure);
      return 1;
    }
  }

  touchBottom = touchTop - 1;
  return 0;
}

static inline void
touchUncropWindow (BrailleDisplay *brl) {
  touchLeft = textStart;
  touchRight = textStart + textCount - 1;
  touchTop = 0;
  touchBottom = brl->textRows - 1;
}

static inline int
touchRecropWindow (BrailleDisplay *brl, const unsigned char *pressure) {
  touchUncropWindow(brl);
  return touchCropWindow(brl, pressure);
}

int
touchAnalyzePressure (BrailleDisplay *brl, const unsigned char *pressure) {
  if (pressure) {
    logBytes(LOG_DEBUG, "Touch Pressure", pressure, brl->textColumns*brl->textRows);
  } else {
    logMessage(LOG_DEBUG, "Touch Pressure off");
  }

  touchRecropWindow(brl, pressure);
  brl->highlightWindow = 1;
  return EOF;
}

void
touchAnalyzeCells (BrailleDisplay *brl, const unsigned char *cells) {
}
