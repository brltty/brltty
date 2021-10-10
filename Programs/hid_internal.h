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

#ifndef BRLTTY_INCLUDED_HID_INTERNAL
#define BRLTTY_INCLUDED_HID_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct HidHandleStruct HidHandle;

typedef HidHandle *HidNewUSBMethod (const HidDeviceFilter_USB *filter);
typedef HidHandle *HidNewBluetoothMethod (const HidDeviceFilter_Bluetooth *filter);
typedef void HidDestroyMethod (HidHandle *handle);

typedef HidItemsDescriptor *HidGetItemsMethod (HidHandle *handle);
typedef int HidGetIdentifiersMethod (HidHandle *handle, uint16_t *vendor, uint16_t *product);

typedef int HidGetReportMethod (HidHandle *handle, char *buffer, size_t size);
typedef int HidSetReportMethod (HidHandle *handle, const char *report, size_t size);

typedef int HidGetFeatureMethod (HidHandle *handle, char *buffer, size_t size);
typedef int HidSetFeatureMethod (HidHandle *handle, const char *feature, size_t size);

typedef int HidMonitorInputMethod (HidHandle *handle, AsyncMonitorCallback *callback, void *data);
typedef int HidAwaitInputMethod (HidHandle *handle, int timeout);

typedef ssize_t HidReadDataMethod (
  HidHandle *handle, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
);

typedef struct {
  HidNewUSBMethod *newUSB;
  HidNewBluetoothMethod *newBluetooth;
  HidDestroyMethod *destroy;

  HidGetItemsMethod *getItems;
  HidGetIdentifiersMethod *getIdentifiers;

  HidGetReportMethod *getReport;
  HidSetReportMethod *setReport;

  HidGetFeatureMethod *getFeature;
  HidSetFeatureMethod *setFeature;

  HidMonitorInputMethod *monitorInput;
  HidAwaitInputMethod *awaitInput;
  HidReadDataMethod *readData;
} HidPlatformMethods;

extern HidPlatformMethods hidPlatformMethods;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_HID_INTERNAL */
