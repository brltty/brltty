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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "scr.h"
#include "scr_frozen.h"


FrozenScreen::FrozenScreen ()
{
  text = NULL;
  attributes = NULL;
}


int
FrozenScreen::open (Screen *source)
{
  source->describe(description);
  if ((text = new unsigned char[description.rows * description.cols])) {
    if ((attributes = new unsigned char[description.rows * description.cols])) {
      if (source->read((ScreenBox){0, 0, description.cols, description.rows}, text, SCR_TEXT)) {
        if (source->read((ScreenBox){0, 0, description.cols, description.rows}, attributes, SCR_ATTRIB)) {
          return 1;
        }
      }
      delete attributes;
      attributes = NULL;
    } else {
      LogError("Attributes buffer allocation");
    }
    delete text;
    text = NULL;
  } else {
    LogError("Text buffer allocation");
  }
  return 0;
}


void
FrozenScreen::close (void)
{
  if (text) {
    delete text;
    text = NULL;
  }
  if (attributes) {
    delete attributes;
    attributes = NULL;
  }
}


void
FrozenScreen::describe (ScreenDescription &desc)
{
  desc = description;
}


unsigned char *
FrozenScreen::read (ScreenBox box, unsigned char *buffer, ScreenMode mode)
{
  if ((box.left >= 0) && (box.width > 0) && ((box.left + box.width) <= description.cols) &&
      (box.top >= 0) && (box.height > 0) && ((box.top + box.height) <= description.rows)) {
    unsigned char *screen = (mode == SCR_TEXT)? text: attributes;
    for (int row=0; row<box.height; row++)
      memcpy(buffer + (row * box.width),
             screen + ((box.top + row) * description.cols) + box.left,
             box.width);
    return buffer;
  }
  return NULL;
}
