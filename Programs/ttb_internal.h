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

#ifndef BRLTTY_INCLUDED_TTB_INTERNAL
#define BRLTTY_INCLUDED_TTB_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "bitmask.h"

typedef uint32_t TextTableOffset;

#define BYTES_PER_CHARSET 0X100

#define UNICODE_CELL_BITS 8
#define UNICODE_ROW_BITS 8
#define UNICODE_PLAIN_BITS 8
#define UNICODE_GROUP_BITS 7

#define UNICODE_CELL_SHIFT 0
#define UNICODE_ROW_SHIFT (UNICODE_CELL_SHIFT + UNICODE_CELL_BITS)
#define UNICODE_PLAIN_SHIFT (UNICODE_ROW_SHIFT + UNICODE_ROW_BITS)
#define UNICODE_GROUP_SHIFT (UNICODE_PLAIN_SHIFT + UNICODE_PLAIN_BITS)

#define UNICODE_CELLS_PER_ROW (1 << UNICODE_CELL_BITS)
#define UNICODE_ROWS_PER_PLAIN (1 << UNICODE_ROW_BITS)
#define UNICODE_PLAINS_PER_GROUP (1 << UNICODE_PLAIN_BITS)
#define UNICODE_GROUP_COUNT (1 << UNICODE_GROUP_BITS)

#define UNICODE_CELL_MASK ((UNICODE_CELLS_PER_ROW - 1) << UNICODE_CELL_SHIFT)
#define UNICODE_ROW_MASK ((UNICODE_ROWS_PER_PLAIN - 1) << UNICODE_ROW_SHIFT)
#define UNICODE_PLAIN_MASK ((UNICODE_PLAINS_PER_GROUP - 1) << UNICODE_PLAIN_SHIFT)
#define UNICODE_GROUP_MASK ((UNICODE_GROUP_COUNT - 1) << UNICODE_GROUP_SHIFT)
#define UNICODE_CHARACTER_MASK (UNICODE_CELL_MASK | UNICODE_ROW_MASK | UNICODE_PLAIN_MASK | UNICODE_GROUP_MASK)

#define UNICODE_CELL_NUMBER(c) (((c) & UNICODE_CELL_MASK) >> UNICODE_CELL_SHIFT)
#define UNICODE_ROW_NUMBER(c) (((c) & UNICODE_ROW_MASK) >> UNICODE_ROW_SHIFT)
#define UNICODE_PLAIN_NUMBER(c) (((c) & UNICODE_PLAIN_MASK) >> UNICODE_PLAIN_SHIFT)
#define UNICODE_GROUP_NUMBER(c) (((c) & UNICODE_GROUP_MASK) >> UNICODE_GROUP_SHIFT)

typedef struct {
  unsigned char dots;
} UnicodeCellEntry;

typedef struct {
  UnicodeCellEntry cells[UNICODE_CELLS_PER_ROW];
  BITMASK(defined, UNICODE_CELLS_PER_ROW, char);
} UnicodeRowEntry;

typedef struct {
  TextTableOffset rows[UNICODE_ROWS_PER_PLAIN];
} UnicodePlainEntry;

typedef struct {
  TextTableOffset plains[UNICODE_PLAINS_PER_GROUP];
} UnicodeGroupEntry;

typedef struct {
  unsigned char byteToDots[BYTES_PER_CHARSET];
  BITMASK(byteDotsDefined, BYTES_PER_CHARSET, char);

  unsigned char dotsToByte[0X100];
  BITMASK(dotsByteDefined, 0X100, char);

  TextTableOffset unicodeGroups[UNICODE_GROUP_COUNT];
  wchar_t dotsToCharacter[0X100];
  BITMASK(dotsCharacterDefined, 0X100, char);
} TextTableHeader;

struct TextTableStruct {
  union {
    TextTableHeader *fields;
    const unsigned char *bytes;
  } header;

  size_t size;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_TTB_INTERNAL */
