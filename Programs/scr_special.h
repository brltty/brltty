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
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_SCR_SPECIAL
#define BRLTTY_INCLUDED_SCR_SPECIAL

#include "scr_internal.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int isFrozenScreen (void);
extern int haveFrozenScreen (void);
extern int activateFrozenScreen (void);
extern void deactivateFrozenScreen (void);

extern int isMenuScreen (void);
extern int haveMenuScreen (void);
extern int activateMenuScreen (void);
extern void deactivateMenuScreen (void);

extern int isHelpScreen (void);
extern int haveHelpScreen (void);
extern int activateHelpScreen (void);
extern void deactivateHelpScreen (void);

extern int constructHelpScreen (void);
extern int addHelpPage (void);
extern unsigned int getHelpPageCount (void);
extern unsigned int getHelpPageNumber (void);
extern int setHelpPageNumber (unsigned int number);
extern int clearHelpPage (void);
extern int addHelpLine (const wchar_t *characters);
extern unsigned int getHelpLineCount (void);

extern void beginSpecialScreens (void);
extern void endSpecialScreens (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SCR_SPECIAL */
