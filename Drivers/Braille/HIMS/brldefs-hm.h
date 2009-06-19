/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2009 Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
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

/* brldefs-ht.h : Useful definitions to handle HandyTech displays */

#ifndef BRLTTY_INCLUDED_HM_BRLDEFS
#define BRLTTY_INCLUDED_HM_BRLDEFS

typedef enum {
  /* dot keys */
  HM_KEY_Dot1 = 1,
  HM_KEY_Dot2 = 2,
  HM_KEY_Dot3 = 3,
  HM_KEY_Dot4 = 4,
  HM_KEY_Dot5 = 5,
  HM_KEY_Dot6 = 6,
  HM_KEY_Dot7 = 7,
  HM_KEY_Dot8 = 8,
  HM_KEY_Space = 9,

  /* Braille Sense keys */
  HM_KEY_BS_F1 = 10,
  HM_KEY_BS_F2 = 11,
  HM_KEY_BS_F3 = 12,
  HM_KEY_BS_F4 = 13,
  HM_KEY_BS_ScrollLeft = 14,
  HM_KEY_BS_ScrollRight = 15,

  /* SyncBraille keys */
  HM_KEY_SB_LeftUp = 13,
  HM_KEY_SB_RightUp = 14,
  HM_KEY_SB_RightDown = 15,
  HM_KEY_SB_LeftDown = 16
} HM_NavigationKey;

typedef enum {
  HM_SET_NavigationKeys = 0,
  HM_SET_RoutingKeys
} HM_KeySet;

#endif /* BRLTTY_INCLUDED_HM_BRLDEFS */ 
