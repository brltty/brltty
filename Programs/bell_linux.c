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

#include "bell.h"

#ifdef HAVE_LINUX_INPUT_H
#include <linux/input.h>

#include "system_linux.h"

static InputEventInterceptor *inputEventInterceptor = NULL;

static int
prepareUinputObject (UinputObject *uinput) {
  if (!enableUinputEventType(uinput, EV_SND)) return 0;
  if (!enableUinputSound(uinput, SND_BELL)) return 0;
  return 1;
}

static void
handleInputEvent (const InputEvent *event) {
  switch (event->type) {
    case EV_SND: {
      int value = event->value;

      switch (event->code) {
        case SND_BELL:
          if (value) alertBell();
          break;

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
canInterceptBell (void) {
  return 1;
}

int
startInterceptingBell (void) {
  if (!inputEventInterceptor) {
    InputEventInterceptor *interceptor = newInputEventInterceptor(
      "Console Bell Interceptor", prepareUinputObject, handleInputEvent
    );

    if (!interceptor) return 0;
    inputEventInterceptor = interceptor;
  }

  return 1;
}

void
stopInterceptingBell (void) {
  if (inputEventInterceptor) {
    destroyInputEventInterceptor(inputEventInterceptor);
    inputEventInterceptor = NULL;
  }
}

#else /* HAVE_LINUX_INPUT_H */
int
canInterceptBell (void) {
  return 0;
}

int
startInterceptingBell (void) {
  return 0;
}

void
stopInterceptingBell (void) {
}
#endif /* HAVE_LINUX_INPUT_H */
