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

#ifndef BRLTTY_INCLUDED_MT_BRLDEFS
#define BRLTTY_INCLUDED_MT_BRLDEFS

typedef enum {
  /* byte 2 */
  MT_KEY_LeftUp      =  6,
  MT_KEY_LeftSelect  =  4,
  MT_KEY_LeftDown    =  2,
  MT_KEY_RightUp     =  3,
  MT_KEY_RightSelect =  1,
  MT_KEY_RightDown   =  0,

  /* byte 3 */
  MT_KEY_CursorLeft  = 10, // S7
  MT_KEY_CursorUp    = 11, // S8
  MT_KEY_CursorRight = 12, // S9
  MT_KEY_CursorDown  = 14  // S10
} MT_NavigationKey;

typedef enum {
  MT_SET_NavigationKeys = 0,

  MT_SET_RoutingKeys1,
  MT_SET_StatusKeys1,

  MT_SET_RoutingKeys2,
  MT_SET_StatusKeys2
} MT_KeySet;

#endif /* BRLTTY_INCLUDED_MT_BRLDEFS */ 
