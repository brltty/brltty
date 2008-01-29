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

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#include "misc.h"
#include "charset.h"
#include "tbl.h"
#include "tbl_internal.h"

const unsigned char tblDotBits[TBL_DOT_COUNT] = {BRL_DOT1, BRL_DOT2, BRL_DOT3, BRL_DOT4, BRL_DOT5, BRL_DOT6, BRL_DOT7, BRL_DOT8};
const unsigned char tblDotNumbers[TBL_DOT_COUNT] = {'1', '2', '3', '4', '5', '6', '7', '8'};
const unsigned char tblNoDots[] = {'0'};
const unsigned char tblNoDotsSize = sizeof(tblNoDots);

static void
tblReportProblem (TblInputData *input, int level, const char *format, va_list args) {
  char message[0X100];
  formatInputError(message, sizeof(message),
                   input->file, (input->line? &input->line: NULL),
                   format, args);
  LogPrint(level, "%s", message);
}

void
tblReportError (TblInputData *input, const char *format, ...) {
  va_list args;
  va_start(args, format);
  tblReportProblem(input, LOG_ERR, format, args);
  va_end(args);
  input->ok = 0;
}

void
tblReportWarning (TblInputData *input, const char *format, ...) {
  va_list args;
  va_start(args, format);
  tblReportProblem(input, LOG_WARNING, format, args);
  va_end(args);
}

int
tblIsEndOfLine (TblInputData *input) {
  return !(*input->location && (*input->location != '#'));
}

void
tblSkipSpace (TblInputData *input) {
  while (*input->location && isspace(*input->location)) ++input->location;
}

const unsigned char *
tblFindSpace (TblInputData *input) {
  const unsigned char *location = input->location;
  while (*location && !isspace(*location)) ++location;
  return location;
}

int
tblTestWord (const unsigned char *location, int length, const char *word) {
  return (length == strlen(word)) && (strncasecmp((const char *)location, word, length) == 0);
}

void
tblSetByte (TblInputData *input, unsigned char index, unsigned char cell) {
  TblByteEntry *byte = &input->bytes[index];
  if (!byte->defined) {
    byte->cell = cell;
    byte->defined = 1;
  } else {
    tblReportError(input, "duplicate byte");
  }
}

size_t
tblPutDots (unsigned char dots, TblDotsBuffer buffer) {
  size_t count = 0;
  if (dots) {
    int dot;
    for (dot=0; dot<TBL_DOT_COUNT; ++dot)
      if (dots & tblDotBit(dot))
        buffer[count++] = tblDotNumbers[dot];
  } else {
    buffer[count++] = tblNoDots[0];
  }
  buffer[count] = 0;
  return count;
}

void
tblSetTable (TblInputData *input, TranslationTable table) {
  TranslationTable dotsDefined;
  memset(dotsDefined, 0, sizeof(TranslationTable));

  {
    int byteIndex;
    for (byteIndex=0; byteIndex<TRANSLATION_TABLE_SIZE; ++byteIndex) {
      TblByteEntry *byte = &input->bytes[byteIndex];
      unsigned char *cell = &table[byteIndex];
      if (byte->defined) {
        *cell = byte->cell;

        if (!dotsDefined[byte->cell]) {
          dotsDefined[byte->cell] = 1;
        } else if (input->options & TBL_DUPLICATE) {
          TblDotsBuffer dotsBuffer;
          tblPutDots(byte->cell, dotsBuffer);
          tblReportWarning(input, "duplicate dots: %s [\\X%02X]", dotsBuffer, byteIndex);
        }
      } else {
        int dotIndex;
        *cell = input->undefined;

        for (dotIndex=0; dotIndex<TBL_DOT_COUNT; ++dotIndex) {
          if (byteIndex & input->masks[dotIndex]) {
            unsigned char bit = tblDotBit(dotIndex);
            if (input->undefined & bit) {
              *cell &= ~bit;
            } else {
              *cell |= bit;
            }
          }
        }

        if (input->options & TBL_UNDEFINED) {
          tblReportWarning(input, "undefined byte: \\X%02X", byteIndex);
        }
      }
    }
  }

  if (input->options & TBL_UNUSED) {
    int cell;
    for (cell=0; cell<TRANSLATION_TABLE_SIZE; ++cell) {
      if (!dotsDefined[cell]) {
        TblDotsBuffer dotsBuffer;
        tblPutDots(cell, dotsBuffer);
        tblReportWarning(input, "unused dots: %s", dotsBuffer);
      }
    }
  }
}

typedef struct {
  TblInputData *input;
  TblLineProcessor process;
} TblProcessLineData;

static int
tblProcessLine (char *line, void *data) {
  TblProcessLineData *pld = data;
  TblInputData *input = pld->input;

  input->line++;
  input->location = (const unsigned char *)line;

  return pld->process(input);
}

int
tblProcessLines (
  const char *path,
  FILE *file,
  TranslationTable table,
  TblLineProcessor process,
  int options
) {
  int ok = 0;
  TblInputData input;
  TblProcessLineData pld;
  int opened = file != NULL;

  memset(&input, 0, sizeof(input));
  input.options = options;
  input.ok = 1;
  input.file = path;
  input.line = 0;
  input.undefined = 0XFF;

  memset(&pld, 0, sizeof(pld));
  pld.input = &input;
  pld.process = process;

  if (opened || (file = openDataFile(path, "r"))) {
    if (processLines(file, tblProcessLine, &pld)) {
      input.line = 0;

      if (input.ok) {
        tblSetTable(&input, table);
        ok = 1;
      }
    }

    if (!opened) fclose(file);
  } else {
    tblReportError(&input, "open error: %s", strerror(errno));
  }

  return ok;
}

int
loadTranslationTable (const char *path, FILE *file, TranslationTable table, int options) {
  return tblLoad_Native(path, file, table, options);
}

unsigned char
convertWcharToDots (TranslationTable table, wchar_t character) {
  const wchar_t cellMask = 0XFF;
  const wchar_t row = character & ~cellMask;

  if (row == BRL_UC_ROW) return character & cellMask;
  if (row == 0XF000) return table[character & cellMask];

  {
    int byte = convertWcharToChar(character);
    if (byte == EOF) byte = '?';
    return table[byte];
  }
}

void
reverseTranslationTable (TranslationTable from, TranslationTable to) {
  int byte;
  memset(to, 0, sizeof(TranslationTable));
  for (byte=TRANSLATION_TABLE_SIZE-1; byte>=0; byte--) to[from[byte]] = byte;
}

static void
fixTranslationTablePath (char **path, const char *prefix) {
  fixPath(path, TRANSLATION_TABLE_EXTENSION, prefix);
}

void
fixTextTablePath (char **path) {
  fixTranslationTablePath(path, TEXT_TABLE_PREFIX);
}

void
fixAttributesTablePath (char **path) {
  fixTranslationTablePath(path, ATTRIBUTES_TABLE_PREFIX);
}
