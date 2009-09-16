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

#ifndef BRLTTY_INCLUDED_BM_BRLDEFS
#define BRLTTY_INCLUDED_BM_BRLDEFS

#define BM_KEYS_DISPLAY 6
#define BM_KEYS_COMMAND 7
#define BM_KEYS_ROCKER 6
#define BM_KEYS_BUTTON 10
#define BM_KEYS_ENTRY 16
#define BM_KEYS_JOYSTICK 5
#define BM_KEYS_WHEEL 4
#define BM_KEYS_STATUS 8

typedef enum {
  BM_KEY_DISPLAY = 1,
  BM_KEY_COMMAND = BM_KEY_DISPLAY + BM_KEYS_DISPLAY,
  BM_KEY_FRONT_ROCKERS = BM_KEY_COMMAND + BM_KEYS_COMMAND,
  BM_KEY_BACK_ROCKERS = BM_KEY_FRONT_ROCKERS + BM_KEYS_ROCKER,
  BM_KEY_FRONT_BUTTONS = BM_KEY_BACK_ROCKERS + BM_KEYS_ROCKER,
  BM_KEY_BACK_BUTTONS = BM_KEY_FRONT_BUTTONS + BM_KEYS_BUTTON,
  BM_KEY_ENTRY = BM_KEY_BACK_BUTTONS + BM_KEYS_BUTTON,
  BM_KEY_JOYSTICK = BM_KEY_ENTRY + BM_KEYS_ENTRY,
  BM_KEY_WHEEL_UP = BM_KEY_JOYSTICK + BM_KEYS_JOYSTICK,
  BM_KEY_WHEEL_DOWN = BM_KEY_WHEEL_UP + BM_KEYS_WHEEL,
  BM_KEY_WHEEL_PRESS = BM_KEY_WHEEL_DOWN + BM_KEYS_WHEEL,
  BM_KEY_STATUS = BM_KEY_WHEEL_PRESS + BM_KEYS_WHEEL,
  BM_KEY_COUNT = BM_KEY_STATUS + BM_KEYS_STATUS
} BM_NavigationKey;

typedef enum {
  BM_SET_NavigationKeys = 0,
  BM_SET_RoutingKeys,
  BM_SET_HorizontalSensors,
  BM_SET_LeftSensors,
  BM_SET_RightSensors
} BM_KeySet;

#endif /* BRLTTY_INCLUDED_BM_BRLDEFS */ 
