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
#include "report.h"
#include "brl_utils.h"
#include "brl_dots.h"
#include "async_wait.h"
#include "ktb.h"
#include "update.h"

void
drainBrailleOutput (BrailleDisplay *brl, int minimumDelay) {
  int duration = brl->writeDelay + 1;

  brl->writeDelay = 0;
  if (duration < minimumDelay) duration = minimumDelay;
  asyncWait(duration);
}

void
setBrailleOffline (BrailleDisplay *brl) {
  if (!brl->isOffline) {
    brl->isOffline = 1;
    logMessage(LOG_DEBUG, "braille offline");

    {
      KeyTable *keyTable = brl->keyTable;

      if (keyTable) releaseAllKeys(keyTable);
    }

    report(REPORT_BRAILLE_OFFLINE, NULL);
  }
}

void
setBrailleOnline (BrailleDisplay *brl) {
  if (brl->isOffline) {
    brl->isOffline = 0;
    logMessage(LOG_DEBUG, "braille online");
    report(REPORT_BRAILLE_ONLINE, NULL);

    brl->writeDelay = 0;
    scheduleUpdate("braille online");
  }
}

/* Functions which support vertical and horizontal status cells. */

unsigned char
toLowerDigit (unsigned char upper) {
  unsigned char lower = 0;
  if (upper & BRL_DOT_1) lower |= BRL_DOT_3;
  if (upper & BRL_DOT_2) lower |= BRL_DOT_7;
  if (upper & BRL_DOT_4) lower |= BRL_DOT_6;
  if (upper & BRL_DOT_5) lower |= BRL_DOT_8;
  return lower;
}

/* Dots for landscape (counterclockwise-rotated) digits. */
const unsigned char landscapeDigits[11] = {
  BRL_DOT_1 | BRL_DOT_5 | BRL_DOT_2,
  BRL_DOT_4,
  BRL_DOT_4 | BRL_DOT_1,
  BRL_DOT_4 | BRL_DOT_5,
  BRL_DOT_4 | BRL_DOT_5 | BRL_DOT_2,
  BRL_DOT_4 | BRL_DOT_2,
  BRL_DOT_4 | BRL_DOT_1 | BRL_DOT_5,
  BRL_DOT_4 | BRL_DOT_1 | BRL_DOT_5 | BRL_DOT_2,
  BRL_DOT_4 | BRL_DOT_1 | BRL_DOT_2,
  BRL_DOT_1 | BRL_DOT_5,
  BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_4 | BRL_DOT_5
};

/* Format landscape representation of numbers 0 through 99. */
unsigned char
makeLandscapeNumber (int x) {
  return landscapeDigits[(x / 10) % 10] | toLowerDigit(landscapeDigits[x % 10]);  
}

/* Format landscape flag state indicator. */
unsigned char
makeLandscapeFlag (int number, int on) {
  unsigned char dots = landscapeDigits[number % 10];
  if (on) dots |= toLowerDigit(landscapeDigits[10]);
  return dots;
}

/* Dots for seascape (clockwise-rotated) digits. */
const unsigned char seascapeDigits[11] = {
  BRL_DOT_5 | BRL_DOT_1 | BRL_DOT_4,
  BRL_DOT_2,
  BRL_DOT_2 | BRL_DOT_5,
  BRL_DOT_2 | BRL_DOT_1,
  BRL_DOT_2 | BRL_DOT_1 | BRL_DOT_4,
  BRL_DOT_2 | BRL_DOT_4,
  BRL_DOT_2 | BRL_DOT_5 | BRL_DOT_1,
  BRL_DOT_2 | BRL_DOT_5 | BRL_DOT_1 | BRL_DOT_4,
  BRL_DOT_2 | BRL_DOT_5 | BRL_DOT_4,
  BRL_DOT_5 | BRL_DOT_1,
  BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_4 | BRL_DOT_5
};

/* Format seascape representation of numbers 0 through 99. */
unsigned char
makeSeascapeNumber (int x) {
  return toLowerDigit(seascapeDigits[(x / 10) % 10]) | seascapeDigits[x % 10];  
}

/* Format seascape flag state indicator. */
unsigned char
makeSeascapeFlag (int number, int on) {
  unsigned char dots = toLowerDigit(seascapeDigits[number % 10]);
  if (on) dots |= seascapeDigits[10];
  return dots;
}

/* Dots for portrait digits - 2 numbers in one cells */
const unsigned char portraitDigits[11] = {
  BRL_DOT_2 | BRL_DOT_4 | BRL_DOT_5,
  BRL_DOT_1,
  BRL_DOT_1 | BRL_DOT_2,
  BRL_DOT_1 | BRL_DOT_4,
  BRL_DOT_1 | BRL_DOT_4 | BRL_DOT_5,
  BRL_DOT_1 | BRL_DOT_5,
  BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_4,
  BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_4 | BRL_DOT_5,
  BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_5,
  BRL_DOT_2 | BRL_DOT_4,
  BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_4 | BRL_DOT_5
};

/* Format portrait representation of numbers 0 through 99. */
unsigned char
makePortraitNumber (int x) {
  return portraitDigits[(x / 10) % 10] | toLowerDigit(portraitDigits[x % 10]);  
}

/* Format portrait flag state indicator. */
unsigned char
makePortraitFlag (int number, int on) {
  unsigned char dots = toLowerDigit(portraitDigits[number % 10]);
  if (on) dots |= portraitDigits[10];
  return dots;
}
