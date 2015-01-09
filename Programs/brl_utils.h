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

#include "brl_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void drainBrailleOutput (BrailleDisplay *brl, int minimumDelay);
extern void setBrailleOffline (BrailleDisplay *brl);
extern void setBrailleOnline (BrailleDisplay *brl);

/* Formatting of status cells. */
extern unsigned char lowerDigit (unsigned char upper);

extern const unsigned char landscapeDigits[11];
extern unsigned char landscapeNumber (int x);
extern unsigned char landscapeFlag (int number, int on);

extern const unsigned char seascapeDigits[11];
extern unsigned char seascapeNumber (int x);
extern unsigned char seascapeFlag (int number, int on);

extern const unsigned char portraitDigits[11];
extern unsigned char portraitNumber (int x);
extern unsigned char portraitFlag (int number, int on);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BRL_UTILS */
