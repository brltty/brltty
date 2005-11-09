/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

/* tbl2hex.c - filter to compile 256-byte table file into C code
 * $Id: tbl2hex.c,v 1.3 1996/09/24 01:04:25 nn201 Exp $
 */

#include "prologue.h"

#include <stdio.h>

#include "options.h"
#include "misc.h"
#include "brl.h"
#include "tbl.h"

BEGIN_OPTION_TABLE
END_OPTION_TABLE

int
main (int argc, char *argv[]) {
  int status;
  char *path;
  TranslationTable table;

  processOptions(optionTable, optionCount,
                 "tbl2hex", &argc, &argv,
                 NULL, NULL, NULL,
                 "translation-table");

  if (argc == 0) {
    LogPrint(LOG_ERR, "missing translation table.");
    exit(2);
  }
  path = *argv++, argc--;

  if (loadTranslationTable(path, NULL, table, 0)) {
    unsigned int columns = 8;
    unsigned int rows = 0X100 / columns;
    unsigned int row;

    for (row=0; row<rows; row++) {
      const unsigned char *buffer = &table[row * columns];
      unsigned int column;

      for (column=0; column<columns; column++) {
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
