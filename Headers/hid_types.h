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
  UsbHidItemType_UsagePage         = 0X04,
  UsbHidItemType_Usage             = 0X08,
  UsbHidItemType_LogicalMinimum    = 0X14,
  UsbHidItemType_UsageMinimum      = 0X18,
  UsbHidItemType_LogicalMaximum    = 0X24,
  UsbHidItemType_UsageMaximum      = 0X28,
  UsbHidItemType_PhysicalMinimum   = 0X34,
  UsbHidItemType_DesignatorIndex   = 0X38,
  UsbHidItemType_PhysicalMaximum   = 0X44,
  UsbHidItemType_DesignatorMinimum = 0X48,
  UsbHidItemType_UnitExponent      = 0X54,
  UsbHidItemType_DesignatorMaximum = 0X58,
  UsbHidItemType_Unit              = 0X64,
  UsbHidItemType_ReportSize        = 0X74,
  UsbHidItemType_StringIndex       = 0X78,
  UsbHidItemType_Input             = 0X80,
  UsbHidItemType_ReportID          = 0X84,
  UsbHidItemType_StringMinimum     = 0X88,
  UsbHidItemType_Output            = 0X90,
  UsbHidItemType_ReportCount       = 0X94,
  UsbHidItemType_StringMaximum     = 0X98,
  UsbHidItemType_Collection        = 0XA0,
  UsbHidItemType_Push              = 0XA4,
  UsbHidItemType_Delimiter         = 0XA8,
  UsbHidItemType_Feature           = 0XB0,
  UsbHidItemType_Pop               = 0XB4,
  UsbHidItemType_EndCollection     = 0XC0,
  UsbHidItemType_Mask              = 0XFC
} UsbHidItemType;

#define USB_HID_ITEM_TYPE(item) ((item) & UsbHidItemType_Mask)
#define USB_HID_ITEM_LENGTH(item) ((item) & ~UsbHidItemType_Mask)
#define USB_HID_ITEM_BIT(type) (UINT64_C(1) << ((type) >> 2))

typedef enum {
  HID_COLLECTION_BEGIN = 0xa1,
  HID_COLLECTION_END   = 0xc0,
  HID_USAGE_PAGE       = 0x05,
  HID_USAGE            = 0x09,
  HID_USAGE_MINIMUM    = 0x19,
  HID_USAGE_MAXIMUM    = 0x29,
  HID_LOGICAL_MINIMUM  = 0x15,
  HID_LOGICAL_MAXIMUM  = 0x25,
  HID_REPORT_COUNT     = 0x95,
  HID_REPORT_SIZE      = 0x75,
  HID_REPORT_INPUT     = 0x81,
} HidInstruction;

typedef enum {
  HID_COLLECTION_PHYSICAl    = 0X00,
  HID_COLLECTION_APPLICATION = 0X01,
} HidCollection;

typedef enum {
  HID_USAGE_PAGE_GENERIC_DESKTOP = 0X01,
  HID_USAGE_PAGE_BUTTON          = 0X09,
  HID_USAGE_PAGE_BRAILLE         = 0X41,
} HidUsagePage;

typedef enum {
  HID_USAGE_GDT_MOUSE   = 0X02,
  HID_USAGE_GDT_X       = 0X30,
  HID_USAGE_GDT_Y       = 0X31,

  HID_USAGE_APP_POINTER = 0X01,

  HID_USAGE_BRL_DISPLAY = 0X01,
} HidUsage;

typedef enum {
  HID_REPORT_ATTRIBUTE_CONSTANT = 0X01,
  HID_REPORT_ATTRIBUTE_DATA     = 0X00,

  HID_REPORT_ATTRIBUTE_VARIABLE = 0X02,
  HID_REPORT_ATTRIBUTE_ARRAY    = 0X00,

  HID_REPORT_ATTRIBUTE_RELATIVE = 0X04,
  HID_REPORT_ATTRIBUTE_ABSOLUTE = 0X00,
} HidReportAttributes;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_HID_TYPES */
