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
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/compiler.h>
#include <linux/usbdevice_fs.h>
#include <linux/serial.h>

#include "misc.h"
#include "usb.h"
#include "usb_definitions.h"

int
usbResetDevice (UsbDevice *device) {
  return ioctl(device->file, USBDEVFS_RESET, NULL);
}

int
usbSetConfiguration (
  UsbDevice *device,
  unsigned int configuration
) {
  return ioctl(device->file, USBDEVFS_SETCONFIGURATION, &configuration);
}

static char *
usbGetDriver (
  UsbDevice *device,
  unsigned int interface
) {
  struct usbdevfs_getdriver arg;
  memset(&arg, 0, sizeof(arg));
  arg.interface = interface;
  if (ioctl(device->file, USBDEVFS_GETDRIVER, &arg) == -1) return NULL;
  return strdup(arg.driver);
}

static int
usbControlDriver (
  UsbDevice *device,
  unsigned int interface,
  int code,
  void *data
) {
  struct usbdevfs_ioctl arg;
  memset(&arg, 0, sizeof(arg));
  arg.ifno = interface;
  arg.ioctl_code = code;
  arg.data = data;
  return ioctl(device->file, USBDEVFS_IOCTL, &arg);
}

int
usbIsSerialDevice (
  UsbDevice *device,
  unsigned int interface
) {
  int yes = 0;
  char *driver = usbGetDriver(device, interface);
  if (driver) {
    if (strcmp(driver, "serial") == 0) {
      yes = 1;
    }
    free(driver);
  }
  return yes;
}

char *
usbGetSerialDevice (
  UsbDevice *device,
  unsigned int interface
) {
  char *path = NULL;
  if (usbIsSerialDevice(device, interface)) {
    struct serial_struct serial;
    if (usbControlDriver(device, interface, TIOCGSERIAL, &serial) != -1) {
      static const char *const prefixTable[] = {"/dev/ttyUSB", "/dev/usb/tts/", NULL};
      const char *const *prefix = prefixTable;
      while (*prefix) {
        char buffer[0X80];
        snprintf(buffer, sizeof(buffer), "%s%d", *prefix, serial.line);
        if (access(buffer, F_OK) != -1) {
          path = strdup(buffer);
          break;
        }
        ++prefix;
      }
    } else {
      LogError("USB control driver");
    }
  }
  return path;
}

int
usbClaimInterface (
  UsbDevice *device,
  unsigned int interface
) {
  return ioctl(device->file, USBDEVFS_CLAIMINTERFACE, &interface);
}

int
usbReleaseInterface (
  UsbDevice *device,
  unsigned int interface
) {
  return ioctl(device->file, USBDEVFS_RELEASEINTERFACE, &interface);
}

int
usbSetAlternative (
  UsbDevice *device,
  unsigned int interface,
  unsigned int alternative
) {
  struct usbdevfs_setinterface arg;
  memset(&arg, 0, sizeof(arg));
  arg.interface = interface;
  arg.altsetting = alternative;
  return ioctl(device->file, USBDEVFS_SETINTERFACE, &arg);
}

int
usbResetEndpoint (
  UsbDevice *device,
  unsigned int endpoint
) {
  return ioctl(device->file, USBDEVFS_RESETEP, &endpoint);
}

int
usbClearEndpoint (
  UsbDevice *device,
  unsigned int endpoint
) {
  return ioctl(device->file, USBDEVFS_CLEAR_HALT, &endpoint);
}

int
usbControlTransfer (
  UsbDevice *device,
  unsigned char recipient,
  unsigned char direction,
  unsigned char type,
  unsigned char request,
  unsigned short value,
  unsigned short index,
  void *data,
  int length,
  int timeout
) {
  union {
    struct usbdevfs_ctrltransfer transfer;
    UsbSetupPacket setup;
  } arg;
  memset(&arg, 0, sizeof(arg));
  arg.setup.bRequestType = direction | type | recipient;
  arg.setup.bRequest = request;
  arg.setup.wValue = value;
  arg.setup.wIndex = index;
  arg.setup.wLength = length;
  arg.transfer.data = data;
  arg.transfer.timeout = timeout;
  return ioctl(device->file, USBDEVFS_CONTROL, &arg);
}

