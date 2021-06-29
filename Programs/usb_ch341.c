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

#include "prologue.h"

#include <string.h>
#include <errno.h>

#include "log.h"
#include "usb_serial.h"
#include "usb_ch341.h"

typedef enum {
  CH341_REQ_READ_VERSION      = 0X5F,
  CH341_REQ_READ_REGISTER     = 0X95,
  CH341_REQ_WRITE_REGISTER    = 0X9A,
  CH341_REQ_SERIAL_INITIALIZE = 0XA1,
  CH341_REQ_MODEM_CCONTROL    = 0XA4,
} CH341_Request;

typedef enum {
  CH341_REG_BREAK     = 0X05,
  CH341_REG_PRESCALER = 0X12,
  CH341_REG_DIVISOR   = 0X13,
  CH341_REG_LCR       = 0X18,
  CH341_REG_LCR2      = 0X25,
} CH341_Register;

/* The device line speed is given by:
 *   baud = clock / (2^(12 - (3 * ps) - fact) * div)
 *   clock = 48_000_000
 *   0 <= ps <= 3
 *   0 <= fact <= 1
 *   2 <= div <= 256 if fact == 0
 *   9 <= div <= 256 if fact == 1
 */

#define CH341_CLOCK_RATE 48000000
#define CH341_CLOCK_DIVISOR(ps, fact) (1 << (12 - (3 * (ps)) - (fact)))
#define CH341_MINIMUM_RATE(ps) (CH341_CLOCK_RATE / (CH341_CLOCK_DIVISOR((ps), 1) * 512))

static const unsigned int CH341_psToMinimumRate[] = {
  CH341_MINIMUM_RATE(0),
  CH341_MINIMUM_RATE(1),
  CH341_MINIMUM_RATE(2),
  CH341_MINIMUM_RATE(3)
};

static int
usbControlWrite_CH341 (UsbDevice *device, unsigned char request, unsigned int value, unsigned int index) {
  logMessage(LOG_CATEGORY(USB_IO), "CH341 request: %02X %04X %04X", request, value, index);
  return usbControlWrite(device, UsbControlRecipient_Device, UsbControlType_Vendor,
                         request, value, index, NULL, 0, 1000) != -1;
}

static int
usbEnableAdapter_CH341 (UsbDevice *device) {
  if (!usbControlWrite_CH341(device, CH341_REQ_SERIAL_INITIALIZE, 0, 0)) return 0;
  return 1;
}

const UsbSerialOperations usbSerialOperations_CH341 = {
  .name = "CH341",     

  .enableAdapter = usbEnableAdapter_CH341,     
};
