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

const KeyboardProperties anyKeyboard = {
  .device = NULL,
  .type = KBD_TYPE_Any,
  .vendor = 0,
  .product = 0
};

int
parseKeyboardProperties (KeyboardProperties *properties, const char *parameters) {
  typedef enum {
    KBD_PARM_DEVICE,
    KBD_PARM_TYPE,
    KBD_PARM_VENDOR,
    KBD_PARM_PRODUCT
  } KeyboardParameter;

  static const char *const names[] = {"device", "type", "vendor", "product", NULL};
  char **values = getParameters(names, NULL, parameters);
  int ok = 1;

  logParameters(names, values, "Keyboard Property");
  *properties = anyKeyboard;

  if (*values[KBD_PARM_DEVICE]) {
    properties->device = values[KBD_PARM_DEVICE];
  }

  if (*values[KBD_PARM_TYPE]) {
    static const char *choices[] = {"any", "ps2", "usb", "bluetooth", NULL};
    unsigned int choice;

    if (validateChoice(&choice, values[KBD_PARM_TYPE], choices)) {
      properties->type = choice;
    } else {
      LogPrint(LOG_WARNING, "invalid keyboard type: %s", values[KBD_PARM_TYPE]);
      ok = 0;
    }
  }

  if (*values[KBD_PARM_VENDOR]) {
    static const int minimum = 0;
    static const int maximum = 0xFFFF;
    int value;

    if (validateInteger(&value, values[KBD_PARM_VENDOR], &minimum, &maximum)) {
      properties->vendor = value;
    } else {
      LogPrint(LOG_WARNING, "invalid keyboard vendor code: %s", values[KBD_PARM_VENDOR]);
      ok = 0;
    }
  }

  if (*values[KBD_PARM_PRODUCT]) {
    static const int minimum = 0;
    static const int maximum = 0xFFFF;
    int value;

    if (validateInteger(&value, values[KBD_PARM_PRODUCT], &minimum, &maximum)) {
      properties->product = value;
    } else {
      LogPrint(LOG_WARNING, "invalid keyboard product code: %s", values[KBD_PARM_PRODUCT]);
      ok = 0;
    }
  }

  deallocateStrings(values);
  return ok;
}

int
checkKeyboardProperties (const KeyboardProperties *actual, const KeyboardProperties *required) {
  if (!required) return 1;
  if (!actual)  actual = &anyKeyboard;

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
