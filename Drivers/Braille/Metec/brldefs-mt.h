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

#ifndef BRLTTY_INCLUDED_MT_BRLDEFS
#define BRLTTY_INCLUDED_MT_BRLDEFS

typedef enum {
  /* byte 2 */
  MT_KEY_LeftRear    =  6, // S1
  MT_KEY_LeftMiddle  =  4, // S2
  MT_KEY_LeftFront   =  2, // S3
  MT_KEY_RightRear   =  3, // S4
  MT_KEY_RightMiddle =  1, // S5
  MT_KEY_RightFront  =  0, // S6

  /* byte 3 */
  MT_KEY_CursorLeft  = 10, // S7
  MT_KEY_CursorUp    = 11, // S8
  MT_KEY_CursorRight = 12, // S9
  MT_KEY_CursorDown  = 14  // S10
} MT_NavigationKey;

typedef enum {
  MT_SET_NavigationKeys = 0,
  MT_SET_RoutingKeys1,
  MT_SET_RoutingKeys2
} MT_KeySet;

#endif /* BRLTTY_INCLUDED_MT_BRLDEFS */ 
