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
extern unsigned char toLowerDigit (unsigned char upper);

typedef const unsigned char DigitsTable[11];
typedef unsigned char MakeNumberFunction (int x);
typedef unsigned char MakeFlagFunction (int number, int on);

extern const DigitsTable landscapeDigits;
extern MakeNumberFunction makeLandscapeNumber;
extern MakeFlagFunction makeLandscapeFlag;

extern const DigitsTable seascapeDigits;
extern MakeNumberFunction makeSeascapeNumber;
extern MakeFlagFunction makeSeascapeFlag;

extern const DigitsTable portraitDigits;
extern MakeNumberFunction makePortraitNumber;
extern MakeFlagFunction makePortraitFlag;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BRL_UTILS */
