/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "options.h"
#include "misc.h"
#include "brl.h"
#include "tbl.h"

BEGIN_OPTION_TABLE
END_OPTION_TABLE

static int
handleOption (const int option) {
  switch (option) {
    default:
      return 0;
  }
  return 1;
}

static void
reportMessage (const char *message) {
  fprintf(stderr, "%s: %s\n", programName, message);
}

int
main (int argc, char *argv[]) {
  int status;
  char *path;

  processOptions(optionTable, optionCount, handleOption,
                 &argc, &argv,
                 NULL, NULL, NULL,
                 "translation-table");

  if (argc == 0) {
    fprintf(stderr, "%s: missing translation table.\n", programName);
    exit(2);
  }
  path = *argv++, argc--;

  if ((path = makePath(DATA_DIRECTORY, path))) {
    TranslationTable table;
    if (loadTranslationTable(path, &table, reportMessage,
                             TBL_UNDEFINED | TBL_DUPLICATE)) {
      status = 0;
    } else {
      status = 3;
    }
    free(path);
  } else {
    status = 3;
  }
  return status;
}
