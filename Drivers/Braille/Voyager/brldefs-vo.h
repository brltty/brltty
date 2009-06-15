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

#ifndef BRLTTY_INCLUDED_VO_BRLDEFS
#define BRLTTY_INCLUDED_VO_BRLDEFS

typedef enum {
  /* The top round keys behind the routing keys, numbered assuming they
   * are to be used for braille input.
   */
  VO_KEY_Dot1 = 1,
  VO_KEY_Dot2 = 2,
  VO_KEY_Dot3 = 3,
  VO_KEY_Dot4 = 4,
  VO_KEY_Dot5 = 5,
  VO_KEY_Dot6 = 6,
  VO_KEY_Dot7 = 7,
  VO_KEY_Dot8 = 8,

  /* The front keys */
  VO_KEY_Thumb1 = 9,              /* Leftmost */
  VO_KEY_Thumb2 = 10,             /* Second from left */
  VO_KEY_Left = 11,     /* Round key to the left of the central pad */
  VO_KEY_Up = 12,            /* Up position of central pad */
  VO_KEY_Down = 13,          /* Down position of central pad */
  VO_KEY_Right = 14,    /* Round key to the right of the central pad */
  VO_KEY_Thumb3 = 15,             /* Second from right */
  VO_KEY_Thumb4 = 16              /* Rightmost */
} VO_NavigationKey;

typedef enum {
  VO_SET_NavigationKeys = 0,
  VO_SET_RoutingKeys
} VO_KeySet;

#endif /* BRLTTY_INCLUDED_VO_BRLDEFS */ 
