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

#include "keys.h"
#include "misc.h"

int
checkKeyboardProperties (const KeyboardProperties *required, const KeyboardProperties *actual) {
  if (!required) return 1;

  if (required->device) {
    if (strcmp(required->device, actual->device) != 0)
      if (strcmp(required->device, locatePathName(actual->device)) != 0)
        return 0;
  }

  if (required->type != KBD_TYPE_Any) {
    if (required->type != actual->type) return 0;
  }

  if (required->vendor) {
    if (required->vendor != actual->vendor) return 0;
  }

  if (required->product) {
    if (required->product != actual->product) return 0;
  }

  return 1;
}
