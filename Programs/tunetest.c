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

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

#include "log.h"
#include "options.h"
#include "tune.h"
#include "tune_utils.h"
#include "notes.h"
#include "parse.h"
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

static void
logProblem (const char *problem) {
  logMessage(LOG_ERR, "%s", problem);
}

static int
parseNumber (
  unsigned int *value, const char **operand,
  const unsigned int *minimum, const unsigned int *maximum,
  int required
) {
  if (isdigit(**operand)) {
    errno = 0;
    char *end;
    unsigned long ul = strtoul(*operand, &end, 10);

    if (errno) return 0;
    if (ul > UINT_MAX) return 0;

    if (minimum && (ul < *minimum)) return 0;
    if (maximum && (ul > *maximum)) return 0;

    *value = ul;
    *operand = end;
  } else if (required) {
    return 0;
  }

  return 1;
}

static int
parseDuration (const char **operand, int *duration) {
  switch (**operand) {
    case '@': {
      *operand += 1;

      static const unsigned int minimum = 1;
      static const unsigned int maximum = INT_MAX;
      unsigned int value;

      if (!parseNumber(&value, operand, &minimum, &maximum, 1)) {
        logProblem("invalid absolute duration");
        return 0;
      }

      *duration = value;
      break;
    }

    default:
      *duration = 256;
      break;
  }

  return 1;
}

static int
parseNote (const char **operand, unsigned char *note) {
  if (**operand == 'r') {
    *operand += 1;
    *note = 0;
  } else {
    const unsigned char lowestNote = getLowestNote();
    const unsigned char highestNote = getHighestNote();
    int noteNumber;

    static const char letters[] = "cdefgab";
    const char *letter = strchr(letters, **operand);

    if (letter && *letter) {
      static const unsigned char offsets[] = {0, 2, 4, 5, 7, 9, 11};

      noteNumber = NOTE_MIDDLE_C + offsets[letter - letters];
      *operand += 1;

      {
        static const unsigned int maximum = 9;
        static const unsigned int offset = 4;
        unsigned int octave = offset;

        if (!parseNumber(&octave, operand, NULL, &maximum, 0)) {
          logProblem("invalid octave");
          return 0;
        }

        noteNumber += ((int)octave - (int)offset) * NOTES_PER_OCTAVE;
      }
    } else {
      const unsigned int minimum = lowestNote;
      const unsigned int maximum = highestNote;
      unsigned int number;

      if (!parseNumber(&number, operand, &minimum, &maximum, 1)) {
        logProblem("invalid note");
        return 0;
      }

      noteNumber = number;
    }

    {
      char sign = **operand;
      int increment;

      switch (sign) {
        case '+':
          increment = 1;
          break;

        case '-':
          increment = -1;
          break;

        default:
          goto noSign;
      }

      do {
        noteNumber += increment;
      } while (*++*operand == sign);
    }
  noSign:

    if (noteNumber < lowestNote) {
      logProblem("note too low");
      return 0;
    }

    if (noteNumber > highestNote) {
      logProblem("note too high");
      return 0;
    }

    *note = noteNumber;
  }

  return 1;
}

static int
parseTone (const char *operand, unsigned char *note, int *duration) {
  if (!parseNote(&operand, note)) return 0;
  if (!parseDuration(&operand, duration)) return 0;
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
  if (!parseTuneDevice(opt_tuneDevice)) return PROG_EXIT_SYNTAX;
  if (!parseTuneVolume(opt_outputVolume)) return PROG_EXIT_SYNTAX;

#ifdef HAVE_MIDI_SUPPORT
  if (!parseTuneInstrument(opt_midiInstrument)) return PROG_EXIT_SYNTAX;
#endif /* HAVE_MIDI_SUPPORT */

  if (!argc) {
    logMessage(LOG_ERR, "missing tune");
    return PROG_EXIT_SYNTAX;
  }

  {
    FrequencyElement elements[argc + 1];

    {
      FrequencyElement *element = elements;

      while (argc) {
        unsigned char note;
        int duration;
        if (!parseTone(*argv, &note, &duration)) return PROG_EXIT_SYNTAX;

        {
          FrequencyElement tone = FREQ_PLAY(duration, GET_NOTE_FREQUENCY(note));
          *(element++) = tone;
        }

        argv += 1;
        argc -= 1;
      }

      {
        FrequencyElement tone = FREQ_STOP();
        *element = tone;
      }
    }

    if (!setTuneDevice()) return PROG_EXIT_SEMANTIC;
    tunePlayFrequencies(elements);
    tuneSynchronize();
  }

  return PROG_EXIT_SUCCESS;
}
