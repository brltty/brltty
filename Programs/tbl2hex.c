/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "options.h"
#include "log.h"
#include "file.h"

#include "ttb.h"
#include "ttb_internal.h"

#include "atb.h"
#include "atb_internal.h"

#include "ctb.h"
#include "ctb_internal.h"

BEGIN_OPTION_TABLE(programOptions)
END_OPTION_TABLE

typedef struct {
  void *object;
  const unsigned char *bytes;
  size_t size;
} TableData;

typedef struct {
  const char *extension;
  int (*load) (const char *path, TableData *data);
  void (*unload) (TableData *data);
} TableEntry;

static int
loadTextTable (const char *path, TableData *data) {
  TextTable *table = compileTextTable(path);
  if (!table) return 0;

  data->object = table;
  data->bytes = table->header.bytes;
  data->size = table->size;
  return 1;
}

static void
unloadTextTable (TableData *data) {
  destroyTextTable(data->object);
}

static int
loadAttributesTable (const char *path, TableData *data) {
  AttributesTable *table = compileAttributesTable(path);
  if (!table) return 0;

  data->object = table;
  data->bytes = table->header.bytes;
  data->size = table->size;
  return 1;
}

static void
unloadAttributesTable (TableData *data) {
  destroyAttributesTable(data->object);
}

static int
loadContractionTable (const char *path, TableData *data) {
  ContractionTable *table = compileContractionTable(path);
  if (!table) return 0;

  data->object = table;
  data->bytes = table->header.bytes;
  data->size = table->size;
  return 1;
}

static void
unloadContractionTable (TableData *data) {
  destroyContractionTable(data->object);
}

static const TableEntry tableEntries[] = {
  {
    .extension = TEXT_TABLE_EXTENSION,
    .load = loadTextTable,
    .unload = unloadTextTable
  }
  ,
  {
    .extension = ATTRIBUTES_TABLE_EXTENSION,
    .load = loadAttributesTable,
    .unload = unloadAttributesTable
  }
  ,
  {
    .extension = CONTRACTION_TABLE_EXTENSION,
    .load = loadContractionTable,
    .unload = unloadContractionTable
  }
  ,
  {
    .extension = NULL
  }
};

static const TableEntry *
findTableEntry (const char *extension) {
  const TableEntry *entry = tableEntries;

  while (entry->extension) {
    if (strcmp(entry->extension, extension) == 0) return entry;
    entry += 1;
  }

  LogPrint(LOG_ERR, "unrecognized file extension: %s", extension);
  return NULL;
}

int
dumpBytes (FILE *stream, const unsigned char *bytes, size_t count) {
  const unsigned char *byte = bytes;
  const unsigned char *end = byte + count;
  int first = 1;
  int digits;

  if (count) {
    char buffer[0X10];
    snprintf(buffer, sizeof(buffer), "%X%n", (unsigned int)count-1, &digits);
  } else {
    digits = 1;
  }

  while (byte < end) {
    while (!*byte && (byte < (end - 1))) byte += 1;

    {
      unsigned int counter = 0;
      unsigned int maximum = 8;

      if ((byte + maximum) != end) {
        while (maximum > 1) {
          if (byte[maximum-1]) break;
          maximum -= 1;
        }
      }

      while (byte < end) {
        if (first) {
          first = 0;
        } else {
          fprintf(stream, ",");
          if (ferror(stream)) goto outputError;

          if (!counter) {
            fprintf(stream, "\n");
            if (ferror(stream)) goto outputError;
          }
        }

        if (!counter) {
          fprintf(stream, "[0X%0*X] =", digits, (unsigned int)(byte-bytes));
          if (ferror(stream)) goto outputError;
        }

        fprintf(stream, " 0X%02X", *byte++);
        if (ferror(stdout)) goto outputError;

        if (++counter == maximum) break;
      }
    }
  }

  if (!first) {
    fprintf(stream, "\n");
    if (ferror(stream)) goto outputError;
  }

  return 1;

outputError:
  LogPrint(LOG_ERR, "table write error: %s", strerror(errno));
  return 0;
}

int
main (int argc, char *argv[]) {
  int status;
  char *path;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "tbl2hex",
      .argumentsSummary = "table-file"
    };
    processOptions(&descriptor, &argc, &argv);
  }

  if (argc == 0) {
    LogPrint(LOG_ERR, "missing table file.");
    exit(2);
  }
  path = *argv++, argc--;

  {
    const TableEntry *entry = findTableEntry(locatePathExtension(path));

    if (entry) {
      TableData data;
      if (entry->load(path, &data)) {
        if (dumpBytes(stdout, data.bytes, data.size)) {
          status = 0;
        } else {
          status = 4;
        }

        entry->unload(&data);
      } else {
        status = 3;
      }
    } else {
      status = 5;
    }
  }

  return status;
}
