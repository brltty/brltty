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
  IV_KEY_DOT1  =  0,
  IV_KEY_DOT2  =  1,
  IV_KEY_DOT3  =  2,
  IV_KEY_DOT4  =  3,
  IV_KEY_DOT5  =  4,
  IV_KEY_DOT6  =  5,
  IV_KEY_DOT7  =  6,
  IV_KEY_DOT8  =  7,
  IV_KEY_SPACE =  8,

  IV_KEY_F1    =  9,
  IV_KEY_F2    = 10,
  IV_KEY_F3    = 11,
  IV_KEY_F4    = 12,

  IV_KEY_RIGHT = 13,
  IV_KEY_LEFT  = 14,
} IV_NavigationKey;

typedef enum {
  IV_GRP_NavigationKeys = 0,
  IV_GRP_RoutingKeys
} IV_KeyGroup;

#endif /* BRLTTY_INCLUDED_IV_BRLDEFS */ 
