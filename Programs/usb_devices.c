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
  const char *const *drivers;
  uint16_t vendor;
  uint16_t product;
} UsbDeviceEntry;

static const UsbDeviceEntry usbDeviceTable[] = {
// BEGIN_USB_DEVICES

// Device: 0403:6001
// Generic Identifier
// Vendor: Future Technology Devices International, Ltd
// Product: FT232 USB-Serial (UART) IC
{ .vendor=0X0403, .product=0X6001,
  .drivers=(const char *const []){"at", "ce", "hm", "ht", "md"}
},

// Device: 0403:DE58
{ .vendor=0X0403, .product=0XDE58,
  .drivers=(const char *const []){"hd"}
},

// Device: 0403:DE59
{ .vendor=0X0403, .product=0XDE59,
  .drivers=(const char *const []){"hd"}
},

// Device: 0403:F208
{ .vendor=0X0403, .product=0XF208,
  .drivers=(const char *const []){"pm"}
},

// Device: 0403:FE70
{ .vendor=0X0403, .product=0XFE70,
  .drivers=(const char *const []){"bm"}
},

// Device: 0403:FE71
{ .vendor=0X0403, .product=0XFE71,
  .drivers=(const char *const []){"bm"}
},

// Device: 0403:FE72
{ .vendor=0X0403, .product=0XFE72,
  .drivers=(const char *const []){"bm"}
},

// Device: 0403:FE73
{ .vendor=0X0403, .product=0XFE73,
  .drivers=(const char *const []){"bm"}
},

// Device: 0403:FE74
{ .vendor=0X0403, .product=0XFE74,
  .drivers=(const char *const []){"bm"}
},

// Device: 0403:FE75
{ .vendor=0X0403, .product=0XFE75,
  .drivers=(const char *const []){"bm"}
},

// Device: 0403:FE76
{ .vendor=0X0403, .product=0XFE76,
  .drivers=(const char *const []){"bm"}
},

// Device: 0403:FE77
{ .vendor=0X0403, .product=0XFE77,
  .drivers=(const char *const []){"bm"}
},

// Device: 0452:0100
{ .vendor=0X0452, .product=0X0100,
  .drivers=(const char *const []){"mt"}
},

// Device: 045E:930A
{ .vendor=0X045E, .product=0X930A,
  .drivers=(const char *const []){"hm"}
},

// Device: 045E:930B
{ .vendor=0X045E, .product=0X930B,
  .drivers=(const char *const []){"hm"}
},

// Device: 0483:A1D3
{ .vendor=0X0483, .product=0XA1D3,
  .drivers=(const char *const []){"bm"}
},

// Device: 06B0:0001
{ .vendor=0X06B0, .product=0X0001,
  .drivers=(const char *const []){"al"}
},

// Device: 0798:0001
{ .vendor=0X0798, .product=0X0001,
  .drivers=(const char *const []){"vo"}
},

// Device: 0798:0600
{ .vendor=0X0798, .product=0X0600,
  .drivers=(const char *const []){"al"}
},

// Device: 0798:0624
{ .vendor=0X0798, .product=0X0624,
  .drivers=(const char *const []){"al"}
},

// Device: 0798:0640
{ .vendor=0X0798, .product=0X0640,
  .drivers=(const char *const []){"al"}
},

// Device: 0798:0680
{ .vendor=0X0798, .product=0X0680,
  .drivers=(const char *const []){"al"}
},

// Device: 0904:2000
{ .vendor=0X0904, .product=0X2000,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:2001
{ .vendor=0X0904, .product=0X2001,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:2002
{ .vendor=0X0904, .product=0X2002,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:2007
{ .vendor=0X0904, .product=0X2007,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:2008
{ .vendor=0X0904, .product=0X2008,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:2009
{ .vendor=0X0904, .product=0X2009,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:2010
{ .vendor=0X0904, .product=0X2010,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:2011
{ .vendor=0X0904, .product=0X2011,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:2014
{ .vendor=0X0904, .product=0X2014,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:2015
{ .vendor=0X0904, .product=0X2015,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:2016
{ .vendor=0X0904, .product=0X2016,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:3000
{ .vendor=0X0904, .product=0X3000,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:3001
{ .vendor=0X0904, .product=0X3001,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:4004
{ .vendor=0X0904, .product=0X4004,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:4005
{ .vendor=0X0904, .product=0X4005,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:4007
{ .vendor=0X0904, .product=0X4007,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:4008
{ .vendor=0X0904, .product=0X4008,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:6001
{ .vendor=0X0904, .product=0X6001,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:6002
{ .vendor=0X0904, .product=0X6002,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:6003
{ .vendor=0X0904, .product=0X6003,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:6004
{ .vendor=0X0904, .product=0X6004,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:6005
{ .vendor=0X0904, .product=0X6005,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:6006
{ .vendor=0X0904, .product=0X6006,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:6007
{ .vendor=0X0904, .product=0X6007,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:6008
{ .vendor=0X0904, .product=0X6008,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:6009
{ .vendor=0X0904, .product=0X6009,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:600A
{ .vendor=0X0904, .product=0X600A,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:6011
{ .vendor=0X0904, .product=0X6011,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:6012
{ .vendor=0X0904, .product=0X6012,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:6013
{ .vendor=0X0904, .product=0X6013,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:6101
{ .vendor=0X0904, .product=0X6101,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:6102
{ .vendor=0X0904, .product=0X6102,
  .drivers=(const char *const []){"bm"}
},

// Device: 0904:6103
{ .vendor=0X0904, .product=0X6103,
  .drivers=(const char *const []){"bm"}
},

// Device: 0921:1200
{ .vendor=0X0921, .product=0X1200,
  .drivers=(const char *const []){"ht"}
},

// Device: 0F4E:0100
{ .vendor=0X0F4E, .product=0X0100,
  .drivers=(const char *const []){"fs"}
},

// Device: 0F4E:0111
{ .vendor=0X0F4E, .product=0X0111,
  .drivers=(const char *const []){"fs"}
},

// Device: 0F4E:0112
{ .vendor=0X0F4E, .product=0X0112,
  .drivers=(const char *const []){"fs"}
},

// Device: 0F4E:0114
{ .vendor=0X0F4E, .product=0X0114,
  .drivers=(const char *const []){"fs"}
},

// Device: 10C4:EA60
// Generic Identifier
// Vendor: Cygnal Integrated Products, Inc.
// Product: CP210x UART Bridge / myAVR mySmartUSB light
{ .vendor=0X10C4, .product=0XEA60,
  .drivers=(const char *const []){"mm", "sk"}
},

// Device: 10C4:EA80
// Generic Identifier
// Vendor: Cygnal Integrated Products, Inc.
// Product: CP210x UART Bridge
{ .vendor=0X10C4, .product=0XEA80,
  .drivers=(const char *const []){"sk"}
},

// Device: 1148:0301
{ .vendor=0X1148, .product=0X0301,
  .drivers=(const char *const []){"mm"}
},

// Device: 1209:ABC0
{ .vendor=0X1209, .product=0XABC0,
  .drivers=(const char *const []){"ic"}
},

// Device: 1C71:C004
{ .vendor=0X1C71, .product=0XC004,
  .drivers=(const char *const []){"bn"}
},

// Device: 1C71:C005
{ .vendor=0X1C71, .product=0XC005,
  .drivers=(const char *const []){"hw"}
},

// Device: 1C71:C006
{ .vendor=0X1C71, .product=0XC006,
  .drivers=(const char *const []){"hw"}
},

// Device: 1C71:C00A
{ .vendor=0X1C71, .product=0XC00A,
  .drivers=(const char *const []){"hw"}
},

// Device: 1FE4:0003
{ .vendor=0X1FE4, .product=0X0003,
  .drivers=(const char *const []){"ht"}
},

// Device: 1FE4:0044
{ .vendor=0X1FE4, .product=0X0044,
  .drivers=(const char *const []){"ht"}
},

// Device: 1FE4:0054
{ .vendor=0X1FE4, .product=0X0054,
  .drivers=(const char *const []){"ht"}
},

// Device: 1FE4:0055
{ .vendor=0X1FE4, .product=0X0055,
  .drivers=(const char *const []){"ht"}
},

// Device: 1FE4:0061
{ .vendor=0X1FE4, .product=0X0061,
  .drivers=(const char *const []){"ht"}
},

// Device: 1FE4:0064
{ .vendor=0X1FE4, .product=0X0064,
  .drivers=(const char *const []){"ht"}
},

// Device: 1FE4:0074
{ .vendor=0X1FE4, .product=0X0074,
  .drivers=(const char *const []){"ht"}
},

// Device: 1FE4:0081
{ .vendor=0X1FE4, .product=0X0081,
  .drivers=(const char *const []){"ht"}
},

// Device: 1FE4:0082
{ .vendor=0X1FE4, .product=0X0082,
  .drivers=(const char *const []){"ht"}
},

// Device: 1FE4:0083
{ .vendor=0X1FE4, .product=0X0083,
  .drivers=(const char *const []){"ht"}
},

// Device: 1FE4:0084
{ .vendor=0X1FE4, .product=0X0084,
  .drivers=(const char *const []){"ht"}
},

// Device: 1FE4:0086
{ .vendor=0X1FE4, .product=0X0086,
  .drivers=(const char *const []){"ht"}
},

// Device: 1FE4:0087
{ .vendor=0X1FE4, .product=0X0087,
  .drivers=(const char *const []){"ht"}
},

// Device: 1FE4:008A
{ .vendor=0X1FE4, .product=0X008A,
  .drivers=(const char *const []){"ht"}
},

// Device: 1FE4:008B
{ .vendor=0X1FE4, .product=0X008B,
  .drivers=(const char *const []){"ht"}
},

// Device: 4242:0001
{ .vendor=0X4242, .product=0X0001,
  .drivers=(const char *const []){"pg"}
},

// Device: C251:1122
{ .vendor=0XC251, .product=0X1122,
  .drivers=(const char *const []){"eu"}
},

// Device: C251:1123
{ .vendor=0XC251, .product=0X1123,
  .drivers=(const char *const []){"eu"}
},

// Device: C251:1124
{ .vendor=0XC251, .product=0X1124,
  .drivers=(const char *const []){"eu"}
},

// Device: C251:1125
{ .vendor=0XC251, .product=0X1125,
  .drivers=(const char *const []){"eu"}
},

// Device: C251:1126
{ .vendor=0XC251, .product=0X1126,
  .drivers=(const char *const []){"eu"}
},

// Device: C251:1127
{ .vendor=0XC251, .product=0X1127,
  .drivers=(const char *const []){"eu"}
},

// Device: C251:1128
{ .vendor=0XC251, .product=0X1128,
  .drivers=(const char *const []){"eu"}
},

// Device: C251:1129
{ .vendor=0XC251, .product=0X1129,
  .drivers=(const char *const []){"eu"}
},

// Device: C251:112A
{ .vendor=0XC251, .product=0X112A,
  .drivers=(const char *const []){"eu"}
},

// Device: C251:112B
{ .vendor=0XC251, .product=0X112B,
  .drivers=(const char *const []){"eu"}
},

// Device: C251:112C
{ .vendor=0XC251, .product=0X112C,
  .drivers=(const char *const []){"eu"}
},

// Device: C251:112D
{ .vendor=0XC251, .product=0X112D,
  .drivers=(const char *const []){"eu"}
},

// Device: C251:112E
{ .vendor=0XC251, .product=0X112E,
  .drivers=(const char *const []){"eu"}
},

// Device: C251:112F
{ .vendor=0XC251, .product=0X112F,
  .drivers=(const char *const []){"eu"}
},

// Device: C251:1130
{ .vendor=0XC251, .product=0X1130,
  .drivers=(const char *const []){"eu"}
},

// Device: C251:1131
{ .vendor=0XC251, .product=0X1131,
  .drivers=(const char *const []){"eu"}
},

// Device: C251:1132
{ .vendor=0XC251, .product=0X1132,
  .drivers=(const char *const []){"eu"}
},

// END_USB_DEVICES
};

static const uint16_t usbDeviceCount = ARRAY_COUNT(usbDeviceTable);

