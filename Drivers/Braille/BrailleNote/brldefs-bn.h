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

#ifndef BRLTTY_INCLUDED_BN_BRLDEFS
#define BRLTTY_INCLUDED_BN_BRLDEFS

#include "ascii.h"

typedef enum {
  BN_REQ_BEGIN    = ESC,
  BN_REQ_DESCRIBE = '?',
  BN_REQ_WRITE    = 'B'
} BN_RequestType;

typedef enum {
  BN_RSP_CHARACTER = 0X80,
  BN_RSP_SPACE     = 0X81,
  BN_RSP_BACKSPACE = 0X82,
  BN_RSP_ENTER     = 0X83,
  BN_RSP_THUMB     = 0X84,
  BN_RSP_ROUTE     = 0X85,
  BN_RSP_DESCRIBE  = 0X86,
  BN_RSP_DISPLAY   = ESC
} BN_ResponseType;

typedef enum {
  BN_KEY_Dot1,
  BN_KEY_Dot2,
  BN_KEY_Dot3,
  BN_KEY_Dot4,
  BN_KEY_Dot5,
  BN_KEY_Dot6,

  BN_KEY_Space,
  BN_KEY_Backspace,
  BN_KEY_Enter,

  BN_KEY_Previous,
  BN_KEY_Back,
  BN_KEY_Advance,
  BN_KEY_Next
} BN_NavigationKey;

typedef enum {
  BN_SET_NavigationKeys,
  BN_SET_RoutingKeys
} SK_KeySet;

#endif /* BRLTTY_INCLUDED_BN_BRLDEFS */ 
