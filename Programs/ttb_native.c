/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#include "prologue.h"

#include "misc.h"
#include "ttb.h"
#include "ttb_internal.h"
#include "ttb_compile.h"

static int
getByteOperand (DataFile *file, unsigned char *byte) {
  DataString string;
  const char *description = "local character";

  if (getDataString(file, &string, 1, description)) {
    if ((string.length == 1) && iswLatin1(string.characters[0])) {
      *byte = string.characters[0];
      return 1;
    } else {
      reportDataError(file, "invalid %s: %.*" PRIws,
                      description, string.length, string.characters);
    }
  }

  return 0;
}

static int
getCharacterOperand (DataFile *file, wchar_t *character) {
  DataString string;
  const char *description = "unicode character";

  if (getDataString(file, &string, 0, description)) {
    if (!(string.characters[0] & ~UNICODE_CHARACTER_MASK)) {
      *character = string.characters[0];
      return 1;
    } else {
      reportDataError(file, "invalid %s: %.*" PRIws,
                      description, string.length, string.characters);
    }
  }

  return 0;
}

static int
getDotsOperand (DataFile *file, unsigned char *dots) {
  if (findDataOperand(file, "cell")) {
    wchar_t character;

    if (getDataCharacter(file, &character)) {
      int noDots = 0;
      wchar_t enclosed = (character == WC_C('('))? WC_C(')'):
                         0;
      *dots = 0;

      if (!enclosed) {
        if (wcschr(WS_C("0"), character)) {
          noDots = 1;
        } else {
          ungetDataCharacters(file, 1);
        }
      }

      while (getDataCharacter(file, &character)) {
        int space = iswspace(character);

        if (enclosed) {
          if (character == enclosed) {
            enclosed = 0;
            break;
          }

          if (space) continue;
        } else if (space) {
          ungetDataCharacters(file, 1);
          break;
        }

        {
          int dot;

          if (noDots || !brlDotNumberToIndex(character, &dot)) {
            reportDataError(file, "invalid dot number: %.1" PRIws, &character);
            return 0;
          }

          {
            unsigned char bit = brlDotBits[dot];

            if (*dots & bit) {
              reportDataError(file, "duplicate dot number: %.1" PRIws, &character);
              return 0;
            }

            *dots |= bit;
          }
        }
      }

      if (enclosed) {
        reportDataError(file, "incomplete cell");
        return 0;
      }

      return 1;
    }
  }

  return 0;
}

static int
processCharOperands (DataFile *file, void *data) {
  TextTableData *ttd = data;
  wchar_t character;

  if (getCharacterOperand(file, &character)) {
    unsigned char dots;

    if (getDotsOperand(file, &dots)) {
      if (!setTextTableCharacter(ttd, character, dots)) return 0;
    }
  }

  return 1;
}

static int
processByteOperands (DataFile *file, void *data) {
  TextTableData *ttd = data;
  unsigned char byte;

  if (getByteOperand(file, &byte)) {
    unsigned char dots;

    if (getDotsOperand(file, &dots)) {
      if (!setTextTableByte(ttd, byte, dots)) return 0;
    }
  }

  return 1;
}

static int
processTextTableLine (DataFile *file, void *data) {
  static const DataProperty properties[] = {
    {.name=WS_C("char"), .processor=processCharOperands},
    {.name=WS_C("byte"), .processor=processByteOperands},
    {.name=WS_C("include"), .processor=processIncludeOperands},
    {.name=NULL, .processor=NULL}
  };

  return processPropertyOperand(file, properties, "text table directive", data);
}

TextTableData *
processTextTableStream (FILE *stream, const char *name) {
  return processTextTableLines(stream, name, processTextTableLine);
}

TextTable *
compileTextTable (const char *name) {
  TextTable *table = NULL;
  FILE *stream;

  if ((stream = openDataFile(name, "r", 0))) {
    TextTableData *ttd;

    if ((ttd = processTextTableStream(stream, name))) {
      table = makeTextTable(ttd);

      destroyTextTableData(ttd);
    }

    fclose(stream);
  }

  return table;
}
