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

#ifndef BRLTTY_INCLUDED_BG_BRLDEFS
#define BRLTTY_INCLUDED_BG_BRLDEFS

typedef enum {
  BG_KEY_Up       = 0X067,
  BG_KEY_Left     = 0X069,
  BG_KEY_Right    = 0X06A,
  BG_KEY_Down     = 0X06C,
  BG_KEY_Enter    = 0X160,

  BG_KEY_Forward  = 0X197,
  BG_KEY_Backward = 0X19C,

  BG_KEY_Dot7     = 0X1F1,
  BG_KEY_Dot3     = 0X1F2,
  BG_KEY_Dot2     = 0X1F3,
  BG_KEY_Dot1     = 0X1F4,
  BG_KEY_Dot4     = 0X1F5,
  BG_KEY_Dot5     = 0X1F6,
  BG_KEY_Dot6     = 0X1F7,
  BG_KEY_Dot8     = 0X1F8,
  BG_KEY_Space    = 0X1F9,

  BG_KEY_ROUTE    = 0X2D0,
} BG_KeyCode;

typedef enum {
  BG_SET_RoutingKeys = 5
} BG_KeySet;

#endif /* BRLTTY_INCLUDED_BG_BRLDEFS */ 
