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

#include "hid_types.h"

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
} HidDeviceFilter_USB;

extern void hidInitializeDeviceFilter_USB (HidDeviceFilter_USB *filter);
extern HidDevice *hidOpenDevice_USB (const HidDeviceFilter_USB *filter);

typedef struct {
  const char *deviceAddress;
  const char *deviceName;
} HidDeviceFilter_Bluetooth;

extern void hidInitializeDeviceFilter_Bluetooth (HidDeviceFilter_Bluetooth *filter);
extern HidDevice *hidOpenDevice_Bluetooth (const HidDeviceFilter_Bluetooth *filter);

extern void hidCloseDevice (HidDevice *device);

extern HidItemsDescriptor *hidGetItems (HidDevice *device);
extern int hidGetIdentifiers (HidDevice *device, uint16_t *vendor, uint16_t *product);

extern int hidGetReport (HidDevice *device, char *buffer, size_t size);
extern int hidSetReport (HidDevice *device, const char *report, size_t size);

extern int hidGetFeature (HidDevice *device, char *buffer, size_t size);
extern int hidSetFeature (HidDevice *device, const char *feature, size_t size);

extern int hidParseIdentifier (uint16_t *identifier, const char *string);
extern int hidMatchString (const char *actualString, const char *testString);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_IO_HID */
