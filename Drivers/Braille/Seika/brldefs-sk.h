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

#ifndef BRLTTY_INCLUDED_SK_BRLDEFS
#define BRLTTY_INCLUDED_SK_BRLDEFS

typedef enum {
  SK_KEY_K1 = 1,
  SK_KEY_K7 = 2,
  SK_KEY_K8 = 3,
  SK_KEY_K6 = 4,
  SK_KEY_K5 = 5,
  SK_KEY_K2 = 10,
  SK_KEY_K3 = 12,
  SK_KEY_K4 = 13
} SK_NavigationKey;

typedef enum {
  SK_SET_NavigationKeys = 0,
  SK_SET_RoutingKeys
} SK_KeySet;

#endif /* BRLTTY_INCLUDED_SK_BRLDEFS */ 
