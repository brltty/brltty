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
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>

#include "program.h"
#include "options.h"
#include "ktb_cmds.h"

BEGIN_OPTION_TABLE(programOptions)
END_OPTION_TABLE

void
commandGroupHook_hotkeys (CommandGroupHookData *data) {
}

void
commandGroupHook_keyboardFunctions (CommandGroupHookData *data) {
}

int
main (int argc, char *argv[]) {
  ProgramExitStatus exitStatus = PROG_EXIT_FATAL;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "brltty-cmdlist"
    };

    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  return exitStatus;
}
