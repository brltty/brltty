/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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
#include <locale.h>
#include <langinfo.h>
#include <wchar.h>

#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif /* HAVE_ICONV_H */

#include "misc.h"
#include "tbl.h"
#include "tbl_internal.h"

const unsigned char tblDotBits[TBL_DOT_COUNT] = {BRL_DOT1, BRL_DOT2, BRL_DOT3, BRL_DOT4, BRL_DOT5, BRL_DOT6, BRL_DOT7, BRL_DOT8};
const unsigned char tblDotNumbers[TBL_DOT_COUNT] = {'1', '2', '3', '4', '5', '6', '7', '8'};
const unsigned char tblNoDots[] = {'0'};
const unsigned char tblNoDotsSize = sizeof(tblNoDots);

const char *tblCharset = "ISO-8859-1";

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

static void
tblPutCell (unsigned char cell, char *buffer, int *count) {
  *count = 0;
  if (cell) {
    int dotIndex;
    for (dotIndex=0; dotIndex<TBL_DOT_COUNT; ++dotIndex)
      if (cell & tblDotBit(dotIndex))
        buffer[(*count)++] = tblDotNumbers[dotIndex];
  } else {
    buffer[(*count)++] = tblNoDots[0];
  }
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
          char dotsBuffer[TBL_DOT_COUNT];
          int dotCount;
          tblPutCell(byte->cell, dotsBuffer, &dotCount);
          tblReportWarning(input, "duplicate dots: %.*s [\\X%02X]", dotCount, dotsBuffer, byteIndex);
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
        char dotsBuffer[TBL_DOT_COUNT];
        int dotCount;
        tblPutCell(cell, dotsBuffer, &dotCount);
        tblReportWarning(input, "unused dots: %.*s", dotCount, dotsBuffer);
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

#ifdef HAVE_ICONV_H
#define TBL_ICONV_NULL ((iconv_t)-1)

#define TBL_ICONV_HANDLE(name) iconv_t iconv##name = TBL_ICONV_NULL
static TBL_ICONV_HANDLE(CharToUtf8);
static TBL_ICONV_HANDLE(Utf8ToChar);
static TBL_ICONV_HANDLE(WcharToUtf8);
static TBL_ICONV_HANDLE(Utf8ToWchar);
static TBL_ICONV_HANDLE(CharToWchar);
static TBL_ICONV_HANDLE(WcharToChar);

static const char *const utf8Charset = "UTF-8";
static const char *const wcharCharset = "WCHAR_T";

#define TBL_UTF8_TO_TYPE(name, type, ret, eof) \
ret tbl##name (char **utf8, size_t *utfs) { \
  type c; \
  type *cp = &c; \
  size_t cs = sizeof(c); \
  if ((iconv(iconv##name, utf8, utfs, (void *)&cp, &cs) == -1) && (errno != E2BIG)) { \
    LogError("iconv (UTF-8 -> " #type ")"); \
    return eof; \
  } \
  return c; \
}
TBL_UTF8_TO_TYPE(Utf8ToWchar, wchar_t, wint_t, WEOF)
TBL_UTF8_TO_TYPE(Utf8ToChar, char, int, EOF)
#undef TBL_UTF8_TO_TYPE

#define TBL_TYPE_TO_UTF8(name, type) \
int tbl##name (type c, Utf8Buffer utf8) { \
  type *cp = &c; \
  size_t cs = sizeof(c); \
  size_t utfs = MB_LEN_MAX; \
  if (iconv(iconv##name, (void *)&cp, &cs, &utf8, &utfs) == -1) { \
    LogError("iconv (" #type " -> UTF-8)"); \
    return 0; \
  } \
  *utf8 = 0; \
  return 1; \
}
TBL_TYPE_TO_UTF8(WcharToUtf8, wchar_t)
TBL_TYPE_TO_UTF8(CharToUtf8, char)
#undef TBL_TYPE_TO_UTF8

#define TBL_TYPE_TO_TYPE(name, from, to, ret, eof) \
ret tbl##name (from f) { \
  from *fp = &f; \
  size_t fs = sizeof(f); \
  to t; \
  to *tp = &t; \
  size_t ts = sizeof(t); \
  if (iconv(iconv##name, (void *)&fp, &fs, (void *)&tp, &ts) == -1) { \
    LogError("iconv (" #from " -> " #to ")"); \
    return eof; \
  } \
  return t; \
}
TBL_TYPE_TO_TYPE(CharToWchar, char, wchar_t, wint_t, WEOF)
TBL_TYPE_TO_TYPE(WcharToChar, wchar_t, char, int, EOF)
#undef TBL_TYPE_TO_TYPE

static int
tblIconvOpen (iconv_t *handle, const char *from, const char *to) {
  if ((*handle = iconv_open(to, from)) != TBL_ICONV_NULL) return 1;
  LogError("iconv_open");
  return 0;
}
#endif /* HAVE_ICONV_H */

int
tblSetCharset (const char *charset) {
#ifdef HAVE_ICONV_H
  typedef struct {
    const char *fromCharset;
    const char *toCharset;
    iconv_t *handle;
    iconv_t newHandle;
  } ConvEntry;
  ConvEntry convTable[] = {
    {charset, utf8Charset, &iconvCharToUtf8},
    {utf8Charset, charset, &iconvUtf8ToChar},
    {charset, wcharCharset, &iconvCharToWchar},
    {wcharCharset, charset, &iconvWcharToChar},
    {NULL, NULL, NULL}
  };
  ConvEntry *conv = convTable;

  while (conv->handle) {
    if (!tblIconvOpen(&conv->newHandle, conv->fromCharset, conv->toCharset)) {
      while (conv != convTable) iconv_close((--conv)->newHandle);
      return 0;
    }

    ++conv;
  }

  while (conv != convTable) {
    --conv;
    if (*conv->handle != TBL_ICONV_NULL) iconv_close(*conv->handle);
    *conv->handle = conv->newHandle;
  }
#endif /* HAVE_ICONV_H */

  tblCharset = charset;
  return 1;
}

int
tblInit (void) {
  int ok = 1;
  const char *locale = setlocale(LC_ALL, "");
  if ((MB_CUR_MAX == 1) &&
      (strcmp(locale, "C") != 0) &&
      (strcmp(locale, "POSIX") != 0)) {
    /* some 8bit locale is set, assume its charset is correct */
    if (!tblSetCharset(nl_langinfo(CODESET))) ok = 0;
  }

#ifdef HAVE_ICONV_H
#define TBL_ICONV_OPEN(name, from, to) if (!tblIconvOpen(&iconv##name, from, to)) ok = 0
  TBL_ICONV_OPEN(Utf8ToWchar, utf8Charset, wcharCharset);
  TBL_ICONV_OPEN(WcharToUtf8, wcharCharset, utf8Charset);
#undef TBL_ICONV_OPEN
#endif /* HAVE_ICONV_H */

  return ok;
}
