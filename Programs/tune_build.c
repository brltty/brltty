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

#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

#include "log.h"
#include "tune_build.h"
#include "notes.h"

static void
logTuneProblem (TuneBuilder *tune, const char *problem) {
  logMessage(LOG_ERR, "%s[%u]: %s",
             tune->source.name, tune->source.index, problem);
}

int
addTone (TuneBuilder *tune, const FrequencyElement *tone) {
  if (tune->tones.count == tune->tones.size) {
    unsigned int newSize = tune->tones.size? (tune->tones.size << 1): 1;
    FrequencyElement *newArray;

    if (!(newArray = realloc(tune->tones.array, ARRAY_SIZE(newArray, newSize)))) {
      logMallocError();
      return 0;
    }

    tune->tones.array = newArray;
    tune->tones.size = newSize;
  }

  tune->tones.array[tune->tones.count++] = *tone;
  return 1;
}

int
addNote (TuneBuilder *tune, unsigned char note, int duration) {
  FrequencyElement tone = FREQ_PLAY(duration, GET_NOTE_FREQUENCY(note));
  return addTone(tune, &tone);
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
parseDuration (TuneBuilder *tune, const char **operand, int *duration) {
  switch (**operand) {
    case '@': {
      *operand += 1;

      static const unsigned int minimum = 1;
      static const unsigned int maximum = INT_MAX;
      unsigned int value;

      if (!parseNumber(&value, operand, &minimum, &maximum, 1)) {
        logTuneProblem(tune, "invalid absolute duration");
        return 0;
      }

      *duration = value;
      break;
    }

    case '/': {
      *operand += 1;

      static const unsigned int minimum = 1;
      static const unsigned int maximum = 128;
      unsigned int divisor;

      if (!parseNumber(&divisor, operand, &minimum, &maximum, 1)) {
        logTuneProblem(tune, "invalid divisor");
        return 0;
      }

      *duration = (60000 * tune->meter.denominator) / (tune->tempo * divisor);
      break;
    }

    case '*': {
      *operand += 1;

      static const unsigned int minimum = 1;
      static const unsigned int maximum = 16;
      unsigned int multiplier;

      if (!parseNumber(&multiplier, operand, &minimum, &maximum, 1)) {
        logTuneProblem(tune, "invalid multiplier");
        return 0;
      }

      *duration = (60000 * tune->meter.denominator * multiplier) / tune->tempo;
      break;
    }

    default:
      *duration = 60000 / tune->tempo;
      break;
  }

  return 1;
}

static int
parseNote (TuneBuilder *tune, const char **operand, unsigned char *note) {
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
          logTuneProblem(tune, "invalid octave");
          return 0;
        }

        noteNumber += ((int)octave - (int)offset) * NOTES_PER_OCTAVE;
      }
    } else {
      const unsigned int minimum = lowestNote;
      const unsigned int maximum = highestNote;
      unsigned int number;

      if (!parseNumber(&number, operand, &minimum, &maximum, 1)) {
        logTuneProblem(tune, "invalid note");
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
      logTuneProblem(tune, "note too low");
      return 0;
    }

    if (noteNumber > highestNote) {
      logTuneProblem(tune, "note too high");
      return 0;
    }

    *note = noteNumber;
  }

  return 1;
}

static int
parseTone (TuneBuilder *tune, const char *operand) {
  unsigned char note;
  if (!parseNote(tune, &operand, &note)) return 0;

  int duration;
  if (!parseDuration(tune, &operand, &duration)) return 0;

  {
    int increment = duration;

    while (*operand == '.') {
      duration += (increment /= 2);
      operand += 1;
    }
  }

  if (*operand) {
    logTuneProblem(tune, "extra data");
    return 0;
  }

  return addNote(tune, note, duration);
}

static int
parseTuneOperand (TuneBuilder *tune, const char *operand) {
  return parseTone(tune, operand);
}

int
parseTuneLine (TuneBuilder *tune, const char *line) {
  char buffer[strlen(line) + 1];
  strcpy(buffer, line);

  static const char delimiters[] = " \t\r\n";
  char *string = buffer;
  char *operand;

  while ((operand = strtok(string, delimiters))) {
    if (!parseTuneOperand(tune, operand)) return 0;
    string = NULL;
  }

  return 1;
}

static void
initializeTones (TuneBuilder *tune) {
  tune->tones.array = NULL;
  tune->tones.size = 0;
  tune->tones.count = 0;
}

void
constructTuneBuilder (TuneBuilder *tune) {
  memset(tune, 0, sizeof(*tune));
  initializeTones(tune);

  tune->tempo = 108;

  tune->meter.denominator = 4;

  tune->source.name = "";
  tune->source.index = 0;
}

void
destructTuneBuilder (TuneBuilder *tune) {
  if (tune->tones.array) free(tune->tones.array);
  initializeTones(tune);
}
