/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include "log.h"
#include "hostcmd.h"

#if defined(USE_HOSTCMD_PACKAGE_NONE)
#include "hostcmd_none.h"
#elif defined(USE_HOSTCMD_PACKAGE_UNIX)
#include "hostcmd_unix.h"
#else /* serial package */
#error host command package not selected
#include "hostcmd_none.h"
#endif /* host command package */

#include "hostcmd_internal.h"

int
processHostCommandStreams (HostCommandStreamProcessor *processor, HostCommandStream *hcs) {
  while (hcs->file) {
    if (*hcs->file) {
      if (!processor(hcs)) return 0;
    }

    hcs += 1;
  }

  return 1;
}

void
initializeHostCommandOptions (HostCommandOptions *options) {
  options->asynchronous = 0;

  options->standardInput = NULL;
  options->standardOutput = NULL;
  options->standardError = NULL;
}

static int
constructHostCommandStream (HostCommandStream *hcs) {
  **hcs->file = NULL;
  subconstructHostCommandStream(hcs);
  return 1;
}

static int
destructHostCommandStream (HostCommandStream *hcs) {
  subdestructHostCommandStream(hcs);

  if (**hcs->file) {
    fclose(**hcs->file);
    **hcs->file = NULL;
  }

  return 1;
}

int
runHostCommand (
  const char *const *command,
  const HostCommandOptions *options
) {
  int result = 0XFF;
  HostCommandOptions defaults;

  if (!options) {
    initializeHostCommandOptions(&defaults);
    options = &defaults;
  }

  logMessage(LOG_DEBUG, "starting host command: %s", command[0]);

  {
    HostCommandStream streams[] = {
      { .file = &options->standardInput,
        .descriptor = 0,
        .input = 1
      },

      { .file = &options->standardOutput,
        .descriptor = 1,
        .input = 0
      },

      { .file = &options->standardError,
        .descriptor = 2,
        .input = 0
      },

      { .file = NULL }
    };

    if (processHostCommandStreams(constructHostCommandStream, streams)) {
      int ok = 0;

      if (processHostCommandStreams(prepareHostCommandStream, streams)) {
        if (runCommand(&result, command, streams, options->asynchronous)) ok = 1;
      }

      if (!ok) processHostCommandStreams(destructHostCommandStream, streams);
    }
  }

  return result;
}
