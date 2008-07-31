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

typedef int (*DataProcessor) (DataFile *file, void *data);

extern int processDataFile (const char *name, DataProcessor processor, void *data);
extern int includeDataFile (DataFile *file, const wchar_t *name, unsigned int length);
extern void reportDataError (DataFile *file, char *format, ...) PRINTF(2, 3);

extern int findDataOperand (DataFile *file, const char *description);
extern int getDataCharacter (DataFile *file, wchar_t *character);
extern int ungetDataCharacters (DataFile *file, unsigned int count);

typedef struct {
  const wchar_t *characters;
  unsigned int length;
} DataOperand;
extern int getDataOperand (DataFile *file, DataOperand *operand, const char *description);

typedef struct {
  unsigned char length;
  wchar_t characters[0XFF];
} DataString;
extern int getDataString (DataFile *file, DataString *string, const char *description);

typedef struct {
  const wchar_t *name;
  DataProcessor processor;
} DataProperty;
extern int processPropertyOperand (DataFile *file, const DataProperty *properties, const char *description, void *data);

extern int processIncludeOperands (DataFile *file, void *data);

#define BRL_DOT_COUNT 8
extern const wchar_t brlDotNumbers[BRL_DOT_COUNT];
extern const unsigned char brlDotBits[BRL_DOT_COUNT];
extern int brlDotNumberToIndex (wchar_t number, int *index);
extern int brlDotBitToIndex (unsigned char bit, int *index);

extern int getDotOperand (DataFile *file, int *index);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_DATAFILE */
