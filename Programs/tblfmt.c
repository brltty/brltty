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

#include "prologue.h"

#include <string.h>

#include "log.h"
#include "tblfmt.h"
#include "parse.h"

struct TableFormatterStruct {
  struct {
    unsigned int columnSpacing;
    unsigned int lineLength;
  } parameter;

  struct {
    TableFormatterColumn **array;
    size_t size;
    size_t count;
  } columns;

  struct {
    TableFormatterRow **array;
    size_t size;
    size_t count;
  } rows;

  struct {
    size_t rows;
    size_t columns;
  } increment;
};

struct TableFormatterColumnStruct {
  TableFormatter *table;
  size_t width;
  unsigned int weight;
};

struct TableFormatterRowStruct {
  TableFormatter *table;
  size_t height;

  struct {
    TableFormatterCell **array;
    size_t size;
    size_t count;
  } cells;
};

struct TableFormatterCellStruct {
  TableFormatterRow *row;
  TableFormatterColumn *column;
  size_t width;

  struct {
    TableFormatterLine **array;
    size_t size;
    size_t count;
  } lines;
};

struct TableFormatterLineStruct {
  TableFormatterCell *cell;
  size_t length;
  char text[];
};

static void
tblSetMaximum (size_t *maximum, size_t value) {
  if (value > *maximum) *maximum = value;
}

static TableFormatterColumn *
tblNewColumn (TableFormatter *table) {
  if (table->columns.count == table->columns.size) {
    size_t newSize = table->columns.size + table->increment.columns;
    TableFormatterColumn **newColumns = realloc(table->columns.array, ARRAY_SIZE(newColumns, newSize));

    if (!newColumns) {
      logMallocError();
      return NULL;
    }

    table->columns.array = newColumns;
    table->columns.size = newSize;
  }

  {
    TableFormatterColumn *column;

    if ((column = malloc(sizeof(*column)))) {
      memset(column, 0, sizeof(*column));
      column->table = table;
      return table->columns.array[table->columns.count++] = column;
    } else {
      logMallocError();
    }
  }

  return NULL;
}

size_t
tblGetColumnCount (const TableFormatter *table) {
  return table->columns.count;
}

TableFormatterColumn *
tblGetColumn (TableFormatter *table, size_t index) {
  do {
    if (index < table->columns.count) return table->columns.array[index];
  } while (tblNewColumn(table));

  return NULL;
}

unsigned int
tblGetWeight (TableFormatterColumn *column) {
  return column->weight;
}

void
tblSetWeight (TableFormatterColumn *column, unsigned int weight) {
  column->weight = weight;
}

static int
tblRemoveColumn (TableFormatter *table) {
  if (!table->columns.count) return 0;
  TableFormatterColumn *column = table->columns.array[--table->columns.count];

  free(column);
  return 1;
}

static void
tblPurgeColumns (TableFormatter *table) {
  while (tblRemoveColumn(table));
}

static void
tblDestroyColumns (TableFormatter *table) {
  TableFormatterColumn **columns = table->columns.array;

  if (columns) {
    tblPurgeColumns(table);
    table->columns.size = 0;
    table->columns.array = NULL;
    free(columns);
  }
}

int
tblAddText (TableFormatterCell *cell, const char *text) {
  int count;
  char **strings = splitString(text, '\n', &count);

  if (strings) {
    if (count > 0) {
      size_t newCount = cell->lines.count + count;

      if (newCount > cell->lines.size) {
        size_t newSize = newCount;
        TableFormatterLine **newLines = realloc(cell->lines.array, ARRAY_SIZE(newLines, newSize));

        if (!newLines) {
          logMallocError();
          goto EXTEND_FAILED;
        }

        cell->lines.array = newLines;
        cell->lines.size = newSize;
      }

      {
        size_t width = 0;
        TableFormatterLine *lines[count];
        size_t index = 0;

        while (index < count) {
          const char *string = strings[index];
          size_t length = strlen(string);
          tblSetMaximum(&width, length);

          TableFormatterLine *line;
          size_t size = sizeof(*line);
          line = malloc(size + length + 1);
          if (!line) break;

          memset(line, 0, size);
          line->cell = cell;
          line->length = length;
          strcpy(line->text, string);

          lines[index++] = line;
        }

        if (index == count) {
          memcpy(&cell->lines.array[cell->lines.count], lines, sizeof(lines));
          cell->lines.count += count;

          tblSetMaximum(&cell->width, width);
          tblSetMaximum(&cell->column->width, cell->width);
          tblSetMaximum(&cell->row->height, cell->lines.count);
          return 1;
        }

        while (index > 0) free(lines[--index]);
      }
    }

  EXTEND_FAILED:
    deallocateStrings(strings);
  }

  return 0;
}

size_t
tblGetLineCount (const TableFormatterCell *cell) {
  return cell->lines.count;
}

const char *
tblGetLine (const TableFormatterCell *cell, size_t index, size_t *length) {
  if (index >= cell->lines.count) return NULL;
  const TableFormatterLine *line = cell->lines.array[index];
  if (length) *length = line->length;
  return line->text;
}

size_t
tblGetWidth (TableFormatterCell *cell) {
  return cell->width;
}

static int
tblRemoveLine (TableFormatterCell *cell) {
  if (!cell->lines.count) return 0;
  TableFormatterLine *line = cell->lines.array[--cell->lines.count];

  free(line);
  return 1;
}

