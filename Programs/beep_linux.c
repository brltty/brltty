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

#include <sys/ioctl.h>
#include <linux/kd.h>

#include "log.h"
#include "device.h"
#include "beep.h"
#include "system_linux.h"

typedef struct BeepInterceptorStruct BeepInterceptor;
static BeepInterceptor *beepInterceptor = NULL;

#define BEEP_DIVIDEND 1193180

static inline BeepFrequency
getWaveLength (BeepFrequency frequency) {
  return frequency? (BEEP_DIVIDEND / frequency): 0;
}

static void
enableBeeps (void) {
  static unsigned char status = 0;

  installKernelModule("pcspkr", &status);
}

int
canBeep (void) {
  enableBeeps();
  return !!getConsole();
}

int
synchronousBeep (BeepFrequency frequency, BeepDuration duration) {
  return 0;
}

int
asynchronousBeep (BeepFrequency frequency, BeepDuration duration) {
  FILE *console = getConsole();

  if (console) {
    if (ioctl(fileno(console), KDMKTONE, ((duration << 0X10) | getWaveLength(frequency))) != -1) return 1;
    logSystemError("ioctl[KDMKTONE]");
  }

  return 0;
}

int
startBeep (BeepFrequency frequency) {
  FILE *console = getConsole();

  if (console) {
    if (ioctl(fileno(console), KIOCSOUND, getWaveLength(frequency)) != -1) return 1;
    logSystemError("ioctl[KIOCSOUND]");
  }

  return 0;
}

int
stopBeep (void) {
  return startBeep(0);
}

void
endBeep (void) {
}

#ifdef HAVE_LINUX_INPUT_H
#include <string.h>
#include <linux/input.h>

#include "async_wait.h"
#include "async_io.h"
#include "tune.h"

struct BeepInterceptorStruct {
  UinputObject *uinputObject;
  int fileDescriptor;
  AsyncHandle asyncHandle;

  FrequencyElement tune[4];
};

ASYNC_CONDITION_TESTER(testBeepInterceptionStopped) {
  BeepInterceptor *bi = data;
  return bi->fileDescriptor == -1;
}

static void
stopBeepInterception (BeepInterceptor *bi) {
  close(bi->fileDescriptor);
  bi->fileDescriptor = -1;
}

static void
playBell (BeepInterceptor *bi) {
  FrequencyElement *tune = bi->tune;

  *tune = *(FrequencyElement[]){
    FREQ_PLAY(40, 750),
    FREQ_PLAY(40, 800),
    FREQ_PLAY(80, 750),
    FREQ_STOP()
  };

  return tunePlayFrequencies(tune);
}

ASYNC_INPUT_CALLBACK(lxHandleSoundEvent) {
  BeepInterceptor *bi = parameters->data;
  static const char label[] = "beep interceptor";

  if (parameters->error) {
    logMessage(LOG_DEBUG, "%s read error: fd=%d: %s",
               label, bi->fileDescriptor, strerror(parameters->error));
    stopBeepInterception(bi);
  } else if (parameters->end) {
    logMessage(LOG_DEBUG, "%s end-of-file: fd=%d",
               label, bi->fileDescriptor);
    stopBeepInterception(bi);
  } else {
    const struct input_event *event = parameters->buffer;

    if (parameters->length >= sizeof(*event)) {
      switch (event->type) {
        case EV_SND: {
          int value = event->value;

          switch (event->code) {
            case SND_BELL:
              if (value) playBell(bi);
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

static BeepInterceptor *
newBeepInterceptor (void) {
  BeepInterceptor *bi;

  if ((bi = malloc(sizeof(*bi)))) {
    memset(bi, 0, sizeof(*bi));

    if ((bi->uinputObject = newUinputObject("beep-interceptor"))) {
      bi->fileDescriptor = getUinputFileDescriptor(bi->uinputObject);

      if (prepareUinputObject(bi->uinputObject)) {
        if (asyncReadFile(&bi->asyncHandle, bi->fileDescriptor,
                          sizeof(struct input_event),
                          lxHandleSoundEvent, bi)) {
          logMessage(LOG_DEBUG, "beep interceptor opened: fd=%d",
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
destroyBeepInterceptor (BeepInterceptor *bi) {
  asyncCancelRequest(bi->asyncHandle);
  destroyUinputObject(bi->uinputObject);
  free(bi);
}

int
canInterceptBeeps (void) {
  return 1;
}

int
startInterceptingBeeps (void) {
  if (!beepInterceptor) {
    if (!(beepInterceptor = newBeepInterceptor())) {
      return 0;
    }
  }

  return 1;
}

void
stopInterceptingBeeps (void) {
  if (beepInterceptor) {
    destroyBeepInterceptor(beepInterceptor);
    beepInterceptor = NULL;
  }
}

#else /* HAVE_LINUX_INPUT_H */
int
canInterceptBeeps (void) {
  return 0;
}

int
startInterceptingBeeps (void) {
  return 0;
}

void
stopInterceptingBeeps (void) {
}
#endif /* HAVE_LINUX_INPUT_H */
