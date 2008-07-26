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

#include "options.h"
#include "misc.h"
#include "attr.h"

BEGIN_OPTION_TABLE(programOptions)
END_OPTION_TABLE

int
main (int argc, char *argv[]) {
  int status;
  char *path;
  AttributesTable table;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "attr2hex",
      .argumentsSummary = "attributes-table"
    };
    processOptions(&descriptor, &argc, &argv);
  }

  if (argc == 0) {
    LogPrint(LOG_ERR, "missing attributes table.");
    exit(2);
  }
  path = *argv++, argc--;

  if (loadAttributesTable(path, table)) {
    unsigned int columns = 8;
    unsigned int rows = 0X100 / columns;
    unsigned int row;

    for (row=0; row<rows; row+=1) {
      const unsigned char *buffer = &table[row * columns];
      unsigned int column;

      for (column=0; column<columns; column+=1) {
        int newline = column == (columns - 1);
        int comma = (row < (rows - 1)) || !newline;

        printf("0X%02X", buffer[column]);
        if (ferror(stdout)) break;

        if (comma) {
          putchar(',');
          if (ferror(stdout)) break;
        }

        putchar(newline? '\n': ' ');
        if (ferror(stdout)) break;
      }

      if (!ferror(stdout)) fflush(stdout);
      status = ferror(stdout)? 4: 0;
    }
  } else {
    status = 3;
  }

  return status;
}
