/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <usb.h>

#include "misc.h"
#include "io_usb.h"
#include "usb_internal.h"

typedef struct {
  struct usb_dev_handle *handle;
} UsbDeviceExtension;

int
usbResetDevice (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  int result = usb_reset(devx->handle);
  if (result >= 0) return 1;

  errno = -result;
  LogError("USB device reset");
  return 0;
}

int
usbSetConfiguration (
  UsbDevice *device,
  unsigned char configuration
) {
  UsbDeviceExtension *devx = device->extension;
  int result = usb_set_configuration(devx->handle, configuration);
  if (result >= 0) return 1;

  errno = -result;
  LogError("USB configuration set");
  return 0;
}

int
usbClaimInterface (
  UsbDevice *device,
  unsigned char interface
) {
  UsbDeviceExtension *devx = device->extension;
  int result = usb_claim_interface(devx->handle, interface);
  if (result >= 0) return 1;

  errno = -result;
  LogError("USB interface claim");
  return 0;
}

int
usbReleaseInterface (
  UsbDevice *device,
  unsigned char interface
) {
  UsbDeviceExtension *devx = device->extension;
  int result = usb_release_interface(devx->handle, interface);
  if (result >= 0) return 1;

  errno = -result;
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
  int result = usb_set_altinterface(devx->handle, alternative);
  if (result >= 0) return 1;

  errno = -result;
  LogError("USB alternative set");
  return 0;
}

int
usbResetEndpoint (
  UsbDevice *device,
  unsigned char endpointAddress
) {
  UsbDeviceExtension *devx = device->extension;
  int result = usb_resetep(devx->handle, endpointAddress);
  if (result >= 0) return 1;

  errno = -result;
  LogError("USB endpoint reset");
  return 0;
}

int
usbClearEndpoint (
  UsbDevice *device,
  unsigned char endpointAddress
) {
  UsbDeviceExtension *devx = device->extension;
  int result = usb_clear_halt(devx->handle, endpointAddress);
  if (result >= 0) return 1;

  errno = -result;
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
  int result = usb_control_msg(devx->handle, direction|recipient|type, request,
                               value, index, buffer, length, timeout);
  if (result >= 0) return result;

  errno = -result;
  LogError("USB control transfer");
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
  return NULL;
}

int
usbCancelRequest (
  UsbDevice *device,
  void *request
) {
  errno = ENOSYS;
  LogError("USB request cancel");
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
    int result = -1;

    switch (transfer) {
      case UsbEndpointTransfer_Bulk:
        result = usb_bulk_read(devx->handle, descriptor->bEndpointAddress,
                               buffer, length, timeout);
        break;

      case UsbEndpointTransfer_Interrupt:
        result = usb_interrupt_read(devx->handle, descriptor->bEndpointAddress,
                                    buffer, length, timeout);
        break;

      default:
        LogPrint(LOG_ERR, "USB endpoint input transfer not supported: 0X%02X", transfer);
        result = -ENOSYS;
        break;
    }

    if (result >= 0) return result;
    errno = -result;
  }

  if (errno != EAGAIN)
#ifdef ETIMEDOUT
    if (errno != ETIMEDOUT)
#endif /* ETIMEDOUT */
      LogError("USB endpoint read");
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

  if ((endpoint = usbGetOutputEndpoint(device, endpointNumber))) {
    const UsbEndpointDescriptor *descriptor = endpoint->descriptor;
    UsbEndpointTransfer transfer = USB_ENDPOINT_TRANSFER(descriptor);
    int result = -1;

    switch (transfer) {
      case UsbEndpointTransfer_Bulk:
        result = usb_bulk_write(devx->handle, descriptor->bEndpointAddress,
                                (char *)buffer, length, timeout);
        break;

      case UsbEndpointTransfer_Interrupt:
        result = usb_interrupt_write(devx->handle, descriptor->bEndpointAddress,
                                     (char *)buffer, length, timeout);
        break;

      default:
        LogPrint(LOG_ERR, "USB endpoint output transfer not supported: 0X%02X", transfer);
        result = -ENOSYS;
        break;
    }

    if (result >= 0) return result;
    errno = -result;
  }

  LogError("USB endpoint write");
  return -1;
}

int
usbReadDeviceDescriptor (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  memcpy(&device->descriptor, &usb_device(devx->handle)->descriptor, UsbDescriptorSize_Device);
  return 1;
}

int
usbAllocateEndpointExtension (UsbEndpoint *endpoint) {
  return 1;
}

void
usbDeallocateEndpointExtension (UsbEndpoint *endpoint) {
}

void
usbDeallocateDeviceExtension (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;

  if (devx->handle) {
    usb_close(devx->handle);
    devx->handle = NULL;
  }

  free(devx);
}

UsbDevice *
usbFindDevice (UsbDeviceChooser chooser, void *data) {
  UsbDevice *device = NULL;
  int result;

  {
    static int initialized = 0;
    if (!initialized) {
      usb_init();
      initialized = 1;
    }
  }

  if ((result = usb_find_busses()) >= 0) {
    if ((result = usb_find_devices()) >= 0) {
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
                  LogPrint(LOG_ERR, "USB open error: vendor=%X product=%X",
                           dev->descriptor.idVendor, dev->descriptor.idProduct);
                }

                free(devx);
              } else {
                LogError("USB device extension allocate");
              }

              if ((dev = dev->next) == dev0) dev = NULL;
            } while (dev);
          }

          if ((bus = bus->next) == bus0) bus = NULL;
        } while (bus);
      }
    } else {
      errno = -result;
      LogError("USB devices find");
    }
  } else {
    errno = -result;
    LogError("USB busses find");
  }

  return device;
}
