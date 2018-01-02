/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include "log.h"
#include "unicode.h"
#include "ascii.h"

#ifdef HAVE_ICU
#include <unicode/uchar.h>
#include <unicode/unorm.h>

static int
isUcharCompatible (wchar_t character) {
  UChar uc = character;

  return uc == character;
}
#endif /* HAVE_ICU */

#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif /* HAVE_ICONV_H */

int
getCharacterName (wchar_t character, char *buffer, size_t size) {
#ifdef HAVE_ICU
  UErrorCode error = U_ZERO_ERROR;

  u_charName(character, U_EXTENDED_CHAR_NAME, buffer, size, &error);
  if (U_SUCCESS(error) && *buffer) return 1;
#endif /* HAVE_ICU */

  return 0;
}

int
getCharacterByName (wchar_t *character, const char *name) {
#ifdef HAVE_ICU
  UErrorCode error = U_ZERO_ERROR;
  UChar uc = u_charFromName(U_EXTENDED_CHAR_NAME, name, &error);

  if (U_SUCCESS(error)) {
    *character = uc;
    return 1;
  }
#endif /* HAVE_ICU */

  return 0;
}

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

  /*  */
  if (character == 0XAD) return 1; /* soft hyphen */
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
  //if ((character >= 0X20000) && (character <= 0X2FFFF)) return 2;

    /* Tertiary Ideographic Plane */
  //if ((character >= 0X30000) && (character <= 0X3FFFF)) return 2;
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

int
isBrailleCharacter (wchar_t character) {
  return (character & ~UNICODE_CELL_MASK) == UNICODE_BRAILLE_ROW;
}

wchar_t
getBaseCharacter (wchar_t character) {
#ifdef HAVE_ICU
  if (isUcharCompatible(character)) {
    UChar source[] = {character};
    const unsigned int resultLength = 0X10;
    UChar resultBuffer[resultLength];
    UErrorCode error = U_ZERO_ERROR;

    unorm_normalize(source, ARRAY_COUNT(source),
                    UNORM_NFD, 0,
                    resultBuffer, resultLength,
                    &error);

    if (U_SUCCESS(error)) {
      return resultBuffer[0];
    }
  }
#endif /* HAVE_ICU */

  return 0;
}

wchar_t
getTransliteratedCharacter (wchar_t character) {
#ifdef HAVE_ICONV_H
  static iconv_t handle = NULL;
  if (!handle) handle = iconv_open("ASCII//TRANSLIT", "WCHAR_T");

  if (handle != (iconv_t)-1) {
    char *inputAddress = (char *)&character;
    size_t inputSize = sizeof(character);
    size_t outputSize = 0X10;
    char outputBuffer[outputSize];
    char *outputAddress = outputBuffer;

    if (iconv(handle, &inputAddress, &inputSize, &outputAddress, &outputSize) != (size_t)-1) {
      if ((outputAddress - outputBuffer) == 1) {
        return outputBuffer[0] & 0XFF;
      }
    }
  }
#endif /* HAVE_ICONV_H */

  return 0;
}

int
handleBestCharacter (wchar_t character, CharacterHandler handleCharacter, void *data) {
  if (isBrailleCharacter(character)) return 0;

  typedef wchar_t CharacterTranslator (wchar_t character);
  static CharacterTranslator *const characterTranslators[] = {
    getBaseCharacter,
    getTransliteratedCharacter,
    NULL
  };

  CharacterTranslator *const *translateCharacter = characterTranslators;
  while (!handleCharacter(character, data)) {
    if (!*translateCharacter) return 0;

    {
      wchar_t alternate = (*translateCharacter++)(character);
      if (alternate) character = alternate;
    }
  }

  return 1;
}
