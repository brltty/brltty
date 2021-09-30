/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_HID_BRAILLE
#define BRLTTY_INCLUDED_HID_BRAILLE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  HID_USG_BRL_BrailleDisplay  = 0X001,

  HID_USG_BRL_CellRow         = 0X002,
  HID_USG_BRL_8DotCell        = 0X003,
  HID_USG_BRL_6DotCell        = 0X004,
  HID_USG_BRL_CellCount       = 0X005,

  HID_USG_BRL_ReaderControls  = 0X006,
  HID_USG_BRL_ReaderUUID      = 0X007,

  HID_USG_BRL_PrimaryRouter   = 0X0FA,
  HID_USG_BRL_SecondaryRouter = 0X0FB,
  HID_USG_BRL_TertiaryRouter  = 0X0FC,
  HID_USG_BRL_CellRouter      = 0X100,
  HID_USG_BRL_RowRouter       = 0X101,

  HID_USG_BRL_Buttons         = 0X200,
  HID_USG_BRL_KeyboardDot1    = 0X201,
  HID_USG_BRL_KeyboardDot2    = 0X202,
  HID_USG_BRL_KeyboardDot3    = 0X203,
  HID_USG_BRL_KeyboardDot4    = 0X204,
  HID_USG_BRL_KeyboardDot5    = 0X205,
  HID_USG_BRL_KeyboardDot6    = 0X206,
  HID_USG_BRL_KeyboardDot7    = 0X207,
  HID_USG_BRL_KeyboardDot8    = 0X208,
  HID_USG_BRL_KeyboardSpace   = 0X209,
  HID_USG_BRL_LeftSpace       = 0X20A,
  HID_USG_BRL_RightSpace      = 0X20B,

  HID_USG_BRL_FrontControls   = 0X20C,
  HID_USG_BRL_LeftControls    = 0X20D,
  HID_USG_BRL_RightControls   = 0X20E,
  HID_USG_BRL_TopControls     = 0X20F,

  HID_USG_BRL_JoystickCenter  = 0X210,
  HID_USG_BRL_JoystickUp      = 0X211,
  HID_USG_BRL_JoystickDown    = 0X212,
  HID_USG_BRL_JoystickLeft    = 0X213,
  HID_USG_BRL_JoystickRight   = 0X214,

  HID_USG_BRL_DPadCenter      = 0X215,
  HID_USG_BRL_DPadUp          = 0X216,
  HID_USG_BRL_DPadDown        = 0X217,
  HID_USG_BRL_DPadLeft        = 0X218,
  HID_USG_BRL_DPadRight       = 0X219,

  HID_USG_BRL_PanLeft         = 0X21A,
  HID_USG_BRL_PanRight        = 0X21B,

  HID_USG_BRL_RockerUp        = 0X21C,
  HID_USG_BRL_RockerDown      = 0X21D,
  HID_USG_BRL_RockerPress     = 0X21E,
} HidBrailleUsage;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_HID_BRAILLE */
