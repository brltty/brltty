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

#include "log.h"
#include "cmdprog.h"
#include "file.h"
#include "parse.h"
#include "program.h"
#include "options.h"

const char standardStreamArgument[] = "-";
const char standardInputName[] = "<standard-input>";
const char standardOutputName[] = "<standard-output>";
const char standardErrorName[] = "<standard-error>";

const char *
nextProgramArgument (char ***argv, int *argc, const char *description) {
  if (*argc) {
    *argc -= 1;
    return *(*argv)++;
  }

  if (!description) return NULL;
  logMessage(LOG_ERR, "missing %s", description);
  exit(PROG_EXIT_SYNTAX);
}

void
noMoreProgramArguments (char ***argv, int *argc) {
  if (*argc) {
    logMessage(LOG_ERR, "too many arguments: %s", (*argv)[0]);
    exit(PROG_EXIT_SYNTAX);
  }
}

int
toAbsoluteInstallPath (char **path) {
  const char *programDirectory = getProgramDirectory();
  if (!programDirectory) programDirectory = CURRENT_DIRECTORY_NAME;
  return anchorRelativePath(path, programDirectory);
}

char *opt_helpersDirectory = HELPERS_DIRECTORY;

char *
makeHelperPath (const char *name) {
  char *path = NULL;
  char *directory = NULL;

  if (changeStringSetting(&directory, opt_helpersDirectory)) {
    if (toAbsoluteInstallPath(&directory)) {
      path = makePath(directory, name);
    }
  }

  if (directory) free(directory);
  return path;
}

