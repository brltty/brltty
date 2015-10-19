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

/* tunetest.c - Test program for the tune playing library
 */

#include "prologue.h"

#include <string.h>

#ifdef HAVE_LINUX_INPUT_H
#include <linux/input.h>
#endif /* HAVE_LINUX_INPUT_H */

#include "log.h"
#include "options.h"
#include "tune.h"
#include "tune_utils.h"
#include "notes.h"
#include "midi.h"
#include "parse.h"
#include "prefs.h"
#include "async_io.h"
#include "async_wait.h"
#include "system_linux.h"

static char *opt_outputVolume;

BEGIN_OPTION_TABLE(programOptions)
  { .letter = 'v',
    .word = "volume",
    .argument = "loudness",
    .setting.string = &opt_outputVolume,
    .description = "Output volume (percentage)."
  },

#ifdef HAVE_PCM_SUPPORT
  { .letter = 'p',
    .word = "pcm-device",
    .flags = OPT_Hidden,
    .argument = "device",
    .setting.string = &opt_pcmDevice,
    .description = "Device specifier for soundcard digital audio."
  },
#endif /* HAVE_PCM_SUPPORT */
END_OPTION_TABLE

typedef struct {
  int fileDescriptor;
} SoundMonitorData;

ASYNC_CONDITION_TESTER(testSoundMonitorStopped) {
  SoundMonitorData *smd = data;
  return smd->fileDescriptor == -1;
}

static void
stopSoundMonitor (SoundMonitorData *smd) {
  close(smd->fileDescriptor);
  smd->fileDescriptor = -1;
}

ASYNC_INPUT_CALLBACK(handleSoundEvent) {
  SoundMonitorData *smd = parameters->data;
  static const char label[] = "sound";

  if (parameters->error) {
    logMessage(LOG_DEBUG, "%s read error: fd=%d: %s",
               label, smd->fileDescriptor, strerror(parameters->error));
    stopSoundMonitor(smd);
  } else if (parameters->end) {
    logMessage(LOG_DEBUG, "%s end-of-file: fd=%d",
               label, smd->fileDescriptor);
    stopSoundMonitor(smd);
  } else {
    const struct input_event *event = parameters->buffer;

    if (parameters->length >= sizeof(*event)) {
      switch (event->type) {
        case EV_SND: {
          int value = event->value;
          logMessage(LOG_NOTICE, "event: c=%d v=%d",event->code, value);

          switch (event->code) {
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
  if (!enableUinputSound(uinput, SND_TONE)) return 0;
  return createUinputDevice(uinput);
}

static int
startSoundMonitor (void) {
  const char *name = "tone-monitor";
  UinputObject *uinput;

  if ((uinput = newUinputObject(name))) {
    if (prepareUinputObject(uinput)) {
      SoundMonitorData smd = {
        .fileDescriptor = getUinputFileDescriptor(uinput)
      };

      if (asyncReadFile(NULL, smd.fileDescriptor, sizeof(struct input_event),
                        handleSoundEvent, &smd)) {
        logMessage(LOG_DEBUG, "tone monitor opened: fd=%d",
                   smd.fileDescriptor);

        asyncWaitFor(testSoundMonitorStopped, &smd);
      }
    }

    destroyUinputObject(uinput);
  }

  return 0;
}

int
main (int argc, char *argv[]) {
  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "beep2pcm"
    };
    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  resetPreferences();
  prefs.tuneDevice = tdPcm;

  if (!setOutputVolume(opt_outputVolume)) return PROG_EXIT_SYNTAX;

  if (argc) {
    logMessage(LOG_ERR, "too many parameters");
    return PROG_EXIT_SYNTAX;
  }

  if (!tuneDevice(prefs.tuneDevice)) {
    logMessage(LOG_ERR, "unsupported tune device: %s", tuneDeviceNames[prefs.tuneDevice]);
    return PROG_EXIT_SEMANTIC;
  }

  if (!startSoundMonitor()) return PROG_EXIT_FATAL;
  return PROG_EXIT_SUCCESS;
}
