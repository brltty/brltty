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

#ifndef BRLTTY_INCLUDED_HID_TYPES
#define BRLTTY_INCLUDED_HID_TYPES

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  HidItemType_UsagePage         = 0X04,
  HidItemType_Usage             = 0X08,
  HidItemType_LogicalMinimum    = 0X14,
  HidItemType_UsageMinimum      = 0X18,
  HidItemType_LogicalMaximum    = 0X24,
  HidItemType_UsageMaximum      = 0X28,
  HidItemType_PhysicalMinimum   = 0X34,
  HidItemType_DesignatorIndex   = 0X38,
  HidItemType_PhysicalMaximum   = 0X44,
  HidItemType_DesignatorMinimum = 0X48,
  HidItemType_UnitExponent      = 0X54,
  HidItemType_DesignatorMaximum = 0X58,
  HidItemType_Unit              = 0X64,
  HidItemType_ReportSize        = 0X74,
  HidItemType_StringIndex       = 0X78,
  HidItemType_Input             = 0X80,
  HidItemType_ReportID          = 0X84,
  HidItemType_StringMinimum     = 0X88,
  HidItemType_Output            = 0X90,
  HidItemType_ReportCount       = 0X94,
  HidItemType_StringMaximum     = 0X98,
  HidItemType_Collection        = 0XA0,
  HidItemType_Push              = 0XA4,
  HidItemType_Delimiter         = 0XA8,
  HidItemType_Feature           = 0XB0,
  HidItemType_Pop               = 0XB4,
  HidItemType_EndCollection     = 0XC0,
  HidItemType_Mask              = 0XFC
} HidItemType;

#define HID_ITEM_TYPE(item) ((item) & HidItemType_Mask)
#define HID_ITEM_LENGTH(item) ((item) & ~HidItemType_Mask)
#define HID_ITEM_BIT(type) (UINT64_C(1) << ((type) >> 2))

typedef enum {
  HidCOLLECTION_PhysicaL    = 0X00,
  HidCOLLECTION_Application = 0X01,
  HidCOLLECTION_Logical     = 0X02,
} HidCollection;

typedef enum {
  HidUsagePage_GenericDesktop = 0X01,
  HidUsagePage_Button         = 0X09,
  HidUsagePage_Braille        = 0X41,
} HidUsagePage;

typedef enum {
  HidUsage_GDT_Mouse           = 0X02,
  HidUsage_GDT_X               = 0X30,
  HidUsage_GDT_Y               = 0X31,

  HidUsage_APP_Pointer         = 0X01,

  HidUsage_BTN_Button1         = 0X01,
  HidUsage_BTN_Button2         = 0X02,
  HidUsage_BTN_Button3         = 0X03,

  HidUsage_BRL_BrailleDisplay  = 0X001,
  HidUsage_BRL_BrailleRow      = 0X002,
  HidUsage_BRL_8DotCell        = 0X003,
  HidUsage_BRL_6DotCell        = 0X004,
  HidUsage_BRL_CellCount       = 0X005,
  HidUsage_BRL_ScreenControls  = 0X006,
  HidUsage_BRL_ScreenUUID      = 0X007,
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
} HidUsage;

typedef enum {
  HidReportAttribute_CONSTANT = 0X01,
  HidReportAttribute_DATA     = 0X00,

  HidReportAttribute_VARIABLE = 0X02,
  HidReportAttribute_ARRAY    = 0X00,

  HidReportAttribute_RELATIVE = 0X04,
  HidReportAttribute_ABSOLUTE = 0X00,
} HidReportAttributes;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_HID_TYPES */
