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
} HidUsagePage;

typedef enum {
  HID_USAGE_APP_POINTER = 0X01,
  HID_USAGE_GDT_MOUSE   = 0X02,
  HID_USAGE_GDT_X       = 0X30,
  HID_USAGE_GDT_Y       = 0X31,
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
