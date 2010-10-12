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

#ifdef HAVE_ICU
#include <unicode/uchar.h>
#endif /* HAVE_ICU */

#include "unicode.h"
#include "ascii.h"

int
getCharacterWidth (wchar_t character) {
#if defined(HAVE_WCWIDTH)
  return wcwidth(character);
#elif defined(HAVE_ICU)
  UCharCategory category = u_getIntPropertyValue(character, UCHAR_GENERAL_CATEGORY);
  UEastAsianWidth width = u_getIntPropertyValue(character, UCHAR_EAST_ASIAN_WIDTH);

  if (character == 0) return 0;
  if (category == U_CONTROL_CHAR) return -1;

  if (category == U_NON_SPACING_MARK) return 0;
  if (category == U_ENCLOSING_MARK) return 0;

  /* Hangul Jamo medial vowels and final consonants */
  if ((character >= 0X1160) && (character <= 0X11FF) && (category == U_OTHER_LETTER)) return 0;

  if (character == 0XAD) return 1;
  if (category == U_FORMAT_CHAR) return 0;

  if (width == U_EA_FULLWIDTH) return 2;
  if (width == U_EA_HALFWIDTH) return 1;

  if (width == U_EA_WIDE) return 2;
  if (width == U_EA_NARROW) return 1;

  if (width == U_EA_AMBIGUOUS) {
    /* CJK Unified Ideographs block */
    if ((character >= 0X4E00) && (character <= 0X9FFF)) return 2;

    /* CJK Unified Ideographs Externsion A block */
    if ((character >= 0X3400) && (character <= 0X4DBF)) return 2;

    /* CJK Compatibility Ideographs block */
    if ((character >= 0XF900) && (character <= 0XFAFF)) return 2;

    /* Supplementary Ideographic Plane */
    if ((character >= 0X20000) && (character <= 0X2FFFF)) return 2;

    /* Tertiary Ideographic Plane */
    if ((character >= 0X30000) && (character <= 0X3FFFF)) return 2;
  }

  if (category == U_UNASSIGNED) return -1;
  return 1;
#else /* character width */
  if (character == NUL) return 0;
  if (character == DEL) return -1;
  if (!(character & 0X60)) return -1;
  return 1;
#endif /* character width */
}
