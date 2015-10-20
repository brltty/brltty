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
  logMessage(LOG_ERR, "%s[%u]: %s: %s",
             tune->source.name, tune->source.index,
             problem, tune->source.text);
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

int
endTune (TuneBuilder *tune) {
  FrequencyElement tone = FREQ_STOP();
  return addTone(tune, &tone);
}

static int
parseNumber (
  unsigned int *value, const char **operand, int required,
  const unsigned int minimum, const unsigned int maximum
) {
  if (isdigit(**operand)) {
    errno = 0;
    char *end;
    unsigned long ul = strtoul(*operand, &end, 10);

    if (errno) return 0;
    if (ul > UINT_MAX) return 0;

    if (ul < minimum) return 0;
    if (ul > maximum) return 0;

    *value = ul;
    *operand = end;
  } else if (required) {
    return 0;
  }

  return 1;
}

static inline int
calculateToneDuration (TuneBuilder *tune, uint8_t multiplier, uint8_t divisor) {
  return (60000 * multiplier) / (tune->tempo.current * divisor);
}

static inline int
calculateNoteDuration (TuneBuilder *tune, uint8_t multiplier, uint8_t divisor) {
  return calculateToneDuration(tune, (tune->meter.denominator.current * multiplier), divisor);
}

static int
parseDuration (TuneBuilder *tune, const char **operand, int *duration) {
  if (**operand == '@') {
    unsigned int value;

    if (!parseNumber(&value, operand, 1, 1, INT_MAX)) {
      logTuneProblem(tune, "invalid absolute duration");
      return 0;
    }

    *duration = value;
    *operand += 1;
  } else {
    unsigned int multiplier;
    unsigned int divisor;

    if (**operand == '*') {
      *operand += 1;

      if (!parseNumber(&multiplier, operand, 1, 1, 16)) {
        logTuneProblem(tune, "invalid multiplier");
        return 0;
      }
    } else {
      multiplier = 1;
    }

    if (**operand == '/') {
      *operand += 1;

      if (!parseNumber(&divisor, operand, 1, 1, 128)) {
        logTuneProblem(tune, "invalid divisor");
        return 0;
      }
    } else {
      divisor = tune->meter.denominator.current;
    }

    *duration = calculateNoteDuration(tune, multiplier, divisor);
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
        static const unsigned int offset = 4;
        unsigned int octave = offset;

        if (!parseNumber(&octave, operand, 0, 0, 9)) {
          logTuneProblem(tune, "invalid octave");
          return 0;
        }

        noteNumber += ((int)octave - (int)offset) * NOTES_PER_OCTAVE;
      }
    } else {
      unsigned int number;

      if (!parseNumber(&number, operand, 1, lowestNote, highestNote)) {
        logTuneProblem(tune, "invalid note");
        return 0;
      }

      noteNumber = number;
    }

    {
      char accidental = **operand;
      int increment;

      switch (accidental) {
        case '+':
          increment = 1;
          break;

        case '-':
          increment = -1;
          break;

        default:
          goto noAccidental;
      }

      do {
        noteNumber += increment;
      } while (*++*operand == accidental);
    }
  noAccidental:

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
  tune->source.text = operand;
  return parseTone(tune, operand);
}

int
parseTuneLine (TuneBuilder *tune, const char *line) {
  tune->source.text = line;

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

void
initializeTuneBuilder (TuneBuilder *tune) {
  memset(tune, 0, sizeof(*tune));

  tune->tones.array = NULL;
  tune->tones.size = 0;
  tune->tones.count = 0;

  tune->tempo.name = "tempo (beats per minute)";
  tune->tempo.minimum = 40;
  tune->tempo.maximum = UINT8_MAX;
  tune->tempo.current = 108;

  tune->meter.denominator.name = "meter denominator";
  tune->meter.denominator.minimum = 2;
  tune->meter.denominator.maximum = 128;
  tune->meter.denominator.current = 4;

  tune->meter.numerator = tune->meter.denominator;
  tune->meter.numerator.name = "meter numerator";

  tune->source.text = "";
  tune->source.name = "";
  tune->source.index = 0;
}

void
resetTuneBuilder (TuneBuilder *tune) {
  if (tune->tones.array) free(tune->tones.array);
  initializeTuneBuilder(tune);
}