static void
tblPurgeLines (TableFormatterCell *cell) {
  while (tblRemoveLine(cell));
}

static void
tblDestroyLines (TableFormatterCell *cell) {
  TableFormatterLine **lines = cell->lines.array;

  if (lines) {
    tblPurgeLines(cell);
    cell->lines.size = 0;
    cell->lines.array = NULL;
    free(lines);
  }
}

TableFormatterCell *
tblNewCell (TableFormatterRow *row) {
  if (row->cells.count == row->cells.size) {
    size_t newSize = row->cells.size + row->table->increment.columns;
    TableFormatterCell **newCells = realloc(row->cells.array, ARRAY_SIZE(newCells, newSize));

    if (!newCells) {
      logMallocError();
      return NULL;
    }

    row->cells.array = newCells;
    row->cells.size = newSize;
  }

  TableFormatterColumn *column = tblGetColumn(row->table, row->cells.count);
  if (!column) return NULL;
  {
    TableFormatterCell *cell;

    if ((cell = malloc(sizeof(*cell)))) {
      memset(cell, 0, sizeof(*cell));

      cell->row = row;
      cell->column = column;

      return row->cells.array[row->cells.count++] = cell;
    } else {
      logMallocError();
    }
  }

  return NULL;
}

size_t
tblGetCellCount (const TableFormatterRow *row) {
  return row->cells.count;
}

TableFormatterCell *
tblGetCell (const TableFormatterRow *row, size_t index) {
  if (index >= row->cells.count) return NULL;
  return row->cells.array[index];
}

size_t
tblGetHeight (TableFormatterRow *row) {
  return row->height;
}

static int
tblRemoveCell (TableFormatterRow *row) {
  if (!row->cells.count) return 0;
  TableFormatterCell *cell = row->cells.array[--row->cells.count];

  tblDestroyLines(cell);
  free(cell);
  return 1;
}

TableFormatterCell *
tblAddCell (TableFormatterRow *row, const char *text) {
  TableFormatterCell *cell = tblNewCell(row);

  if (cell) {
    if (tblAddText(cell, text)) return cell;
    tblRemoveCell(row);
  }

  return NULL;
}

static void
tblPurgeCells (TableFormatterRow *row) {
  while (tblRemoveCell(row));
}

static void
tblDestroyCells (TableFormatterRow *row) {
  TableFormatterCell **cells = row->cells.array;

  if (cells) {
    tblPurgeCells(row);
    row->cells.size = 0;
    row->cells.array = NULL;
    free(cells);
  }
}

TableFormatterRow *
tblNewRow (TableFormatter *table) {
  if (table->rows.count == table->rows.size) {
    size_t newSize = table->rows.size + table->increment.rows;
    TableFormatterRow **newRows = realloc(table->rows.array, ARRAY_SIZE(newRows, newSize));

    if (!newRows) {
      logMallocError();
      return NULL;
    }

    table->rows.array = newRows;
    table->rows.size = newSize;
  }

  {
    TableFormatterRow *row;

    if ((row = malloc(sizeof(*row)))) {
      memset(row, 0, sizeof(*row));
      row->table = table;
      return table->rows.array[table->rows.count++] = row;
    } else {
      logMallocError();
    }
  }

  return NULL;
}

size_t
tblGetRowCount (const TableFormatter *table) {
  return table->rows.count;
}

TableFormatterRow *
tblGetRow (const TableFormatter *table, size_t index) {
  if (index >= table->rows.count) return NULL;
  return table->rows.array[index];
}

static int
tblRemoveRow (TableFormatter *table) {
  if (!table->rows.count) return 0;
  TableFormatterRow *row = table->rows.array[--table->rows.count];

  tblDestroyCells(row);
  free(row);
  return 1;
}

static void
tblPurgeRows (TableFormatter *table) {
  while (tblRemoveRow(table));
}

static void
tblDestroyRows (TableFormatter *table) {
  TableFormatterRow **rows = table->rows.array;

  if (rows) {
    tblPurgeRows(table);
    table->rows.size = 0;
    table->rows.array = NULL;
    free(rows);
  }
}

TableFormatter *
tblNewFormatter (void) {
  TableFormatter *table;

  if ((table = malloc(sizeof(*table)))) {
    memset(table, 0, sizeof(*table));

    table->parameter.columnSpacing = 2;
    table->parameter.lineLength = 0;

    table->increment.rows = 1;
    table->increment.columns = 1;

    return table;
  } else {
    logMallocError();
  }

  return NULL;
}

unsigned int
tblGetColumnSpacing (const TableFormatter *table) {
  return table->parameter.columnSpacing;
}

void
tblSetColumnSpacing (TableFormatter *table, unsigned int spacing) {
  table->parameter.columnSpacing = spacing;
}

unsigned int
tblGetLineLength (const TableFormatter *table) {
  return table->parameter.lineLength;
}

void
tblSetLineLength (TableFormatter *table, unsigned int length) {
  table->parameter.lineLength = length;
}

void
tblDestroyFormatter (TableFormatter *table) {
  tblDestroyRows(table);
  tblDestroyColumns(table);
  free(table);
}

char **
tblFormatLines (TableFormatter *table) {
  return NULL;
}
