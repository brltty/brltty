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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

#include "log.h"
#include "tune_build.h"
#include "notes.h"

typedef unsigned int TuneNumber;

static void
logSyntaxError (TuneBuilder *tune, const char *message) {
  tune->status = TUNE_BUILD_SYNTAX;

  logMessage(LOG_ERR, "%s[%u]: %s: %s",
             tune->source.name, tune->source.index,
             message, tune->source.text);
}

int
addTone (TuneBuilder *tune, const FrequencyElement *tone) {
  if (tune->tones.count == tune->tones.size) {
    unsigned int newSize = tune->tones.size? (tune->tones.size << 1): 1;
    FrequencyElement *newArray;

    if (!(newArray = realloc(tune->tones.array, ARRAY_SIZE(newArray, newSize)))) {
      tune->status = TUNE_BUILD_FATAL;
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
  if (!duration) return 1;

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
  TuneBuilder *tune,
  TuneNumber *number, const char **operand, int required,
  const TuneNumber minimum, const TuneNumber maximum,
  const char *name
) {
  if (isdigit(**operand)) {
    errno = 0;
    char *end;
    unsigned long ul = strtoul(*operand, &end, 10);

    if (errno) goto invalidValue;
    if (ul > UINT_MAX) goto invalidValue;

    if (ul < minimum) goto invalidValue;
    if (ul > maximum) goto invalidValue;

    *number = ul;
    *operand = end;
  } else if (required) {
    goto invalidValue;
  }

  return 1;

invalidValue:
  if (name) {
    char message[0X80];
    snprintf(message, sizeof(message), "invalid %s", name);
    logSyntaxError(tune, message);
  }

  return 0;
}

static int
parseParameter (TuneBuilder *tune, TuneParameter *parameter, const char **operand) {
  TuneNumber number;
  int ok = parseNumber(tune, &number, operand, 1, parameter->minimum, parameter->maximum, parameter->name);
  if (ok) parameter->current = number;
  return ok;
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
    TuneNumber value;

    if (!parseNumber(tune, &value, operand, 1, 1, INT_MAX, "absolute duration")) {
      return 0;
    }

    *duration = value;
    *operand += 1;
  } else {
    TuneNumber multiplier;
    TuneNumber divisor;

    if (**operand == '*') {
      *operand += 1;

      if (!parseNumber(tune, &multiplier, operand, 1, 1, 16, "multiplier")) {
        return 0;
      }
    } else {
      multiplier = 1;
    }

    if (**operand == '/') {
      *operand += 1;

      if (!parseNumber(tune, &divisor, operand, 1, 1, 128, "divisor")) {
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
        static const TuneNumber offset = 4;
        TuneNumber octave = offset;

        if (!parseNumber(tune, &octave, operand, 0, 0, 9, "octave")) {
          return 0;
        }

        noteNumber += ((int)octave - (int)offset) * NOTES_PER_OCTAVE;
      }
    } else {
      TuneNumber number;

      if (!parseNumber(tune, &number, operand, 1, lowestNote, highestNote, "note")) {
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
      logSyntaxError(tune, "note too low");
      return 0;
    }

    if (noteNumber > highestNote) {
      logSyntaxError(tune, "note too high");
      return 0;
    }

    *note = noteNumber;
  }

  return 1;
}

static int
parseTone (TuneBuilder *tune, const char **operand) {
  unsigned char note;
  if (!parseNote(tune, operand, &note)) return 0;

  int duration;
  if (!parseDuration(tune, operand, &duration)) return 0;

  {
    int increment = duration;

    while (**operand == '.') {
      duration += (increment /= 2);
      *operand += 1;
    }
  }

  int onDuration = (duration * tune->percentage.current) / 100;
  if (!addNote(tune, note, onDuration)) return 0;
  if (!addNote(tune, 0, (duration - onDuration))) return 0;

  return 1;
}

static int
parseTuneOperand (TuneBuilder *tune, const char *operand) {
  tune->source.text = operand;

  switch (*operand) {
    case 'p':
      operand += 1;
      if (!parseParameter(tune, &tune->percentage, &operand)) return 0;
      break;

    case 't':
      operand += 1;
      if (!parseParameter(tune, &tune->tempo, &operand)) return 0;
      break;

    default:
      if (!parseTone(tune, &operand)) return 0;
      break;
  }

  if (*operand) {
    logSyntaxError(tune, "extra data");
    return 0;
  }

  return 1;
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

  tune->percentage.name = "percentage";
  tune->percentage.minimum = 1;
  tune->percentage.maximum = 100;
  tune->percentage.current = 80;

  tune->meter.denominator.name = "meter denominator";
  tune->meter.denominator.minimum = 2;
  tune->meter.denominator.maximum = 128;
  tune->meter.denominator.current = 4;

  tune->meter.numerator = tune->meter.denominator;
  tune->meter.numerator.name = "meter numerator";

  tune->source.text = "";
  tune->source.name = "";
  tune->source.index = 0;

  tune->status = TUNE_BUILD_OK;
}

void
resetTuneBuilder (TuneBuilder *tune) {
  if (tune->tones.array) free(tune->tones.array);
  initializeTuneBuilder(tune);
}
