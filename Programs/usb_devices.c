/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

typedef struct {
  const char *const *driverCodes;
  uint16_t vendorIdentifier;
  uint16_t productIdentifier;
} UsbDeviceEntry;

#define USB_DEVICE_ENTRY(vendor,product,...) \
  { .vendorIdentifier = vendor, \
    .productIdentifier = product, \
    .driverCodes = (const char *const []){__VA_ARGS__, NULL} \
  }

static const UsbDeviceEntry usbDeviceTable[] = {
// BEGIN_USB_DEVICES

// Device: 0403:6001
// Generic Identifier
// Vendor: Future Technology Devices International, Ltd
// Product: FT232 USB-Serial (UART) IC
USB_DEVICE_ENTRY(0X0403, 0X6001, "at", "ce", "hm", "ht", "md"),

// Device: 0403:DE58
USB_DEVICE_ENTRY(0X0403, 0XDE58, "hd"),

// Device: 0403:DE59
USB_DEVICE_ENTRY(0X0403, 0XDE59, "hd"),

// Device: 0403:F208
USB_DEVICE_ENTRY(0X0403, 0XF208, "pm"),

// Device: 0403:FE70
USB_DEVICE_ENTRY(0X0403, 0XFE70, "bm"),

// Device: 0403:FE71
USB_DEVICE_ENTRY(0X0403, 0XFE71, "bm"),

// Device: 0403:FE72
USB_DEVICE_ENTRY(0X0403, 0XFE72, "bm"),

// Device: 0403:FE73
USB_DEVICE_ENTRY(0X0403, 0XFE73, "bm"),

// Device: 0403:FE74
USB_DEVICE_ENTRY(0X0403, 0XFE74, "bm"),

// Device: 0403:FE75
USB_DEVICE_ENTRY(0X0403, 0XFE75, "bm"),

// Device: 0403:FE76
USB_DEVICE_ENTRY(0X0403, 0XFE76, "bm"),

// Device: 0403:FE77
USB_DEVICE_ENTRY(0X0403, 0XFE77, "bm"),

// Device: 0452:0100
USB_DEVICE_ENTRY(0X0452, 0X0100, "mt"),

// Device: 045E:930A
USB_DEVICE_ENTRY(0X045E, 0X930A, "hm"),

// Device: 045E:930B
USB_DEVICE_ENTRY(0X045E, 0X930B, "hm"),

// Device: 0483:A1D3
USB_DEVICE_ENTRY(0X0483, 0XA1D3, "bm"),

// Device: 06B0:0001
USB_DEVICE_ENTRY(0X06B0, 0X0001, "al"),

// Device: 0798:0001
USB_DEVICE_ENTRY(0X0798, 0X0001, "vo"),

// Device: 0798:0600
USB_DEVICE_ENTRY(0X0798, 0X0600, "al"),

// Device: 0798:0624
USB_DEVICE_ENTRY(0X0798, 0X0624, "al"),

// Device: 0798:0640
USB_DEVICE_ENTRY(0X0798, 0X0640, "al"),

// Device: 0798:0680
USB_DEVICE_ENTRY(0X0798, 0X0680, "al"),

// Device: 0904:2000
USB_DEVICE_ENTRY(0X0904, 0X2000, "bm"),

// Device: 0904:2001
USB_DEVICE_ENTRY(0X0904, 0X2001, "bm"),

// Device: 0904:2002
USB_DEVICE_ENTRY(0X0904, 0X2002, "bm"),

// Device: 0904:2007
USB_DEVICE_ENTRY(0X0904, 0X2007, "bm"),

// Device: 0904:2008
USB_DEVICE_ENTRY(0X0904, 0X2008, "bm"),

// Device: 0904:2009
USB_DEVICE_ENTRY(0X0904, 0X2009, "bm"),

// Device: 0904:2010
USB_DEVICE_ENTRY(0X0904, 0X2010, "bm"),

// Device: 0904:2011
USB_DEVICE_ENTRY(0X0904, 0X2011, "bm"),

// Device: 0904:2014
USB_DEVICE_ENTRY(0X0904, 0X2014, "bm"),

// Device: 0904:2015
USB_DEVICE_ENTRY(0X0904, 0X2015, "bm"),

// Device: 0904:2016
USB_DEVICE_ENTRY(0X0904, 0X2016, "bm"),

// Device: 0904:3000
USB_DEVICE_ENTRY(0X0904, 0X3000, "bm"),

// Device: 0904:3001
USB_DEVICE_ENTRY(0X0904, 0X3001, "bm"),

// Device: 0904:4004
USB_DEVICE_ENTRY(0X0904, 0X4004, "bm"),

// Device: 0904:4005
USB_DEVICE_ENTRY(0X0904, 0X4005, "bm"),

// Device: 0904:4007
USB_DEVICE_ENTRY(0X0904, 0X4007, "bm"),

// Device: 0904:4008
USB_DEVICE_ENTRY(0X0904, 0X4008, "bm"),

// Device: 0904:6001
USB_DEVICE_ENTRY(0X0904, 0X6001, "bm"),

// Device: 0904:6002
USB_DEVICE_ENTRY(0X0904, 0X6002, "bm"),

// Device: 0904:6003
USB_DEVICE_ENTRY(0X0904, 0X6003, "bm"),

// Device: 0904:6004
USB_DEVICE_ENTRY(0X0904, 0X6004, "bm"),

// Device: 0904:6005
USB_DEVICE_ENTRY(0X0904, 0X6005, "bm"),

// Device: 0904:6006
USB_DEVICE_ENTRY(0X0904, 0X6006, "bm"),

// Device: 0904:6007
USB_DEVICE_ENTRY(0X0904, 0X6007, "bm"),

// Device: 0904:6008
USB_DEVICE_ENTRY(0X0904, 0X6008, "bm"),

// Device: 0904:6009
USB_DEVICE_ENTRY(0X0904, 0X6009, "bm"),

// Device: 0904:600A
USB_DEVICE_ENTRY(0X0904, 0X600A, "bm"),

// Device: 0904:6011
USB_DEVICE_ENTRY(0X0904, 0X6011, "bm"),

// Device: 0904:6012
USB_DEVICE_ENTRY(0X0904, 0X6012, "bm"),

// Device: 0904:6013
USB_DEVICE_ENTRY(0X0904, 0X6013, "bm"),

// Device: 0904:6101
USB_DEVICE_ENTRY(0X0904, 0X6101, "bm"),

// Device: 0904:6102
USB_DEVICE_ENTRY(0X0904, 0X6102, "bm"),

// Device: 0904:6103
USB_DEVICE_ENTRY(0X0904, 0X6103, "bm"),

// Device: 0921:1200
USB_DEVICE_ENTRY(0X0921, 0X1200, "ht"),

// Device: 0F4E:0100
USB_DEVICE_ENTRY(0X0F4E, 0X0100, "fs"),

// Device: 0F4E:0111
USB_DEVICE_ENTRY(0X0F4E, 0X0111, "fs"),

// Device: 0F4E:0112
USB_DEVICE_ENTRY(0X0F4E, 0X0112, "fs"),

// Device: 0F4E:0114
USB_DEVICE_ENTRY(0X0F4E, 0X0114, "fs"),

// Device: 10C4:EA60
// Generic Identifier
// Vendor: Cygnal Integrated Products, Inc.
// Product: CP210x UART Bridge / myAVR mySmartUSB light
USB_DEVICE_ENTRY(0X10C4, 0XEA60, "mm", "sk"),

// Device: 10C4:EA80
// Generic Identifier
// Vendor: Cygnal Integrated Products, Inc.
// Product: CP210x UART Bridge
USB_DEVICE_ENTRY(0X10C4, 0XEA80, "sk"),

// Device: 1148:0301
USB_DEVICE_ENTRY(0X1148, 0X0301, "mm"),

// Device: 1209:ABC0
USB_DEVICE_ENTRY(0X1209, 0XABC0, "ic"),

// Device: 1C71:C004
USB_DEVICE_ENTRY(0X1C71, 0XC004, "bn"),

// Device: 1C71:C005
USB_DEVICE_ENTRY(0X1C71, 0XC005, "hw"),

// Device: 1C71:C006
USB_DEVICE_ENTRY(0X1C71, 0XC006, "hw"),

// Device: 1C71:C00A
USB_DEVICE_ENTRY(0X1C71, 0XC00A, "hw"),

// Device: 1FE4:0003
USB_DEVICE_ENTRY(0X1FE4, 0X0003, "ht"),

// Device: 1FE4:0044
USB_DEVICE_ENTRY(0X1FE4, 0X0044, "ht"),

// Device: 1FE4:0054
USB_DEVICE_ENTRY(0X1FE4, 0X0054, "ht"),

// Device: 1FE4:0055
USB_DEVICE_ENTRY(0X1FE4, 0X0055, "ht"),

// Device: 1FE4:0061
USB_DEVICE_ENTRY(0X1FE4, 0X0061, "ht"),

// Device: 1FE4:0064
USB_DEVICE_ENTRY(0X1FE4, 0X0064, "ht"),

// Device: 1FE4:0074
USB_DEVICE_ENTRY(0X1FE4, 0X0074, "ht"),

// Device: 1FE4:0081
USB_DEVICE_ENTRY(0X1FE4, 0X0081, "ht"),

// Device: 1FE4:0082
USB_DEVICE_ENTRY(0X1FE4, 0X0082, "ht"),

// Device: 1FE4:0083
USB_DEVICE_ENTRY(0X1FE4, 0X0083, "ht"),

// Device: 1FE4:0084
USB_DEVICE_ENTRY(0X1FE4, 0X0084, "ht"),

// Device: 1FE4:0086
USB_DEVICE_ENTRY(0X1FE4, 0X0086, "ht"),

// Device: 1FE4:0087
USB_DEVICE_ENTRY(0X1FE4, 0X0087, "ht"),

// Device: 1FE4:008A
USB_DEVICE_ENTRY(0X1FE4, 0X008A, "ht"),

// Device: 1FE4:008B
USB_DEVICE_ENTRY(0X1FE4, 0X008B, "ht"),

// Device: 4242:0001
USB_DEVICE_ENTRY(0X4242, 0X0001, "pg"),

// Device: C251:1122
USB_DEVICE_ENTRY(0XC251, 0X1122, "eu"),

// Device: C251:1123
USB_DEVICE_ENTRY(0XC251, 0X1123, "eu"),

// Device: C251:1124
USB_DEVICE_ENTRY(0XC251, 0X1124, "eu"),

// Device: C251:1125
USB_DEVICE_ENTRY(0XC251, 0X1125, "eu"),

// Device: C251:1126
USB_DEVICE_ENTRY(0XC251, 0X1126, "eu"),

// Device: C251:1127
USB_DEVICE_ENTRY(0XC251, 0X1127, "eu"),

// Device: C251:1128
USB_DEVICE_ENTRY(0XC251, 0X1128, "eu"),

// Device: C251:1129
USB_DEVICE_ENTRY(0XC251, 0X1129, "eu"),

// Device: C251:112A
USB_DEVICE_ENTRY(0XC251, 0X112A, "eu"),

// Device: C251:112B
USB_DEVICE_ENTRY(0XC251, 0X112B, "eu"),

// Device: C251:112C
USB_DEVICE_ENTRY(0XC251, 0X112C, "eu"),

// Device: C251:112D
USB_DEVICE_ENTRY(0XC251, 0X112D, "eu"),

// Device: C251:112E
USB_DEVICE_ENTRY(0XC251, 0X112E, "eu"),

// Device: C251:112F
USB_DEVICE_ENTRY(0XC251, 0X112F, "eu"),

// Device: C251:1130
USB_DEVICE_ENTRY(0XC251, 0X1130, "eu"),

// Device: C251:1131
USB_DEVICE_ENTRY(0XC251, 0X1131, "eu"),

// Device: C251:1132
USB_DEVICE_ENTRY(0XC251, 0X1132, "eu"),

// END_USB_DEVICES
};

static const uint16_t usbDeviceCount = ARRAY_COUNT(usbDeviceTable);

