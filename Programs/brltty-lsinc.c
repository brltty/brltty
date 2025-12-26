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

#include <stdio.h>
#include <string.h>
#include <search.h>

#include "log.h"
#include "cmdline.h"
#include "cmdput.h"
#include "file.h"

BEGIN_COMMAND_LINE_OPTIONS(programOptions)
END_COMMAND_LINE_OPTIONS(programOptions)

static const char *firstFile;

BEGIN_COMMAND_LINE_PARAMETERS(programParameters)
  { .name = "file",
    .description = "the name of (or path to) the file to process",
    .setting = &firstFile,
  },
END_COMMAND_LINE_PARAMETERS(programParameters)

BEGIN_COMMAND_LINE_NOTES(programNotes)
END_COMMAND_LINE_NOTES

BEGIN_COMMAND_LINE_DESCRIPTOR(programDescriptor)
  .name = "brltty-lsinc",
  .purpose = strtext("List the paths to a data file and those which it recursively includes."),

  .options = &programOptions,
  .parameters = &programParameters,
  .notes = COMMAND_LINE_NOTES(programNotes),

  .extraParameters = {
    .name = "more",
    .description = "additional files to process",
  },
END_COMMAND_LINE_DESCRIPTOR

static void
noMemory (void) {
  logMallocError();
  exit(PROG_EXIT_FATAL);
}

static int
compareNames (const void *Name1, const void *Name2) {
  return strcmp(Name1, Name2);
}

static void
logFileName (const char *name, void *data) {
  static void *namesTree = NULL;

  if (!tfind(name, &namesTree, compareNames)) {
    name = strdup(name);
    if (!name) noMemory();
    if (!tsearch(name, &namesTree, compareNames)) noMemory();
    putf("%s\n", name);
  }
}

static DATA_CONDITION_TESTER(testConditionOperand) {
  return 1;
}

static DATA_OPERANDS_PROCESSOR(processUnknownDirective) {
  DataOperand directive;

  if (getDataOperand(file, &directive, NULL)) {
    if (directive.length >= 2) {
      if (isKeyword(WS_C("if"), directive.characters, 2)) {
        return processConditionOperands(file, testConditionOperand, 0, NULL, data);
      }
    }
  }

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

static int
processFile (const char *path) {
  const DataFileParameters parameters = {
    .processOperands = processOperands,
    .logFileName = logFileName
  };

  if (testProgramPath(path)) {
    logFileName(path, parameters.data);
  } else if (!processDataFile(path, &parameters)) {
    return 0;
  }

  return 1;
}

int
main (int argc, char *argv[]) {
  PROCESS_COMMAND_LINE(programDescriptor, argc, argv);

  int ok = processFile(firstFile);

  while (argc > 0) {
    if (!processFile(*argv++)) ok = 0;
    argc -= 1;
  }

  return ok? PROG_EXIT_SUCCESS: PROG_EXIT_SEMANTIC;
}
