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
#include <linux/usbdevice_fs.h>
#include <linux/serial.h>

#include "misc.h"
#include "usbio.h"

struct UsbInputElement {
  struct UsbInputElement *next;
  struct UsbInputElement *previous;
  struct usbdevfs_urb *urb;
};

struct UsbDeviceStruct {
  UsbDeviceDescriptor descriptor;
  int file;
  struct UsbInputElement *inputElements;
  struct usbdevfs_urb *inputRequest;
  unsigned char inputEndpoint;
  int inputLength;
  unsigned int inputFlags;
  unsigned int stringLanguage;
};

UsbDevice *
usbOpenDevice (const char *path) {
  UsbDevice *device;
  if ((device = malloc(sizeof(*device)))) {
    memset(device, 0, sizeof(*device));
    if ((device->file = open(path, O_RDWR)) != -1) {
      if (read(device->file, &device->descriptor, USB_DT_DEVICE_SIZE) == USB_DT_DEVICE_SIZE)
        if (device->descriptor.bDescriptorType == USB_DT_DEVICE)
          if (device->descriptor.bLength == USB_DT_DEVICE_SIZE)
            return device;
      close(device->file);
    }
    free(device);
  }
  return NULL;
}

void
usbCloseDevice (UsbDevice *device) {
  close(device->file);
  free(device);
}

