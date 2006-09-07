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

#ifndef BRLTTY_INCLUDED_TBL_INTERNAL
#define BRLTTY_INCLUDED_TBL_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <wchar.h>
#include <limits.h>

extern const char *tblSetCharset (const char *name);
extern const char *tblGetCharset (void);

#define TBL_DOT_COUNT 8

extern const unsigned char tblDotBits[TBL_DOT_COUNT];
#define tblDotBit(dot) (tblDotBits[(dot)])

extern const unsigned char tblDotNumbers[TBL_DOT_COUNT];
extern const unsigned char tblNoDots[];
extern const unsigned char tblNoDotsSize;

typedef struct {
  unsigned char cell;
  unsigned char defined;
} TblByteEntry;

typedef struct {
  int options;
  unsigned ok:1;
  const char *file;
  int line;
  const unsigned char *location;
  TblByteEntry bytes[0X100];
  unsigned char undefined;
  unsigned char masks[TBL_DOT_COUNT];
} TblInputData;

typedef int (*TblLineProcessor) (TblInputData *input);
extern int tblProcessLines (
  const char *path,
  FILE *file,
  TranslationTable table,
  TblLineProcessor process,
  int options
);

extern void tblReportError (TblInputData *input, const char *format, ...);
extern void tblReportWarning (TblInputData *input, const char *format, ...);

extern int tblIsEndOfLine (TblInputData *input);
extern void tblSkipSpace (TblInputData *input);
extern const unsigned char *tblFindSpace (TblInputData *input);

extern int tblTestWord (const unsigned char *location, int length, const char *word);

extern void tblSetByte (TblInputData *input, unsigned char index, unsigned char cell);
extern void tblSetTable (TblInputData *input, TranslationTable table);

#ifdef HAVE_ICONV_H
typedef char Utf8Buffer[MB_LEN_MAX+1];

extern wint_t tblUtf8ToWchar (char **utf8, size_t *utfs);
extern int tblUtf8ToChar (char **utf8, size_t *utfs);
extern int tblWcharToUtf8 (wchar_t wc, Utf8Buffer utf8);
extern int tblCharToUtf8 (char c, Utf8Buffer utf8);
extern wint_t tblCharToWchar (char c);
extern int tblWcharToChar (wchar_t wc);
#endif /* HAVE_ICONV_H */

typedef int TblLoader (const char *path, FILE *file, TranslationTable table, int options);
extern TblLoader tblLoad_Native;
extern TblLoader tblLoad_Gnome;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_TBL_INTERNAL */
