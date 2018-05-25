/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
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

#include <stdio.h>

#include "log.h"
#include "program.h"
#include "options.h"

BEGIN_OPTION_TABLE(programOptions)
END_OPTION_TABLE

static void
logFileName (const char *name, void *data) {
  printf("%s\n", name);
}

static DATA_OPERANDS_PROCESSOR(processUnknownDirective) {
  return 1;
}

static DATA_OPERANDS_PROCESSOR(processOperands) {
  BEGIN_DATA_DIRECTIVE_TABLE
    DATA_NESTING_DIRECTIVES,
    DATA_CONDITION_DIRECTIVES,
    DATA_VARIABLE_DIRECTIVES,
    {NULL, processUnknownDirective},
  END_DATA_DIRECTIVE_TABLE

  return processDirectiveOperand(file, &directives, "attributes table directive", data);
}

int
main (int argc, char *argv[]) {
  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "brltty-lsinc"
    };

    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  if (argc == 0) {
    logMessage(LOG_ERR, "missing table file.");
    return PROG_EXIT_SYNTAX;
  }

  do {
    const char *path = *argv++;
    argc -= 1;

    const DataFileParameters parameters = {
      .processOperands = processOperands,
      .logFileName = logFileName
    };

    processDataFile(path, &parameters);
  } while (argc);

  return PROG_EXIT_SUCCESS;
}
