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
api_flush (BrailleDisplay *brl) {
  return 0;
}

int
api_handleCommand (int command) {
  return command;
}

int
api_handleKeyEvent (unsigned char set, unsigned char key, int press) {
  return 0;
}
#endif /* ENABLE_API */

int apiStarted;

static int apiDriverClaimed;

void
apiUnlink (void) {
  if (apiStarted) api_unlink(&brl);
}

void
apiLink (void) {
  if (apiStarted) api_link(&brl);
}

int
apiClaimDriver (void) {
  if (!apiDriverClaimed && apiStarted) {
    if (!api_claimDriver(&brl)) return 0;
    apiDriverClaimed = 1;
  }

  return 1;
}

void
apiReleaseDriver (void) {
  if (apiDriverClaimed) {
    api_releaseDriver(&brl);
    apiDriverClaimed = 0;
  }
}
