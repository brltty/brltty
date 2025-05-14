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
#include "async_wait.h"
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
  const char *prompt;
  LearnModeState state;
} LearnModeData;

static int
showText (const char *text, int wait, LearnModeData *lmd) {
  MessageOptions options = MSG_SYNC;
  if (!wait) options |= MSG_NODELAY;
  return message(lmd->mode, text, options);
}

static int
showPrompt (LearnModeData *lmd) {
  return showText(lmd->prompt, 0, lmd);
}

static int
handleLearnModeCommands (int command, void *data) {
  LearnModeData *lmd = data;

  logMessage(LOG_DEBUG, "learn: command=%06X", command);
  lmd->state = LMS_CONTINUE;

  switch (command & BRL_MSK_CMD) {
    case BRL_CMD_LEARN:
      lmd->state = LMS_EXIT;
      return 1;

    case BRL_CMD_NOOP:
      return 1;

    default:
      switch (command & BRL_MSK_BLK) {
        case BRL_CMD_BLK(TOUCH_AT):
          return 1;

        default:
          break;
      }
      break;
  }

  {
    char description[0X100];

    describeCommand(
      description, sizeof(description), command,
      (CDO_IncludeName | CDO_IncludeOperand)
    );

    logMessage(LOG_DEBUG, "learn: %s", description);
    if (!showText(description, 1, lmd)) lmd->state = LMS_ERROR;
  }

  if (!showPrompt(lmd)) lmd->state = LMS_ERROR;
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
    .prompt = gettext("Learn Mode"),
  };

  pushCommandEnvironment("learnMode", NULL, NULL);
  pushCommandHandler("learnMode", KTB_CTX_DEFAULT,
                     handleLearnModeCommands, NULL, &lmd);

  if (setStatusText(&brl, lmd.mode)) {
    if (showPrompt(&lmd)) {
      do {
        lmd.state = LMS_TIMEOUT;
        if (!asyncAwaitCondition(timeout, testEndLearnWait, &lmd)) break;
      } while (lmd.state == LMS_CONTINUE);

      if (lmd.state == LMS_TIMEOUT) {
        if (!showText(gettext("done"), 1, &lmd)) {
          lmd.state = LMS_ERROR;
        }
      }
    }
  }

  popCommandEnvironment();
  return lmd.state != LMS_ERROR;
}
