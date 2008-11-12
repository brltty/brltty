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

#ifndef BRLTTY_INCLUDED_UNICODE
#define BRLTTY_INCLUDED_UNICODE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define UNICODE_REPLACEMENT_CHARACTER 0XFFFD
#define UNICODE_BRAILLE_ROW 0X2800

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

#define UNICODE_CELL_MAXIMUM (UNICODE_CELLS_PER_ROW - 1)
#define UNICODE_ROW_MAXIMUM (UNICODE_ROWS_PER_PLAIN - 1)
#define UNICODE_PLAIN_MAXIMUM (UNICODE_PLAINS_PER_GROUP - 1)
#define UNICODE_GROUP_MAXIMUM (UNICODE_GROUP_COUNT - 1)

#define UNICODE_CELL_MASK (UNICODE_CELL_MAXIMUM << UNICODE_CELL_SHIFT)
#define UNICODE_ROW_MASK (UNICODE_ROW_MAXIMUM << UNICODE_ROW_SHIFT)
#define UNICODE_PLAIN_MASK (UNICODE_PLAIN_MAXIMUM << UNICODE_PLAIN_SHIFT)
#define UNICODE_GROUP_MASK (UNICODE_GROUP_MAXIMUM << UNICODE_GROUP_SHIFT)
#define UNICODE_CHARACTER_MASK (UNICODE_CELL_MASK | UNICODE_ROW_MASK | UNICODE_PLAIN_MASK | UNICODE_GROUP_MASK)

#define UNICODE_CELL_NUMBER(c) (((c) & UNICODE_CELL_MASK) >> UNICODE_CELL_SHIFT)
#define UNICODE_ROW_NUMBER(c) (((c) & UNICODE_ROW_MASK) >> UNICODE_ROW_SHIFT)
#define UNICODE_PLAIN_NUMBER(c) (((c) & UNICODE_PLAIN_MASK) >> UNICODE_PLAIN_SHIFT)
#define UNICODE_GROUP_NUMBER(c) (((c) & UNICODE_GROUP_MASK) >> UNICODE_GROUP_SHIFT)
#define UNICODE_CHARACTER(group,plain,row,cell) (((group) << UNICODE_GROUP_SHIFT) | ((plain) << UNICODE_PLAIN_SHIFT) | ((row) << UNICODE_ROW_SHIFT) | ((cell) << UNICODE_CELL_SHIFT))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_UNICODE */
