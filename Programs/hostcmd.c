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
#elif defined(USE_HOSTCMD_PACKAGE_WINDOWS)
#include "hostcmd_windows.h"
#else /* host command package */
#error host command package not selected
#include "hostcmd_none.h"
#endif /* host command package */

#include "hostcmd_internal.h"

int
finishHostCommandStream (HostCommandStream *hcs, int fileDescriptor) {
  const char *mode = hcs->isInput? "w": "r";

  if ((**hcs->streamVariable = fdopen(fileDescriptor, mode))) {
    return 1;
  } else {
    logSystemError("fdopen");
  }

  return 0;
}

int
processHostCommandStreams (
  HostCommandStream *hcs,
  HostCommandStreamProcessor *processStream,
  void *data
) {
  while (hcs->streamVariable) {
    if (*hcs->streamVariable) {
      if (!processStream(hcs, data)) {
        return 0;
      }
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
constructHostCommandStream (HostCommandStream *hcs, void *data) {
  **hcs->streamVariable = NULL;
  return constructHostCommandPackageData(&hcs->package);
}

static int
destructHostCommandStream (HostCommandStream *hcs, void *data) {
  destructHostCommandPackageData(&hcs->package);

  if (**hcs->streamVariable) {
    fclose(**hcs->streamVariable);
    **hcs->streamVariable = NULL;
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
      { .streamVariable = &options->standardInput,
        .fileDescriptor = 0,
        .isInput = 1
      },

      { .streamVariable = &options->standardOutput,
        .fileDescriptor = 1,
        .isInput = 0
      },

      { .streamVariable = &options->standardError,
        .fileDescriptor = 2,
        .isInput = 0
      },

      { .streamVariable = NULL }
    };

    if (processHostCommandStreams(streams, constructHostCommandStream, NULL)) {
      int ok = 0;

      if (processHostCommandStreams(streams, prepareHostCommandStream, NULL)) {
        if (runCommand(&result, command, streams, options->asynchronous)) {
          ok = 1;
        }
      }

      if (!ok) processHostCommandStreams(streams, destructHostCommandStream, NULL);
    }
  }

  return result;
}

int
executeHostCommand (const char *const *command) {
  HostCommandOptions options;

  initializeHostCommandOptions(&options);
  return runHostCommand(command, &options);
}
