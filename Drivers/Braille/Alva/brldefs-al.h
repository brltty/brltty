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

#ifndef BRLTTY_INCLUDED_AL_BRLDEFS
#define BRLTTY_INCLUDED_AL_BRLDEFS

#define AL_KEYS_OPERATION 14
#define AL_KEYS_STATUS 6
#define AL_KEYS_SATELLITE 6
#define AL_KEYS_ETOUCH 4
#define AL_KEYS_SMARTPAD 9
#define AL_KEYS_THUMB 5

typedef enum {
  AL_KEY_OPERATION = 1,
  AL_KEY_STATUS1 = AL_KEY_OPERATION + AL_KEYS_OPERATION,
  AL_KEY_STATUS2 = AL_KEY_STATUS1 + AL_KEYS_STATUS,
  AL_KEY_LEFT_PAD = AL_KEY_STATUS2 + AL_KEYS_STATUS,
  AL_KEY_RIGHT_PAD = AL_KEY_LEFT_PAD + AL_KEYS_SATELLITE,
  AL_KEY_ETOUCH = AL_KEY_RIGHT_PAD + AL_KEYS_SATELLITE,
  AL_KEY_SMARTPAD = AL_KEY_ETOUCH + AL_KEYS_ETOUCH,
  AL_KEY_THUMB = AL_KEY_SMARTPAD + AL_KEYS_SMARTPAD,

  AL_KEY_Prog = AL_KEY_OPERATION,
  AL_KEY_Home,
  AL_KEY_Cursor,
  AL_KEY_Up,
  AL_KEY_Left,
  AL_KEY_Right,
  AL_KEY_Down,
  AL_KEY_Cursor2,
  AL_KEY_Home2,
  AL_KEY_Prog2,
  AL_KEY_LeftTumblerLeft,
  AL_KEY_LeftTumblerRight,
  AL_KEY_RightTumblerLeft,
  AL_KEY_RightTumblerRight,

  AL_KEY_LeftPadF1 = AL_KEY_LEFT_PAD,
  AL_KEY_LeftPadUp,
  AL_KEY_LeftPadLeft,
  AL_KEY_LeftPadDown,
  AL_KEY_LeftPadRight,
  AL_KEY_LeftPadF2,

  AL_KEY_RightPadF1 = AL_KEY_RIGHT_PAD,
  AL_KEY_RightPadUp,
  AL_KEY_RightPadLeft,
  AL_KEY_RightPadDown,
  AL_KEY_RightPadRight,
  AL_KEY_RightPadF2,

  AL_KEY_ETouchLeftRear = AL_KEY_ETOUCH,
  AL_KEY_ETouchLeftFront,
  AL_KEY_ETouchRightRear,
  AL_KEY_ETouchRightFront,

  AL_KEY_SmartpadF1 = AL_KEY_SMARTPAD,
  AL_KEY_SmartpadF2,
  AL_KEY_SmartpadLeft,
  AL_KEY_SmartpadEnter,
  AL_KEY_SmartpadUp,
  AL_KEY_SmartpadDown,
  AL_KEY_SmartpadRight,
  AL_KEY_SmartpadF3,
  AL_KEY_SmartpadF4,

  AL_KEY_RELEASE = 0X80
} AL_NavigationKey;

typedef enum {
  AL_SET_NavigationKeys = 0,
  AL_SET_RoutingKeys1,
  AL_SET_RoutingKeys2
} AL_KeySet;

#endif /* BRLTTY_INCLUDED_AL_BRLDEFS */ 
