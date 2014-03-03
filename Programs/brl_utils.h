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

#ifndef BRLTTY_INCLUDED_BRL_UTILS
#define BRLTTY_INCLUDED_BRL_UTILS

#include "brldefs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void drainBrailleOutput (BrailleDisplay *brl, int minimumDelay);

/* Formatting of status cells. */
extern unsigned char lowerDigit (unsigned char upper);
extern const unsigned char landscapeDigits[11];
extern int landscapeNumber (int x);
extern int landscapeFlag (int number, int on);
extern const unsigned char seascapeDigits[11];
extern int seascapeNumber (int x);
extern int seascapeFlag (int number, int on);
extern const unsigned char portraitDigits[11];
extern int portraitNumber (int x);
extern int portraitFlag (int number, int on);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BRL_UTILS */
