/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

#ifndef _USB_DEFINITIONS_H
#define _USB_DEFINITIONS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "queue.h"

typedef struct {
  UsbDevice *device;
  UsbEndpointDescriptor *descriptor;

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
  UsbDescriptor *configurationDescriptor;
  int configurationLength;
  Queue *endpoints;
  int file;
  uint16_t language;
};

extern UsbDevice *usbTestDevice (
  const char *path,
  UsbDeviceChooser chooser,
  void *data
);
extern int usbReadDeviceDescriptor (UsbDevice *device);

extern UsbEndpoint *usbGetEndpoint (UsbDevice *device, unsigned char endpointAddress);
extern UsbEndpoint *usbGetInputEndpoint (UsbDevice *device, unsigned char endpointNumber);
extern UsbEndpoint *usbGetOutputEndpoint (UsbDevice *device, unsigned char endpointNumber);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _USB_DEFINITIONS_H */
