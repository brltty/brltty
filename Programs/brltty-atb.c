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
#include "options.h"
#include "log.h"
#include "atb.h"

char *opt_tablesDirectory;

BEGIN_COMMAND_LINE_OPTIONS(programOptions)
  { .word = "tables-directory",
    .letter = 'T',
    .argument = strtext("directory"),
    .setting.string = &opt_tablesDirectory,
    .internal.setting = TABLES_DIRECTORY,
    .internal.adjust = toAbsoluteInstallPath,
    .description = strtext("Path to directory containing tables.")
  },
END_COMMAND_LINE_OPTIONS(programOptions)

static const char *tableName;

BEGIN_COMMAND_LINE_PARAMETERS(programParameters)
  { .name = "table",
    .description = "the name of (or path to) the attributes table",
    .setting = &tableName,
  },
END_COMMAND_LINE_PARAMETERS(programParameters)

BEGIN_COMMAND_LINE_NOTES(programNotes)
END_COMMAND_LINE_NOTES

BEGIN_COMMAND_LINE_DESCRIPTOR(programDescriptor)
  .name = "brltty-atb",
  .purpose = strtext("Check an attributes table."),

  .options = &programOptions,
  .parameters = &programParameters,
  .notes = COMMAND_LINE_NOTES(programNotes),
END_COMMAND_LINE_DESCRIPTOR

int
main (int argc, char *argv[]) {
  PROCESS_COMMAND_LINE(programDescriptor, argc, argv);

  ProgramExitStatus exitStatus = PROG_EXIT_SUCCESS;
  char *tablePath = makeAttributesTablePath(opt_tablesDirectory, tableName);

  if (tablePath) {
    if ((attributesTable = compileAttributesTable(tablePath))) {
      exitStatus = PROG_EXIT_SUCCESS;

      destroyAttributesTable(attributesTable);
    } else {
      exitStatus = PROG_EXIT_FATAL;
    }

    free(tablePath);
  } else {
    exitStatus = PROG_EXIT_FATAL;
  }

  return exitStatus;
}
