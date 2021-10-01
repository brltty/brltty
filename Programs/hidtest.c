/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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
#include "options.h"
#include "log.h"
#include "hid.h"
#include "io_usb.h"

BEGIN_OPTION_TABLE(programOptions)
END_OPTION_TABLE

int
main (int argc, char *argv[]) {
  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "hidtest",
      .argumentsSummary = "device"
    };

    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  if (!argc) {
    logMessage(LOG_ERR, "device not specified");
    return PROG_EXIT_SYNTAX;
  }

  const char *deviceIdentifier = *argv++;
  argc -= 1;

  if (argc) {
    logMessage(LOG_ERR, "too many parameters");
    return PROG_EXIT_SYNTAX;
  }

  logMessage(LOG_NOTICE, "device: %s", deviceIdentifier);
  return PROG_EXIT_SUCCESS;
}
