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

/* tunetest.c - Test program for the tune playing library
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "options.h"
#include "tunes.h"
#include "log.h"
#include "parse.h"
#include "defaults.h"
#include "prefs.h"
#include "message.h"
#include "brl.h"

int updateInterval = DEFAULT_UPDATE_INTERVAL;

static char *opt_tuneDevice;
static char *opt_outputVolume;

#ifdef ENABLE_PCM_SUPPORT
char *opt_pcmDevice;
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
char *opt_midiDevice;
static char *opt_midiInstrument;
#endif /* ENABLE_MIDI_SUPPORT */

BEGIN_OPTION_TABLE(programOptions)
  { .letter = 'v',
    .word = "volume",
    .argument = "loudness",
    .setting.string = &opt_outputVolume,
    .description = "Output volume (percentage)."
  },

  { .letter = 'd',
    .word = "device",
    .argument = "device",
    .setting.string = &opt_tuneDevice,
    .description = "Name of tune device."
  },

#ifdef ENABLE_PCM_SUPPORT
  { .letter = 'p',
    .word = "pcm-device",
    .flags = OPT_Hidden,
    .argument = "device",
    .setting.string = &opt_pcmDevice,
    .description = "Device specifier for soundcard digital audio."
  },
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
  { .letter = 'm',
    .word = "midi-device",
    .flags = OPT_Hidden,
    .argument = "device",
    .setting.string = &opt_midiDevice,
    .description = "Device specifier for the Musical Instrument Digital Interface."
  },

  { .letter = 'i',
    .word = "instrument",
    .argument = "instrument",
    .setting.string = &opt_midiInstrument,
    .description = "Name of MIDI instrument."
  },
#endif /* ENABLE_MIDI_SUPPORT */
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
  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "tunetest",
      .argumentsSummary = "{note duration} ..."
    };
    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  resetPreferences();
  prefs.alertMessages = 0;
  prefs.alertDots = 0;
  prefs.alertTunes = 1;

  if (opt_tuneDevice && *opt_tuneDevice) {
    unsigned int device;

    if (!validateChoice(&device, opt_tuneDevice, deviceNames)) {
      logMessage(LOG_ERR, "%s: %s", "invalid tune device", opt_tuneDevice);
      return PROG_EXIT_SYNTAX;
    }

    prefs.tuneDevice = device;
  }

#ifdef ENABLE_MIDI_SUPPORT
  if (opt_midiInstrument && *opt_midiInstrument) {
    unsigned char instrument;

    if (!validateInstrument(&instrument, opt_midiInstrument)) {
      logMessage(LOG_ERR, "%s: %s", "invalid musical instrument", opt_midiInstrument);
      return PROG_EXIT_SYNTAX;
    }

    prefs.midiInstrument = instrument;
  }
#endif /* ENABLE_MIDI_SUPPORT */

  if (opt_outputVolume && *opt_outputVolume) {
    static const int minimum = 0;
    static const int maximum = 100;
    int volume;

    if (!validateInteger(&volume, opt_outputVolume, &minimum, &maximum)) {
      logMessage(LOG_ERR, "%s: %s", "invalid volume percentage", opt_outputVolume);
      return PROG_EXIT_SYNTAX;
    }

    switch (prefs.tuneDevice) {
      case tdPcm:
        prefs.pcmVolume = volume;
        break;

      case tdMidi:
        prefs.midiVolume = volume;
        break;

      case tdFm:
        prefs.fmVolume = volume;
        break;

      default:
        break;
    }
  }

  if (!argc) {
    logMessage(LOG_ERR, "missing tune.");
    return PROG_EXIT_SYNTAX;
  }

  if (argc % 2) {
    logMessage(LOG_ERR, "missing note duration.");
    return PROG_EXIT_SYNTAX;
  }

  {
    unsigned int count = argc / 2;
    TuneElement *elements = malloc((sizeof(*elements) * count) + 1);

    if (elements) {
      TuneElement *element = elements;

      while (argc) {
        int note;
        int duration;

        {
          static const int minimum = 0X01;
          static const int maximum = 0X7F;
          const char *argument = *argv++;
          if (!validateInteger(&note, argument, &minimum, &maximum)) {
            logMessage(LOG_ERR, "%s: %s", "invalid note number", argument);
            return PROG_EXIT_SYNTAX;
          }
          --argc;
        }

        {
          static const int minimum = 1;
          static const int maximum = 255;
          const char *argument = *argv++;
          if (!validateInteger(&duration, argument, &minimum, &maximum)) {
            logMessage(LOG_ERR, "%s: %s", "invalid note duration", argument);
            return PROG_EXIT_SYNTAX;
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
    } else {
      logMallocError();
      return PROG_EXIT_FATAL;
    }

    if (!setTuneDevice(prefs.tuneDevice)) {
      logMessage(LOG_ERR, "unsupported tune device: %s", deviceNames[prefs.tuneDevice]);
      return PROG_EXIT_SEMANTIC;
    }

    {
      TuneDefinition tune = {NULL, 0, elements};
      playTune(&tune);
      closeTuneDevice(1);
    }

    free(elements);
  }
  return PROG_EXIT_SUCCESS;
}

int
message (const char *mode, const char *text, short flags) {
  return 1;
}
int
showDotPattern (unsigned char dots, unsigned char duration) {
  return 1;
}
