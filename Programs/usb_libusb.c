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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include </usr/include/usb.h>

#include "misc.h"
#include "usb.h"
#include "usb_internal.h"

typedef struct {
  struct usb_dev_handle *handle;
} UsbDeviceExtension;

int
usbResetDevice (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  if (usb_reset(devx->handle) >= 0) return 1;
  LogError("USB device reset");
  return 0;
}

int
usbSetConfiguration (
  UsbDevice *device,
  unsigned char configuration
) {
  UsbDeviceExtension *devx = device->extension;
  if (usb_set_configuration(devx->handle, configuration) >= 0) return 1;
  LogError("USB configuration set");
  return 0;
}

int
usbClaimInterface (
  UsbDevice *device,
  unsigned char interface
) {
  UsbDeviceExtension *devx = device->extension;
  if (usb_claim_interface(devx->handle, interface) >= 0) return 1;
  LogError("USB interface claim");
  return 0;
}

int
usbReleaseInterface (
  UsbDevice *device,
  unsigned char interface
) {
  UsbDeviceExtension *devx = device->extension;
  if (usb_release_interface(devx->handle, interface) >= 0) return 1;
  LogError("USB interface release");
  return 0;
}

int
usbSetAlternative (
  UsbDevice *device,
  unsigned char interface,
  unsigned char alternative
) {
  UsbDeviceExtension *devx = device->extension;
  if (usb_set_altinterface(devx->handle, alternative) >= 0) return 1;
  LogError("USB alternative set");
  return 0;
}

int
usbResetEndpoint (
  UsbDevice *device,
  unsigned char endpointAddress
) {
  UsbDeviceExtension *devx = device->extension;
  if (usb_resetep(devx->handle, endpointAddress) >= 0) return 1;
  LogError("USB endpoint reset");
  return 0;
}

int
usbClearEndpoint (
  UsbDevice *device,
  unsigned char endpointAddress
) {
  UsbDeviceExtension *devx = device->extension;
  if (usb_clear_halt(devx->handle, endpointAddress) >= 0) return 1;
  LogError("USB endpoint clear");
  return 0;
}

int
usbControlTransfer (
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
) {
  UsbDeviceExtension *devx = device->extension;
  int count = usb_control_msg(devx->handle, direction|recipient|type, request,
                              value, index, buffer, length, timeout);
  if (count >= 0) return count;
  LogError("USB control transfer");
  return -1;
}

int
usbAllocateEndpointExtension (UsbEndpoint *endpoint) {
  return 1;
}

void
usbDeallocateEndpointExtension (UsbEndpoint *endpoint) {
}

int
usbReadEndpoint (
  UsbDevice *device,
  unsigned char endpointNumber,
  void *buffer,
  int length,
  int timeout
) {
  UsbDeviceExtension *devx = device->extension;
  const UsbEndpoint *endpoint;

  if ((endpoint = usbGetInputEndpoint(device, endpointNumber))) {
    const UsbEndpointDescriptor *descriptor = endpoint->descriptor;
    UsbEndpointTransfer transfer = USB_ENDPOINT_TRANSFER(descriptor);
    int count = -1;

    switch (transfer) {
      case UsbEndpointTransfer_Bulk:
        count = usb_bulk_read(devx->handle, descriptor->bEndpointAddress,
                              buffer, length, count);
        break;

      case UsbEndpointTransfer_Interrupt:
        count = usb_interrupt_read(devx->handle, descriptor->bEndpointAddress,
                                   buffer, length, count);
        break;

      default:
        LogPrint(LOG_ERR, "USB endpoint input transfer not supported: 0X%02X", transfer);
        errno = ENOSYS;
        break;
    }

    if (count >= 0) return count;
  }

  if (errno != EAGAIN) LogError("USB endpoint read");
  return -1;
}

int
usbWriteEndpoint (
  UsbDevice *device,
  unsigned char endpointNumber,
  const void *buffer,
  int length,
  int timeout
) {
  UsbDeviceExtension *devx = device->extension;
  const UsbEndpoint *endpoint;

  if ((endpoint = usbGetInputEndpoint(device, endpointNumber))) {
    const UsbEndpointDescriptor *descriptor = endpoint->descriptor;
    UsbEndpointTransfer transfer = USB_ENDPOINT_TRANSFER(descriptor);
    int count = -1;

    switch (transfer) {
      case UsbEndpointTransfer_Bulk:
        count = usb_bulk_write(devx->handle, descriptor->bEndpointAddress,
                               (char *)buffer, length, count);
        break;

      case UsbEndpointTransfer_Interrupt:
        count = usb_interrupt_write(devx->handle, descriptor->bEndpointAddress,
                                    (char *)buffer, length, count);
        break;

      default:
        LogPrint(LOG_ERR, "USB endpoint output transfer not supported: 0X%02X", transfer);
        errno = ENOSYS;
        break;
    }

    if (count >= 0) return count;
  }

  LogError("USB endpoint write");
  return -1;
}

void *
usbSubmitRequest (
  UsbDevice *device,
  unsigned char endpointAddress,
  void *buffer,
  int length,
  void *data
) {
  errno = ENOSYS;
  LogError("USB request submission");
  return NULL;
}

int
usbCancelRequest (
  UsbDevice *device,
  void *request
) {
  errno = ENOSYS;
  LogError("USB request cancellation");
  return 0;
}

void *
usbReapResponse (
  UsbDevice *device,
  unsigned char endpointAddress,
  UsbResponse *response,
  int wait
) {
  errno = ENOSYS;
  LogError("USB request reap");
  return NULL;
}

int
usbReadDeviceDescriptor (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  memcpy(&device->descriptor, &usb_device(devx->handle)->descriptor, UsbDescriptorSize_Device);
  return 1;
}

void
usbDeallocateDeviceExtension (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  usb_close(devx->handle);
  free(devx);
}

UsbDevice *
usbFindDevice (UsbDeviceChooser chooser, void *data) {
  UsbDevice *device = NULL;

  {
    static int initialized = 0;
    if (!initialized) {
      usb_init();
      initialized = 1;
    }
  }

  if (usb_find_busses() >= 0) {
    if (usb_find_devices() >= 0) {
      struct usb_bus *bus = usb_get_busses();

      if (bus) {
        struct usb_bus *bus0 = bus;

        do {
          struct usb_device *dev = bus->devices;

          if (dev) {
            struct usb_device *dev0 = dev;

            do {
              UsbDeviceExtension *devx;

              if ((devx = malloc(sizeof(*devx)))) {
                if ((devx->handle = usb_open(dev))) {
                  if ((device = usbTestDevice(devx, chooser, data))) return device;

                  usb_close(devx->handle);
                } else {
                }

                free(devx);
              } else {
                LogError("USB device extension allocation");
              }

              if ((dev = dev->next) == dev0) dev = NULL;
            } while (dev);
          }

          if ((bus = bus->next) == bus0) bus = NULL;
        } while (bus);
      }
    } else {
      LogPrint(LOG_ERR, "USB device find error.");
    }
  } else {
    LogPrint(LOG_ERR, "USB bus find error.");
  }

  return device;
}
