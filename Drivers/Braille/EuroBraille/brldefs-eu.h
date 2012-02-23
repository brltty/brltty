/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2010 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_EU_BRLDEFS
#define BRLTTY_INCLUDED_EU_BRLDEFS

typedef enum {
  EU_CMD_L1 = 0,
  EU_CMD_L2 =  1,
  EU_CMD_L3 =  2,
  EU_CMD_L4 =  3,
  EU_CMD_L5 =  4,
  EU_CMD_L6 =  5,
  EU_CMD_L7 =  6,
  EU_CMD_L8 =  7,
  EU_CMD_UA =  8,
  EU_CMD_LA =  9,
  EU_CMD_RA = 10,
  EU_CMD_DA = 11,

  EU_CMD_F1 =  0,
  EU_CMD_F2 =  1,
  EU_CMD_F3 =  2,
  EU_CMD_F4 =  3,
  EU_CMD_F8 =  4,
  EU_CMD_F7 =  5,
  EU_CMD_F6 =  6,
  EU_CMD_F5 =  7,

  EU_CMD_S1R =  0,
  EU_CMD_S1L =  1,
  EU_CMD_S2R =  2,
  EU_CMD_S2L =  3,
  EU_CMD_S3R =  4,
  EU_CMD_S3L =  5,
  EU_CMD_S4R =  6,
  EU_CMD_S4L =  7,
  EU_CMD_S5R =  8,
  EU_CMD_S5L =  9,
  EU_CMD_S6R = 10,
  EU_CMD_S6L = 11,

  EU_CMD_J1U = 16,
  EU_CMD_J1D = 17,
  EU_CMD_J1R = 18,
  EU_CMD_J1L = 19,
  EU_CMD_J1P = 20, // activates internal menu

  EU_CMD_J2U = 24,
  EU_CMD_J2D = 25,
  EU_CMD_J2R = 26,
  EU_CMD_J2L = 27,
  EU_CMD_J2P = 28,
} EU_CommandKey;

typedef enum {
  EU_BRL_Dot1      =  0,
  EU_BRL_Dot2      =  1,
  EU_BRL_Dot3      =  2,
  EU_BRL_Dot4      =  3,
  EU_BRL_Dot5      =  4,
  EU_BRL_Dot6      =  5,
  EU_BRL_Dot7      =  6,
  EU_BRL_Dot8      =  7,
  EU_BRL_Backspace =  8,
  EU_BRL_Space     =  9
} EU_BrailleKey;

typedef enum {
  EU_SET_CommandKeys,
  EU_SET_BrailleKeys,
  EU_SET_RoutingKeys1,
  EU_SET_RoutingKeys2
} EU_KeySet;

#endif /* BRLTTY_INCLUDED_EU_BRLDEFS */ 
