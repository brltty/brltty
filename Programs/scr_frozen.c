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
#include "scr_frozen.h"

static ScreenDescription screenDescription;
static unsigned char *screenText;
static unsigned char *screenAttributes;

static int
open_FrozenScreen (BaseScreen *source) {
  source->describe(&screenDescription);
  if ((screenText = calloc(screenDescription.rows*screenDescription.cols, sizeof(*screenText)))) {
    if ((screenAttributes = calloc(screenDescription.rows*screenDescription.cols, sizeof(*screenAttributes)))) {
      if (source->read((ScreenBox){0, 0, screenDescription.cols, screenDescription.rows}, screenText, SCR_TEXT)) {
        if (source->read((ScreenBox){0, 0, screenDescription.cols, screenDescription.rows}, screenAttributes, SCR_ATTRIB)) {
          return 1;
        }
      }
      free(screenAttributes);
      screenAttributes = NULL;
    } else {
      LogError("Attributes buffer allocation");
    }
    free(screenText);
    screenText = NULL;
  } else {
    LogError("Text buffer allocation");
  }
  return 0;
}

static void
close_FrozenScreen (void) {
  if (screenText) {
    free(screenText);
    screenText = NULL;
  }
  if (screenAttributes) {
    free(screenAttributes);
    screenAttributes = NULL;
  }
}

static void
describe_FrozenScreen (ScreenDescription *description) {
  *description = screenDescription;
}

static int
read_FrozenScreen (ScreenBox box, unsigned char *buffer, ScreenMode mode) {
  if (validateScreenBox(&box, screenDescription.cols, screenDescription.rows)) {
    unsigned char *screen = (mode == SCR_TEXT)? screenText: screenAttributes;
    int row;
    for (row=0; row<box.height; row++)
      memcpy(buffer + (row * box.width),
             screen + ((box.top + row) * screenDescription.cols) + box.left,
             box.width);
    return 1;
  }
  return 0;
}

static int
currentvt_FrozenScreen (void) {
  return screenDescription.number;
}

void
initializeFrozenScreen (FrozenScreen *frozen) {
  initializeBaseScreen(&frozen->base);
  frozen->base.describe = describe_FrozenScreen;
  frozen->base.read = read_FrozenScreen;
  frozen->base.currentvt = currentvt_FrozenScreen;
  frozen->open = open_FrozenScreen;
  frozen->close = close_FrozenScreen;
  screenText = NULL;
  screenAttributes = NULL;
}
