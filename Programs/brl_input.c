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

#include <stdio.h>

#include "log.h"
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

  if (!isSuspended) {
#ifdef ENABLE_API
    apiClaimDriver();
#endif /* ENABLE_API */

    if (processInput()) processed = 1;

#ifdef ENABLE_API
    apiReleaseDriver();
#endif /* ENABLE_API */
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

  if (apiStarted) {
    if (!api_flush(&brl)) {
      restartRequired = 1;
    }
  }
#endif /* ENABLE_API */

  return processed;
}

static void setPollAlarm (int delay, void *data);
static AsyncHandle pollAlarm = NULL;

static void
handlePollAlarm (const AsyncAlarmResult *result) {
  asyncDiscardHandle(pollAlarm);
  pollAlarm = NULL;

  setPollAlarm((handleInput()? 0: 40), result->data);
}

static void
setPollAlarm (int delay, void *data) {
  if (!pollAlarm) {
    asyncSetAlarmIn(&pollAlarm, delay, handlePollAlarm, data);
  }
}

static int
monitorInput (const AsyncMonitorResult *result) {
  handleInput();
  return 1;
}

void
startBrailleCommands (void) {
  if (brl.gioEndpoint) {
    if (gioMonitorInput(brl.gioEndpoint, monitorInput, NULL)) {
      return;
    }
  }

  setPollAlarm(0, NULL);
}

void
stopBrailleCommands (void) {
  if (pollAlarm) {
    asyncCancelRequest(pollAlarm);
    pollAlarm = NULL;
  }
}
