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
  UsbHidRequest_GetReport   = 0X01,
  UsbHidRequest_GetIdle     = 0X02,
  UsbHidRequest_GetProtocol = 0X03,
  UsbHidRequest_SetReport   = 0X09,
  UsbHidRequest_SetIdle     = 0X0A,
  UsbHidRequest_SetProtocol = 0X0B
} UsbHidRequest;

typedef enum {
  UsbHidReportType_Input   = 0X01,
  UsbHidReportType_Output  = 0X02,
  UsbHidReportType_Feature = 0X03
} UsbHidReportType;

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
} HidCollection;

typedef enum {
  HidUsagePage_GenericDesktop = 0X01,
  HidUsagePage_Button         = 0X09,
  HidUsagePage_Braille        = 0X41,
} HidUsagePage;

typedef enum {
  HidUsage_GDT_Mouse   = 0X02,
  HidUsage_GDT_X       = 0X30,
  HidUsage_GDT_Y       = 0X31,

  HidUsage_APP_Pointer = 0X01,

  HidUsage_BRL_Display = 0X01,
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
