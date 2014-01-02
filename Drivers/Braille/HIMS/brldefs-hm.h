/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_HM_BRLDEFS
#define BRLTTY_INCLUDED_HM_BRLDEFS

typedef enum {
  /* dot keys */
  HM_KEY_Dot1  = 0,
  HM_KEY_Dot2  = 1,
  HM_KEY_Dot3  = 2,
  HM_KEY_Dot4  = 3,
  HM_KEY_Dot5  = 4,
  HM_KEY_Dot6  = 5,
  HM_KEY_Dot7  = 6,
  HM_KEY_Dot8  = 7,
  HM_KEY_Space = 8,

  /* Braille Sense/Edge keys */
  HM_KEY_F1 =  9,
  HM_KEY_F2 = 10,
  HM_KEY_F3 = 11,
  HM_KEY_F4 = 12,

  /* Braille Sense keys */
  HM_KEY_Backward = 13,
  HM_KEY_Forward  = 14,

  /* SyncBraille keys */
  HM_KEY_LeftUp    = 12,
  HM_KEY_RightUp   = 13,
  HM_KEY_RightDown = 14,
  HM_KEY_LeftDown  = 15,

  /* Braille Edge keys */
  HM_KEY_LeftScrollUp    = 16,
  HM_KEY_RightScrollUp   = 17,
  HM_KEY_LeftScrollDown  = 18,
  HM_KEY_RightScrollDown = 19,

  HM_KEY_F5 = 20,
  HM_KEY_F6 = 21,
  HM_KEY_F7 = 22,
  HM_KEY_F8 = 23,

  HM_KEY_LeftPadUp     = 24,
  HM_KEY_LeftPadDown   = 25,
  HM_KEY_LeftPadLeft   = 26,
  HM_KEY_LeftPadRight  = 27,

  HM_KEY_RightPadUp    = 28,
  HM_KEY_RightPadDown  = 29,
  HM_KEY_RightPadLeft  = 30,
  HM_KEY_RightPadRight = 31
} HM_NavigationKey;

typedef enum {
  HM_SET_NavigationKeys = 0,
  HM_SET_RoutingKeys
} HM_KeySet;

#endif /* BRLTTY_INCLUDED_HM_BRLDEFS */ 
