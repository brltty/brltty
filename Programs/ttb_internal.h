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
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
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
#include "unicode.h"

typedef uint32_t TextTableOffset;

#define CHARSET_BYTE_BITS 8
#define CHARSET_BYTE_COUNT (1 << CHARSET_BYTE_BITS)
#define CHARSET_BYTE_MAXIMUM (CHARSET_BYTE_COUNT - 1)

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
