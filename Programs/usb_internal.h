/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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
  const UsbEndpointDescriptor *descriptor;
  void *extension;

  union {
    struct {
      Queue *pending;
      void *completed;
      unsigned char *buffer;
      int length;
    } input;

    struct {
      char structMayNotBeEmpty;
    } output;
  } direction;
} UsbEndpoint;

struct UsbDeviceStruct {
  UsbDeviceDescriptor descriptor;
  void *extension;
  UsbConfigurationDescriptor *configuration;
  const UsbInterfaceDescriptor *interface;
  Queue *endpoints;
  Queue *inputFilters;
  uint16_t language;
};

extern UsbDevice *usbTestDevice (
  void *extension,
  UsbDeviceChooser chooser,
  void *data
);
extern UsbEndpoint *usbGetEndpoint (UsbDevice *device, unsigned char endpointAddress);
extern UsbEndpoint *usbGetInputEndpoint (UsbDevice *device, unsigned char endpointNumber);
extern UsbEndpoint *usbGetOutputEndpoint (UsbDevice *device, unsigned char endpointNumber);
extern int usbApplyInputFilters (UsbDevice *device, void *buffer, int size, int *length);

extern int usbSetConfiguration (
  UsbDevice *device,
  unsigned char configuration
);
extern int usbClaimInterface (
  UsbDevice *device,
  unsigned char interface
);
extern int usbReleaseInterface (
  UsbDevice *device,
  unsigned char interface
);
extern int usbSetAlternative (
  UsbDevice *device,
  unsigned char interface,
  unsigned char alternative
);
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
extern int usbReadDeviceDescriptor (UsbDevice *device);
extern int usbAllocateEndpointExtension (UsbEndpoint *endpoint);
extern void usbDeallocateEndpointExtension (UsbEndpoint *endpoint);
extern void usbDeallocateDeviceExtension (UsbDevice *device);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_USB_INTERNAL */
