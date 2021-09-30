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
  HID_ITM_UsagePage         = 0X04,
  HID_ITM_Usage             = 0X08,
  HID_ITM_LogicalMinimum    = 0X14,
  HID_ITM_UsageMinimum      = 0X18,
  HID_ITM_LogicalMaximum    = 0X24,
  HID_ITM_UsageMaximum      = 0X28,
  HID_ITM_PhysicalMinimum   = 0X34,
  HID_ITM_DesignatorIndex   = 0X38,
  HID_ITM_PhysicalMaximum   = 0X44,
  HID_ITM_DesignatorMinimum = 0X48,
  HID_ITM_UnitExponent      = 0X54,
  HID_ITM_DesignatorMaximum = 0X58,
  HID_ITM_Unit              = 0X64,
  HID_ITM_ReportSize        = 0X74,
  HID_ITM_StringIndex       = 0X78,
  HID_ITM_Input             = 0X80,
  HID_ITM_ReportID          = 0X84,
  HID_ITM_StringMinimum     = 0X88,
  HID_ITM_Output            = 0X90,
  HID_ITM_ReportCount       = 0X94,
  HID_ITM_StringMaximum     = 0X98,
  HID_ITM_Collection        = 0XA0,
  HID_ITM_Push              = 0XA4,
  HID_ITM_Delimiter         = 0XA8,
  HID_ITM_Feature           = 0XB0,
  HID_ITM_Pop               = 0XB4,
  HID_ITM_EndCollection     = 0XC0,

  HID_ITEM_TYPE_MASK        = 0XFC
} HidItemType;

#define HID_ITEM_TYPE(item) ((item) & HID_ITEM_TYPE_MASK)
#define HID_ITEM_LENGTH(item) ((item) & ~HID_ITEM_TYPE_MASK)
#define HID_ITEM_BIT(type) (UINT64_C(1) << ((type) >> 2))

typedef enum {
  HID_COL_PhysicaL    = 0X00,
  HID_COL_Application = 0X01,
  HID_COL_Logical     = 0X02,
} HidCollectionType;

typedef enum {
  HID_UPG_GenericDesktop = 0X01,
  HID_UPG_Buttons        = 0X09,
  HID_UPG_Braille        = 0X41,
} HidUsagePage;

typedef enum {
  HID_USG_GDT_Mouse = 0X02,
  HID_USG_GDT_X     = 0X30,
  HID_USG_GDT_Y     = 0X31,
} HidGenericDesktopUsage;

typedef enum {
  HID_USG_APP_Pointer = 0X01,
} HidApplicationUsage;

typedef enum {
  HID_USG_BTN_Button1 = 0X01,
  HID_USG_BTN_Button2 = 0X02,
  HID_USG_BTN_Button3 = 0X03,
} HidButtonUsage;

typedef enum {
  HID_REP_FLG_CONSTANT = 0X01,
  HID_REP_FLG_DATA     = 0X00,

  HID_REP_FLG_VARIABLE = 0X02,
  HID_REP_FLG_ARRAY    = 0X00,

  HID_REP_FLG_RELATIVE = 0X04,
  HID_REP_FLG_ABSOLUTE = 0X00,
} HidReportFlags;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_HID_TYPES */
