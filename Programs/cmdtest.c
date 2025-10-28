/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2025 by The BRLTTY Developers.
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

#include "program.h"
#include "cmdline.h"
#include "log.h"
#include "parse.h"

static int flagOption;
static int counterOption;

static char *textOption;
static char *listOption;

BEGIN_COMMAND_LINE_OPTIONS(programOptions)
  { .word = "flag",
    .letter = 'f',
    .setting.flag = &flagOption,
    .description = "defaults to false - true if specified at least once",
  },

  { .word = "counter",
    .letter = 'c',
    .flags = OPT_Extend,
    .setting.flag = &counterOption,
    .description = "counts the number of times this option is specified",
  },

  { .word = "text",
    .letter = 't',
    .argument = "text",
    .internal.setting = "this is the default text (because this option wasn't specified)",
    .setting.string = &textOption,
    .description = "arbitrary text - rightmost specification wins",
  },

  { .word = "list",
    .letter = 'l',
    .argument = "item",
    .flags = OPT_Extend,
    .setting.string = &listOption,
    .description = "each specification adds an item to the list",
  },
END_COMMAND_LINE_OPTIONS(programOptions)

static const char *firstParameter;
static const char *secondParameter;

BEGIN_COMMAND_LINE_PARAMETERS(programParameters)
  { .name = "first",
    .description = "this parameter is required",
    .setting = &firstParameter,
  },

  { .name = "second",
    .description = "this parameter is optional",
    .setting = &secondParameter,
    .optional = 1,
  },
END_COMMAND_LINE_PARAMETERS(programParameters)

int
main (int argc, char *argv[]) {
  int logLevel = LOG_NOTICE;

  {
    const CommandLineDescriptor descriptor = {
      .options = &programOptions,
      .parameters = &programParameters,

      .extraParameters = {
         .name = "extra",
         .description = "zero or more additional parameters",
      },

      .applicationName = "cmdtest",

      .usage = {
        .purpose = "Test the command line parser.",
      }
    };

    PROCESS_COMMAND_LINE(descriptor, argc, argv);
  }

  logMessage(logLevel, "Flag: %s", (flagOption? fkpTrueFalse.on: fkpTrueFalse.off));
  logMessage(logLevel, "Counter: %d", counterOption);

  logMessage(logLevel, "Text: %s", textOption);
  logMessage(logLevel, "List: %s", listOption);

  logMessage(logLevel, "First Parameter: %s", firstParameter);
  logMessage(logLevel, "Second Parameter: %s", secondParameter);

  for (int index=0; index<argc; index+=1) {
    logMessage(logLevel, "Extra Parameter: %s", argv[index]);
  }

  return PROG_EXIT_SUCCESS;
}
