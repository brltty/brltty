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
  LRN_CONTINUE,
  LRN_TIMEOUT,
  LRN_EXIT,
  LRN_ERROR
} LearnState;

typedef struct {
  const char *mode;
  int timeout;

  LearnState state;
} LearnData;

static int
testEndLearnWait (void *data) {
  LearnData *lrn = data;

  return lrn->state != LRN_TIMEOUT;
}

static int
handleLearnCommand (int command, void *data) {
  LearnData *lrn = data;
  logMessage(LOG_DEBUG, "learn: command=%06X", command);

  switch (command & BRL_MSK_CMD) {
    case BRL_CMD_LEARN:
      lrn->state = LRN_EXIT;
      return 1;

    case BRL_CMD_NOOP:
      lrn->state = LRN_CONTINUE;
      return 1;

    default:
      lrn->state = LRN_CONTINUE;
      break;
  }

  {
    char buffer[0X100];

    describeCommand(command, buffer, sizeof(buffer),
                    CDO_IncludeName | CDO_IncludeOperand);
    logMessage(LOG_DEBUG, "learn: %s", buffer);
    if (!message(lrn->mode, buffer, MSG_NODELAY)) lrn->state = LRN_ERROR;
  }

  return 1;
}

static void
presentLearnMode (void *data) {
  LearnData *lrn = data;

  suspendUpdates();
  pushCommandHandler(KTB_CTX_DEFAULT, handleLearnCommand, lrn);

  if (setStatusText(&brl, lrn->mode)) {
    if (message(lrn->mode, gettext("Learn Mode"), MSG_NODELAY|MSG_SYNC)) {
      do {
        lrn->state = LRN_TIMEOUT;
        if (!asyncAwaitCondition(lrn->timeout, testEndLearnWait, lrn)) break;
      } while (lrn->state == LRN_CONTINUE);

      if (lrn->state == LRN_TIMEOUT) {
        if (!message(lrn->mode, gettext("done"), 0)) {
          lrn->state = LRN_ERROR;
        }
      }
    }
  }

  popCommandHandler();
  resumeUpdates();
  free(lrn);
}

int
learnMode (int timeout) {
  LearnData *lrn;

  if ((lrn = malloc(sizeof(*lrn)))) {
    memset(lrn, 0, sizeof(*lrn));
    lrn->mode = "lrn";
    lrn->timeout = timeout;

    if (asyncCallFunction(NULL, presentLearnMode, lrn)) {
      return 1;
    }

    free(lrn);
  } else {
    logMallocError();
  }

  return 0;
}
