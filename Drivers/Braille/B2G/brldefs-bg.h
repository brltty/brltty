/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2013 Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
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

  BG_KEY_ROUTE    = 0X1D0,

  BG_KEY_Dot7     = 0X1F1,
  BG_KEY_Dot3     = 0X1F2,
  BG_KEY_Dot2     = 0X1F3,
  BG_KEY_Dot1     = 0X1F4,
  BG_KEY_Dot4     = 0X1F5,
  BG_KEY_Dot5     = 0X1F6,
  BG_KEY_Dot6     = 0X1F7,
  BG_KEY_Dot8     = 0X1F8,
  BG_KEY_Space    = 0X1F9,
} BG_KeyCodes;

#endif /* BRLTTY_INCLUDED_BG_BRLDEFS */ 
