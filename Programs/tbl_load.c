/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#include "misc.h"
#include "brl.h"
#include "tbl_load.h"

static const unsigned char dotBits[] = {B1, B2, B3, B4, B5, B6, B7, B8};
#define DOT_BIT(dot) (dotBits[(dot)])
#define DOT_COUNT (sizeof(dotBits))

typedef struct {
  unsigned char cell;
  unsigned char defined;
} ByteEntry;

typedef struct {
  TranslationTableReporter report;
  unsigned ok:1;
  const char *file;
  int line;
  const char *location;
  ByteEntry bytes[0X100];
  unsigned char undefined;
  unsigned char masks[DOT_COUNT];
} InputData;

static void
reportError (InputData *input, const char *format, ...) {
  va_list args;
  char message[0X100];

  va_start(args, format);
  vsnprintf(message, sizeof(message), format, args);
  va_end(args);

  input->report(message);
  input->ok = 0;
}

static void
syntaxError (InputData *input, const char *problem) {
  reportError(input, "%s[%d]: %s", input->file, input->line, problem);
}

static int
testWord (const char *location, int length, const char *word) {
  return (length == strlen(word)) && (strncasecmp(location, word, length) == 0);
}

static int
testCharacter (char character, unsigned char *index, const char *characters, unsigned char count) {
  const char *address = memchr(characters, character, count);
  if (!address) return 0;
  *index = address - characters;
  return 1;
}

static int
testDotNumber (char character, unsigned char *index) {
  static const char dots[] = {'1', '2', '3', '4', '5', '6', '7', '8'};
  return testCharacter(character, index, dots, sizeof(dots));
}

static int
testHexadecimalDigit (char character, unsigned char *index) {
  static const char digits[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
  };
  return testCharacter(character, index, digits, sizeof(digits));
}

static void
skipSpace (InputData *input) {
  while (*input->location && isspace(*input->location)) ++input->location;
}

static const char *
findSpace (InputData *input) {
  const char *location = input->location;
  while (*location && !isspace(*location)) ++location;
  return location;
}

static int
getBit (InputData *input, unsigned char *set, unsigned char *mask) {
  const char *location = input->location;
  int length = findSpace(input) - location;

  if (!*location) {
    syntaxError(input, "missing bit state");
    return 0;
  }

  *mask = 0;
  if (testWord(location, length, "on")) {
    *set = 1;
  } else if (testWord(location, length, "off")) {
    *set = 0;
  } else {
    static const char operators[] = {'=', '~'};
    if (testCharacter(*input->location, set, operators, sizeof(operators))) {
      typedef struct {
        const char *name;
        unsigned char mask;
      } BitEntry;
      static const BitEntry bitTable[] = {
        {"fg-blue"  , 0X01},
        {"fg-green" , 0X02},
        {"fg-red"   , 0X04},
        {"fg-bright", 0X08},
        {"bg-blue"  , 0X10},
        {"bg-green" , 0X20},
        {"bg-red"   , 0X40},
        {"blink"    , 0X80},
        {NULL       , 0X00},
      };
      const BitEntry *bit = bitTable;

      ++location, --length;
      while (bit->name) {
        if (testWord(location, length, bit->name)) {
          *mask = bit->mask;
          break;
        }
        ++bit;
      }
    }

    if (!*mask) {
      syntaxError(input, "invalid bit state");
      return 0;
    }
  }

  input->location = location + length;
  return 1;
}

static int
getByte (InputData *input, unsigned char *byte) {
  const char *location = input->location;
  switch (*location++) {
    default:
      *byte = *(location - 1);
      break;

    case '\\':
      switch (*location++) {
        default:
          syntaxError(input, "invalid special byte");
          return 0;

        case 'X':
        case 'x': {
          int count = 2;
          *byte = 0;
          while (count--) {
            unsigned char digit;
            if (!testHexadecimalDigit(tolower(*location++), &digit)) {
              syntaxError(input, "invalid hexadecimal byte");
              return 0;
            }
            *byte = (*byte << 4) | digit;
          }
          break;
        }

        case '\\':
          *byte = *(location - 1);
          break;
      }
      break;

    case 0:
      syntaxError(input, "missing byte");
      return 0;
  }

  if (*location && !isspace(*location)) {
    syntaxError(input, "invalid byte");
    return 0;
  }

  input->location = location;
  return 1;
}

