/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2024 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_NH_BRLDEFS
#define BRLTTY_INCLUDED_NH_BRLDEFS

typedef enum {
  NH_KEY_Dot1  =  0,
  NH_KEY_Dot2  =  1,
  NH_KEY_Dot3  =  2,
  NH_KEY_Dot4  =  3,
  NH_KEY_Dot5  =  4,
  NH_KEY_Dot6  =  5,
  NH_KEY_Dot7  =  6,
  NH_KEY_Dot8  =  7,
  NH_KEY_Space =  8,
} NH_NavigationKey;

typedef enum {
  NH_GRP_NavigationKeys = 0,
  NH_GRP_RoutingKeys
} NH_KeyGroup;

#endif /* BRLTTY_INCLUDED_NH_BRLDEFS */ 
