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
  return usbControlTransfer(device, USB_RECIPIENT_DEVICE, USB_DIRECTION_INPUT, USB_TYPE_STANDARD,
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
  } else {
    LogError("USB string allocation");
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
  return usbBulkTransfer(device, endpoint|USB_DIRECTION_INPUT, data, length, timeout);
}

int
usbBulkWrite (
  UsbDevice *device,
  unsigned char endpoint,
  const void *data,
  int length,
  int timeout
) {
  return usbBulkTransfer(device, endpoint|USB_DIRECTION_OUTPUT, (unsigned char *)data, length, timeout);
}

static struct UsbInputElement *
usbAddInputElement (
  UsbDevice *device
) {
  struct UsbInputElement *input;
  if ((input = malloc(sizeof(*input)))) {
    memset(input, 0, sizeof(*input));
    if ((input->request = usbSubmitRequest(device, device->inputEndpoint,
                                           device->inputTransfer,
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
  } else {
    LogError("USB input element allocation");
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
  unsigned char transfer,
  int size,
  int count
) {
  int actual = 0;

  device->inputRequest = NULL;
  device->inputEndpoint = endpoint | USB_DIRECTION_INPUT;
  device->inputTransfer = transfer;
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
      int count = device->inputLength;
      if (length < count) count = length;
      memcpy(target, device->inputBuffer, count);

      if ((device->inputLength -= count)) {
        device->inputBuffer += count;
      } else {
        device->inputLength = 0;
        device->inputBuffer = NULL;
        free(device->inputRequest);
        device->inputRequest = NULL;
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
  } else {
    LogError("USB device allocation");
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

static int
usbSetBelkinAttribute (UsbDevice *device, unsigned char request, int value) {
  return usbControlTransfer(device,
                            USB_RECIPIENT_DEVICE, USB_DIRECTION_OUTPUT, USB_TYPE_VENDOR, 
                            request, value, 0, NULL, 0, 1000);
}
static int
usbSetBelkinBaud (UsbDevice *device, int rate) {
  return usbSetBelkinAttribute(device, 0, 230400/rate);
}
static int
usbSetBelkinStopBits (UsbDevice *device, int bits) {
  return usbSetBelkinAttribute(device, 1, bits-1);
}
static int
usbSetBelkinDataBits (UsbDevice *device, int bits) {
  return usbSetBelkinAttribute(device, 2, bits-5);
}
static int
usbSetBelkinParity (UsbDevice *device, UsbSerialParity parity) {
  int value;
  switch (parity) {
    case USB_SERIAL_PARITY_SPACE: value = 4; break;
    case USB_SERIAL_PARITY_ODD:   value = 2; break;
    case USB_SERIAL_PARITY_EVEN:  value = 1; break;
    case USB_SERIAL_PARITY_MARK:  value = 3; break;
    default:
    case USB_SERIAL_PARITY_NONE:  value = 0; break;
  }
  return usbSetBelkinAttribute(device, 3, value);
}
static int
usbSetBelkinDtrState (UsbDevice *device, int state) {
  return usbSetBelkinAttribute(device, 10, state);
}
static int
usbSetBelkinRtsState (UsbDevice *device, int state) {
  return usbSetBelkinAttribute(device, 11, state);
}
static int
usbSetBelkinFlowControl (UsbDevice *device, int flow) {
  int value = 0;
  if (flow & USB_SERIAL_FLOW_OUTPUT_XON) value |= 0X0080;
  if (flow & USB_SERIAL_FLOW_OUTPUT_CTS) value |= 0X0001;
  if (flow & USB_SERIAL_FLOW_OUTPUT_DSR) value |= 0X0002;
  if (flow & USB_SERIAL_FLOW_OUTPUT_RTS) value |= 0X0020;
  if (flow & USB_SERIAL_FLOW_INPUT_XON ) value |= 0X0100;
  if (flow & USB_SERIAL_FLOW_INPUT_RTS ) value |= 0X0010;
  if (flow & USB_SERIAL_FLOW_INPUT_DTR ) value |= 0X0008;
  if (flow & USB_SERIAL_FLOW_INPUT_DSR ) value |= 0X0004;
  return usbSetBelkinAttribute(device, 16, value);
}
static const UsbSerialOperations usbBelkinOperations = {
  usbSetBelkinBaud,
  usbSetBelkinFlowControl,
  usbSetBelkinDataBits,
  usbSetBelkinStopBits,
  usbSetBelkinParity,
  usbSetBelkinDtrState,
  usbSetBelkinRtsState
};

const UsbSerialOperations *
usbGetSerialOperations (const UsbDevice *device) {
  typedef struct {
    uint16_t vendor;
    uint16_t product;
    const UsbSerialOperations *operations;
  } UsbSerialDevice;
  static const UsbSerialDevice usbSerialDevices[] = {
    {0X050D, 0X1203, &usbBelkinOperations}, /* Belkin DockStation */
    {0X050D, 0X0103, &usbBelkinOperations}, /* Belkin Serial Adapter */
    {0X056C, 0X8007, &usbBelkinOperations}, /* Belkin Old Single Port Serial Converter */
    {0X0565, 0X0001, &usbBelkinOperations}, /* Peracom Single Port Serial Converter */
    {0X0921, 0X1000, &usbBelkinOperations}, /* GoHubs Single Port Serial Converter */
    {0X0921, 0X1200, &usbBelkinOperations}, /* GoHubs HandyLink */
    {}
  };
  const UsbSerialDevice *serial = usbSerialDevices;
  while (serial->vendor) {
    if (serial->vendor == device->descriptor.idVendor)
      if (!serial->product || (serial->product == device->descriptor.idProduct))
        return serial->operations;
    ++serial;
  }
  return NULL;
}
