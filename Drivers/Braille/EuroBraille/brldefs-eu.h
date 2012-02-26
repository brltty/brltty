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
  EU_IRIS_20             = 0X01,
  EU_IRIS_40             = 0X02,
  EU_IRIS_S20            = 0X03,
  EU_IRIS_S32            = 0X04,
  EU_IRIS_KB20           = 0X05,
  EU_IRIS_KB40           = 0X06,
  EU_ESYS_12             = 0X07,
  EU_ESYS_40             = 0X08,
  EU_ESYS_LIGHT_40       = 0X09,
  EU_ESYS_24             = 0X0A,
  EU_ESYS_64             = 0X0B,
  EU_ESYS_80             = 0X0C,
  EU_ESYTIME_32          = 0X0E,
  EU_ESYTIME_32_STANDARD = 0X0F
} EsysirisModelIdentifier;

typedef enum {
  /* Iris linear and arrow keys */
  EU_CMD_L1    =  0,
  EU_CMD_L2    =  1,
  EU_CMD_L3    =  2,
  EU_CMD_L4    =  3,
  EU_CMD_L5    =  4,
  EU_CMD_L6    =  5,
  EU_CMD_L7    =  6,
  EU_CMD_L8    =  7,
  EU_CMD_Up    =  8,
  EU_CMD_Down  =  9,
  EU_CMD_Right = 10,
  EU_CMD_Left  = 11,

  /* Esytime function keys */
  EU_CMD_F1 =  0,
  EU_CMD_F2 =  1,
  EU_CMD_F3 =  2,
  EU_CMD_F4 =  3,
  EU_CMD_F8 =  4,
  EU_CMD_F7 =  5,
  EU_CMD_F6 =  6,
  EU_CMD_F5 =  7,

  /* Esys switches */
  EU_CMD_Switch1Right =  0,
  EU_CMD_Switch1Left  =  1,
  EU_CMD_Switch2Right =  2,
  EU_CMD_Switch2Left  =  3,
  EU_CMD_Switch3Right =  4,
  EU_CMD_Switch3Left  =  5,
  EU_CMD_Switch4Right =  6,
  EU_CMD_Switch4Left  =  7,
  EU_CMD_Switch5Right =  8,
  EU_CMD_Switch5Left  =  9,
  EU_CMD_Switch6Right = 10,
  EU_CMD_Switch6Left  = 11,

  /* Esys and Esytime joystick #1 */
  EU_CMD_LeftJoystickUp    = 16,
  EU_CMD_LeftJoystickDown  = 17,
  EU_CMD_LeftJoystickRight = 18,
  EU_CMD_LeftJoystickLeft  = 19,
  EU_CMD_LeftJoystickPress = 20, // activates internal menu

  /* Esys and Esytime joystick #2 */
  EU_CMD_RightJoystickUp    = 24,
  EU_CMD_RightJoystickDown  = 25,
  EU_CMD_RightJoystickRight = 26,
  EU_CMD_RightJoystickLeft  = 27,
  EU_CMD_RightJoystickPress = 28,
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
