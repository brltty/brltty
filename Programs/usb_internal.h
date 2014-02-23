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

#ifndef BRLTTY_INCLUDED_USB_INTERNAL
#define BRLTTY_INCLUDED_USB_INTERNAL

#include "bitfield.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  UsbInputFilter *filter;
} UsbInputFilterEntry;

typedef struct UsbEndpointExtensionStruct UsbEndpointExtension;

typedef struct UsbEndpointStruct UsbEndpoint;

struct UsbEndpointStruct {
  UsbDevice *device;
  const UsbEndpointDescriptor *descriptor;
  UsbEndpointExtension *extension;
  int (*prepare) (UsbEndpoint *endpoint);

  union {
    struct {
      Queue *pending;
      unsigned asynchronous:1;

      struct {
        void *request;
        unsigned char *buffer;
        size_t length;
      } completed;

      struct {
        FileDescriptor input;
        FileDescriptor output;
        AsyncHandle monitor;
        int error;
      } pipe;
    } input;

    struct {
      char structMayNotBeEmpty;
    } output;
  } direction;
};

typedef struct UsbDeviceExtensionStruct UsbDeviceExtension;

struct UsbDeviceStruct {
  UsbDeviceDescriptor descriptor;
  UsbDeviceExtension *extension;
  const UsbSerialOperations *serialOperations;
  const void *serialData;
  UsbConfigurationDescriptor *configuration;
  const UsbInterfaceDescriptor *interface;
  Queue *endpoints;
  Queue *inputFilters;
  uint16_t language;
};

extern UsbDevice *usbTestDevice (
  UsbDeviceExtension *extension,
  UsbDeviceChooser chooser,
  void *data
);
extern UsbEndpoint *usbGetEndpoint (UsbDevice *device, unsigned char endpointAddress);
extern UsbEndpoint *usbGetInputEndpoint (UsbDevice *device, unsigned char endpointNumber);
extern UsbEndpoint *usbGetOutputEndpoint (UsbDevice *device, unsigned char endpointNumber);
extern int usbApplyInputFilters (UsbDevice *device, void *buffer, size_t size, ssize_t *length);

extern int usbSetSerialOperations (UsbDevice *device);

extern int usbSetConfiguration (UsbDevice *device, unsigned char configuration);

extern int usbClaimInterface (UsbDevice *device, unsigned char interface);

extern int usbReleaseInterface (UsbDevice *device, unsigned char interface);

extern int usbSetAlternative (
  UsbDevice *device,
  unsigned char interface,
  unsigned char alternative
);

extern int usbMakeInputPipe (UsbEndpoint *endpoint);
extern void usbDestroyInputPipe (UsbEndpoint *endpoint);
extern int usbEnqueueInput (UsbEndpoint *endpoint, const void *buffer, size_t length);
extern void usbSetInputError (UsbEndpoint *endpoint, int error);

extern int usbMonitorInputPipe (
  UsbDevice *device, unsigned char endpointNumber,
  AsyncMonitorCallback *callback, void *data
);

extern ssize_t usbControlTransfer (
  UsbDevice *device,
  uint8_t direction,
  uint8_t recipient,
  uint8_t type,
  uint8_t request,
  uint16_t value,
  uint16_t index,
  void *buffer,
  uint16_t length,
  int timeout
);

extern int usbReadDeviceDescriptor (UsbDevice *device);
extern int usbAllocateEndpointExtension (UsbEndpoint *endpoint);
extern void usbDeallocateEndpointExtension (UsbEndpointExtension *eptx);
extern void usbDeallocateDeviceExtension (UsbDeviceExtension *devx);

extern void usbLogSetupPacket (const UsbSetupPacket *setup);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_USB_INTERNAL */
