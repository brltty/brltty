/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include "log.h"
#include "leds.h"

#ifdef HAVE_LINUX_INPUT_H
#include <linux/input.h>

#include "system_linux.h"

static InputEventInterceptor *inputEventInterceptor = NULL;

static int
prepareUinputObject (UinputObject *uinput) {
  if (!enableUinputEventType(uinput, EV_LED)) return 0;
  if (!enableUinputLed(uinput, LED_NUML)) return 0;
  if (!enableUinputLed(uinput, LED_CAPSL)) return 0;
  if (!enableUinputLed(uinput, LED_SCROLLL)) return 0;
  return 1;
}

static void
handleInputEvent (const InputEvent *event) {
logMessage(LOG_NOTICE, "LED event: t=%u c=%u v=%d", event->type, event->code, event->value);
  switch (event->type) {
    case EV_LED: {
      switch (event->code) {
        default:
          break;
      }

      break;
    }

    default:
      break;
  }
}

int
canMonitorLeds (void) {
  return 1;
}

int
startMonitoringLeds (void) {
  if (!inputEventInterceptor) {
    InputEventInterceptor *interceptor = newInputEventInterceptor(
      "Keyboard LED Monitor", prepareUinputObject, handleInputEvent
    );

    if (!interceptor) return 0;
    inputEventInterceptor = interceptor;
  }

  return 1;
}

void
stopMonitoringLeds (void) {
  if (inputEventInterceptor) {
    destroyInputEventInterceptor(inputEventInterceptor);
    inputEventInterceptor = NULL;
  }
}

#else /* HAVE_LINUX_INPUT_H */
int
canMonitorLeds (void) {
  return 0;
}

int
startMonitoringLeds (void) {
  return 0;
}

void
stopMonitoringLeds (void) {
}
#endif /* HAVE_LINUX_INPUT_H */
