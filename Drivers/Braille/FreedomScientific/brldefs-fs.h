/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2010 Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
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

#ifndef BRLTTY_INCLUDED_FS_BRLDEFS
#define BRLTTY_INCLUDED_FS_BRLDEFS

#define FS_KEYS_WHEEL 8
#define FS_KEYS_HOT 8

typedef enum {
  FS_KEY_Dot1 = 0,
  FS_KEY_Dot2 = 1,
  FS_KEY_Dot3 = 2,
  FS_KEY_Dot4 = 3,
  FS_KEY_Dot5 = 4,
  FS_KEY_Dot6 = 5,
  FS_KEY_Dot7 = 6,
  FS_KEY_Dot8 = 7,

  FS_KEY_LeftWheel = 8,
  FS_KEY_RightWheel = 9,
  FS_KEY_LeftShift = 10,
  FS_KEY_RightShift = 11,
  FS_KEY_LeftAdvance = 12,
  FS_KEY_RightAdvance = 13,
  FS_KEY_Space = 15,

  FS_KEY_LeftGdf = 16,
  FS_KEY_RightGdf = 17,
  FS_KEY_LeftRockerUp = 20,
  FS_KEY_LeftRockerDown = 21,
  FS_KEY_RightRockerUp = 22,
  FS_KEY_RightRockerDown = 23,

  FS_KEY_WHEEL,
  FS_KEY_HOT = FS_KEY_WHEEL + FS_KEYS_WHEEL
} FS_NavigationKey;

typedef enum {
  FS_SET_NavigationKeys = 0,
  FS_SET_RoutingKeys1,
  FS_SET_RoutingKeys2
} FS_KeySet;

#endif /* BRLTTY_INCLUDED_FS_BRLDEFS */ 
