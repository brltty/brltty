/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2012 Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
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

#ifndef BRLTTY_INCLUDED_NP_BRLDEFS
#define BRLTTY_INCLUDED_NP_BRLDEFS

#define NP_REQ_Identify 0XF8
#define NP_RSP_Identity 0XFE

#define NP_PKT_BEGIN 0X79
#define NP_PKT_END 0X16
#define NP_PKT_REQ_Write 0X01
#define NP_PKT_RSP_Confirm 0X07

typedef enum {
  NP_KEY_PadLeft1     = 0X0C,
  NP_KEY_PadUp1       = 0X0F,
  NP_KEY_PadCenter1   = 0X10,
  NP_KEY_PadDown1     = 0X13,
  NP_KEY_PadRight1    = 0X14,

  NP_KEY_LeftUpper1   = 0X07,
  NP_KEY_LeftMiddle1  = 0X0B,
  NP_KEY_LeftLower1   = 0X1B,
  NP_KEY_RightUpper1  = 0X03,
  NP_KEY_RightMiddle1 = 0X17,
  NP_KEY_RightLower1  = 0X1F,

  NP_KEY_PadLeft2     = 0X06,
  NP_KEY_PadUp2       = 0X1A,
  NP_KEY_PadCenter2   = 0X0A,
  NP_KEY_PadDown2     = 0X19,
  NP_KEY_PadRight2    = 0X0E,

  NP_KEY_LeftUpper2   = 0X16,
  NP_KEY_LeftMiddle2  = 0X12,
  NP_KEY_LeftLower2   = 0X15,
  NP_KEY_RightUpper2  = 0X1E,
  NP_KEY_RightMiddle2 = 0X0D,
  NP_KEY_RightLower2  = 0X1D,

  NP_KEY_ROUTING_MIN = 0X20,
  NP_KEY_ROUTING_MAX = 0X6F,

  NP_KEY_RELEASE     = 0X80
} NP_NavigationKey;

typedef enum {
  NP_SET_NavigationKey,
  NP_SET_RoutingKey
} NP_KeySet;

#endif /* BRLTTY_INCLUDED_NP_BRLDEFS */ 
