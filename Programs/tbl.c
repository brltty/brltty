/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#include "misc.h"
#include "brl.h"
#include "tbl.h"

#define DOT_COUNT 8
static const unsigned char noDots[] = {'0'};
static const unsigned char dotNumbers[DOT_COUNT] = {'1', '2', '3', '4', '5', '6', '7', '8'};
static const unsigned char dotBits[DOT_COUNT] = {BRL_DOT1, BRL_DOT2, BRL_DOT3, BRL_DOT4, BRL_DOT5, BRL_DOT6, BRL_DOT7, BRL_DOT8};
#define DOT_BIT(dot) (dotBits[(dot)])

typedef struct {
  unsigned char cell;
  unsigned char defined;
} ByteEntry;

typedef struct {
  int options;
  unsigned ok:1;
  const char *file;
  int line;
  const unsigned char *location;
  ByteEntry bytes[0X100];
  unsigned char undefined;
  unsigned char masks[DOT_COUNT];
} InputData;

static void
reportError (InputData *input, const char *format, ...) {
  char message[0X100];

  {
    va_list args;
    va_start(args, format);
    formatInputError(message, sizeof(message),
                     input->file, (input->line? &input->line: NULL),
                     format, args);
    va_end(args);
  }

  LogPrint(LOG_ERR, "%s", message);
  input->ok = 0;
}

static void
putCell (unsigned char cell, char *buffer, int *count) {
  *count = 0;
  if (cell) {
    int dotIndex;
    for (dotIndex=0; dotIndex<DOT_COUNT; ++dotIndex)
      if (cell & DOT_BIT(dotIndex))
        buffer[(*count)++] = dotNumbers[dotIndex];
  } else {
    buffer[(*count)++] = noDots[0];
  }
}

static int
testWord (const unsigned char *location, int length, const char *word) {
  return (length == strlen(word)) && (strncasecmp((const char *)location, word, length) == 0);
}

static int
testCharacter (unsigned char character, unsigned char *index, const unsigned char *characters, unsigned char count) {
  const unsigned char *address = memchr(characters, character, count);
  if (!address) return 0;
  *index = address - characters;
  return 1;
}

static int
testDotNumber (unsigned char character, unsigned char *index) {
  return testCharacter(character, index, dotNumbers, sizeof(dotNumbers));
}

static int
testHexadecimalDigit (unsigned char character, unsigned char *index) {
  static const unsigned char digits[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
  };
  return testCharacter(character, index, digits, sizeof(digits));
}

static void
skipSpace (InputData *input) {
  while (*input->location && isspace(*input->location)) ++input->location;
}

static const unsigned char *
findSpace (InputData *input) {
  const unsigned char *location = input->location;
  while (*location && !isspace(*location)) ++location;
  return location;
}

static int
getBit (InputData *input, unsigned char *set, unsigned char *mask) {
  const unsigned char *location = input->location;
  int length = findSpace(input) - location;

  if (!*location) {
    reportError(input, "missing bit state");
    return 0;
  }

  *mask = 0;
  if (testWord(location, length, "on")) {
    *set = 1;
  } else if (testWord(location, length, "off")) {
    *set = 0;
  } else {
    static const unsigned char operators[] = {'=', '~'};
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
        {"bit01"    , 0X01},
        {"bit02"    , 0X02},
        {"bit04"    , 0X04},
        {"bit08"    , 0X08},
        {"bit10"    , 0X10},
        {"bit20"    , 0X20},
        {"bit40"    , 0X40},
        {"bit80"    , 0X80},
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
      reportError(input, "invalid bit state");
      return 0;
    }
  }

  input->location = location + length;
  return 1;
}

static int
getByte (InputData *input, unsigned char *byte) {
  const unsigned char *location = input->location;
  switch (*location++) {
    default:
      *byte = *(location - 1);
      break;

    case '\\':
      switch (*location++) {
        default:
          reportError(input, "invalid special byte");
          return 0;

        case 'X':
        case 'x': {
          int count = 2;
          *byte = 0;
          while (count--) {
            unsigned char digit;
            if (!testHexadecimalDigit(tolower(*location++), &digit)) {
              reportError(input, "invalid hexadecimal byte");
              return 0;
            }
            *byte = (*byte << 4) | digit;
          }
          break;
        }

        case '\\':
        case '#':
          *byte = *(location - 1);
          break;

        case 's':
          *byte = ' ';
          break;
      }
      break;

    case 0:
      reportError(input, "missing byte");
      return 0;
  }

  if (*location && !isspace(*location)) {
    reportError(input, "invalid byte");
    return 0;
  }

  input->location = location;
  return 1;
}

