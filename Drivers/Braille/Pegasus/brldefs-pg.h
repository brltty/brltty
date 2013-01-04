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

#ifndef BRLTTY_INCLUDED_PG_BRLDEFS
#define BRLTTY_INCLUDED_PG_BRLDEFS

typedef enum {
  PG_KEY_None = 0,

  PG_KEY_LeftShift,
  PG_KEY_RightShift,
  PG_KEY_LeftControl,
  PG_KEY_RighTControl,

  PG_KEY_Left,
  PG_KEY_Right,
  PG_KEY_Up,
  PG_KEY_Down,

  PG_KEY_Home,
  PG_KEY_End,
  PG_KEY_Enter,
  PG_KEY_Escape,

  PG_KEY_Status
} PG_NavigationKey;

typedef enum {
  PG_SET_NavigationKeys = 0,
  PG_SET_RoutingKeys
} PG_KeySet;

#endif /* BRLTTY_INCLUDED_PG_BRLDEFS */ 
