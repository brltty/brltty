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

#ifndef BRLTTY_INCLUDED_IO_HID
#define BRLTTY_INCLUDED_IO_HID

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct HidDeviceStruct HidDevice;

typedef struct {
  const char *manufacturerName;
  const char *productDescription;
  const char *serialNumber;
  uint16_t vendorIdentifier;
  uint16_t productIdentifier;
} HidDeviceDescription_USB;

extern void hidInitializeDeviceDescription_USB (HidDeviceDescription_USB *description);
extern HidDevice *hidOpenDevice_USB (const HidDeviceDescription_USB *description);

typedef struct {
  const char *deviceAddress;
} HidDeviceDescription_Bluetooth;

extern void hidInitializeDeviceDescription_Bluetooth (HidDeviceDescription_Bluetooth *description);
extern HidDevice *hidOpenDevice_Bluetooth (const HidDeviceDescription_Bluetooth *description);

extern void hidCloseDevice (HidDevice *hid);

extern int hidParseIdentifier (uint16_t *identifier, const char *string);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_IO_HID */
