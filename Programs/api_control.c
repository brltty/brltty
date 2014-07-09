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

#include "log.h"
#include "api_control.h"
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
  return 0;
}

int
api_handleKeyEvent (KeyGroup group, KeyNumber number, int press) {
  return 0;
}

int
api_flush (BrailleDisplay *brl) {
  return 0;
}
#endif /* ENABLE_API */

int apiStarted;
static int driverClaimed;

int
apiStart (char **parameters) {
  if (api_start(&brl, parameters)) {
    apiStarted = 1;
    return 1;
  }

  return 0;
}

void
apiStop (void) {
  api_stop(&brl);
  apiStarted = 0;
}

void
apiLink (void) {
  if (apiStarted) api_link(&brl);
}

void
apiUnlink (void) {
  if (apiStarted) api_unlink(&brl);
}

void
apiSuspend (void) {
#ifdef ENABLE_API
  if (apiStarted) {
    api_suspend(&brl);
  } else
#endif /* ENABLE_API */

  {
    destructBrailleDriver();
  }
}

int
apiResume (void) {
#ifdef ENABLE_API
  if (apiStarted) return api_resume(&brl);
#endif /* ENABLE_API */

  return constructBrailleDriver();
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

int
apiFlush (void) {
  if (!apiStarted) return 1;
  return api_flush(&brl);
}