int
usbBulkTransfer (
  UsbDevice *device,
  unsigned char endpoint,
  void *data,
  int length,
  int timeout
) {
  struct usbdevfs_bulktransfer arg;
  memset(&arg, 0, sizeof(arg));
  arg.ep = endpoint;
  arg.data = data;
  arg.len = length;
  arg.timeout = timeout;
  return ioctl(device->file, USBDEVFS_BULK, &arg);
}

void *
usbSubmitRequest (
  UsbDevice *device,
  unsigned char endpoint,
  unsigned char transfer,
  void *buffer,
  int length,
  unsigned int flags,
  void *data
) {
  struct usbdevfs_urb *urb;
  if ((urb = malloc(sizeof(*urb) + length))) {
    memset(urb, 0, sizeof(*urb));
    urb->endpoint = endpoint;
    switch (transfer) {
      case USB_ENDPOINT_TRANSFER_CONTROL:
        urb->type = USBDEVFS_URB_TYPE_CONTROL;
        break;
      case USB_ENDPOINT_TRANSFER_ISOCHRONOUS:
        urb->type = USBDEVFS_URB_TYPE_ISO;
        break;
      case USB_ENDPOINT_TRANSFER_BULK:
        urb->type = USBDEVFS_URB_TYPE_BULK;
        break;
      case USB_ENDPOINT_TRANSFER_INTERRUPT:
        urb->type = USBDEVFS_URB_TYPE_INTERRUPT;
        break;
    }
    urb->buffer = (urb->buffer_length = length)? (urb + 1): NULL;
    if (buffer) memcpy(urb->buffer, buffer, length);
    urb->flags = flags;
    urb->signr = 0;
    urb->usercontext = data;
    if (ioctl(device->file, USBDEVFS_SUBMITURB, urb) != -1) return urb;
    free(urb);
  }
  return NULL;
}

void *
usbReapResponse (
  UsbDevice *device,
  UsbResponse *response,
  int wait
) {
  struct usbdevfs_urb *urb;

  response->buffer = NULL;
  response->length = 0;
  response->context = NULL;

  if (ioctl(device->file,
            wait? USBDEVFS_REAPURB: USBDEVFS_REAPURBNDELAY,
            &urb) == -1) {
    urb = NULL;
  } else if (!urb) {
    errno = EAGAIN;
  } else if (urb->status) {
    int status = urb->status;
    if (status < 0) status = -status;
    free(urb);
    urb = NULL;
    errno = status;
  } else {
    response->buffer = urb->buffer;
    response->length = urb->actual_length;
    response->context = urb->usercontext;
  }
  return urb;
}

int
usbReadDeviceDescriptor (UsbDevice *device) {
  int count = read(device->file, &device->descriptor, USB_DESCRIPTOR_SIZE_DEVICE);
  return count == USB_DESCRIPTOR_SIZE_DEVICE;
}

static UsbDevice *
usbSearchDevice (const char *root, UsbDeviceChooser chooser, void *data) {
  UsbDevice *device = NULL;
  DIR *directory;
  if ((directory = opendir(root))) {
    struct dirent *entry;
    while ((entry = readdir(directory))) {
      struct stat status;
      char path[PATH_MAX+1];
      if (strlen(entry->d_name) != 3) continue;
      if (!isdigit(entry->d_name[0]) ||
          !isdigit(entry->d_name[1]) ||
          !isdigit(entry->d_name[2])) continue;
      snprintf(path, sizeof(path), "%s/%s", root, entry->d_name);
      if (stat(path, &status) == -1) continue;
      if (S_ISDIR(status.st_mode)) {
        if ((device = usbSearchDevice(path, chooser, data))) break;
      } else if (S_ISREG(status.st_mode)) {
        if ((device = usbTestDevice(path, chooser, data))) break;
      }
    }
    closedir(directory);
  }
  return device;
}

UsbDevice *
usbFindDevice (UsbDeviceChooser chooser, void *data) {
  return usbSearchDevice("/proc/bus/usb", chooser, data);
}
