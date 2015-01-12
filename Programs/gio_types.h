/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_GIO_TYPES
#define BRLTTY_INCLUDED_GIO_TYPES

#include "serial_types.h"
#include "usb_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  const void *applicationData;
  int readyDelay;
  int inputTimeout;
  int outputTimeout;
  int requestTimeout;
} GioOptions;

typedef ssize_t GioUsbWriteDataMethod (
  UsbDevice *device, const UsbChannelDefinition *definition,
  const void *data, size_t size, int timeout
);

typedef int GioUsbAwaitInputMethod (
  UsbDevice *device, const UsbChannelDefinition *definition,
  int timeout
);

typedef ssize_t GioUsbReadDataMethod (
  UsbDevice *device, const UsbChannelDefinition *definition,
  void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
);

typedef struct {
  const void *applicationData;
  GioUsbWriteDataMethod *writeData;
  GioUsbAwaitInputMethod *awaitInput;
  GioUsbReadDataMethod *readData;
  UsbInputFilter *inputFilter;
} GioUsbConnectionProperties;

typedef void GioUsbSetConnectionPropertiesMethod (
  GioUsbConnectionProperties *properties,
  const UsbChannelDefinition *definition
);

typedef struct {
  struct {
    const SerialParameters *parameters;
    GioOptions options;
  } serial;

  struct {
    const UsbChannelDefinition *channelDefinitions;
    GioUsbSetConnectionPropertiesMethod *setConnectionProperties;
    GioOptions options;
  } usb;

  struct {
    uint8_t channelNumber;
    unsigned discoverChannel:1;
    GioOptions options;
  } bluetooth;
} GioDescriptor;

typedef struct GioEndpointStruct GioEndpoint;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_GIO_TYPES */
