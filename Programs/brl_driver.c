/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#include "prologue.h"

#include <string.h>

#include "prefs.h"
#include "drivers.h"
#include "brl.h"
#include "brl.auto.h"

#define BRLSYMBOL noBraille
#define DRIVER_NAME NoBraille
#define DRIVER_CODE no
#define DRIVER_COMMENT "no braille support"
#define DRIVER_VERSION ""
#define DRIVER_DEVELOPERS ""
#include "brl_driver.h"

static int
brl_construct (BrailleDisplay *brl UNUSED, char **parameters UNUSED, const char *device UNUSED) {
  brl->keyBindings = NULL;
  return 1;
}

static void
brl_destruct (BrailleDisplay *brl UNUSED) {
}

static int
brl_readCommand (BrailleDisplay *brl UNUSED, KeyTableCommandContext context UNUSED) {
  return EOF;
}

static int
brl_writeWindow (BrailleDisplay *brl UNUSED, const wchar_t *characters UNUSED) {
  return 1;
}

const BrailleDriver *braille = &noBraille;

const DotsTable dotsTable_ISO11548_1 = {
  BRL_DOT1, BRL_DOT2, BRL_DOT3, BRL_DOT4,
  BRL_DOT5, BRL_DOT6, BRL_DOT7, BRL_DOT8
};

int
haveBrailleDriver (const char *code) {
  return haveDriver(code, BRAILLE_DRIVER_CODES, driverTable);
}

const char *
getDefaultBrailleDriver (void) {
  return getDefaultDriver(driverTable);
}

const BrailleDriver *
loadBrailleDriver (const char *code, void **driverObject, const char *driverDirectory) {
  return loadDriver(code, driverObject,
                    driverDirectory, driverTable,
                    "braille", 'b', "brl",
                    &noBraille, &noBraille.definition);
}

void
identifyBrailleDriver (const BrailleDriver *driver, int full) {
  identifyDriver("Braille", &driver->definition, full);
}

void
identifyBrailleDrivers (int full) {
  const DriverEntry *entry = driverTable;
  while (entry->address) {
    const BrailleDriver *driver = entry++->address;
    identifyBrailleDriver(driver, full);
  }
}

int
cellsHaveChanged (
  unsigned char *cells, const unsigned char *new, unsigned int count,
  unsigned int *from, unsigned int *to, int *force
) {
  unsigned int first = 0;

  if (force && *force) {
    *force = 0;
  } else if (memcmp(cells, new, count) != 0) {
    if (to) {
      while (count) {
        unsigned int last = count - 1;
        if (cells[last] != new[last]) break;
        count = last;
      }
    }

    if (from) {
      while (first < count) {
        if (cells[first] != new[first]) break;
        first += 1;
      }
    }
  } else {
    return 0;
  }

  if (from) *from = first;
  if (to) *to = count;

  memcpy(cells+first, new+first, count-first);
  return 1;
}

int
textHasChanged (
  wchar_t *text, const wchar_t *new, unsigned int count,
  unsigned int *from, unsigned int *to, int *force
) {
  unsigned int first = 0;

  if (force && *force) {
    *force = 0;
  } else if (wmemcmp(text, new, count) != 0) {
    if (to) {
      while (count) {
        unsigned int last = count - 1;
        if (text[last] != new[last]) break;
        count = last;
      }
    }

    if (from) {
      while (first < count) {
        if (text[first] != new[first]) break;
        first += 1;
      }
    }
  } else {
    return 0;
  }

  if (from) *from = first;
  if (to) *to = count;

  wmemcpy(text+first, new+first, count-first);
  return 1;
}

int
cursorHasChanged (int *cursor, int new, int *force) {
  if (force && *force) {
    *force = 0;
  } else if (new == *cursor) {
    return 0;
  }

  *cursor = new;
  return 1;
}

void
makeTranslationTable (const DotsTable dots, TranslationTable table) {
  int byte;

  for (byte=0; byte<TRANSLATION_TABLE_SIZE; byte+=1) {
    unsigned char cell = 0;
    int dot;

    for (dot=0; dot<DOTS_TABLE_SIZE; dot+=1)
      if (byte & dotsTable_ISO11548_1[dot])
        cell |= dots[dot];

    table[byte] = cell;
  }
}

void
reverseTranslationTable (const TranslationTable from, TranslationTable to) {
  int byte;
  memset(to, 0, sizeof(TranslationTable));
  for (byte=TRANSLATION_TABLE_SIZE-1; byte>=0; byte--) to[from[byte]] = byte;
}

static inline void *
translateCells (
  const TranslationTable table,
  unsigned char *target, const unsigned char *source, size_t count
) {
  if (table) {
    while (count--) *target++ = table[*source++];
    return target;
  }

  if (target == source) return target + count;
  return mempcpy(target, source, count);
}

static inline unsigned char
translateCell (const TranslationTable table, unsigned char cell) {
  return table? table[cell]: cell;
}

static TranslationTable internalOutputTable;
static const unsigned char *outputTable;

void
setOutputTable (const TranslationTable table) {
  outputTable = table;
}

void
makeOutputTable (const DotsTable dots) {
  if (memcmp(dots, dotsTable_ISO11548_1, DOTS_TABLE_SIZE) == 0) {
    outputTable = NULL;
  } else {
    makeTranslationTable(dots, internalOutputTable);
    outputTable = internalOutputTable;
  }
}

void *
translateOutputCells (unsigned char *target, const unsigned char *source, size_t count) {
  return translateCells(outputTable, target, source, count);
}

unsigned char
translateOutputCell (unsigned char cell) {
  return translateCell(outputTable, cell);
}

static TranslationTable internalInputTable;
static const unsigned char *inputTable;

void
makeInputTable (void) {
  if (outputTable) {
    reverseTranslationTable(outputTable, internalInputTable);
    inputTable = internalInputTable;
  } else {
    inputTable = NULL;
  }
}

void *
translateInputCells (unsigned char *target, const unsigned char *source, size_t count) {
  return translateCells(inputTable, target, source, count);
}

unsigned char
translateInputCell (unsigned char cell) {
  return translateCell(inputTable, cell);
}

void
applyBrailleOrientation (unsigned char *cells, size_t count) {
  switch (prefs.brailleOrientation) {
    case BRL_ORIENTATION_ROTATED: {
      static TranslationTable rotateTable = {[1] = 0};

      const unsigned char *source = cells;
      const unsigned char *end = source + count;

      unsigned char buffer[count];
      unsigned char *target = &buffer[count];

      if (!rotateTable[1]) {
        static const DotsTable dotsTable = {
          BRL_DOT8, BRL_DOT6, BRL_DOT5, BRL_DOT7,
          BRL_DOT3, BRL_DOT2, BRL_DOT4, BRL_DOT1
        };

        makeTranslationTable(dotsTable, rotateTable);
      }

      while (source < end) *--target = rotateTable[*source++];
      memcpy(cells, buffer, count);
      break;
    }

    default:
    case BRL_ORIENTATION_NORMAL:
      break;
  }
}
