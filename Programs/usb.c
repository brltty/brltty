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
usbAwaitInput (
  UsbDevice *device,
  int timeout
) {
  if (!device->inputRequest) {
    UsbResponse response;
    while (!(device->inputRequest = usbReapResponse(device, &response, 0))) {
      if (errno != EAGAIN) return 0;
      if (timeout <= 0) return 0;
      {
        const int interval = 10;
        delay(interval);
        timeout -= interval;
      }
    }
    usbAddInputElement(device);

    {
      struct UsbInputElement *input = response.context;
      input->request = NULL;
      usbDeleteInputElement(device, input);
    }

    device->inputBuffer = response.buffer;
    device->inputLength = response.length;
  }
  return 1;
}

int
usbReapInput (
  UsbDevice *device,
  void *buffer,
  int length,
  int timeout
) {
  unsigned char *bytes = buffer;
  unsigned char *target = bytes;
  while (length > 0) {
    if (!usbAwaitInput(device, timeout)) return -1;

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
  const int base = 230400;
  if (base % rate) {
    LogPrint(LOG_WARNING, "Unsupported Belkin baud: %d", rate);
    errno = EINVAL;
    return -1;
  }
  return usbSetBelkinAttribute(device, 0, base/rate);
}
static int
usbSetBelkinStopBits (UsbDevice *device, int bits) {
  if ((bits < 1) || (bits > 2)) {
    LogPrint(LOG_WARNING, "Unsupported Belkin stop bits: %d", bits);
    errno = EINVAL;
    return -1;
  }
  return usbSetBelkinAttribute(device, 1, bits-1);
}
static int
usbSetBelkinDataBits (UsbDevice *device, int bits) {
  if ((bits < 5) || (bits > 8)) {
    LogPrint(LOG_WARNING, "Unsupported Belkin data bits: %d", bits);
    errno = EINVAL;
    return -1;
  }
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
    case USB_SERIAL_PARITY_NONE:  value = 0; break;
    default:
      LogPrint(LOG_WARNING, "Unsupported Belkin parity: %d", parity);
      errno = EINVAL;
      return -1;
  }
  return usbSetBelkinAttribute(device, 3, value);
}
static int
usbSetBelkinDtrState (UsbDevice *device, int state) {
  if ((state < 0) || (state > 1)) {
    LogPrint(LOG_WARNING, "Unsupported Belkin DTR state: %d", state);
    errno = EINVAL;
    return -1;
  }
  return usbSetBelkinAttribute(device, 10, state);
}
static int
usbSetBelkinRtsState (UsbDevice *device, int state) {
  if ((state < 0) || (state > 1)) {
    LogPrint(LOG_WARNING, "Unsupported Belkin RTS state: %d", state);
    errno = EINVAL;
    return -1;
  }
  return usbSetBelkinAttribute(device, 11, state);
}
static int
usbSetBelkinFlowControl (UsbDevice *device, int flow) {
  int value = 0;
#define BELKIN_FLOW(from,to) if (flow & from) flow &= ~from, value |= to
  BELKIN_FLOW(USB_SERIAL_FLOW_OUTPUT_XON, 0X0080);
  BELKIN_FLOW(USB_SERIAL_FLOW_OUTPUT_CTS, 0X0001);
  BELKIN_FLOW(USB_SERIAL_FLOW_OUTPUT_DSR, 0X0002);
  BELKIN_FLOW(USB_SERIAL_FLOW_OUTPUT_RTS, 0X0020);
  BELKIN_FLOW(USB_SERIAL_FLOW_INPUT_XON , 0X0100);
  BELKIN_FLOW(USB_SERIAL_FLOW_INPUT_RTS , 0X0010);
  BELKIN_FLOW(USB_SERIAL_FLOW_INPUT_DTR , 0X0008);
  BELKIN_FLOW(USB_SERIAL_FLOW_INPUT_DSR , 0X0004);
#undef BELKIN_FLOW
  if (flow) {
    LogPrint(LOG_WARNING, "Unsupported Belkin flow control: %02X", flow);
  }
  return usbSetBelkinAttribute(device, 16, value);
}
static int
usbSetBelkinDataFormat (UsbDevice *device, int dataBits, int stopBits, UsbSerialParity parity) {
  if (usbSetBelkinDataBits(device, dataBits) != -1)
    if (usbSetBelkinStopBits(device, stopBits) != -1)
      if (usbSetBelkinParity(device, parity) != -1)
        return 0;
  return -1;
}
static const UsbSerialOperations usbBelkinOperations = {
  usbSetBelkinBaud,
  usbSetBelkinFlowControl,
  usbSetBelkinDataFormat,
  usbSetBelkinDtrState,
  usbSetBelkinRtsState
};

