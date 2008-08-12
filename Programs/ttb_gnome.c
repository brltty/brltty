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
#include "ttb_compile.h"

static int inUcsBlock;

static int
getUnicodeCharacter (DataFile *file, wchar_t *character, const char *description) {
  DataOperand string;

  if (getDataOperand(file, &string, description)) {
    if (string.length > 2) {
      if ((string.characters[0] == WC_C('U')) &&
          (string.characters[1] == WC_C('+'))) {
        const wchar_t *digit = &string.characters[2];
        int length = string.length - 2;

        *character = 0;
        while (length) {
          int value;
          int shift;
          if (!isHexadecimalDigit(*digit++, &value, &shift)) break;

          *character <<= shift;
          *character |= value;
          length -= 1;
        }
        if (!length) return 1;
      }
    }

    reportDataError(file, "invalid Unicode character: %.*" PRIws,
                    string.length, string.characters);
  }

  return 0;
}

static int
testBrailleRepresentation (DataFile *file, wchar_t representation, unsigned char *dots) {
  if ((representation & ~UNICODE_CELL_MASK) == UNICODE_BRAILLE_ROW) {
    *dots = representation & UNICODE_CELL_MASK;
    return 1;
  } else {
    reportDataError(file, "invalid braille representation");
  }

  return 0;
}

static int
processEncodingOperands (DataFile *file, void *data) {
  DataOperand encoding;

  if (getDataOperand(file, &encoding, "character encoding name")) {
    if (!isKeyword(WS_C("UTF-8"), encoding.characters, encoding.length)) {
      reportDataError(file, "unsupported character encoding: %.*" PRIws,
                      encoding.length, encoding.characters);
    }
  }

  return 1;
}

static int
processDelegateOperands (DataFile *file, void *data) {
  DataOperand type;

  if (getDataOperand(file, &type, "delegate type")) {
    if (isKeyword(WS_C("FILE"), type.characters, type.length)) {
      DataOperand name;

      if (getDataOperand(file, &name, "file name")) {
        return includeDataFile(file, name.characters, name.length);
      }
    } else {
      return includeDataFile(file, type.characters, type.length);
    }
  }

  return 1;
}

static int
processUcsBlockOperands (DataFile *file, void *data) {
  DataOperand action;

  if (getDataOperand(file, &action, "UCS block action")) {
    const wchar_t *expected = inUcsBlock? WS_C("END"): WS_C("START");

    if (isKeyword(expected, action.characters, action.length)) {
      inUcsBlock = !inUcsBlock;
    } else {
      reportDataError(file, "unexpected UCS block action: %.*" PRIws " (expecting %" PRIws ")",
                      action.length, action.characters, expected);
    }
  }

  return 1;
}

static int
processUcsCharOperands (DataFile *file, void *data) {
  TextTableData *ttd = data;
  DataOperand string;

  if (getDataOperand(file, &string, "character string")) {
    if (string.length == 1) {
      DataOperand representation;

      if (getDataOperand(file, &representation, "braille representation")) {
        if (representation.length == 1) {
          unsigned char dots;

          if (testBrailleRepresentation(file, representation.characters[0], &dots)) {
            if (!setTextTableCharacter(ttd, string.characters[0], dots)) return 0;
          }
        } else {
          reportDataError(file, "multi-cell braille representation not supported");
        }
      }
    } else {
      reportDataError(file, "multi-character string not supported");
    }
  }

  return 1;
}

static int
processUnicodeCharOperands (DataFile *file, void *data) {
  TextTableData *ttd = data;
  wchar_t character;

  if (getUnicodeCharacter(file, &character, "character")) {
    wchar_t representation;

    if (getUnicodeCharacter(file, &representation, "braille representation")) {
      unsigned char dots;

      if (testBrailleRepresentation(file, representation, &dots)) {
        if (!setTextTableCharacter(ttd, character, dots)) return 0;
      }
    }
  }

  return 1;
}

static int
processGnomeBrailleLine (DataFile *file, void *data) {
  const DataProperty *properties;

  if (inUcsBlock) {
    static const DataProperty propertyTable[] = {
      {.name=WC_C("UCS-BLOCK"), .processor=processUcsBlockOperands},
      {.name=NULL, .processor=processUcsCharOperands}
    };

    properties = propertyTable;
  } else {
    static const DataProperty propertyTable[] = {
      {.name=WC_C("ENCODING"), .processor=processEncodingOperands},
  //  {.name=WC_C("NAME"), .processor=processNameOperands},
  //  {.name=WC_C("LOCALES"), .processor=processLocalesOperands},
  //  {.name=WC_C("UCS-SUFFIX"), .processor=processUcsSuffixOperands},
      {.name=WC_C("DELEGATE"), .processor=processDelegateOperands},
  //  {.name=WC_C("UTF8-STRING"), .processor=processUtf8StringOperands},
      {.name=WC_C("UCS-BLOCK"), .processor=processUcsBlockOperands},
      {.name=WC_C("UCS-CHAR"), .processor=processUcsCharOperands},
      {.name=WC_C("UNICODE-CHAR"), .processor=processUnicodeCharOperands},
  //  {.name=WC_C("UNKNOWN-CHAR"), .processor=processUnknownCharOperands},
      {.name=NULL, .processor=NULL}
    };

    properties = propertyTable;
  }

  return processPropertyOperand(file, properties, "gnome braille directive", data);
}

TextTableData *
processGnomeBrailleStream (FILE *stream, const char *name) {
  TextTableData *ttd;

  inUcsBlock = 0;
  if ((ttd = processTextTableLines(stream, name, processGnomeBrailleLine))) {
    if (inUcsBlock) {
      reportDataError(NULL, "unterminated UCS block");
    }
  };

  return ttd;
}