static int
getCell (InputData *input, unsigned char *cell) {
  const unsigned char *location = input->location;
  int none = 0;
  unsigned char enclosed = (*location == '(')? ')':
                           0;
  *cell = 0;

  if (enclosed) {
    ++location;
  } else if (!*location) {
    reportError(input, "missing cell");
    return 0;
  } else if (memchr(noDots, *location, sizeof(noDots))) {
    ++location;
    none = 1;
  }

  while (*location) {
    unsigned char character = *location++;
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
      if (none || !testDotNumber(character, &dot)) {
        reportError(input, "invalid dot number");
        return 0;
      }

      {
        unsigned char bit = DOT_BIT(dot);
        if (*cell & bit) {
          reportError(input, "duplicate dot number");
          return 0;
        }
        *cell |= bit;
      }
    }
  }

  if (enclosed) {
    reportError(input, "incomplete cell");
    return 0;
  }

  input->location = location;
  return 1;
}

static int
getDot (InputData *input, unsigned char *dot) {
  if (!*input->location) {
    reportError(input, "missing dot number");
    return 0;
  }

  if (!testDotNumber(*input->location, dot) ||
      ((findSpace(input) - input->location) != 1)) {
    reportError(input, "invalid dot number");
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
          reportError(input, "duplicate byte");
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
  input->location = (const unsigned char *)line;

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
setTable (InputData *input, TranslationTable table) {
  TranslationTable dotsDefined;
  memset(dotsDefined, 0, sizeof(TranslationTable));

  {
    int byteIndex;
    for (byteIndex=0; byteIndex<TRANSLATION_TABLE_SIZE; ++byteIndex) {
      ByteEntry *byte = &input->bytes[byteIndex];
      unsigned char *cell = &table[byteIndex];
      if (byte->defined) {
        *cell = byte->cell;

        if (!dotsDefined[byte->cell]) {
          dotsDefined[byte->cell] = 1;
        } else if (input->options & TBL_DUPLICATE) {
          char dotsBuffer[DOT_COUNT];
          int dotCount;
          putCell(byte->cell, dotsBuffer, &dotCount);
          reportError(input, "duplicate dots: %.*s [\\X%02X]", dotCount, dotsBuffer, byteIndex);
        }
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

        if (input->options & TBL_UNDEFINED) {
          reportError(input, "undefined byte: \\X%02X", byteIndex);
        }
      }
    }
  }

  if (input->options & TBL_UNUSED) {
    int cell;
    for (cell=0; cell<TRANSLATION_TABLE_SIZE; ++cell) {
      if (!dotsDefined[cell]) {
        char dotsBuffer[DOT_COUNT];
        int dotCount;
        putCell(cell, dotsBuffer, &dotCount);
        reportError(input, "unused dots: %.*s", dotCount, dotsBuffer);
      }
    }
  }
}

int
loadTranslationTable (
  const char *path,
  FILE *file,
  TranslationTable table,
  int options
) {
  int ok = 0;
  InputData input;
  int opened = file != NULL;

  memset(&input, 0, sizeof(input));
  input.options = options;
  input.ok = 1;
  input.file = path;
  input.line = 0;
  input.undefined = 0XFF;

  if (opened || (file = openDataFile(path, "r"))) {
    if (processLines(file, processTableLine, &input)) {
      input.line = 0;

      if (input.ok) {
        setTable(&input, table);
        ok = 1;
      }
    }

    if (!opened) fclose(file);
  } else {
    reportError(&input, "open error: %s", strerror(errno));
  }

  return ok;
}

void
reverseTranslationTable (TranslationTable from, TranslationTable to) {
  int byte;
  memset(to, 0, sizeof(TranslationTable));
  for (byte=TRANSLATION_TABLE_SIZE-1; byte>=0; byte--) to[from[byte]] = byte;
}

static void
fixTranslationTablePath (char **path, const char *prefix) {
  fixPath(path, TRANSLATION_TABLE_EXTENSION, prefix);
}

void
fixTextTablePath (char **path) {
  fixTranslationTablePath(path, TEXT_TABLE_PREFIX);
}

void
fixAttributesTablePath (char **path) {
  fixTranslationTablePath(path, ATTRIBUTES_TABLE_PREFIX);
}
