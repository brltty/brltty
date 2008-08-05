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
#include "ttb_compile.h"

struct TextTableDataStruct {
  DataArea *area;
};

void *
getTextTableItem (TextTableData *ttd, TextTableOffset offset) {
  return getDataItem(ttd->area, offset);
}

TextTableHeader *
getTextTableHeader (TextTableData *ttd) {
  return getTextTableItem(ttd, 0);
}

static DataOffset
getUnicodeGroupOffset (TextTableData *ttd, wchar_t character, int allocate) {
  unsigned int groupNumber = UNICODE_GROUP_NUMBER(character);
  DataOffset groupOffset = getTextTableHeader(ttd)->unicodeGroups[groupNumber];

  if (!groupOffset && allocate) {
    if (!allocateDataItem(ttd->area, &groupOffset, 
                          sizeof(UnicodeGroupEntry),
                          __alignof__(UnicodeGroupEntry)))
      return 0;

    getTextTableHeader(ttd)->unicodeGroups[groupNumber] = groupOffset;
  }

  return groupOffset;
}

static DataOffset
getUnicodePlainOffset (TextTableData *ttd, wchar_t character, int allocate) {
  DataOffset groupOffset = getUnicodeGroupOffset(ttd, character, allocate);
  if (!groupOffset) return 0;

  {
    UnicodeGroupEntry *group = getDataItem(ttd->area, groupOffset);
    unsigned int plainNumber = UNICODE_PLAIN_NUMBER(character);
    DataOffset plainOffset = group->plains[plainNumber];

    if (!plainOffset && allocate) {
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
getUnicodeRowOffset (TextTableData *ttd, wchar_t character, int allocate) {
  DataOffset plainOffset = getUnicodePlainOffset(ttd, character, allocate);
  if (!plainOffset) return 0;

  {
    UnicodePlainEntry *plain = getDataItem(ttd->area, plainOffset);
    unsigned int rowNumber = UNICODE_ROW_NUMBER(character);
    DataOffset rowOffset = plain->rows[rowNumber];

    if (!rowOffset && allocate) {
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
getUnicodeRowEntry (TextTableData *ttd, wchar_t character, int allocate) {
  DataOffset rowOffset = getUnicodeRowOffset(ttd, character, allocate);
  if (!rowOffset) return NULL;
  return getDataItem(ttd->area, rowOffset);
}

const UnicodeCellEntry *
getUnicodeCellEntry (TextTableData *ttd, wchar_t character) {
  const UnicodeRowEntry *row = getUnicodeRowEntry(ttd, character, 0);

  if (row) {
    unsigned int cellNumber = UNICODE_CELL_NUMBER(character);
    if (BITMASK_TEST(row->defined, cellNumber)) return &row->cells[cellNumber];
  }

  return NULL;
}

int
setTextTableCharacter (TextTableData *ttd, wchar_t character, unsigned char dots) {
  UnicodeRowEntry *row = getUnicodeRowEntry(ttd, character, 1);
  if (!row) return 0;

  {
    unsigned int cellNumber = UNICODE_CELL_NUMBER(character);
    row->cells[cellNumber].dots = dots;
    BITMASK_SET(row->defined, cellNumber);
  }

  {
    TextTableHeader *header = getTextTableHeader(ttd);
    if (!BITMASK_TEST(header->dotsCharacterDefined, dots)) {
      header->dotsToCharacter[dots] = character;
      BITMASK_SET(header->dotsCharacterDefined, dots);
    }
  }

  return 1;
}

void
unsetTextTableCharacter (TextTableData *ttd, wchar_t character) {
  UnicodeRowEntry *row = getUnicodeRowEntry(ttd, character, 0);
  if (row) {
    unsigned int cellNumber = UNICODE_CELL_NUMBER(character);

    if (BITMASK_TEST(row->defined, cellNumber)) {
      UnicodeCellEntry *cell = &row->cells[cellNumber];
      TextTableHeader *header = getTextTableHeader(ttd);

      if (BITMASK_TEST(header->dotsCharacterDefined, cell->dots)) {
        if (header->dotsToCharacter[cell->dots] == character) {
          header->dotsToCharacter[cell->dots] = 0;
          BITMASK_CLEAR(header->dotsCharacterDefined, cell->dots);
        }
      }

      cell->dots = 0;
      BITMASK_CLEAR(row->defined, cellNumber);
    }
  }
}

int
setTextTableByte (TextTableData *ttd, unsigned char byte, unsigned char dots) {
  wint_t character = convertCharToWchar(byte);

  if (character != WEOF)
    if (!setTextTableCharacter(ttd, character, dots))
      return 0;

  return 1;
}

TextTableData *
newTextTableData (void) {
  TextTableData *ttd;

  if ((ttd = malloc(sizeof(*ttd)))) {
    memset(ttd, 0, sizeof(*ttd));

    if ((ttd->area = newDataArea())) {
      if (allocateDataItem(ttd->area, NULL, sizeof(TextTableHeader), __alignof__(TextTableHeader))) {
        return ttd;
      }

      destroyDataArea(ttd->area);
    }

    free(ttd);
  }

  return NULL;;
}

void
destroyTextTableData (TextTableData *ttd) {
  destroyDataArea(ttd->area);
  free(ttd);
}

TextTable *
makeTextTable (TextTableData *ttd) {
  TextTable *table = malloc(sizeof(*table));

  if (table) {
    table->header.fields = getTextTableHeader(ttd);
    table->size = getDataSize(ttd->area);
    resetDataArea(ttd->area);
  }

  return table;
}

TextTableData *
processTextTableStream (FILE *stream, const char *name, DataProcessor processor) {
  TextTableData *ttd;

  if ((ttd = newTextTableData())) {
    if (processDataStream(stream, name, processor, ttd)) return ttd;
    destroyTextTableData(ttd);
  }

  return NULL;
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
