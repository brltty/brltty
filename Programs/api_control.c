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
#include "api_control.h"
#include "async_alarm.h"
#include "brltty.h"

#ifndef ENABLE_API
void
api_identify (int full) {
}

const char *const api_parameters[] = {NULL};

int
api_start (BrailleDisplay *brl, char **parameters) {
  return 0;
}

void
api_stop (BrailleDisplay *brl) {
}

void
api_link (BrailleDisplay *brl) {
}

void
api_unlink (BrailleDisplay *brl) {
}

void
api_suspend (BrailleDisplay *brl) {
}

int
api_resume (BrailleDisplay *brl) {
  return 0;
}

int
api_claimDriver (BrailleDisplay *brl) {
  return 0;
}

void
api_releaseDriver (BrailleDisplay *brl) {
}

int
api_handleCommand (int command) {
  return command;
}

int
api_handleKeyEvent (unsigned char set, unsigned char key, int press) {
  return 0;
}

int
api_flush (BrailleDisplay *brl) {
  return 0;
}
#endif /* ENABLE_API */

int apiStarted;
static int driverClaimed;

static void setFlushAlarm (int delay, void *data);
static AsyncHandle flushAlarm = NULL;

static void
handleFlushAlarm (const AsyncAlarmCallbackParameters *parameters) {
  asyncDiscardHandle(flushAlarm);
  flushAlarm = NULL;

  api_flush(&brl);
  setFlushAlarm(100, parameters->data);
}

static void
setFlushAlarm (int delay, void *data) {
  if (!flushAlarm) {
    asyncSetAlarmIn(&flushAlarm, delay, handleFlushAlarm, data);
  }
}

static void
stopFlushAlarm () {
  if (flushAlarm) {
    asyncCancelRequest(flushAlarm);
    flushAlarm = NULL;
  }
}

int
apiStart (char **parameters) {
  if (api_start(&brl, parameters)) {
    apiStarted = 1;
    setFlushAlarm(0, NULL);
    return 1;
  }

  return 0;
}

void
apiStop (void) {
  api_stop(&brl);
  apiStarted = 0;
  stopFlushAlarm();
}

void
apiLink (void) {
  if (apiStarted) api_link(&brl);
}

void
apiUnlink (void) {
  if (apiStarted) api_unlink(&brl);
}

int
apiClaimDriver (void) {
  if (!driverClaimed && apiStarted) {
    if (!api_claimDriver(&brl)) return 0;
    driverClaimed = 1;
  }

  return 1;
}

void
apiReleaseDriver (void) {
  if (driverClaimed) {
    api_releaseDriver(&brl);
    driverClaimed = 0;
  }
}