static int
getCell (InputData *input, unsigned char *cell) {
  const char *location = input->location;
  char enclosed = (*location == '(')? ')':
                  0;
  *cell = 0;

  if (enclosed) {
    ++location;
  } else if (!*location) {
    syntaxError(input, "missing cell");
    return 0;
  }

  while (*location) {
    char character = *location++;
    int space = isspace(character);

    if (enclosed) {
      if (character == enclosed) {
        enclosed = 0;
        break;
      }
      if (space) continue;
    } else if (space) {
      --location;
      break;
    }

    {
      unsigned char dot;
      if (!testDotNumber(character, &dot)) {
        syntaxError(input, "invalid dot number");
        return 0;
      }

      {
        unsigned char bit = DOT_BIT(dot);
        if (*cell & bit) {
          syntaxError(input, "duplicate dot number");
          return 0;
        }
        *cell |= bit;
      }
    }
  }

  if (enclosed) {
    syntaxError(input, "incomplete cell");
    return 0;
  }

  input->location = location;
  return 1;
}

static int
getDot (InputData *input, unsigned char *dot) {
  if (!*input->location) {
    syntaxError(input, "missing dot number");
    return 0;
  }

  if (!testDotNumber(*input->location, dot) ||
      ((findSpace(input) - input->location) != 1)) {
    syntaxError(input, "invalid dot number");
    return 0;
  }

  ++input->location;
  return 1;
}

static int
isEnd (InputData *input) {
  return !(*input->location && (*input->location != '#'));
}

static void
processByteDirective (InputData *input) {
  unsigned char index;
  skipSpace(input);
  if (getByte(input, &index)) {
    unsigned char cell;
    skipSpace(input);
    if (getCell(input, &cell)) {
      skipSpace(input);
      if (isEnd(input)) {
        ByteEntry *byte = &input->bytes[index];
        if (!byte->defined) {
          byte->cell = cell;
          byte->defined = 1;
        } else {
          syntaxError(input, "duplicate byte");
        }
      }
    }
  }
}

static void
processDotDirective (InputData *input) {
  unsigned char index;
  skipSpace(input);
  if (getDot(input, &index)) {
    unsigned char set;
    unsigned char mask;
    skipSpace(input);
    if (getBit(input, &set, &mask)) {
      skipSpace(input);
      if (isEnd(input)) {
        input->masks[index] = mask;
        mask = DOT_BIT(index);
        if (set) {
          input->undefined |= mask;
        } else {
          input->undefined &= ~mask;
        }
      }
    }
  }
}

static int
processTableLine (char *line, void *data) {
  InputData *input = data;

  input->line++;
  input->location = line;

  skipSpace(input);
  if (!isEnd(input)) {
    typedef struct {
      const char *name;
      void (*handler) (InputData *input);
    } DirectiveEntry;
    static const DirectiveEntry directiveTable[] = {
      {"byte", processByteDirective},
      {"dot" , processDotDirective },
      {NULL  , processByteDirective}
    };
    const DirectiveEntry *directive = directiveTable;
    int length = findSpace(input) - input->location;

    while (directive->name) {
      if (testWord(input->location, length, directive->name)) {
        input->location += length;
        break;
      }
      ++directive;
    }

    directive->handler(input);
  }

  return 1;
}

static void
setTable (InputData *input, TranslationTable *table) {
  int byteIndex;

  for (byteIndex=0; byteIndex<0X100; ++byteIndex) {
    ByteEntry *byte = &input->bytes[byteIndex];
    unsigned char *cell = &(*table)[byteIndex];
    if (byte->defined) {
      *cell = byte->cell;
    } else {
      int dotIndex;
      *cell = input->undefined;
      for (dotIndex=0; dotIndex<DOT_COUNT; ++dotIndex) {
        if (byteIndex & input->masks[dotIndex]) {
          unsigned char bit = DOT_BIT(dotIndex);
          if (input->undefined & bit) {
            *cell &= ~bit;
          } else {
            *cell |= bit;
          }
        }
      }
    }
  }
}

int
loadTranslationTable (
  const char *file,
  TranslationTable *table,
  TranslationTableReporter report
) {
  int ok = 0;
  InputData input;
  FILE *stream;

  memset(&input, 0, sizeof(input));
  input.report = report;
  input.ok = 1;
  input.file = file;
  input.line = 0;
  input.undefined = 0XFF;

  if ((stream = fopen(file, "r"))) {
    if (processLines(stream, processTableLine, &input)) {
      if (input.ok) {
        setTable(&input, table);
        ok = 1;
      }
    }

    fclose(stream);
  } else {
    reportError(&input, "Cannot open translation table '%s': %s",
                file, strerror(errno));
  }

  return ok;
}
