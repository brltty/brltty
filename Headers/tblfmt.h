/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2020 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_TBLFMT
#define BRLTTY_INCLUDED_TBLFMT

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct TableFormatterStruct TableFormatter;
typedef struct TableFormatterColumnStruct TableFormatterColumn;
typedef struct TableFormatterRowStruct TableFormatterRow;
typedef struct TableFormatterCellStruct TableFormatterCell;
typedef struct TableFormatterLineStruct TableFormatterLine;

extern TableFormatter *tblNewFormatter (void);
extern void tblDestroyFormatter (TableFormatter *table);

extern unsigned int tblGetColumnSpacing (const TableFormatter *table);
extern void tblSetColumnSpacing (TableFormatter *table, unsigned int spacing);

extern unsigned int tblGetLineLength (const TableFormatter *table);
extern void tblSetLineLength (TableFormatter *table, unsigned int length);

extern size_t tblGetColumnCount (const TableFormatter *table);
extern TableFormatterColumn *tblGetColumn (TableFormatter *table, size_t index);

extern unsigned int tblGetWeight (TableFormatterColumn *column);
extern void tblSetWeight (TableFormatterColumn *column, unsigned int weight);

extern TableFormatterRow *tblNewRow (TableFormatter *table);
extern size_t tblGetRowCount (const TableFormatter *table);
extern TableFormatterRow *tblGetRow (const TableFormatter *table, size_t index);

extern TableFormatterCell *tblNewCell (TableFormatterRow *table);
extern TableFormatterCell *tblAddCell (TableFormatterRow *row, const char *text);
extern size_t tblGetCellCount (const TableFormatterRow *row);
extern TableFormatterCell *tblGetCell (const TableFormatterRow *row, size_t index);
extern size_t tblGetHeight (TableFormatterRow *row);

extern int tblAddText (TableFormatterCell *cell, const char *text);
extern size_t tblGetLineCount (const TableFormatterCell *cell);
extern const char *tblGetLine (const TableFormatterCell *cell, size_t index, size_t *length);
extern size_t tblGetWidth (TableFormatterCell *cell);

extern char **tblFormatLines (TableFormatter *table);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_TBLFMT */
