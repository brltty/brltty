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

#include "ttb.h"
#include "ttb_internal.h"

static const unsigned char internalTextTableBytes[] = {
#include "text.auto.h"
};

static TextTable internalTextTable = {
  .header.bytes = internalTextTableBytes,
  .size = 0
};

TextTable *textTable = &internalTextTable;

static const void *
getTextTableItem (TextTable *table, TextTableOffset offset) {
  return &table->header.bytes[offset];
}

static const UnicodeGroupEntry *
getUnicodeGroupEntry (TextTable *table, wchar_t group) {
  TextTableOffset offset = table->header.fields->unicodeGroups[group];
  if (offset) return getTextTableItem(table, offset);
  return NULL;
}

static const UnicodePlainEntry *
getUnicodePlainEntry (TextTable *table, wchar_t plain) {
  const UnicodeGroupEntry *group = getUnicodeGroupEntry(table, (plain >> UNICODE_PLAIN_BITS));

  if (group) {
    TextTableOffset offset = group->plains[plain & (UNICODE_PLAINS_PER_GROUP - 1)];
    if (offset) return getTextTableItem(table, offset);
  }

  return NULL;
}

static const UnicodeRowEntry *
getUnicodeRowEntry (TextTable *table, wchar_t row) {
  const UnicodePlainEntry *plain = getUnicodePlainEntry(table, (row >> UNICODE_ROW_BITS));

  if (plain) {
    TextTableOffset offset = plain->rows[row & (UNICODE_ROWS_PER_PLAIN - 1)];
    if (offset) return getTextTableItem(table, offset);
  }

  return NULL;
}

static const UnicodeCellEntry *
getUnicodeCellEntry (TextTable *table, wchar_t cell) {
  const UnicodeRowEntry *row = getUnicodeRowEntry(table, (cell >> UNICODE_CELL_BITS));
  if (row) return &row->cells[cell & (UNICODE_CELLS_PER_ROW - 1)];
  return NULL;
}

unsigned char
convertCharacterToDots (TextTable *table, wchar_t character) {
  const wchar_t cellMask = UNICODE_CELLS_PER_ROW - 1;

  switch (character & ~cellMask) {
    case UNICODE_BRAILLE_ROW:
      return character & cellMask;

    case 0XF000:
      return table->header.fields->byteToDots[character & cellMask];

    default: {
      const UnicodeCellEntry *cell = getUnicodeCellEntry(table, character);
      if (cell) return cell->dots;
      return 0;
    }
  }
}

wchar_t
convertDotsToCharacter (TextTable *table, unsigned char dots) {
  return table->header.fields->dotsToCharacter[dots];
}
