/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#include <string.h>
#include <strings.h>
#include <ctype.h>

#include "tbl.h"
#include "tbl_internal.h"

static int
testCharacter (unsigned char character, unsigned char *index, const unsigned char *characters, unsigned char count) {
  const unsigned char *address = memchr(characters, character, count);
  if (!address) return 0;
  *index = address - characters;
  return 1;
}

static int
testDotNumber (unsigned char character, unsigned char *index) {
  return testCharacter(character, index, tblDotNumbers, sizeof(tblDotNumbers));
}

static int
testHexadecimalDigit (unsigned char character, unsigned char *index) {
  static const unsigned char digits[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
  };
  return testCharacter(character, index, digits, sizeof(digits));
}

static int
getByte (TblInputData *input, unsigned char *byte) {
  const unsigned char *location = input->location;
  switch (*location++) {
    default:
      *byte = *(location - 1);
      break;

    case '\\':
      switch (*location++) {
        default:
          tblReportError(input, "invalid special byte");
          return 0;

        case 'X':
        case 'x': {
          int count = 2;
          *byte = 0;
          while (count--) {
            unsigned char digit;
            if (!testHexadecimalDigit(tolower(*location++), &digit)) {
              tblReportError(input, "invalid hexadecimal byte");
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
      tblReportError(input, "missing byte");
      return 0;
  }

  if (*location && !isspace(*location)) {
    tblReportError(input, "invalid byte");
    return 0;
  }

  input->location = location;
  return 1;
}

static int
getCell (TblInputData *input, unsigned char *cell) {
  const unsigned char *location = input->location;
  int none = 0;
  unsigned char enclosed = (*location == '(')? ')':
                           0;
  *cell = 0;

  if (enclosed) {
    ++location;
  } else if (!*location) {
    tblReportError(input, "missing cell");
    return 0;
  } else if (memchr(tblNoDots, *location, tblNoDotsSize)) {
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
        tblReportError(input, "invalid dot number");
        return 0;
      }

      {
        unsigned char bit = tblDotBit(dot);
        if (*cell & bit) {
          tblReportError(input, "duplicate dot number");
          return 0;
        }
        *cell |= bit;
      }
    }
  }

  if (enclosed) {
    tblReportError(input, "incomplete cell");
    return 0;
  }

  input->location = location;
  return 1;
}

static void
processByteDirective (TblInputData *input) {
  unsigned char index;
  tblSkipSpace(input);
  if (getByte(input, &index)) {
    unsigned char cell;
    tblSkipSpace(input);
    if (getCell(input, &cell)) {
      tblSkipSpace(input);
      if (tblIsEndOfLine(input)) {
        tblSetByte(input, index, cell);
      }
    }
  }
}

static int
processTableLine (TblInputData *input) {
  tblSkipSpace(input);
  if (!tblIsEndOfLine(input)) {
    typedef struct {
      const char *name;
      void (*handler) (TblInputData *input);
    } DirectiveEntry;
    static const DirectiveEntry directiveTable[] = {
      {"byte", processByteDirective},
      {NULL  , processByteDirective}
    };
    const DirectiveEntry *directive = directiveTable;
    int length = tblFindSpace(input) - input->location;

    while (directive->name) {
      if (tblTestWord(input->location, length, directive->name)) {
        input->location += length;
        break;
      }
      ++directive;
    }

    directive->handler(input);
  }

  return 1;
}

int
tblLoad_Native (const char *path, FILE *file, TranslationTable table, int options) {
  return tblProcessLines(path, file, table, processTableLine, options);
}
