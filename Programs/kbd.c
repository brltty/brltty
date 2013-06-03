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

#include <string.h>

#include "log.h"
#include "parse.h"
#include "bitmask.h"

#include "kbd.h"
#include "kbd_internal.h"

const KeyboardProperties anyKeyboard = {
  .type = KBD_TYPE_Any,
  .vendor = 0,
  .product = 0
};

int
parseKeyboardProperties (KeyboardProperties *properties, const char *string) {
  typedef enum {
    KBD_PARM_TYPE,
    KBD_PARM_VENDOR,
    KBD_PARM_PRODUCT
  } KeyboardParameter;

  static const char *const names[] = {"type", "vendor", "product", NULL};
  char **parameters = getParameters(names, NULL, string);
  int ok = 1;

  logParameters(names, parameters, "Keyboard Property");
  *properties = anyKeyboard;

  if (*parameters[KBD_PARM_TYPE]) {
    static const KeyboardType types[] = {KBD_TYPE_Any, KBD_TYPE_PS2, KBD_TYPE_USB, KBD_TYPE_Bluetooth};
    static const char *choices[] = {"any", "ps2", "usb", "bluetooth", NULL};
    unsigned int choice;

    if (validateChoice(&choice, parameters[KBD_PARM_TYPE], choices)) {
      properties->type = types[choice];
    } else {
      logMessage(LOG_WARNING, "invalid keyboard type: %s", parameters[KBD_PARM_TYPE]);
      ok = 0;
    }
  }

  if (*parameters[KBD_PARM_VENDOR]) {
    static const int minimum = 0;
    static const int maximum = 0XFFFF;
    int value;

    if (validateInteger(&value, parameters[KBD_PARM_VENDOR], &minimum, &maximum)) {
      properties->vendor = value;
    } else {
      logMessage(LOG_WARNING, "invalid keyboard vendor code: %s", parameters[KBD_PARM_VENDOR]);
      ok = 0;
    }
  }

  if (*parameters[KBD_PARM_PRODUCT]) {
    static const int minimum = 0;
    static const int maximum = 0XFFFF;
    int value;

    if (validateInteger(&value, parameters[KBD_PARM_PRODUCT], &minimum, &maximum)) {
      properties->product = value;
    } else {
      logMessage(LOG_WARNING, "invalid keyboard product code: %s", parameters[KBD_PARM_PRODUCT]);
      ok = 0;
    }
  }

  deallocateStrings(parameters);
  return ok;
}

int
checkKeyboardProperties (const KeyboardProperties *actual, const KeyboardProperties *required) {
  if (!required) return 1;
  if (!actual)  actual = &anyKeyboard;

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

int
startKeyboardMonitor (const KeyboardProperties *properties, KeyEventHandler handleKeyEvent) {
  KeyboardCommonData *kcd;

  if ((kcd = malloc(sizeof(*kcd)))) {
    memset(kcd, 0, sizeof(*kcd));
    kcd->handleKeyEvent = handleKeyEvent;
    kcd->requiredProperties = *properties;

    if (monitorKeyboards(kcd)) {
      return 1;
    }

    free(kcd);
  } else {
    logMallocError();
  }

  return 0;
}

KeyboardInstanceData *
newKeyboardInstanceData (KeyboardCommonData *kcd) {
  KeyboardInstanceData *kid;
  unsigned int count = BITMASK_ELEMENT_COUNT(keyCodeLimit, BITMASK_ELEMENT_SIZE(unsigned char));
  size_t size = sizeof(*kid) + count;

  if ((kid = malloc(size))) {
    memset(kid, 0, size);

    kid->kcd = kcd;
    kid->actualProperties = anyKeyboard;

    kid->keyEventBuffer = NULL;
    kid->keyEventLimit = 0;
    kid->keyEventCount = 0;

    kid->justModifiers = 0;
    kid->handledKeysSize = count;

    return kid;
  } else {
    logMallocError();
  }

  return NULL;
}

void
deallocateKeyboardInstanceData (KeyboardInstanceData *kid) {
  if (kid->keyEventBuffer) free(kid->keyEventBuffer);
  free(kid);
}

void
handleKeyEvent (KeyboardInstanceData *kid, int code, int press) {
  int mapped = 0;
  KeyTableState state = KTS_UNBOUND;

  if ((code >= 0) && (code < keyCodeLimit)) {
    const KeyValue *kv = &keyCodeMap[code];

    if ((kv->set != KBD_SET_SPECIAL) || (kv->key != KBD_KEY_SPECIAL_Unmapped)) {
      mapped = 1;
      state = kid->kcd->handleKeyEvent(kv->set, kv->key, press);
    }
  }

  if (!mapped) {
    logMessage(LOG_INFO, "unmapped key code: %d", code);
    forwardKeyEvent(code, press);
  } else if (state != KTS_HOTKEY) {
    typedef enum {
      WKA_NONE,
      WKA_CURRENT,
      WKA_ALL
    } WriteKeysAction;
    WriteKeysAction action = WKA_NONE;

    if (press) {
      kid->justModifiers = state == KTS_MODIFIERS;

      if (state == KTS_UNBOUND) {
        action = WKA_ALL;
      } else {
        if (kid->keyEventCount == kid->keyEventLimit) {
          unsigned int newLimit = kid->keyEventLimit? kid->keyEventLimit<<1: 0X1;
          KeyEventEntry *newBuffer = realloc(kid->keyEventBuffer, (newLimit * sizeof(*newBuffer)));

          if (newBuffer) {
            kid->keyEventBuffer = newBuffer;
            kid->keyEventLimit = newLimit;
          } else {
            logMallocError();
          }
        }

        if (kid->keyEventCount < kid->keyEventLimit) {
          KeyEventEntry *event = &kid->keyEventBuffer[kid->keyEventCount++];

          event->code = code;
          event->press = press;

          BITMASK_SET(kid->handledKeysMask, code);
        }
      }
    } else if (kid->justModifiers) {
      kid->justModifiers = 0;
      action = WKA_ALL;
    } else if (BITMASK_TEST(kid->handledKeysMask, code)) {
      KeyEventEntry *to = kid->keyEventBuffer;
      const KeyEventEntry *from = to;
      unsigned int count = kid->keyEventCount;

      while (count) {
        if (from->code != code)
          if (to != from)
            *to++ = *from;

        from += 1, count -= 1;
      }

      kid->keyEventCount = to - kid->keyEventBuffer;
      BITMASK_CLEAR(kid->handledKeysMask, code);
    } else {
      action = WKA_CURRENT;
    }

    switch (action) {
      case WKA_ALL: {
        const KeyEventEntry *event = kid->keyEventBuffer;

        while (kid->keyEventCount) {
          forwardKeyEvent(event->code, event->press);
          event += 1, kid->keyEventCount -= 1;
        }

        memset(kid->handledKeysMask, 0, kid->handledKeysSize);
      }

      case WKA_CURRENT:
        forwardKeyEvent(code, press);

      case WKA_NONE:
        break;
    }
  }
}
