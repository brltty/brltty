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
#include "bell.h"

#ifdef HAVE_LINUX_INPUT_H
#include <string.h>
#include <linux/input.h>

#include "system_linux.h"
#include "async_io.h"
#include "alert.h"

typedef struct {
  UinputObject *uinputObject;
  int fileDescriptor;
  AsyncHandle asyncHandle;
} BellInterceptor;

static BellInterceptor *bellInterceptor = NULL;

static void
stopBellInterception (BellInterceptor *bi) {
  close(bi->fileDescriptor);
  bi->fileDescriptor = -1;
}

ASYNC_INPUT_CALLBACK(lxHandleSoundEvent) {
  BellInterceptor *bi = parameters->data;
  static const char label[] = "bell interceptor";

  if (parameters->error) {
    logMessage(LOG_DEBUG, "%s read error: fd=%d: %s",
               label, bi->fileDescriptor, strerror(parameters->error));
    stopBellInterception(bi);
  } else if (parameters->end) {
    logMessage(LOG_DEBUG, "%s end-of-file: fd=%d",
               label, bi->fileDescriptor);
    stopBellInterception(bi);
  } else {
    const struct input_event *event = parameters->buffer;

    if (parameters->length >= sizeof(*event)) {
      switch (event->type) {
        case EV_SND: {
          int value = event->value;

          switch (event->code) {
            case SND_BELL:
              if (value) alert(ALERT_CONSOLE_BELL);
              break;

            default:
              break;
          }

          break;
        }

        default:
          break;
      }

      return sizeof(*event);
    }
  }

  return 0;
}

static int
prepareUinputObject (UinputObject *uinput) {
  if (!enableUinputEventType(uinput, EV_SND)) return 0;
  if (!enableUinputSound(uinput, SND_BELL)) return 0;
  return createUinputDevice(uinput);
}

static BellInterceptor *
newBellInterceptor (void) {
  BellInterceptor *bi;

  if ((bi = malloc(sizeof(*bi)))) {
    memset(bi, 0, sizeof(*bi));

    if ((bi->uinputObject = newUinputObject("Console Bell Interceptor"))) {
      bi->fileDescriptor = getUinputFileDescriptor(bi->uinputObject);

      if (prepareUinputObject(bi->uinputObject)) {
        if (asyncReadFile(&bi->asyncHandle, bi->fileDescriptor,
                          sizeof(struct input_event),
                          lxHandleSoundEvent, bi)) {
          logMessage(LOG_DEBUG, "bell interceptor opened: fd=%d",
                     bi->fileDescriptor);

          return bi;
        }
      }

      destroyUinputObject(bi->uinputObject);
    }

    free(bi);
  } else {
    logMallocError();
  }

  return NULL;
}

static void
destroyBellInterceptor (BellInterceptor *bi) {
  asyncCancelRequest(bi->asyncHandle);
  destroyUinputObject(bi->uinputObject);
  free(bi);
}

int
canInterceptBell (void) {
  return 1;
}

int
startInterceptingBell (void) {
  if (!bellInterceptor) {
    if (!(bellInterceptor = newBellInterceptor())) {
      return 0;
    }
  }

  return 1;
}

void
stopInterceptingBell (void) {
  if (bellInterceptor) {
    destroyBellInterceptor(bellInterceptor);
    bellInterceptor = NULL;
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
