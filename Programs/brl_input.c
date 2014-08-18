/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

#include <stdio.h>

#include "log.h"
#include "report.h"
#include "embed.h"
#include "parameters.h"
#include "ktb.h"
#include "brl_input.h"
#include "brl_cmds.h"
#include "cmd_queue.h"
#include "cmd_enqueue.h"
#include "update.h"
#include "io_generic.h"
#include "api_control.h"
#include "brltty.h"

static int
processInput (void) {
  int command = readBrailleCommand(&brl, getCurrentCommandContext());

  if (command != EOF) {
    switch (command & BRL_MSK_CMD) {
      case BRL_CMD_OFFLINE:
        if (!isOffline) {
          logMessage(LOG_DEBUG, "braille offline");
          isOffline = 1;

          {
            KeyTable *keyTable = brl.keyTable;

            if (keyTable) releaseAllKeys(keyTable);
          }

          report(REPORT_BRAILLE_OFF, NULL);
        }
        return 0;

      default:
        break;
    }
  }

  if (isOffline) {
    logMessage(LOG_DEBUG, "braille online");
    isOffline = 0;
    report(REPORT_BRAILLE_ON, NULL);
    scheduleUpdate("braille online");
  }

  if (command == EOF) return 0;
  enqueueCommand(command);
  return 1;
}

static
GIO_INPUT_HANDLER(handleBrailleInput) {
  int processed = 0;

  suspendCommandQueue();

  if (!isSuspended) {
    apiClaimDriver();
    if (processInput()) processed = 1;
    apiReleaseDriver();
  }

#ifdef ENABLE_API
  else if (apiStarted) {
    switch (readBrailleCommand(&brl, KTB_CTX_DEFAULT)) {
      case BRL_CMD_RESTARTBRL:
        restartBrailleDriver();
        break;

      default:
        processed = 1;
      case EOF:
        break;
    }
  }
#endif /* ENABLE_API */

  resumeCommandQueue();
  return processed;
}

static GioHandleInputObject *handleBrailleInputObject = NULL;

void
stopBrailleInput (void) {
  if (handleBrailleInputObject) {
    gioDestroyHandleInputObject(handleBrailleInputObject);
    handleBrailleInputObject = NULL;
  }
}

void
startBrailleInput (void) {
  stopBrailleInput();
  handleBrailleInputObject = gioNewHandleInputObject(brl.gioEndpoint, BRAILLE_INPUT_POLL_INTERVAL, handleBrailleInput, &brl);
}
