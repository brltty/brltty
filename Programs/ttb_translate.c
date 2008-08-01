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
#include "brldots.h"

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
getUnicodeGroupEntry (TextTable *table, wchar_t character) {
  TextTableOffset offset = table->header.fields->unicodeGroups[UNICODE_GROUP_NUMBER(character)];
  if (offset) return getTextTableItem(table, offset);
  return NULL;
}

static const UnicodePlainEntry *
getUnicodePlainEntry (TextTable *table, wchar_t character) {
  const UnicodeGroupEntry *group = getUnicodeGroupEntry(table, character);

  if (group) {
    TextTableOffset offset = group->plains[UNICODE_PLAIN_NUMBER(character)];
    if (offset) return getTextTableItem(table, offset);
  }

  return NULL;
}

static const UnicodeRowEntry *
getUnicodeRowEntry (TextTable *table, wchar_t character) {
  const UnicodePlainEntry *plain = getUnicodePlainEntry(table, character);

  if (plain) {
    TextTableOffset offset = plain->rows[UNICODE_ROW_NUMBER(character)];
    if (offset) return getTextTableItem(table, offset);
  }

  return NULL;
}

static const UnicodeCellEntry *
getUnicodeCellEntry (TextTable *table, wchar_t character) {
  const UnicodeRowEntry *row = getUnicodeRowEntry(table, character);

  if (row) {
    unsigned int cellNumber = UNICODE_CELL_NUMBER(character);
    if (BITMASK_TEST(row->defined, cellNumber)) return &row->cells[cellNumber];
  }

  return NULL;
}

unsigned char
convertCharacterToDots (TextTable *table, wchar_t character) {
  switch (character & ~UNICODE_CELL_MASK) {
    case UNICODE_BRAILLE_ROW:
      return character & UNICODE_CELL_MASK;

    case 0XF000:
      return table->header.fields->byteToDots[character & UNICODE_CELL_MASK];

    default: {
      const UnicodeCellEntry *cell;

      if ((cell = getUnicodeCellEntry(table, character))) return cell->dots;
      if ((cell = getUnicodeCellEntry(table, UNICODE_REPLACEMENT_CHARACTER))) return cell->dots;
      if ((cell = getUnicodeCellEntry(table, WC_C('?')))) return cell->dots;
      return BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8;
    }
  }
}

wchar_t
convertDotsToCharacter (TextTable *table, unsigned char dots) {
  return table->header.fields->dotsToCharacter[dots];
}
