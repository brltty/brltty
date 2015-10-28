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
  struct {
    ReportListenerInstance *brailleWindowMoved;
    ReportListenerInstance *brailleWindowUpdated;
  } reportListeners;
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

static void
handleBrailleWindowUpdated (const BrailleWindowUpdatedReport *report, TouchCommandData *tcd) {
}

REPORT_LISTENER(brailleWindowMovedListener) {
  TouchCommandData *tcd = parameters->listenerData;
  const BrailleWindowMovedReport *report = parameters->reportData;

  handleBrailleWindowMoved(report, tcd);
}

REPORT_LISTENER(brailleWindowUpdatedListener) {
  TouchCommandData *tcd = parameters->listenerData;
  const BrailleWindowUpdatedReport *report = parameters->reportData;

  handleBrailleWindowUpdated(report, tcd);
}

static TouchCommandData *
newTouchCommandData (void) {
  TouchCommandData *tcd;

  if ((tcd = malloc(sizeof(*tcd)))) {
    memset(tcd, 0, sizeof(*tcd));

    if ((tcd->reportListeners.brailleWindowMoved = registerReportListener(REPORT_BRAILLE_WINDOW_MOVED, brailleWindowMovedListener, tcd))) {
      if ((tcd->reportListeners.brailleWindowUpdated = registerReportListener(REPORT_BRAILLE_WINDOW_UPDATED, brailleWindowUpdatedListener, tcd))) {
        return tcd;
      }

      unregisterReportListener(tcd->reportListeners.brailleWindowMoved);
    }

    free(tcd);
  } else {
    logMallocError();
  }

  return NULL;
}

static void
destroyTouchCommandData (TouchCommandData *tcd) {
  unregisterReportListener(tcd->reportListeners.brailleWindowUpdated);
  unregisterReportListener(tcd->reportListeners.brailleWindowMoved);
  free(tcd);
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
destructTouchCommandData (void *data) {
  TouchCommandData *tcd = data;

  destroyTouchCommandData(tcd);
}

int
addTouchCommands (void) {
  TouchCommandData *tcd;

  if ((tcd = newTouchCommandData())) {
    if (pushCommandHandler("touch", KTB_CTX_DEFAULT, handleTouchCommands,
                           destructTouchCommandData, tcd)) {
      return 1;
    }

    destroyTouchCommandData(tcd);
  }

  return 0;
}
