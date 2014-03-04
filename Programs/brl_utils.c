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

#include "brl_utils.h"
#include "brl_dots.h"
#include "async_wait.h"

void
drainBrailleOutput (BrailleDisplay *brl, int minimumDelay) {
  do {
    int duration = brl->writeDelay + 1;
    brl->writeDelay = 0;

    if (duration < minimumDelay) duration = minimumDelay;
    minimumDelay = 0;

    asyncWait(duration);
  } while (brl->writeDelay > 0);
}

/* Functions which support vertical and horizontal status cells. */

unsigned char
lowerDigit (unsigned char upper) {
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
int
landscapeNumber (int x) {
  return landscapeDigits[(x / 10) % 10] | lowerDigit(landscapeDigits[x % 10]);  
}

/* Format landscape flag state indicator. */
int
landscapeFlag (int number, int on) {
  int dots = landscapeDigits[number % 10];
  if (on) dots |= lowerDigit(landscapeDigits[10]);
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
int
seascapeNumber (int x) {
  return lowerDigit(seascapeDigits[(x / 10) % 10]) | seascapeDigits[x % 10];  
}

/* Format seascape flag state indicator. */
int
seascapeFlag (int number, int on) {
  int dots = lowerDigit(seascapeDigits[number % 10]);
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
int
portraitNumber (int x) {
  return portraitDigits[(x / 10) % 10] | lowerDigit(portraitDigits[x % 10]);  
}

/* Format portrait flag state indicator. */
int
portraitFlag (int number, int on) {
  int dots = lowerDigit(portraitDigits[number % 10]);
  if (on) dots |= portraitDigits[10];
  return dots;
}