static UsbDevice *
usbTestDevice (const char *path, UsbDeviceChooser chooser, void *data) {
  UsbDevice *device;
  if ((device = usbOpenDevice(path))) {
    if (chooser(device, data)) return device;
    usbCloseDevice(device);
  }
  return NULL;
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

const UsbDeviceDescriptor *
usbDeviceDescriptor (UsbDevice *device) {
  return &device->descriptor;
}

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

char *
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

int
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

char *
usbGetSerialDevice (
  UsbDevice *device,
  unsigned int interface
) {
  char *path = NULL;
  char *driver;
  if ((driver = usbGetDriver(device, interface))) {
LogPrint(LOG_WARNING, "USB driver: %s", driver);
    if (strcmp(driver, "serial") == 0) {
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
    free(driver);
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
usbGetDescriptor (
  UsbDevice *device,
  unsigned char type,
  unsigned char number,
  unsigned int index,
  UsbDescriptor *descriptor,
  int timeout
) {
  return usbControlTransfer(device, USB_RECIP_DEVICE, USB_DIR_IN, USB_TYPE_STANDARD,
                            USB_REQ_GET_DESCRIPTOR, (type << 8) | number, index,
                            descriptor->bytes, sizeof(descriptor->bytes), timeout);
}

char *
usbGetString (
  UsbDevice *device,
  unsigned char number,
  int timeout
) {
  UsbDescriptor descriptor;
  int count;
  unsigned char *string;

  if (!device->stringLanguage) {
    if ((count = usbGetDescriptor(device, USB_DT_STRING, 0, 0,
                                  &descriptor, timeout)) == -1)
      return NULL;
    if (count < 4) {
      errno = ENODATA;
      return NULL;
    }
    device->stringLanguage = descriptor.string.wData[0];
  }

  if ((count = usbGetDescriptor(device, USB_DT_STRING,
                                number, device->stringLanguage,
                                &descriptor, timeout)) == -1)
    return NULL;
  count = (count - 2) / sizeof(descriptor.string.wData[0]);

  if ((string = malloc(count+1))) {
    string[count] = 0;
    while (count--) {
      uint16_t character = descriptor.string.wData[count];
      if (character & 0XFF00) character = '?';
      string[count] = character;
    }
  }
  return string;
}

static int
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

int
usbBulkRead (
  UsbDevice *device,
  unsigned char endpoint,
  void *data,
  int length,
  int timeout
) {
  return usbBulkTransfer(device, endpoint|USB_DIR_IN, data, length, timeout);
}

int
usbBulkWrite (
  UsbDevice *device,
  unsigned char endpoint,
  void *data,
  int length,
  int timeout
) {
  return usbBulkTransfer(device, endpoint|USB_DIR_OUT, data, length, timeout);
}

struct usbdevfs_urb *
usbSubmitRequest (
  UsbDevice *device,
  unsigned char endpoint,
  unsigned char type,
  void *buffer,
  int length,
  unsigned int flags,
  void *data
) {
  struct usbdevfs_urb *urb;
  if ((urb = malloc(sizeof(*urb) + length))) {
    memset(urb, 0, sizeof(*urb));
    urb->endpoint = endpoint;
    urb->type = type;
    urb->buffer = (urb->buffer_length = length)? (urb + 1): NULL;
    if (buffer) memcpy(urb->buffer, buffer, length);
    urb->flags = flags;
    urb->signr = 0;
    urb->usercontext = data;
    LogPrint(LOG_DEBUG, "submit urb: ept=%x typ=%d flg=%x sig=%d len=%d urb=%p",
             urb->endpoint, urb->type, urb->flags, urb->signr, urb->buffer_length, urb);
    if (ioctl(device->file, USBDEVFS_SUBMITURB, urb) != -1) return urb;
    free(urb);
  }
  return NULL;
}

struct usbdevfs_urb *
usbReapRequest (
  UsbDevice *device,
  int wait
) {
  struct usbdevfs_urb *urb;
  LogPrint(LOG_DEBUG, "reaping urb");
  if (ioctl(device->file,
            wait? USBDEVFS_REAPURB: USBDEVFS_REAPURBNDELAY,
            &urb) == -1) {
    urb = NULL;
  } else if (!urb) {
    errno = EAGAIN;
  } else {
    LogPrint(LOG_DEBUG, "reaped urb: %p st=%d len=%d",
             urb, urb->status, urb->actual_length);
  }
  return urb;
}

static struct UsbInputElement *
usbAddInputElement (
  UsbDevice *device
) {
  struct UsbInputElement *input;
  if ((input = malloc(sizeof(*input)))) {
    memset(input, 0, sizeof(*input));
    if ((input->urb = usbSubmitRequest(device, device->inputEndpoint,
                                       USBDEVFS_URB_TYPE_INTERRUPT,
                                       NULL, device->inputLength,
                                       device->inputFlags, input))) {
      if (device->inputElements) {
        device->inputElements->previous = input;
        input->next = device->inputElements;
      } else {
        input->next = NULL;
      }
      input->previous = NULL;
      device->inputElements = input;
      return input;
    }
    free(input);
  }
  return NULL;
}

static void
usbDeleteInputElement (
  UsbDevice *device,
  struct UsbInputElement *input
) {
  if (input->previous) {
    input->previous->next = input->next;
  } else {
    device->inputElements = input->next;
  }
  if (input->next) input->next->previous = input->previous;
  free(input);
}

int
usbBeginInput (
  UsbDevice *device,
  unsigned char endpoint,
  int length,
  int count
) {
  device->inputRequest = NULL;
  device->inputEndpoint = endpoint | USB_DIR_IN;
  device->inputLength = length;
  device->inputFlags = 0;
  while (count--) {
    if (!usbAddInputElement(device)) return -1;
  }
  return 0;
}

int
usbReapInput (
  UsbDevice *device,
  void *buffer,
  int length,
  int wait
) {
  unsigned char *bytes = buffer;
  unsigned char *target = bytes;
  while (length > 0) {
    if (!device->inputRequest) {
      if (!(device->inputRequest = usbReapRequest(device, wait))) return -1;
      usbDeleteInputElement(device, device->inputRequest->usercontext);
      usbAddInputElement(device);
    }

    {
      struct usbdevfs_urb *urb = device->inputRequest;
      unsigned char *source = urb->buffer;
      int count = urb->actual_length;
      if (length < count) count = length;
      memcpy(target, source, count);

      if ((urb->actual_length -= count)) {
        urb->buffer = source + count;
      } else {
        free(urb);
        device->inputRequest = NULL;
      }

      target += count;
      length -= count;
    }
  }
  return target - bytes;
}
