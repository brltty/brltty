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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <dev/usb/usb.h>

#include "misc.h"
#include "usb.h"
#include "usb_internal.h"

static int
usbSetTimeout (int file, int timeout) {
  int arg = timeout;
  if (ioctl(file, USB_SET_TIMEOUT, &arg) != -1) return 1;
  LogError("USB set timeout");
  return 0;
}

int
usbResetDevice (UsbDevice *device) {
  errno = ENOSYS;
  LogError("USB device reset");
  return 0;
}

int
usbSetConfiguration (
  UsbDevice *device,
  unsigned char configuration
) {
  int arg = configuration;
  if (ioctl(device->file, USB_SET_CONFIG, &arg) != -1) return 1;
  LogError("USB configuration set");
  return 0;
}

int
usbIsSerialDevice (
  UsbDevice *device,
  unsigned char interface
) {
  return 0;
}

int
usbClaimInterface (
  UsbDevice *device,
  unsigned char interface
) {
  return 1;
/*
  errno = ENOSYS;
  LogError("USB interface claim");
  return 0;
*/
}

int
usbReleaseInterface (
  UsbDevice *device,
  unsigned char interface
) {
  return 1;
/*
  errno = ENOSYS;
  LogError("USB interface release");
  return 0;
*/
}

int
usbSetAlternative (
  UsbDevice *device,
  unsigned char interface,
  unsigned char alternative
) {
  struct usb_alt_interface arg;
  memset(&arg, 0, sizeof(arg));
  arg.uai_interface_index = interface;
  arg.uai_alt_no = alternative;
  if (ioctl(device->file, USB_SET_ALTINTERFACE, &arg) != -1) return 1;
  LogError("USB alternative set");
  return 0;
}

int
usbResetEndpoint (
  UsbDevice *device,
  unsigned char endpointAddress
) {
  errno = ENOSYS;
  LogError("USB endpoint reset");
  return 0;
}

int
usbClearEndpoint (
  UsbDevice *device,
  unsigned char endpointAddress
) {
  errno = ENOSYS;
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
  struct usb_ctl_request arg;
  memset(&arg, 0, sizeof(arg));
  arg.ucr_request.bmRequestType = direction | recipient | type;
  arg.ucr_request.bRequest = request;
  USETW(arg.ucr_request.wValue, value);
  USETW(arg.ucr_request.wIndex, index);
  USETW(arg.ucr_request.wLength, length);
  arg.ucr_data = buffer;
  arg.ucr_flags = USBD_SHORT_XFER_OK;
  usbSetTimeout(device->file, timeout);
  if (ioctl(device->file, USB_DO_REQUEST, &arg) != -1) return arg.ucr_actlen;
  LogError("USB control transfer");
  return -1;
}

int
usbBulkRead (
  UsbDevice *device,
  unsigned char endpointNumber,
  void *buffer,
  int length,
  int timeout
) {
  errno = ENOSYS;
  LogError("USB bulk read");
  return -1;
}

int
usbBulkWrite (
  UsbDevice *device,
  unsigned char endpointNumber,
  const void *buffer,
  int length,
  int timeout
) {
  errno = ENOSYS;
  LogError("USB bulk write");
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
  UsbResponse *response,
  int wait
) {
  errno = ENOSYS;
  LogError("USB request reap");
  return NULL;
}

int
usbOpenEndpoint (const UsbEndpointDescriptor *descriptor, void **system) {
  return 1;
}

void
usbCloseEndpoint (void *system) {
}

int
usbReadDeviceDescriptor (UsbDevice *device) {
  if (ioctl(device->file, USB_GET_DEVICE_DESC, &device->descriptor) != -1) {
    device->descriptor.bcdUSB = getLittleEndian(device->descriptor.bcdUSB);
    device->descriptor.idVendor = getLittleEndian(device->descriptor.idVendor);
    device->descriptor.idProduct = getLittleEndian(device->descriptor.idProduct);
    device->descriptor.bcdDevice = getLittleEndian(device->descriptor.bcdDevice);
    return 1;
  }
  LogError("USB device descriptor read");
  return 0;
}

UsbDevice *
usbFindDevice (UsbDeviceChooser chooser, void *data) {
  UsbDevice *device = NULL;
  int busNumber = 0;
  while (1) {
    char busPath[PATH_MAX+1];
    int bus;
    snprintf(busPath, sizeof(busPath), "/dev/usb%d", busNumber);
    if ((bus = open(busPath, O_RDONLY)) != -1) {
      int deviceNumber;
      for (deviceNumber=1; deviceNumber<USB_MAX_DEVICES; deviceNumber++) {
	struct usb_device_info info;
	memset(&info, 0, sizeof(info));
	info.udi_addr = deviceNumber;
	if (ioctl(bus, USB_DEVICEINFO, &info) != -1) {
	  static const char *driver = "ugen";
	  const char *deviceName = info.udi_devnames[0];
	  if (strncmp(deviceName, driver, strlen(driver)) == 0) {
	    char devicePath[PATH_MAX+1];
	    snprintf(devicePath, sizeof(devicePath), "/dev/%s.00", deviceName);
	    if ((device = usbTestDevice(devicePath, chooser, data))) {
	      close(bus);
	      return device;
	    }
	  }
	} else if (errno != ENXIO) {
	  LogError("USB device query");
	}
      }
      close(bus);
    } else if (errno == ENOENT) {
      break;
    } else if (errno != ENXIO) {
      LogError("USB bus open");
    }
    busNumber++;
  }
  return device;
}
