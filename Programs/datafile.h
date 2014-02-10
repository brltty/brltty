/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_DATAFILE
#define BRLTTY_INCLUDED_DATAFILE

#include <stdio.h>

#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int setGlobalDataVariable (const char *name, const char *value);
extern int setGlobalTableVariables (const char *tableExtension, const char *subtableExtension);

typedef struct DataFileStruct DataFile;

typedef int DataProcessor (DataFile *file, void *data);

extern int processDataFile (const char *name, DataProcessor *processLine, void *data);
extern void reportDataError (DataFile *file, char *format, ...) PRINTF(2, 3);

extern int processDataStream (
  Queue *variables,
  FILE *stream, const char *name,
  DataProcessor *processLine, void *data
);

extern int isKeyword (const wchar_t *keyword, const wchar_t *characters, size_t length);
extern int isNumber (int *number, const wchar_t *characters, int length);
extern int isHexadecimalDigit (wchar_t character, int *value, int *shift);
extern int isOctalDigit (wchar_t character, int *value, int *shift);

extern int findDataOperand (DataFile *file, const char *description);
extern int getDataCharacter (DataFile *file, wchar_t *character);
extern int ungetDataCharacters (DataFile *file, unsigned int count);

typedef struct {
  const wchar_t *characters;
  unsigned int length;
} DataOperand;

extern int getDataOperand (DataFile *file, DataOperand *operand, const char *description);
extern int getDataText (DataFile *file, DataOperand *text, const char *description);

typedef struct {
  unsigned char length;
  wchar_t characters[0XFF];
} DataString;

extern int parseDataString (DataFile *file, DataString *string, const wchar_t *characters, int length, int noUnicode);
extern int getDataString (DataFile *file, DataString *string, int noUnicode, const char *description);

extern int writeHexadecimalCharacter (FILE *stream, wchar_t character);
extern int writeEscapedCharacter (FILE *stream, wchar_t character);
extern int writeEscapedCharacters (FILE *stream, const wchar_t *characters, size_t count);

typedef struct {
  unsigned char length;
  unsigned char bytes[0XFF];
} ByteOperand;

#define CELLS_OPERAND_DELIMITER WC_C('-')
#define CELLS_OPERAND_SPACE WC_C('0')

extern int parseCellsOperand (DataFile *file, ByteOperand *cells, const wchar_t *characters, int length);
extern int getCellsOperand (DataFile *file, ByteOperand *cells, const char *description);

extern int writeDots (FILE *stream, unsigned char cell);
extern int writeDotsCell (FILE *stream, unsigned char cell);
extern int writeDotsCells (FILE *stream, const unsigned char *cells, size_t count);

extern int writeUtf8Cell (FILE *stream, unsigned char cell);
extern int writeUtf8Cells (FILE *stream, const unsigned char *cells, size_t count);

typedef int DataConditionTester (DataFile *file, const DataOperand *name, void *data);

extern int processConditionOperands (
  DataFile *file,
  DataConditionTester *testCondition, int negateCondition,
  const char *description, void *data
);

extern int processIfVarOperands (DataFile *file, void *data);
extern int processIfNoVarOperands (DataFile *file, void *data);
extern int processEndIfOperands (DataFile *file, void *data);

typedef struct {
  const wchar_t *name;
  DataProcessor *processor;
  unsigned unconditional:1;
} DataProperty;

extern int processPropertyOperand (DataFile *file, const DataProperty *properties, const char *description, void *data);

extern int processAssignOperands (DataFile *file, void *data);

extern int processIncludeOperands (DataFile *file, void *data);
extern int includeDataFile (DataFile *file, const wchar_t *name, unsigned int length);

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
