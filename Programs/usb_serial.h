/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_USB_SERIAL
#define BRLTTY_INCLUDED_USB_SERIAL

#include "io_usb.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  uint16_t vendor;
  uint16_t product;
  const UsbSerialOperations *operations;
  unsigned generic:1;
} UsbSerialAdapter;

extern const UsbSerialAdapter *usbFindSerialAdapter (const UsbDeviceDescriptor *descriptor);

extern int usbSkipInitialBytes (UsbInputFilterData *data, unsigned int count);

extern const UsbSerialOperations usbSerialOperations_CDC_ACM;
extern const UsbSerialOperations usbSerialOperations_Belkin;
extern const UsbSerialOperations usbSerialOperations_CP2101;
extern const UsbSerialOperations usbSerialOperations_CP2110;
extern const UsbSerialOperations usbSerialOperations_FTDI_SIO;
extern const UsbSerialOperations usbSerialOperations_FTDI_FT8U232AM;
extern const UsbSerialOperations usbSerialOperations_FTDI_FT232BM;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_USB_SERIAL */
