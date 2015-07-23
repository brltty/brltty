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
#include "tune.h"
#include "notes.h"
#include "midi.h"
#include "log.h"
#include "parse.h"
#include "defaults.h"
#include "prefs.h"

static char *opt_tuneDevice;
static char *opt_outputVolume;

#ifdef HAVE_MIDI_SUPPORT
static char *opt_midiInstrument;
#endif /* HAVE_MIDI_SUPPORT */

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

#ifdef HAVE_PCM_SUPPORT
  { .letter = 'p',
    .word = "pcm-device",
    .flags = OPT_Hidden,
    .argument = "device",
    .setting.string = &opt_pcmDevice,
    .description = "Device specifier for soundcard digital audio."
  },
#endif /* HAVE_PCM_SUPPORT */

#ifdef HAVE_MIDI_SUPPORT
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
#endif /* HAVE_MIDI_SUPPORT */
END_OPTION_TABLE

static const char *deviceNames[] = {"beeper", "pcm", "midi", "fm", NULL};

#ifdef HAVE_MIDI_SUPPORT
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
#endif /* HAVE_MIDI_SUPPORT */

static int
parseTone (const char *operand, int *note, int *duration) {
  const size_t operandSize = strlen(operand) + 1;
  char noteOperand[operandSize];
  char durationOperand[operandSize];

  int noteValue = NOTE_MIDDLE_C;
  int durationValue = 255;

  {
    const char *delimiter = strchr(operand, '/');

    if (delimiter) {
      const size_t length = delimiter - operand;

      memcpy(noteOperand, operand, length);
      noteOperand[length] = 0;

      strcpy(durationOperand, delimiter+1);
    } else {
      strcpy(noteOperand, operand);
      *durationOperand = 0;
    }
  }

  if (*noteOperand) {
    const char *c = noteOperand;

    static const char letters[] = "cdefgab";
    const char *letter = strchr(letters, *c);

    if (letter) {
      static const unsigned char offsets[] = {0, 2, 4, 5, 7, 9, 11};

      noteValue += offsets[letter - letters];
      c += 1;

      if (*c) {
        static const int minimum = -1;
        static const int maximum = 9;
        int octave = 4;

        if (!validateInteger(&octave, c, &minimum, &maximum)) {
          logMessage(LOG_ERR, "invalid octave: %s", c);
          return 0;
        }

        noteValue += (octave - 4) * NOTES_PER_OCTAVE;
      }
    } else {
      static const int minimum = 0;
      const int maximum = getMaximumNote();

      if (!validateInteger(&noteValue, c, &minimum, &maximum)) {
        logMessage(LOG_ERR, "invalid note: %s", noteOperand);
        return 0;
      }
    }
  }

  if ((noteValue < 0) || (noteValue > getMaximumNote())) {
    logMessage(LOG_ERR, "note out of range: %s", noteOperand);
    return 0;
  }

  if (*durationOperand) {
    static const int minimum = 1;
    int maximum = durationValue;

    if (!validateInteger(&durationValue, durationOperand, &minimum, &maximum)) {
      logMessage(LOG_ERR, "invalid duration: %s", durationOperand);
      return 0;
    }
  }

  *note = noteValue;
  *duration = durationValue;
  return 1;
}

int
main (int argc, char *argv[]) {
  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "tunetest",
      .argumentsSummary = "[note][/[duration]] ..."
    };
    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  resetPreferences();

  if (opt_tuneDevice && *opt_tuneDevice) {
    unsigned int device;

    if (!validateChoice(&device, opt_tuneDevice, deviceNames)) {
      logMessage(LOG_ERR, "%s: %s", "invalid tune device", opt_tuneDevice);
      return PROG_EXIT_SYNTAX;
    }

    prefs.tuneDevice = device;
  }

#ifdef HAVE_MIDI_SUPPORT
  if (opt_midiInstrument && *opt_midiInstrument) {
    unsigned char instrument;

    if (!validateInstrument(&instrument, opt_midiInstrument)) {
      logMessage(LOG_ERR, "%s: %s", "invalid musical instrument", opt_midiInstrument);
      return PROG_EXIT_SYNTAX;
    }

    prefs.midiInstrument = instrument;
  }
#endif /* HAVE_MIDI_SUPPORT */

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
    logMessage(LOG_ERR, "missing tune");
    return PROG_EXIT_SYNTAX;
  }

  {
    TuneElement elements[argc + 1];

    {
      TuneElement *element = elements;

      while (argc) {
        int note;
        int duration;

        if (!parseTone(*argv, &note, &duration)) {
          return PROG_EXIT_SYNTAX;
        }

        {
          TuneElement tone = TUNE_NOTE(duration, note);
          *(element++) = tone;
        }

        argv += 1;
        argc -= 1;
      }

      {
        TuneElement tone = TUNE_STOP();
        *element = tone;
      }
    }

    if (!setTuneDevice(prefs.tuneDevice)) {
      logMessage(LOG_ERR, "unsupported tune device: %s", deviceNames[prefs.tuneDevice]);
      return PROG_EXIT_SEMANTIC;
    }

    playTune(elements);
    closeTuneDevice();
  }

  return PROG_EXIT_SUCCESS;
}
