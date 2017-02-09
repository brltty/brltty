/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2016 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_IV_BRLDEFS
#define BRLTTY_INCLUDED_IV_BRLDEFS

typedef union {
  unsigned char bytes[10];

  struct {
    unsigned char start;
    unsigned char type;
    unsigned char count;
    unsigned char data;
    unsigned char reserved[4];
    unsigned char checksum;
    unsigned char end;
  } PACKED fields;
} InputPacket;

typedef enum {
  IV_KEY_Dot1          =  0,
  IV_KEY_Dot2          =  1,
  IV_KEY_Dot3          =  2,
  IV_KEY_Dot4          =  3,
  IV_KEY_Dot5          =  4,
  IV_KEY_Dot6          =  5,
  IV_KEY_Dot7          =  6,
  IV_KEY_Dot8          =  7,

  IV_KEY_RightPrevious =  8,
  IV_KEY_RightNext     =  9,
  IV_KEY_LeftPrevious  = 10,
  IV_KEY_LeftNext      = 11,

  IV_KEY_Back          = 12,
  IV_KEY_Space         = 13,
  IV_KEY_Shift         = 14,
} IV_NavigationKey;

typedef enum {
  IV_GRP_NavigationKeys = 0,
  IV_GRP_RoutingKeys
} IV_KeyGroup;

#endif /* BRLTTY_INCLUDED_IV_BRLDEFS */ 
