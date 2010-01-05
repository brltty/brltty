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

#ifndef BRLTTY_INCLUDED_PM_BRLDEFS
#define BRLTTY_INCLUDED_PM_BRLDEFS

typedef enum {
  PM_KEY_BAR = 1,
  PM_KEY_SWITCH = PM_KEY_BAR + 8,
  PM_KEY_FRONT = PM_KEY_SWITCH + 8,
  PM_KEY_STATUS = PM_KEY_FRONT + 13,
  PM_KEY_KEYBOARD = PM_KEY_STATUS + 22,

  PM_KEY_BarLeft1 = PM_KEY_BAR,
  PM_KEY_BarLeft2,
  PM_KEY_BarUp1,
  PM_KEY_BarUp2,
  PM_KEY_BarRight1,
  PM_KEY_BarRight2,
  PM_KEY_BarDown1,
  PM_KEY_BarDown2,

  PM_KEY_LeftSwitchRear = PM_KEY_SWITCH,
  PM_KEY_LeftSwitchFront,
  PM_KEY_LeftKeyRear,
  PM_KEY_LeftKeyFront,
  PM_KEY_RightKeyRear,
  PM_KEY_RightKeyFront,
  PM_KEY_RightSwitchRear,
  PM_KEY_RightSwitchFront,

  PM_KEY_Dot1 = PM_KEY_KEYBOARD,
  PM_KEY_Dot2,
  PM_KEY_Dot3,
  PM_KEY_Dot4,
  PM_KEY_Dot5,
  PM_KEY_Dot6,
  PM_KEY_Dot7,
  PM_KEY_Dot8,
  PM_KEY_RightThumb,
  PM_KEY_Space,
  PM_KEY_LeftThumb,
  PM_KEY_RightSpace,
  PM_KEY_LeftSpace
} PM_NavigationKey;

typedef enum {
  PM_SET_NavigationKeys = 0,
  PM_SET_RoutingKeys1,
  PM_SET_RoutingKeys2,
  PM_SET_StatusKeys2
} PM_KeySet;

#endif /* BRLTTY_INCLUDED_PM_BRLDEFS */ 
