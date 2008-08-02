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

#include "prologue.h"

#include <stdio.h>
#include <string.h>

#include "options.h"
#include "misc.h"

#include "ttb.h"
#include "ttb_internal.h"

#include "atb.h"
#include "atb_internal.h"

#include "ctb.h"
#include "ctb_internal.h"

BEGIN_OPTION_TABLE(programOptions)
END_OPTION_TABLE

int
dumpBytes (FILE *stream, const unsigned char *bytes, size_t count) {
  const unsigned char *byte = bytes;
  const unsigned char *end = byte + count;
  int first = 1;
  int digits;

  if (count) {
    char buffer[0X10];
    snprintf(buffer, sizeof(buffer), "%X%n", count-1, &digits);
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
          if (ferror(stream)) return 0;

          if (!counter) {
            fprintf(stream, "\n");
            if (ferror(stream)) return 0;
          }
        }

        if (!counter) {
          fprintf(stream, "[0X%0*X] =", digits, byte-bytes);
          if (ferror(stream)) return 0;
        }

        fprintf(stream, " 0X%02X", *byte++);
        if (ferror(stdout)) return 0;

        if (++counter == maximum) break;
      }
    }
  }

  if (!first) {
    fprintf(stream, "\n");
    if (ferror(stream)) return 0;
  }

  return 1;
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
    const char *extension = locatePathExtension(path);

    if (strcmp(extension, TEXT_TABLE_EXTENSION) == 0) {
      TextTable *table;

      if ((table = compileTextTable(path))) {
        if (dumpBytes(stdout, table->header.bytes, table->size)) {
          status = 0;
        } else {
          status = 4;
        }

        destroyTextTable(table);
      } else {
        status = 3;
      }
    } else

    if (strcmp(extension, ATTRIBUTES_TABLE_EXTENSION) == 0) {
      AttributesTable *table;

      if ((table = compileAttributesTable(path))) {
        if (dumpBytes(stdout, table->header.bytes, table->size)) {
          status = 0;
        } else {
          status = 4;
        }

        destroyAttributesTable(table);
      } else {
        status = 3;
      }
    } else

    if (strcmp(extension, CONTRACTION_TABLE_EXTENSION) == 0) {
      ContractionTable *table;

      if ((table = compileContractionTable(path))) {
        if (dumpBytes(stdout, table->header.bytes, table->size)) {
          status = 0;
        } else {
          status = 4;
        }

        destroyContractionTable(table);
      } else {
        status = 3;
      }
    } else

    {
      LogPrint(LOG_ERR, "unrecognized file extension: %s", extension);
      status = 5;
    }
  }

  return status;
}
