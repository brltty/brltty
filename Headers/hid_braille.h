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
  HidUsage_BRL_BrailleDisplay  = 0X001,

  HidUsage_BRL_BrailleRow      = 0X002,
  HidUsage_BRL_8DotCell        = 0X003,
  HidUsage_BRL_6DotCell        = 0X004,
  HidUsage_BRL_CellCount       = 0X005,

  HidUsage_BRL_ReaderControls  = 0X006,
  HidUsage_BRL_ReaderUUID      = 0X007,

  HidUsage_BRL_PrimaryRouter   = 0X0FA,
  HidUsage_BRL_SecondaryRouter = 0X0FB,
  HidUsage_BRL_TertiaryRouter  = 0X0FC,
  HidUsage_BRL_CellRouter      = 0X100,
  HidUsage_BRL_RowRouter       = 0X101,

  HidUsage_BRL_Buttons         = 0X200,
  HidUsage_BRL_KeyboardDot1    = 0X201,
  HidUsage_BRL_KeyboardDot2    = 0X202,
  HidUsage_BRL_KeyboardDot3    = 0X203,
  HidUsage_BRL_KeyboardDot4    = 0X204,
  HidUsage_BRL_KeyboardDot5    = 0X205,
  HidUsage_BRL_KeyboardDot6    = 0X206,
  HidUsage_BRL_KeyboardDot7    = 0X207,
  HidUsage_BRL_KeyboardDot8    = 0X208,
  HidUsage_BRL_KeyboardSpace   = 0X209,
  HidUsage_BRL_LeftSpace       = 0X20A,
  HidUsage_BRL_RightSpace      = 0X20B,

  HidUsage_BRL_FrontControls   = 0X20C,
  HidUsage_BRL_LeftControls    = 0X20D,
  HidUsage_BRL_RightControls   = 0X20E,
  HidUsage_BRL_TopControls     = 0X20F,

  HidUsage_BRL_JoystickCenter  = 0X210,
  HidUsage_BRL_JoystickUp      = 0X211,
  HidUsage_BRL_JoystickDown    = 0X212,
  HidUsage_BRL_JoystickLeft    = 0X213,
  HidUsage_BRL_JoystickRight   = 0X214,

  HidUsage_BRL_DPadCenter      = 0X215,
  HidUsage_BRL_DPadUp          = 0X216,
  HidUsage_BRL_DPadDown        = 0X217,
  HidUsage_BRL_DPadLeft        = 0X218,
  HidUsage_BRL_DPadRight       = 0X219,

  HidUsage_BRL_PanLeft         = 0X21A,
  HidUsage_BRL_PanRight        = 0X21B,

  HidUsage_BRL_RockerUp        = 0X21C,
  HidUsage_BRL_RockerDown      = 0X21D,
  HidUsage_BRL_RockerPress     = 0X21E,
} HidBrailleUsage;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_HID_BRAILLE */
