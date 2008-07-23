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

#ifndef BRLTTY_INCLUDED_DATAFILE
#define BRLTTY_INCLUDED_DATAFILE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct DataFileStruct DataFile;

typedef int (*DataParser) (DataFile *file, void *data);

extern int processDataFile (const char *name, DataParser parser, void *data);
extern int includeDataFile (DataFile *file, const wchar_t *name, int length);
extern void reportDataError (DataFile *file, char *format, ...) PRINTF(2, 3);

typedef struct {
  const wchar_t *characters;
  int length;
} DataOperand;
extern int getDataOperand (DataFile *file, DataOperand *operand, const char *description);

typedef struct {
  unsigned char length;
  wchar_t characters[0XFF];
} DataString;
extern int getDataString (DataFile *file, DataString *string, const char *description);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_DATAFILE */
