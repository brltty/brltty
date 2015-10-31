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

static const char noteLetters[] = "cdefgab";
static const unsigned char noteOffsets[] = {0, 2, 4, 5, 7, 9, 11};
static const signed char scaleAccidentals[] = {0, 2, 4, -1, 1, 3, 5};
static const unsigned char accidentalTable[] = {3, 0, 4, 1, 5, 2, 6};

typedef struct {
  const char *name;
  signed char accidentals;
} ModeEntry;

static const ModeEntry modeTable[] = {
  {.name="major", .accidentals=0},
  {.name="minor", .accidentals=-3},

  {.name="ionian", .accidentals=0},
  {.name="dorian", .accidentals=-2},
  {.name="phrygian", .accidentals=-4},
  {.name="lydian", .accidentals=1},
  {.name="mixolydian", .accidentals=-1},
  {.name="aeolian", .accidentals=-3},
  {.name="locrian", .accidentals=-5},
};
static const unsigned char modeCount = ARRAY_COUNT(modeTable);

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
  const char *problem = "invalid";

  if (isdigit(**operand)) {
    errno = 0;
    char *end;
    unsigned long ul = strtoul(*operand, &end, 10);

    if (errno) goto PROBLEM_ENCOUNTERED;
    if (ul > UINT_MAX) goto PROBLEM_ENCOUNTERED;

    if (ul < minimum) goto PROBLEM_ENCOUNTERED;
    if (ul > maximum) goto PROBLEM_ENCOUNTERED;

    *number = ul;
    *operand = end;
    return 1;
  }

  if (!required) return 1;
  problem = "missing";

