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

#include <string.h>

#include "log.h"
#include "learn.h"
#include "message.h"
#include "async_handle.h"
#include "async_wait.h"
#include "async_alarm.h"
#include "timing.h"
#include "stdio.h"
#include "cmd.h"
#include "cmd_queue.h"
#include "brl.h"
#include "brl_cmds.h"
#include "core.h"

typedef enum {
  LMS_CONTINUE,
  LMS_TIMEOUT,
  LMS_EXIT,
  LMS_ERROR
} LearnModeState;

typedef struct {
  const char *mode;
  LearnModeState state;

  struct {
    const char *text;
    AsyncHandle alarm;

    struct {
      int initial;
      int left;
    } seconds;
  } prompt;
} LearnModeData;

static int
showText (const char *text, MessageOptions options, LearnModeData *lmd) {
  return message(lmd->mode, text, (options | MSG_SYNC));
}

ASYNC_ALARM_CALLBACK(updateLearnModePrompt) {
  LearnModeData *lmd = parameters->data;
  int *secondsLeft = &lmd->prompt.seconds.left;

  {
    char prompt[0X100];
    snprintf(prompt, sizeof(prompt), "%s - %d", lmd->prompt.text, *secondsLeft);
    showText(prompt, (MSG_NODELAY | MSG_SILENT), lmd);
  }

  *secondsLeft -= 1;
}

static void
endPrompt (LearnModeData *lmd) {
  AsyncHandle *alarm = &lmd->prompt.alarm;

  if (*alarm) {
    asyncCancelRequest(*alarm);
    *alarm = NULL;
  }
}

static int
beginPrompt (LearnModeData *lmd) {
  endPrompt(lmd);
  AsyncHandle *alarm = &lmd->prompt.alarm;

  lmd->prompt.seconds.left = lmd->prompt.seconds.initial;

  if (asyncNewRelativeAlarm(alarm, 0, updateLearnModePrompt, lmd)) {
    if (asyncResetAlarmInterval(*alarm, MSECS_PER_SEC)) {
      sayMessage(lmd->prompt.text);
      return 1;
    }

    asyncCancelRequest(*alarm);
    *alarm = NULL;
  }

  return 0;
}

static int
handleLearnModeCommands (int command, void *data) {
  LearnModeData *lmd = data;
  endPrompt(lmd);

  logMessage(LOG_DEBUG, "learn: command=%06X", command);
  lmd->state = LMS_CONTINUE;

  switch (command & BRL_MSK_CMD) {
    case BRL_CMD_LEARN:
      lmd->state = LMS_EXIT;
      return 1;

    case BRL_CMD_NOOP:
      return 1;

    default: {
      switch (command & BRL_MSK_BLK) {
        case BRL_CMD_BLK(TOUCH_AT):
          return 1;

        default:
          break;
      }

      break;
    }
  }

  {
    char description[0X100];

    describeCommand(
      description, sizeof(description), command,
      (CDO_IncludeName | CDO_IncludeOperand)
    );

    logMessage(LOG_DEBUG, "learn: %s", description);
    if (!showText(description, 0, lmd)) lmd->state = LMS_ERROR;
  }

  if (!beginPrompt(lmd)) lmd->state = LMS_ERROR;
  return 1;
}

ASYNC_CONDITION_TESTER(testEndLearnWait) {
  LearnModeData *lmd = data;

  return lmd->state != LMS_TIMEOUT;
}

int
learnMode (int timeout) {
  LearnModeData lmd = {
    .mode = "lrn",

    .prompt = {
      .text = gettext("Learn Mode"),
      .alarm = NULL,
      .seconds.initial = timeout / MSECS_PER_SEC,
    },
  };

  pushCommandEnvironment("learnMode", NULL, NULL);
  pushCommandHandler("learnMode", KTB_CTX_DEFAULT,
                     handleLearnModeCommands, NULL, &lmd);

  if (setStatusText(&brl, lmd.mode)) {
    if (beginPrompt(&lmd)) {
      do {
        lmd.state = LMS_TIMEOUT;
        if (!asyncAwaitCondition(timeout, testEndLearnWait, &lmd)) break;
      } while (lmd.state == LMS_CONTINUE);

      endPrompt(&lmd);

      if (lmd.state == LMS_TIMEOUT) {
        if (!showText(gettext("done"), 0, &lmd)) {
          lmd.state = LMS_ERROR;
        }
      }
    }
  }

  popCommandEnvironment();
  return lmd.state != LMS_ERROR;
}