static int
usbSetFtdiAttribute (UsbDevice *device, unsigned char request, int value, int index) {
  return usbControlTransfer(device,
                            USB_RECIPIENT_DEVICE, USB_DIRECTION_OUTPUT, USB_TYPE_VENDOR, 
                            request, value, index, NULL, 0, 1000);
}
static int
usbSetFtdiModemState (UsbDevice *device, int state, int shift, const char *name) {
  if ((state < 0) || (state > 1)) {
    LogPrint(LOG_WARNING, "Unsupported FTDI %s state: %d", name, state);
    errno = EINVAL;
    return -1;
  }
  return usbSetFtdiAttribute(device, 1, ((1 << (shift + 8)) | (state << shift)), 0);
}
static int
usbSetFtdiDtrState (UsbDevice *device, int state) {
  return usbSetFtdiModemState(device, state, 0, "DTR");
}
static int
usbSetFtdiRtsState (UsbDevice *device, int state) {
  return usbSetFtdiModemState(device, state, 1, "RTS");
}
static int
usbSetFtdiFlowControl (UsbDevice *device, int flow) {
  int index = 0;
#define FTDI_FLOW(from,to) if (flow & from) flow &= ~from, index |= to
  FTDI_FLOW(USB_SERIAL_FLOW_OUTPUT_XON, 0X0080);
  FTDI_FLOW(USB_SERIAL_FLOW_OUTPUT_CTS, 0X0001);
  FTDI_FLOW(USB_SERIAL_FLOW_OUTPUT_DSR, 0X0002);
  FTDI_FLOW(USB_SERIAL_FLOW_OUTPUT_RTS, 0X0020);
  FTDI_FLOW(USB_SERIAL_FLOW_INPUT_XON , 0X0100);
  FTDI_FLOW(USB_SERIAL_FLOW_INPUT_RTS , 0X0010);
  FTDI_FLOW(USB_SERIAL_FLOW_INPUT_DTR , 0X0008);
  FTDI_FLOW(USB_SERIAL_FLOW_INPUT_DSR , 0X0004);
#undef FTDI_FLOW
  if (flow) {
    LogPrint(LOG_WARNING, "Unsupported FTDI flow control: %02X", flow);
  }
  return usbSetFtdiAttribute(device, 2, ((index & 0X200)? 0X1311: 0), index);
}
static int
usbSetFtdiBaud (UsbDevice *device, int divisor) {
  return usbSetFtdiAttribute(device, 3, divisor&0XFFFF, divisor>>0X10);
}
static int
usbSetFtdiBaud_SIO (UsbDevice *device, int rate) {
  int divisor;
  switch (rate) {
    case    300: divisor = 0; break;
    case    600: divisor = 1; break;
    case   1200: divisor = 2; break;
    case   2400: divisor = 3; break;
    case   4800: divisor = 4; break;
    case   9600: divisor = 5; break;
    case  19200: divisor = 6; break;
    case  38400: divisor = 7; break;
    case  57600: divisor = 8; break;
    case 115200: divisor = 9; break;
    default:
      LogPrint(LOG_WARNING, "Unsupported FTDI SIO baud: %d", rate);
      errno = EINVAL;
      return -1;
  }
  return usbSetFtdiBaud(device, divisor);
}
static int
usbSetFtdiBaud_FT8U232AM (UsbDevice *device, int rate) {
  if (rate > 3000000) {
    LogPrint(LOG_WARNING, "Unsupported FTDI FT8U232AM baud: %d", rate);
    errno = EINVAL;
    return -1;
  }
  {
    const int base = 48000000;
    int eighths = base / 2 / rate;
    int divisor;
    if ((eighths & 07) == 7) eighths++;
    divisor = eighths >> 3;
    divisor |= (eighths & 04)? 0X4000:
               (eighths & 02)? 0X8000:
               (eighths & 01)? 0XC000:
                               0X0000;
    if (divisor == 1) divisor = 0;
    return usbSetFtdiBaud(device, divisor);
  }
}
static int
usbSetFtdiBaud_FT232BM (UsbDevice *device, int rate) {
  if (rate > 3000000) {
    LogPrint(LOG_WARNING, "Unsupported FTDI FT232BM baud: %d", rate);
    errno = EINVAL;
    return -1;
  }
  {
    static const unsigned char mask[8] = {00, 03, 02, 04, 01, 05, 06, 07};
    const int base = 48000000;
    const int eighths = base / 2 / rate;
    int divisor = (eighths >> 3) | (mask[eighths & 07] << 14);
    if (divisor == 1) {
      divisor = 0;
    } else if (divisor == 0X4001) {
      divisor = 1;
    }
    return usbSetFtdiBaud(device, divisor);
  }
}
static int
usbSetFtdiDataFormat (UsbDevice *device, int dataBits, int stopBits, UsbSerialParity parity) {
  int ok = 1;
  int value = dataBits & 0XFF;
  if (dataBits != value) {
    LogPrint(LOG_WARNING, "Unsupported FTDI data bits: %d", dataBits);
    ok = 0;
  }
  switch (parity) {
    case USB_SERIAL_PARITY_NONE:  value |= 0X000; break;
    case USB_SERIAL_PARITY_ODD:   value |= 0X100; break;
    case USB_SERIAL_PARITY_EVEN:  value |= 0X200; break;
    case USB_SERIAL_PARITY_MARK:  value |= 0X300; break;
    case USB_SERIAL_PARITY_SPACE: value |= 0X400; break;
    default:
      LogPrint(LOG_WARNING, "Unsupported FTDI parity: %d", parity);
      ok = 0;
      break;
  }
  switch (stopBits) {
    case 1: value |= 0X0000; break;
    case 2: value |= 0X1000; break;
    default:
      LogPrint(LOG_WARNING, "Unsupported FTDI stop bits: %d", stopBits);
      ok = 0;
      break;
  }
  if (!ok) {
    errno = EINVAL;
    return -1;
  }
  return usbSetFtdiAttribute(device, 4, value, 0);
}
static const UsbSerialOperations usbFtdiOperations_SIO = {
  usbSetFtdiBaud_SIO,
  usbSetFtdiFlowControl,
  usbSetFtdiDataFormat,
  usbSetFtdiDtrState,
  usbSetFtdiRtsState
};
static const UsbSerialOperations usbFtdiOperations_FT8U232AM = {
  usbSetFtdiBaud_FT8U232AM,
  usbSetFtdiFlowControl,
  usbSetFtdiDataFormat,
  usbSetFtdiDtrState,
  usbSetFtdiRtsState
};
static const UsbSerialOperations usbFtdiOperations_FT232BM = {
  usbSetFtdiBaud_FT232BM,
  usbSetFtdiFlowControl,
  usbSetFtdiDataFormat,
  usbSetFtdiDtrState,
  usbSetFtdiRtsState
};

const UsbSerialOperations *
usbGetSerialOperations (const UsbDevice *device) {
  typedef struct {
    uint16_t vendor;
    uint16_t product;
    const UsbSerialOperations *operations;
  } UsbSerialAdapter;
  static const UsbSerialAdapter usbSerialAdapters[] = {
    {0X0921, 0X1200, &usbBelkinOperations}, /* GoHubs HandyLink */
    {}
  };
  const UsbSerialAdapter *sa = usbSerialAdapters;
  while (sa->vendor) {
    if (sa->vendor == device->descriptor.idVendor)
      if (!sa->product || (sa->product == device->descriptor.idProduct))
        return sa->operations;
    ++sa;
  }
  return NULL;
}
