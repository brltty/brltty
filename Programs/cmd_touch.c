/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>

#include "log.h"
#include "cmd_utils.h"
#include "cmd_queue.h"
#include "cmd_touch.h"
#include "brl_cmds.h"
#include "report.h"

typedef struct {
  ReportListenerInstance *brailleWindowMovedListener;
} TouchCommandData;

static void
handleTouchAt (int offset, TouchCommandData *tcd) {
}

static void
handleTouchOff (TouchCommandData *tcd) {
}

static void
handleBrailleWindowMoved (const BrailleWindowMovedReport *report, TouchCommandData *tcd) {
}

REPORT_LISTENER(brailleWindowMovedListener) {
  TouchCommandData *tcd = parameters->listenerData;
  const BrailleWindowMovedReport *report = parameters->reportData;

  handleBrailleWindowMoved(report, tcd);
}

static int
constructTouchCommandData (TouchCommandData *tcd) {
  if ((tcd->brailleWindowMovedListener = registerReportListener(REPORT_BRAILLE_WINDOW_MOVED, brailleWindowMovedListener, tcd))) {
    return 1;
  }

  return 0;
}

static void
destructTouchCommandData (TouchCommandData *tcd) {
  unregisterReportListener(tcd->brailleWindowMovedListener);
}

static int
handleTouchCommands (int command, void *data) {
  switch (command & BRL_MSK_BLK) {
    case BRL_CMD_BLK(TOUCH): {
      int arg = command & BRL_MSK_ARG;

      if (arg == BRL_MSK_ARG) {
        handleTouchOff(data);
      } else if (isTextOffset(&arg, 0, 0)) {
        handleTouchAt(arg, data);
      }

      break;
    }

    default:
      return 0;
  }

  return 1;
}

static void
destroyTouchCommandData (void *data) {
  TouchCommandData *tcd = data;

  destructTouchCommandData(tcd);
  free(tcd);
}

int
addTouchCommands (void) {
  TouchCommandData *tcd;

  if ((tcd = malloc(sizeof(*tcd)))) {
    memset(tcd, 0, sizeof(*tcd));

    if (constructTouchCommandData(tcd)) {
      if (pushCommandHandler("touch", KTB_CTX_DEFAULT,
                             handleTouchCommands, destroyTouchCommandData, tcd)) {
        return 1;
      }

      destructTouchCommandData(tcd);
    }

    free(tcd);
  } else {
    logMallocError();
  }

  return 0;
}
