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
#include <regex.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "misc.h"
#include "usb.h"
#include "usb_internal.h"

int
usbGetDescriptor (
  UsbDevice *device,
  unsigned char type,
  unsigned char number,
  unsigned int index,
  UsbDescriptor *descriptor,
  int timeout
) {
  return usbControlTransfer(device, USB_RECIPIENT_DEVICE, USB_DIR_IN, USB_TYPE_STANDARD,
                            USB_REQ_GET_DESCRIPTOR, (type << 8) | number, index,
                            descriptor->bytes, sizeof(descriptor->bytes), timeout);
}

int
usbGetLanguage (
  UsbDevice *device,
  uint16_t *language,
  int timeout
) {
  UsbDescriptor descriptor;
  int count;
  if ((count = usbGetDescriptor(device, USB_DESCRIPTOR_TYPE_STRING,
                                0, 0, &descriptor, timeout)) != -1) {
    if (count >= 4) {
      *language = getLittleEndian(descriptor.string.wData[0]);
      return 1;
    } else {
      errno = ENODATA;
    }
  }
  return 0;
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

  if (!device->language)
    if (!usbGetLanguage(device, &device->language, timeout))
      return NULL;

  if ((count = usbGetDescriptor(device, USB_DESCRIPTOR_TYPE_STRING,
                                number, device->language,
                                &descriptor, timeout)) == -1)
    return NULL;
  count = (count - 2) / sizeof(descriptor.string.wData[0]);

  if ((string = malloc(count+1))) {
    string[count] = 0;
    while (count--) {
      uint16_t character = getLittleEndian(descriptor.string.wData[count]);
      if (character & 0XFF00) character = '?';
      string[count] = character;
    }
  }
  return string;
}

void
usbLogString (
  UsbDevice *device,
  int index,
  const char *description
) {
  if (index) {
    char *string = usbGetString(device, index, 1000);
    if (string) {
      LogPrint(LOG_INFO, "USB: %s: %s", description, string);
      free(string);
    }
  }
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
  const void *data,
  int length,
  int timeout
) {
  return usbBulkTransfer(device, endpoint|USB_DIR_OUT, (unsigned char *)data, length, timeout);
}

static struct UsbInputElement *
usbAddInputElement (
  UsbDevice *device
) {
  struct UsbInputElement *input;
  if ((input = malloc(sizeof(*input)))) {
    memset(input, 0, sizeof(*input));
    if ((input->request = usbSubmitRequest(device, device->inputEndpoint,
                                           USB_ENDPOINT_TRANSFER_BULK,
                                           NULL, device->inputSize,
                                           input))) {
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
  if (input->request) usbCancelRequest(device, input->request);
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
  int size,
  int count
) {
  int actual = 0;

  device->inputRequest = NULL;
  device->inputEndpoint = endpoint | USB_DIR_IN;
  device->inputSize = size;

  while (actual < count) {
    if (!usbAddInputElement(device)) break;
    actual++;
  }
  return actual;
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
      UsbResponse response;
      if (!(device->inputRequest = usbReapResponse(device, &response, wait))) return -1;
      usbAddInputElement(device);
      {
        struct UsbInputElement *input = response.context;
        input->request = NULL;
        usbDeleteInputElement(device, input);
      }
      device->inputBuffer = response.buffer;
      device->inputLength = response.length;
    }

    {
      unsigned char *source = device->inputBuffer;
      int count = device->inputLength;
      if (length < count) count = length;
      memcpy(target, source, count);

      if ((device->inputLength -= count)) {
        device->inputBuffer = source + count;
      } else {
        free(device->inputRequest);
        device->inputRequest = NULL;
        device->inputBuffer = NULL;
        device->inputLength = 0;
      }

      target += count;
      length -= count;
    }
  }
  return target - bytes;
}

void
usbCloseDevice (UsbDevice *device) {
  if (device->inputRequest) free(device->inputRequest);
  while (device->inputElements) usbDeleteInputElement(device, device->inputElements);
  close(device->file);
  free(device);
}

UsbDevice *
usbOpenDevice (const char *path) {
  UsbDevice *device;
  if ((device = malloc(sizeof(*device)))) {
    memset(device, 0, sizeof(*device));
    if ((device->file = open(path, O_RDWR)) != -1) {
      if (usbReadDeviceDescriptor(device))
        if (device->descriptor.bDescriptorType == USB_DESCRIPTOR_TYPE_DEVICE)
          if (device->descriptor.bLength == USB_DESCRIPTOR_SIZE_DEVICE)
            return device;
      close(device->file);
    }
    free(device);
  }
  return NULL;
}

UsbDevice *
usbTestDevice (const char *path, UsbDeviceChooser chooser, void *data) {
  UsbDevice *device;
  if ((device = usbOpenDevice(path))) {
    LogPrint(LOG_DEBUG, "USB: testing: vendor=%04X product=%04X",
             device->descriptor.idVendor,
             device->descriptor.idProduct);
    if (chooser(device, data)) {
      usbLogString(device, device->descriptor.iManufacturer, "Manufacturer Name");
      usbLogString(device, device->descriptor.iProduct, "Product Description");
      usbLogString(device, device->descriptor.iSerialNumber, "Serial Number");
      return device;
    }
    usbCloseDevice(device);
  }
  return NULL;
}

const UsbDeviceDescriptor *
usbDeviceDescriptor (UsbDevice *device) {
  return &device->descriptor;
}

int
usbStringEquals (const char *reference, const char *value) {
  return strcmp(reference, value) == 0;
}

int
usbStringMatches (const char *reference, const char *value) {
  int ok = 0;
  regex_t expression;
  if (regcomp(&expression, value, REG_EXTENDED|REG_NOSUB) == 0) {
    if (regexec(&expression, reference, 0, NULL, 0) == 0) {
      ok = 1;
    }
    regfree(&expression);
  }
  return ok;
}

int
usbVerifyString (
  UsbDevice *device,
  UsbStringVerifier verify,
  unsigned char index,
  const char *value
) {
  int ok = 0;
  if (!(value && *value)) return 1;

  if (index) {
    char *reference = usbGetString(device, index, 1000);
    if (reference) {
      if (verify(reference, value)) ok = 1;
      free(reference);
    }
  }
  return ok;
}

int
usbVerifyManufacturer (UsbDevice *device, const char *eRegExp) {
  return usbVerifyString(device, usbStringMatches,
                         device->descriptor.iManufacturer, eRegExp);
}

int
usbVerifyProduct (UsbDevice *device, const char *eRegExp) {
  return usbVerifyString(device, usbStringMatches,
                         device->descriptor.iProduct, eRegExp);
}

int
usbVerifySerialNumber (UsbDevice *device, const char *string) {
  return usbVerifyString(device, usbStringEquals,
                         device->descriptor.iSerialNumber, string);
}

int
isUsbDevice (const char **path) {
  return isQualifiedDevice(path, "usb");
}
