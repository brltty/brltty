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
typedef void HidDestroyHandleMethod (HidHandle *handle);

typedef HidItemsDescriptor *HidGetItemsMethod (HidHandle *handle);
typedef int HidGetIdentifiersMethod (HidHandle *handle, uint16_t *vendor, uint16_t *product);

typedef int HidGetReportMethod (HidHandle *handle, unsigned char *buffer, size_t size);
typedef int HidSetReportMethod (HidHandle *handle, const unsigned char *report, size_t size);

typedef int HidGetFeatureMethod (HidHandle *handle, unsigned char *buffer, size_t size);
typedef int HidSetFeatureMethod (HidHandle *handle, const unsigned char *feature, size_t size);

typedef int HidWriteDataMethod (HidHandle *handle, const unsigned char *data, size_t size);
typedef int HidMonitorInputMethod (HidHandle *handle, AsyncMonitorCallback *callback, void *data);
typedef int HidAwaitInputMethod (HidHandle *handle, int timeout);

typedef ssize_t HidReadDataMethod (
  HidHandle *handle, unsigned char *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
);

typedef const char *HidGetDeviceAddressMethod (HidHandle *handle);
typedef const char *HidGetDeviceNameMethod (HidHandle *handle);

typedef const char *HidGetHostPathMethod (HidHandle *handle);
typedef const char *HidGetHostDeviceMethod (HidHandle *handle);

typedef struct {
  HidDestroyHandleMethod *destroyHandle;

  HidGetItemsMethod *getItems;
  HidGetIdentifiersMethod *getIdentifiers;

  HidGetReportMethod *getReport;
  HidSetReportMethod *setReport;

  HidGetFeatureMethod *getFeature;
  HidSetFeatureMethod *setFeature;

  HidWriteDataMethod *writeData;
  HidMonitorInputMethod *monitorInput;
  HidAwaitInputMethod *awaitInput;
  HidReadDataMethod *readData;

  HidGetDeviceAddressMethod *getDeviceAddress;
  HidGetDeviceNameMethod *getDeviceName;

  HidGetHostPathMethod *getHostPath;
  HidGetHostDeviceMethod *getHostDevice;
} HidHandleMethods;

typedef HidHandle *HidNewUSBHandleMethod (const HidUSBFilter *filter);
typedef HidHandle *HidNewBluetoothHandleMethod (const HidBluetoothFilter *filter);

typedef struct {
  const char *packageName;
  const HidHandleMethods *handleMethods;

  HidNewUSBHandleMethod *newUSBHandle;
  HidNewBluetoothHandleMethod *newBluetoothHandle;
} HidPackageDescriptor;

extern const HidPackageDescriptor hidPackageDescriptor;

typedef int HidGetStringMethod (
  HidHandle *handle, char *buffer, size_t size, void *data
);

extern const char *hidCacheString (
  HidHandle *handle, char **cachedValue,
  char *buffer, size_t size,
  HidGetStringMethod *getString, void *data
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_HID_INTERNAL */
