/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#include <stdio.h>

#include "log.h"
#include "file.h"
#include "charset.h"
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

static inline const void *
getTextTableItem (TextTable *table, TextTableOffset offset) {
  return &table->header.bytes[offset];
}

static inline const UnicodeGroupEntry *
getUnicodeGroupEntry (TextTable *table, wchar_t character) {
  TextTableOffset offset = table->header.fields->unicodeGroups[UNICODE_GROUP_NUMBER(character)];
  if (offset) return getTextTableItem(table, offset);
  return NULL;
}

static inline const UnicodePlaneEntry *
getUnicodePlaneEntry (TextTable *table, wchar_t character) {
  const UnicodeGroupEntry *group = getUnicodeGroupEntry(table, character);

  if (group) {
    TextTableOffset offset = group->planes[UNICODE_PLANE_NUMBER(character)];
    if (offset) return getTextTableItem(table, offset);
  }

  return NULL;
}

static inline const UnicodeRowEntry *
getUnicodeRowEntry (TextTable *table, wchar_t character) {
  const UnicodePlaneEntry *plane = getUnicodePlaneEntry(table, character);

  if (plane) {
    TextTableOffset offset = plane->rows[UNICODE_ROW_NUMBER(character)];
    if (offset) return getTextTableItem(table, offset);
  }

  return NULL;
}

static inline const unsigned char *
getUnicodeCellEntry (TextTable *table, wchar_t character) {
  const UnicodeRowEntry *row = getUnicodeRowEntry(table, character);

  if (row) {
    unsigned int cellNumber = UNICODE_CELL_NUMBER(character);
    if (BITMASK_TEST(row->defined, cellNumber)) return &row->cells[cellNumber];
  }

  return NULL;
}

typedef struct {
  TextTable *const table;
  unsigned char dots;
} SetBrailleRepresentationData;

static int
setBrailleRepresentation (wchar_t character, void *data) {
  SetBrailleRepresentationData *sbr = data;
  const unsigned char *cell = getUnicodeCellEntry(sbr->table, character);

  if (cell) {
    sbr->dots = *cell;
    return 1;
  }

  return 0;
}

unsigned char
convertCharacterToDots (TextTable *table, wchar_t character) {
  switch (character & ~UNICODE_CELL_MASK) {
    case UNICODE_BRAILLE_ROW:
      return character & UNICODE_CELL_MASK;

    case 0XF000: {
      wint_t wc = convertCharToWchar(character & UNICODE_CELL_MASK);
      if (wc == WEOF) goto unknownCharacter;
      character = wc;
    }

    default: {
      {
        SetBrailleRepresentationData sbr = {
          .table = table,
          .dots = 0
        };

        if (handleBestCharacter(character, setBrailleRepresentation, &sbr)) {
          return sbr.dots;
        }
      }

    unknownCharacter:
      {
        const unsigned char *cell;

        if ((cell = getUnicodeCellEntry(table, UNICODE_REPLACEMENT_CHARACTER))) return *cell;
        if ((cell = getUnicodeCellEntry(table, WC_C('?')))) return *cell;
      }

      return BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8;
    }
  }
}

wchar_t
convertDotsToCharacter (TextTable *table, unsigned char dots) {
  const TextTableHeader *header = table->header.fields;
  if (BITMASK_TEST(header->dotsCharacterDefined, dots)) return header->dotsToCharacter[dots];
  return UNICODE_REPLACEMENT_CHARACTER;
}

int
replaceTextTable (const char *directory, const char *name) {
  int ok = 0;
  char *path;

  if ((path = makeTextTablePath(directory, name))) {
    TextTable *newTable;

    logMessage(LOG_DEBUG, "compiling text table: %s", path);
    if ((newTable = compileTextTable(path))) {
      TextTable *oldTable = textTable;
      textTable = newTable;
      destroyTextTable(oldTable);
      ok = 1;
    } else {
      logMessage(LOG_ERR, "%s: %s", gettext("cannot compile text table"), path);
    }

    free(path);
  }

  if (!ok) logMessage(LOG_ERR, "%s: %s", gettext("cannot load text table"), name);
  return ok;
}
