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

/* tunetest.c - Test program for the tune playing library
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "options.h"
#include "tunes.h"
#include "misc.h"
#include "defaults.h"
#include "brltty.h"
#include "message.h"
#include "brl.h"

int updateInterval = DEFAULT_UPDATE_INTERVAL;
Preferences prefs;

static char *opt_tuneDevice;
static char *opt_outputVolume;

#ifdef ENABLE_PCM_SUPPORT
char *opt_pcmDevice;
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
char *opt_midiDevice;
static char *opt_midiInstrument;
#endif /* ENABLE_MIDI_SUPPORT */

BEGIN_OPTION_TABLE
  { .letter = 'd',
    .word = "device",
    .argument = "device",
    .setting.string = &opt_tuneDevice,
    .description = "Name of tune device."
  },

#ifdef ENABLE_MIDI_SUPPORT
  { .letter = 'i',
    .word = "instrument",
    .argument = "instrument",
    .setting.string = &opt_midiInstrument,
    .description = "Name of MIDI instrument."
  },
#endif /* ENABLE_MIDI_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
  { .letter = 'm',
    .word = "midi-device",
    .argument = "device",
    .setting.string = &opt_midiDevice,
    .description = "Device specifier for the Musical Instrument Digital Interface."
  },
#endif /* ENABLE_MIDI_SUPPORT */

#ifdef ENABLE_PCM_SUPPORT
  { .letter = 'p',
    .word = "pcm-device",
    .argument = "device",
    .setting.string = &opt_pcmDevice,
    .description = "Device specifier for soundcard digital audio."
  },
#endif /* ENABLE_PCM_SUPPORT */

  { .letter = 'v',
    .word = "volume",
    .argument = "loudness",
    .setting.string = &opt_outputVolume,
    .description = "Output volume (percentage)."
  },
END_OPTION_TABLE

static const char *deviceNames[] = {"beeper", "pcm", "midi", "fm", NULL};

#ifdef ENABLE_MIDI_SUPPORT
static int
validateInstrument (unsigned char *value, const char *string) {
  size_t stringLength = strlen(string);
  unsigned char instrument;
  for (instrument=0; instrument<midiInstrumentCount; ++instrument) {
    const char *component = midiInstrumentTable[instrument];
    size_t componentLeft = strlen(component);
    const char *word = string;
    size_t wordLeft = stringLength;
    {
      const char *delimiter = memchr(component, '(', componentLeft);
      if (delimiter) componentLeft = delimiter - component;
    }
    while (1) {
      while (*component == ' ') component++, componentLeft--;
      if ((componentLeft == 0) != (wordLeft == 0)) break; 
      if (!componentLeft) {
        *value = instrument;
        return 1;
      }
      {
        size_t wordLength = wordLeft;
        size_t componentLength = componentLeft;
        const char *delimiter;
        if ((delimiter = memchr(word, '-', wordLeft))) wordLength = delimiter - word;
        if ((delimiter = memchr(component, ' ', componentLeft))) componentLength = delimiter - component;
        if (strncasecmp(word, component, wordLength) != 0) break;
        word += wordLength; wordLeft -= wordLength;
        if (*word) word++, wordLeft--;
        component += componentLength; componentLeft -= componentLength;
      }
    }
  }
  return 0;
}
#endif /* ENABLE_MIDI_SUPPORT */

int
main (int argc, char *argv[]) {
  TuneDevice tuneDevice;
  unsigned char outputVolume;

#ifdef ENABLE_MIDI_SUPPORT
  unsigned char midiInstrument;
#endif /* ENABLE_MIDI_SUPPORT */

  processOptions(optionTable, optionCount,
                 "tunetest", &argc, &argv,
                 NULL, NULL, NULL,
                 "{note duration} ...");

  if (opt_tuneDevice && *opt_tuneDevice) {
    unsigned int device;
    if (!validateChoice(&device, opt_tuneDevice, deviceNames)) {
      LogPrint(LOG_ERR, "%s: %s", "invalid tune device", opt_tuneDevice);
      exit(2);
    }
    tuneDevice = device;
  } else {
    tuneDevice = getDefaultTuneDevice();
  }

#ifdef ENABLE_MIDI_SUPPORT
  if (opt_midiInstrument && *opt_midiInstrument) {
    if (!validateInstrument(&midiInstrument, opt_midiInstrument)) {
      LogPrint(LOG_ERR, "%s: %s", "invalid musical instrument", opt_midiInstrument);
      exit(2);
    }
  } else {
    midiInstrument = 0;
  }
#endif /* ENABLE_MIDI_SUPPORT */

  if (opt_outputVolume && *opt_outputVolume) {
    static const int minimum = 0;
    static const int maximum = 100;
    int volume;
    if (!validateInteger(&volume, opt_outputVolume, &minimum, &maximum)) {
      LogPrint(LOG_ERR, "%s: %s", "invalid volume percentage", opt_outputVolume);
      exit(2);
    }
    outputVolume = volume;
  } else {
    outputVolume = 50;
  }

  if (!argc) {
    LogPrint(LOG_ERR, "missing tune.");
    exit(2);
  }

  if (argc % 2) {
    LogPrint(LOG_ERR, "missing note duration.");
    exit(2);
  }

  {
    unsigned int count = argc / 2;
    TuneElement *elements = mallocWrapper((sizeof(*elements) * count) + 1);
    TuneElement *element = elements;

    while (argc) {
      int note;
      int duration;

      {
        static const int minimum = 0X01;
        static const int maximum = 0X7F;
        const char *argument = *argv++;
        if (!validateInteger(&note, argument, &minimum, &maximum)) {
          LogPrint(LOG_ERR, "%s: %s", "invalid note number", argument);
          exit(2);
        }
        --argc;
      }

      {
        static const int minimum = 1;
        static const int maximum = 255;
        const char *argument = *argv++;
        if (!validateInteger(&duration, argument, &minimum, &maximum)) {
          LogPrint(LOG_ERR, "%s: %s", "invalid note duration", argument);
          exit(2);
        }
        --argc;
      }

      {
        TuneElement te = TUNE_NOTE(duration, note);
        *(element++) = te;
      }
    }

    {
      TuneElement te = TUNE_STOP();
      *element = te;
    }

    if (!setTuneDevice(tuneDevice)) {
      LogPrint(LOG_ERR, "unsupported tune device: %s", deviceNames[tuneDevice]);
      exit(3);
    }

    memset(&prefs, 0, sizeof(prefs));
    prefs.alertMessages = 0;
    prefs.alertDots = 0;
    prefs.alertTunes = 1;
    switch (tuneDevice) {
      default:
        break;
      case tdPcm:
        prefs.pcmVolume = outputVolume;
        break;
      case tdMidi:
        prefs.midiVolume = outputVolume;
        break;
      case tdFm:
        prefs.fmVolume = outputVolume;
        break;
    }
#ifdef ENABLE_MIDI_SUPPORT
    prefs.midiInstrument = midiInstrument;
#endif /* ENABLE_MIDI_SUPPORT */
    {
      TuneDefinition tune = {NULL, 0, elements};
      playTune(&tune);
      closeTuneDevice(1);
    }

    free(elements);
  }
  return 0;
}

void
message (const char *text, short flags) {
}
void
showDotPattern (unsigned char dots, unsigned char duration) {
}
