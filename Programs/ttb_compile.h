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

#ifndef BRLTTY_INCLUDED_TTB_COMPILE
#define BRLTTY_INCLUDED_TTB_COMPILE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>

#include "datafile.h"

typedef struct TextTableDataStruct TextTableData;
extern TextTableData *newTextTableData (void);
extern void destroyTextTableData (TextTableData *ttd);

extern TextTableData *processTextTableStream (FILE *stream, const char *name, DataProcessor processor);
extern TextTable *newTextTable (TextTableData *ttd);

extern DataProcessor processTextTableLine;
extern DataProcessor processGnomeBrailleLine;

extern void *getTextTableItem (TextTableData *ttd, TextTableOffset offset);
extern TextTableHeader *getTextTableHeader (TextTableData *ttd);
extern const UnicodeCellEntry *getUnicodeCellEntry (TextTableData *ttd, wchar_t character);

extern int setTextTableCharacter (TextTableData *ttd, wchar_t character, unsigned char dots);
extern int setTextTableByte (TextTableData *ttd, unsigned char byte, unsigned char dots);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_TTB_COMPILE */
