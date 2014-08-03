/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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
#include "program.h"
#include "parse.h"
#include "bitmask.h"

#include "keyboard.h"
#include "keyboard_internal.h"

const KeyboardProperties anyKeyboard = {
  .type = KBD_TYPE_Any,
  .vendor = 0,
  .product = 0
};

int
parseKeyboardProperties (KeyboardProperties *properties, const char *string) {
  enum {
    KBD_PARM_TYPE,
    KBD_PARM_VENDOR,
    KBD_PARM_PRODUCT
  };

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

void
claimKeyboardCommonData (KeyboardCommonData *kcd) {
  kcd->referenceCount += 1;
}

void
releaseKeyboardCommonData (KeyboardCommonData *kcd) {
  if (!(kcd->referenceCount -= 1)) {
    if (kcd->instanceQueue) deallocateQueue(kcd->instanceQueue);
    free(kcd);
  }
}

static void
logKeyCode (const char *action, int code, int press) {
  logMessage(LOG_CATEGORY(KEYBOARD_KEYS),
             "%s %d: %s",
             (press? "press": "release"), code, action);
}

static void
flushKeyEvents (KeyboardInstanceData *kid) {
  const KeyEventEntry *event = kid->events.buffer;

  while (kid->events.count) {
    logKeyCode("flushing", event->code, event->press);
    forwardKeyEvent(event->code, event->press);
    event += 1, kid->events.count -= 1;
  }

  memset(kid->deferred.mask, 0, kid->deferred.size);
}

KeyboardInstanceData *
newKeyboardInstanceData (KeyboardCommonData *kcd) {
  KeyboardInstanceData *kid;
  unsigned int count = BITMASK_ELEMENT_COUNT(keyCodeLimit, BITMASK_ELEMENT_SIZE(unsigned char));
  size_t size = sizeof(*kid) + count;

  if ((kid = malloc(size))) {
    memset(kid, 0, size);

    kid->kcd = NULL;
    kid->kpd = NULL;

    kid->actualProperties = anyKeyboard;

    kid->events.buffer = NULL;
    kid->events.size = 0;
    kid->events.count = 0;

    kid->justModifiers = 0;
    kid->deferred.size = count;

    if (enqueueItem(kcd->instanceQueue, kid)) {
      kid->kcd = kcd;
      claimKeyboardCommonData(kcd);

      return kid;
    }

    free(kid);
  } else {
    logMallocError();
  }

  return NULL;
}

void
deallocateKeyboardInstanceData (KeyboardInstanceData *kid) {
  flushKeyEvents(kid);
  if (kid->events.buffer) free(kid->events.buffer);

  deleteItem(kid->kcd->instanceQueue, kid);
  releaseKeyboardCommonData(kid->kcd);

  if (kid->kpd) deallocateKeyboardPlatformData(kid->kpd);
  free(kid);
}

static int
exitKeyboardInstanceData (void *item, void *data) {
  KeyboardInstanceData *kid = item;

  deallocateKeyboardInstanceData(kid);
  return 0;
}

static void
exitKeyboardMonitor (void *data) {
  KeyboardCommonData *kcd = data;

  kcd->isActive = 0;
  processQueue(kcd->instanceQueue, exitKeyboardInstanceData, NULL);
  releaseKeyboardCommonData(kcd);
}

int
startKeyboardMonitor (const KeyboardProperties *properties, KeyEventHandler handleKeyEvent) {
  KeyboardCommonData *kcd;

  if ((kcd = malloc(sizeof(*kcd)))) {
    memset(kcd, 0, sizeof(*kcd));

    kcd->referenceCount = 0;
    claimKeyboardCommonData(kcd);

    kcd->handleKeyEvent = handleKeyEvent;
    kcd->requiredProperties = *properties;

    if ((kcd->instanceQueue = newQueue(NULL, NULL))) {
      if (monitorKeyboards(kcd)) {
        kcd->isActive = 1;
        onProgramExit("keyboard-monitor", exitKeyboardMonitor, kcd);
        return 1;
      }

      deallocateQueue(kcd->instanceQueue);
    }

    releaseKeyboardCommonData(kcd);
  } else {
    logMallocError();
  }

  return 0;
}

void
handleKeyEvent (KeyboardInstanceData *kid, int code, int press) {
  KeyTableState state = KTS_UNBOUND;

  logKeyCode("received", code, press);

  if (kid->kcd->isActive) {
    if ((code >= 0) && (code < keyCodeLimit)) {
      const KeyValue *kv = &keyCodeMap[code];

      if ((kv->group != KBD_GROUP(SPECIAL)) || (kv->number != KBD_KEY(SPECIAL, Unmapped))) {
        if ((kv->group == KBD_GROUP(SPECIAL)) && (kv->number == KBD_KEY(SPECIAL, Ignore))) return;
        state = kid->kcd->handleKeyEvent(kv->group, kv->number, press);
      }
    }
  }

  if (state == KTS_HOTKEY) {
    logKeyCode("ignoring", code, press);
  } else {
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
        if (kid->events.count == kid->events.size) {
          unsigned int newSize = kid->events.size? kid->events.size<<1: 0X1;
          KeyEventEntry *newBuffer = realloc(kid->events.buffer, (newSize * sizeof(*newBuffer)));

          if (newBuffer) {
            kid->events.buffer = newBuffer;
            kid->events.size = newSize;
          } else {
            logMallocError();
          }
        }

        if (kid->events.count < kid->events.size) {
          KeyEventEntry *event = &kid->events.buffer[kid->events.count++];

          event->code = code;
          event->press = press;
          BITMASK_SET(kid->deferred.mask, code);

          logKeyCode("deferring", code, press);
        } else {
          logKeyCode("discarding", code, press);
        }
      }
    } else if (kid->justModifiers) {
      kid->justModifiers = 0;
      action = WKA_ALL;
    } else if (BITMASK_TEST(kid->deferred.mask, code)) {
      KeyEventEntry *to = kid->events.buffer;
      const KeyEventEntry *from = to;
      unsigned int count = kid->events.count;

      while (count) {
        if (from->code == code) {
          logKeyCode("dropping", from->code, from->press);
        } else if (to != from) {
          *to++ = *from;
        } else {
          to += 1;
        }

        from += 1, count -= 1;
      }

      kid->events.count = to - kid->events.buffer;
      BITMASK_CLEAR(kid->deferred.mask, code);
    } else {
      action = WKA_CURRENT;
    }

    switch (action) {
      case WKA_ALL:
        flushKeyEvents(kid);

      case WKA_CURRENT:
        logKeyCode("forwarding", code, press);
        forwardKeyEvent(code, press);

      case WKA_NONE:
        break;
    }
  }
}
