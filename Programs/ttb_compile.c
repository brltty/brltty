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

#include "misc.h"
#include "datafile.h"
#include "dataarea.h"
#include "charset.h"
#include "ttb.h"
#include "ttb_internal.h"

#define BYTE_DEFINED 0X01

typedef struct {
  unsigned char flags;
} ByteData;

typedef struct {
  DataArea *area;
  ByteData bytes[BYTES_PER_CHARSET];
} TextTableData;

static inline TextTableHeader *
getTextTableHeader (TextTableData *ttd) {
  return getDataItem(ttd->area, 0);
}

static DataOffset
getUnicodeGroupOffset (wchar_t character, TextTableData *ttd) {
  unsigned int groupNumber = UNICODE_GROUP_NUMBER(character);
  DataOffset groupOffset = getTextTableHeader(ttd)->unicodeGroups[groupNumber];

  if (!groupOffset) {
    if (!allocateDataItem(ttd->area, &groupOffset, 
                          sizeof(UnicodeGroupEntry),
                          __alignof__(UnicodeGroupEntry)))
      return 0;

    getTextTableHeader(ttd)->unicodeGroups[groupNumber] = groupOffset;
  }

  return groupOffset;
}

static DataOffset
getUnicodePlainOffset (wchar_t character, TextTableData *ttd) {
  DataOffset groupOffset = getUnicodeGroupOffset(character, ttd);
  if (!groupOffset) return 0;

  {
    UnicodeGroupEntry *group = getDataItem(ttd->area, groupOffset);
    unsigned int plainNumber = UNICODE_PLAIN_NUMBER(character);
    DataOffset plainOffset = group->plains[plainNumber];

    if (!plainOffset) {
      if (!allocateDataItem(ttd->area, &plainOffset, 
                            sizeof(UnicodePlainEntry),
                            __alignof__(UnicodePlainEntry)))
        return 0;

      group = getDataItem(ttd->area, groupOffset);
      group->plains[plainNumber] = plainOffset;
    }

    return plainOffset;
  }
}

static DataOffset
getUnicodeRowOffset (wchar_t character, TextTableData *ttd) {
  DataOffset plainOffset = getUnicodePlainOffset(character, ttd);
  if (!plainOffset) return 0;

  {
    UnicodePlainEntry *plain = getDataItem(ttd->area, plainOffset);
    unsigned int rowNumber = UNICODE_ROW_NUMBER(character);
    DataOffset rowOffset = plain->rows[rowNumber];

    if (!rowOffset) {
      if (!allocateDataItem(ttd->area, &rowOffset, 
                            sizeof(UnicodeRowEntry),
                            __alignof__(UnicodeRowEntry)))
        return 0;

      plain = getDataItem(ttd->area, plainOffset);
      plain->rows[rowNumber] = rowOffset;
    }

    return rowOffset;
  }
}

static UnicodeRowEntry *
getUnicodeRowEntry (wchar_t character, TextTableData *ttd) {
  DataOffset rowOffset = getUnicodeRowOffset(character, ttd);
  if (!rowOffset) return NULL;
  return getDataItem(ttd->area, rowOffset);
}

static int
setByte (unsigned char byte, unsigned char dots, TextTableData *ttd) {
  TextTableHeader *header = getTextTableHeader(ttd);

  header->byteToDots[byte] = dots;
  return 1;
}

static int
setCharacter (wchar_t character, unsigned char dots, TextTableData *ttd) {
  UnicodeRowEntry *row = getUnicodeRowEntry(character, ttd);
  if (!row) return 0;

  {
    unsigned int cellNumber = UNICODE_CELL_NUMBER(character);
    row->cells[cellNumber].dots = dots;
    BITMASK_SET(row->defined, cellNumber);
  }

  getTextTableHeader(ttd)->dotsToCharacter[dots] = character;
  return 1;
}

static int
getByteOperand (DataFile *file, unsigned char *byte) {
  DataString string;
  const char *description = "local character";

  if (getDataString(file, &string, description)) {
    if ((string.length == 1) && (string.characters[0] < BYTES_PER_CHARSET)) {
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

  if (getDataString(file, &string, description)) {
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
processByteOperands (DataFile *file, void *data) {
  TextTableData *ttd = data;
  unsigned char byte;

  if (getByteOperand(file, &byte)) {
    unsigned char dots;

    if (getDotsOperand(file, &dots)) {
      if (!setByte(byte, dots, ttd)) return 0;

      {
        wint_t character = convertCharToWchar(byte);

        if (character != WEOF)
          if (!setCharacter(character, dots, ttd))
            return 0;
      }
    }
  }

  return 1;
}

static int
processCharOperands (DataFile *file, void *data) {
  TextTableData *ttd = data;
  wchar_t character;

  if (getCharacterOperand(file, &character)) {
    unsigned char dots;

    if (getDotsOperand(file, &dots)) {
      if (!setCharacter(character, dots, ttd)) return 0;

      {
        int byte = convertWcharToChar(character);

        if (byte != EOF)
          if (!setByte(byte, dots, ttd))
            return 0;
      }
    }
  }

  return 1;
}

static int
processTextTableLine (DataFile *file, void *data) {
  static const DataProperty properties[] = {
    {.name=WS_C("byte"), .processor=processByteOperands},
    {.name=WS_C("char"), .processor=processCharOperands},
    {.name=WS_C("include"), .processor=processIncludeOperands},
    {.name=NULL, .processor=processByteOperands}
  };

  return processPropertyOperand(file, properties, "text table directive", data);
}

TextTable *
compileTextTable (const char *name) {
  TextTable *table = NULL;
  TextTableData ttd;

  memset(&ttd, 0, sizeof(ttd));

  if ((ttd.area = newDataArea())) {
    if (allocateDataItem(ttd.area, NULL, sizeof(TextTableHeader), __alignof__(TextTableHeader))) {
      if (processDataFile(name, processTextTableLine, &ttd)) {
        if ((table = malloc(sizeof(*table)))) {
          table->header.fields = getTextTableHeader(&ttd);
          table->size = getDataSize(ttd.area);
          resetDataArea(ttd.area);
        }
      }
    }

    destroyDataArea(ttd.area);
  }

  return table;
}

void
destroyTextTable (TextTable *table) {
  if (table->size) {
    free(table->header.fields);
    free(table);
  }
}

void
fixTextTablePath (char **path) {
  fixPath(path, TEXT_TABLE_EXTENSION);
}
