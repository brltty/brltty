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
#include "parameters.h"
#include "brl_input.h"
#include "cmd_queue.h"
#include "async_alarm.h"
#include "api_control.h"
#include "brltty.h"

static int
processInput (void) {
  int command = readBrailleCommand(&brl, getCurrentCommandContext());

  if (command != EOF) {
    switch (command & BRL_MSK_CMD) {
      case BRL_CMD_OFFLINE:
        if (!isOffline) {
          logMessage(LOG_DEBUG, "braille display offline");
          isOffline = 1;
        }
        return 0;

      default:
        break;
    }
  }

  if (isOffline) {
    logMessage(LOG_DEBUG, "braille display online");
    isOffline = 0;
  }

  if (command == EOF) return 0;
  enqueueCommand(command);
  return 1;
}

static int
handleInput (void) {
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

static void setBrailleInputAlarm (int delay, void *data);
static AsyncHandle brailleInputAlarm = NULL;

ASYNC_ALARM_CALLBACK(handleBrailleInputAlarm) {
  asyncDiscardHandle(brailleInputAlarm);
  brailleInputAlarm = NULL;

  setBrailleInputAlarm((handleInput()? 0: BRAILLE_INPUT_POLL_INTERVAL), parameters->data);
}

static void
setBrailleInputAlarm (int delay, void *data) {
  if (!brailleInputAlarm) {
    asyncSetAlarmIn(&brailleInputAlarm, delay, handleBrailleInputAlarm, data);
  }
}

ASYNC_MONITOR_CALLBACK(monitorBrailleInput) {
  handleInput();
  return !restartRequired;
}

void
startBrailleInput (void) {
  if (brl.gioEndpoint) {
    if (gioMonitorInput(brl.gioEndpoint, monitorBrailleInput, NULL)) {
      handleInput();
      return;
    }
  }

  setBrailleInputAlarm(0, NULL);
}

void
stopBrailleInput (void) {
  if (brailleInputAlarm) {
    asyncCancelRequest(brailleInputAlarm);
    brailleInputAlarm = NULL;
  }
}