PROBLEM_ENCOUNTERED:
  if (name) {
    char message[0X80];
    snprintf(message, sizeof(message), "%s %s", problem, name);
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
setCurrentDuration (TuneBuilder *tune, TuneNumber multiplier, TuneNumber divisor) {
  tune->duration.current = (60000 * multiplier) / (tune->tempo.current * divisor);
}

static void
setBaseDuration (TuneBuilder *tune) {
  setCurrentDuration(tune, 1, 1);
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
    const char *originalOperand = *operand;
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

    if (*operand != originalOperand) setCurrentDuration(tune, multiplier, divisor);
    *duration = tune->duration.current;
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

static void
setAccidentals (TuneBuilder *tune, int accidentals) {
  int quotient = accidentals / NOTES_PER_SCALE;
  int remainder = accidentals % NOTES_PER_SCALE;

  for (unsigned int index=0; index<ARRAY_COUNT(tune->accidentals); index+=1) {
    tune->accidentals[index] = quotient;
  }

  while (remainder > 0) {
    tune->accidentals[accidentalTable[--remainder]] += 1;
  }

  while (remainder < 0) {
    tune->accidentals[accidentalTable[NOTES_PER_SCALE + remainder++]] -= 1;
  }
}

static int
parseNoteLetter (unsigned char *index, const char **operand) {
  const char *letter = strchr(noteLetters, **operand);

  if (!letter) return 0;
  if (!*letter) return 0;

  *index = letter - noteLetters;
  *operand += 1;
  return 1;
}

static int
parseMode (TuneBuilder *tune, int *accidentals, const char **operand) {
  const char *from = *operand;
  if (!isalpha(*from)) return 1;

  const char *to = from;
  while (isalpha(*++to));
  unsigned int length = to - from;

  const ModeEntry *mode = NULL;
  const ModeEntry *current = modeTable;
  const ModeEntry *end = current + modeCount;

  while (current < end) {
    if (strncmp(current->name, from, length) == 0) {
      if (mode) {
        logSyntaxError(tune, "ambiguous mode");
        return 0;
      }

      mode = current;
    }

    current += 1;
  }

  if (!mode) {
    logSyntaxError(tune, "unrecognized mode");
    return 0;
  }

  *accidentals += mode->accidentals;
  *operand = to;
  return 1;
}

static int
parseKeySignature (TuneBuilder *tune, const char **operand) {
  int accidentals;
  int increment;

  {
    unsigned char index;

    if (parseNoteLetter(&index, operand)) {
      accidentals = scaleAccidentals[index];
      increment = NOTES_PER_SCALE;
      if (!parseMode(tune, &accidentals, operand)) return 0;
    } else {
      accidentals = 0;
      increment = 1;
    }
  }

  TuneNumber count = 0;
  if (!parseNumber(tune, &count, operand, 0, 1, NOTES_PER_OCTAVE-1, "accidental count")) {
    return 0;
  }

  int haveCount = count != 0;
  char accidental = **operand;

  switch (accidental) {
    case '-':
      increment = -increment;
    case '+':
      if (haveCount) {
        *operand += 1;
      } else {
        do {
          count += 1;
        } while (*++*operand == accidental);
      }
      break;

    default:
      if (!haveCount) break;
      logSyntaxError(tune, "accidental not specified");
      return 0;
  }

  accidentals += increment * count;
  setAccidentals(tune, accidentals);
  return 1;
}

static int
parseNote (TuneBuilder *tune, const char **operand, unsigned char *note) {
  int noteNumber;

  if (**operand == 'r') {
    *operand += 1;
    noteNumber = 0;
  } else {
    int defaultAccidentals = 0;

    if (**operand == 'n') {
      *operand += 1;
      TuneParameter parameter = tune->note;

      if (!parseRequiredParameter(tune, &parameter, operand)) {
        return 0;
      }

      noteNumber = parameter.current;
    } else {
      unsigned char noteIndex;

      if (parseNoteLetter(&noteIndex, operand)) {
        const char *originalOperand = *operand;
        TuneParameter octave = tune->octave;

        if (!parseOptionalParameter(tune, &octave, operand)) {
          return 0;
        }

        noteNumber = (octave.current * NOTES_PER_OCTAVE) + noteOffsets[noteIndex];
        defaultAccidentals = tune->accidentals[noteIndex];

        if (*operand == originalOperand) {
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
      } else {
        return 0;
      }
    }

    tune->note.current = noteNumber;
    setOctave(tune);

    {
      char accidental = **operand;

      switch (accidental) {
        {
          int increment;

        case '+':
          increment = 1;
          goto doAccidental;

        case '-':
          increment = -1;
          goto doAccidental;

        doAccidental:
          do {
            noteNumber += increment;
          } while (*++*operand == accidental);

          break;
        }

        case '=':
          *operand += 1;
          break;

        default:
          noteNumber += defaultAccidentals;
          break;
      }
    }

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
  }

  *note = noteNumber;
  return 1;
}

static int
parseTone (TuneBuilder *tune, const char **operand) {
  while (1) {
    unsigned char note;

    {
      const char *originalOperand = *operand;
      if (!parseNote(tune, operand, &note)) return *operand == originalOperand;
    }

    int duration;
    if (!parseDuration(tune, operand, &duration)) return 0;

    if (note) {
      int onDuration = (duration * tune->percentage.current) / 100;
      if (!addNote(tune, note, onDuration)) return 0;
      duration -= onDuration;
    }

    if (!addNote(tune, 0, duration)) return 0;
  }

  return 1;
}

static int
parseTuneOperand (TuneBuilder *tune, const char *operand) {
  tune->source.text = operand;

  switch (*operand) {
    case 'k':
      operand += 1;
      if (!parseKeySignature(tune, &operand)) return 0;
      break;

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
      setBaseDuration(tune);
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
    if (*operand == '#') break;
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
  tune->status = TUNE_BUILD_OK;

  tune->tones.array = NULL;
  tune->tones.size = 0;
  tune->tones.count = 0;

  setParameter(&tune->duration, "note duration", 1, UINT16_MAX, 0);
  setParameter(&tune->note, "MIDI note number", getLowestNote(), getHighestNote(), NOTE_MIDDLE_C+noteOffsets[2]);
  setParameter(&tune->octave, "octave number", 0, 10, 0);
  setParameter(&tune->percentage, "percentage", 1, 100, 80);
  setParameter(&tune->tempo, "tempo", 40, UINT8_MAX, (60 * 2));

  setAccidentals(tune, 0);
  setBaseDuration(tune);
  setOctave(tune);

  tune->source.text = "";
  tune->source.name = "";
  tune->source.index = 0;
}

void
resetTuneBuilder (TuneBuilder *tune) {
  if (tune->tones.array) free(tune->tones.array);
  initializeTuneBuilder(tune);
}
