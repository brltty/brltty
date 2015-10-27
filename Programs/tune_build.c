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
parseParameter (
  TuneBuilder *tune, TuneParameter *parameter,
  const char **operand, int required
) {
  return parseNumber(tune, &parameter->current, operand, required,
                     parameter->minimum, parameter->maximum, parameter->name);
}

static int
parseOptionalParameter (TuneBuilder *tune, TuneParameter *parameter, const char **operand) {
  return parseParameter(tune, parameter, operand, 0);
}

static int
parseRequiredParameter (TuneBuilder *tune, TuneParameter *parameter, const char **operand) {
  return parseParameter(tune, parameter, operand, 1);
}

static int
parseOctave (TuneBuilder *tune, const char **operand) {
  return parseRequiredParameter(tune, &tune->octave, operand);
}

static int
parsePercentage (TuneBuilder *tune, const char **operand) {
  return parseRequiredParameter(tune, &tune->percentage, operand);
}

static int
parseTempo (TuneBuilder *tune, const char **operand) {
  return parseRequiredParameter(tune, &tune->tempo, operand);
}

static void
setDuration (TuneBuilder *tune) {
  tune->duration.current = 60000 / tune->tempo.current;
}

static int
parseDuration (TuneBuilder *tune, const char **operand, int *duration) {
  if (**operand == '@') {
    *operand += 1;
    TuneParameter parameter = tune->duration;

    if (!parseRequiredParameter(tune, &parameter, operand)) {
      return 0;
    }

    *duration = parameter.current;
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
      divisor = 1;
    }

    *duration = tune->duration.current * multiplier / divisor;
  }

  tune->duration.current = *duration;

  {
    int increment = *duration;

    while (**operand == '.') {
      *duration += (increment /= 2);
      *operand += 1;
    }
  }

  return 1;
}

static TuneNumber
toOctave (TuneNumber note) {
  return note / NOTES_PER_OCTAVE;
}

static void
setOctave (TuneBuilder *tune) {
  tune->octave.current = toOctave(tune->note.current);
}

static int
parseNote (TuneBuilder *tune, const char **operand, unsigned char *note) {
  if (**operand == 'r') {
    *operand += 1;
    *note = 0;
  } else {
    int noOctave = 0;
    int noteNumber;

    static const char letters[] = "cdefgab";
    const char *letter = strchr(letters, **operand);

    if (letter && *letter) {
      *operand += 1;

      static const unsigned char offsets[] = {0, 2, 4, 5, 7, 9, 11};
      noteNumber = offsets[letter - letters];

      {
        const char *originalOperand = *operand;
        TuneParameter octave = tune->octave;

        if (!parseOptionalParameter(tune, &octave, operand)) {
          return 0;
        }

        noteNumber += octave.current * NOTES_PER_OCTAVE;
        if (*operand == originalOperand) noOctave = 1;
      }
    } else {
      TuneParameter parameter = tune->note;

      if (!parseRequiredParameter(tune, &parameter, operand)) {
        return 0;
      }

      noteNumber = parameter.current;
    }

    if (noOctave) {
      int adjustOctave = 0;
      TuneNumber previousNote = tune->note.current;
      TuneNumber currentNote = noteNumber;

      if (currentNote < previousNote) {
        currentNote += NOTES_PER_OCTAVE;
        if ((currentNote - previousNote) <= 3) adjustOctave = 1;
      } else if (currentNote > previousNote) {
        currentNote -= NOTES_PER_OCTAVE;
        if ((previousNote - currentNote) <= 3) adjustOctave = 1;
      }

      if (adjustOctave) noteNumber = currentNote;
    }

    tune->note.current = noteNumber;
    setOctave(tune);

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

    {
      const unsigned char lowestNote = getLowestNote();
      const unsigned char highestNote = getHighestNote();

      if (noteNumber < lowestNote) {
        logSyntaxError(tune, "note too low");
        return 0;
      }

      if (noteNumber > highestNote) {
        logSyntaxError(tune, "note too high");
        return 0;
      }
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

  if (note) {
    int onDuration = (duration * tune->percentage.current) / 100;
    if (!addNote(tune, note, onDuration)) return 0;
    duration -= onDuration;
  }

  return addNote(tune, 0, duration);
}

static int
parseTuneOperand (TuneBuilder *tune, const char *operand) {
  tune->source.text = operand;

  switch (*operand) {
    case 'o':
      operand += 1;
      if (!parseOctave(tune, &operand)) return 0;
      break;

    case 'p':
      operand += 1;
      if (!parsePercentage(tune, &operand)) return 0;
      break;

    case 't':
      operand += 1;
      if (!parseTempo(tune, &operand)) return 0;
      setDuration(tune);
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

int
endTune (TuneBuilder *tune) {
  FrequencyElement tone = FREQ_STOP();
  return addTone(tune, &tone);
}

static inline void
setParameter (
  TuneParameter *parameter, const char *name,
  TuneNumber minimum, TuneNumber maximum, TuneNumber current
) {
  parameter->name = name;
  parameter->minimum = minimum;
  parameter->maximum = maximum;
  parameter->current = current;
}

void
initializeTuneBuilder (TuneBuilder *tune) {
  memset(tune, 0, sizeof(*tune));

  tune->tones.array = NULL;
  tune->tones.size = 0;
  tune->tones.count = 0;

  setParameter(&tune->duration, "duration", 1, UINT16_MAX, 0);
  setParameter(&tune->note, "note", getLowestNote(), getHighestNote(), NOTE_MIDDLE_C);
  setParameter(&tune->octave, "octave", 0, 10, 0);
  setParameter(&tune->percentage, "percentage", 1, 100, 80);
  setParameter(&tune->tempo, "tempo", 40, UINT8_MAX, 108);

  setDuration(tune);
  setOctave(tune);

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
