/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_USB_INTERNAL
#define BRLTTY_INCLUDED_USB_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "queue.h"

typedef struct {
  UsbInputFilter filter;
} UsbInputFilterEntry;

typedef struct {
  UsbDevice *device;
  UsbEndpointDescriptor *descriptor;
  void *extension;

  union {
    struct {
      Queue *pending;
      void *completed;
      unsigned char *buffer;
      int length;
    } input;

    struct {
    } output;
  } direction;
} UsbEndpoint;

struct UsbDeviceStruct {
  UsbDeviceDescriptor descriptor;
  void *extension;
  UsbDescriptor *configurationDescriptor;
  int configurationLength;
  Queue *endpoints;
  Queue *inputFilters;
  uint16_t language;
};

extern int usbControlTransfer (
  UsbDevice *device,
  unsigned char direction,
  unsigned char recipient,
  unsigned char type,
  unsigned char request,
  unsigned short value,
  unsigned short index,
  void *buffer,
  int length,
  int timeout
);

extern UsbDevice *usbTestDevice (
  void *extension,
  UsbDeviceChooser chooser,
  void *data
);
extern int usbReadDeviceDescriptor (UsbDevice *device);
extern void usbDeallocateDeviceExtension (UsbDevice *device);

extern UsbEndpoint *usbGetEndpoint (UsbDevice *device, unsigned char endpointAddress);
extern UsbEndpoint *usbGetInputEndpoint (UsbDevice *device, unsigned char endpointNumber);
extern UsbEndpoint *usbGetOutputEndpoint (UsbDevice *device, unsigned char endpointNumber);

extern int usbAllocateEndpointExtension (UsbEndpoint *endpoint);
extern void usbDeallocateEndpointExtension (UsbEndpoint *endpoint);

extern int usbApplyInputFilters (UsbDevice *device, void *buffer, int size, int *length);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_USB_INTERNAL */
