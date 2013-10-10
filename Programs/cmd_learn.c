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

#include <string.h>

#include "log.h"
#include "message.h"
#include "async_call.h"
#include "async_wait.h"
#include "ktbdefs.h"
#include "cmd_queue.h"
#include "cmd_learn.h"
#include "cmd.h"
#include "brl.h"
#include "brltty.h"

typedef enum {
  LMS_CONTINUE,
  LMS_TIMEOUT,
  LMS_EXIT,
  LMS_ERROR
} LearnModeState;

typedef struct {
  const char *mode;
  LearnModeState state;
} LearnModeData;

static int
testEndLearnWait (void *data) {
  LearnModeData *lmd = data;

  return lmd->state != LMS_TIMEOUT;
}

static int
handleLearnCommand (int command, void *data) {
  LearnModeData *lmd = data;
  logMessage(LOG_DEBUG, "learn: command=%06X", command);

  switch (command & BRL_MSK_CMD) {
    case BRL_CMD_LEARN:
      lmd->state = LMS_EXIT;
      return 1;

    case BRL_CMD_NOOP:
      lmd->state = LMS_CONTINUE;
      return 1;

    default:
      lmd->state = LMS_CONTINUE;
      break;
  }

  {
    char buffer[0X100];

    describeCommand(command, buffer, sizeof(buffer),
                    CDO_IncludeName | CDO_IncludeOperand);

    logMessage(LOG_DEBUG, "learn: %s", buffer);
    if (!message(lmd->mode, buffer, MSG_SYNC|MSG_NODELAY)) lmd->state = LMS_ERROR;
  }

  return 1;
}

typedef struct {
  int timeout;
} LearnModeParameters;

static void
presentLearnMode (void *data) {
  LearnModeParameters *lmp = data;

  LearnModeData lmd = {
    .mode = "lrn"
  };

  suspendUpdates();
  pushCommandHandler(KTB_CTX_DEFAULT, handleLearnCommand, &lmd);

  if (setStatusText(&brl, lmd.mode)) {
    if (message(lmd.mode, gettext("Learn Mode"), MSG_SYNC|MSG_NODELAY)) {
      do {
        lmd.state = LMS_TIMEOUT;
        if (!asyncAwaitCondition(lmp->timeout, testEndLearnWait, &lmd)) break;
      } while (lmd.state == LMS_CONTINUE);

      if (lmd.state == LMS_TIMEOUT) {
        if (!message(lmd.mode, gettext("done"), MSG_SYNC)) {
          lmd.state = LMS_ERROR;
        }
      }
    }
  }

  popCommandHandler();
  resumeUpdates();
  free(lmp);
}

int
learnMode (int timeout) {
  LearnModeParameters *lmp;

  if ((lmp = malloc(sizeof(*lmp)))) {
    memset(lmp, 0, sizeof(*lmp));
    lmp->timeout = timeout;

    if (asyncCallFunction(NULL, presentLearnMode, lmp)) {
      return 1;
    }

    free(lmp);
  } else {
    logMallocError();
  }

  return 0;
}
