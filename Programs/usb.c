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
  return usbControlTransfer(device, USB_DIRECTION_INPUT,
                            USB_RECIPIENT_DEVICE, USB_TYPE_STANDARD,
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
  int size = usbGetDescriptor(device, USB_DESCRIPTOR_TYPE_STRING,
                              0, 0, &descriptor, timeout);
  if (size != -1) {
    if (size >= 4) {
      *language = getLittleEndian(descriptor.string.wData[0]);
      return 1;
    }
    errno = ENODATA;
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
  unsigned char number,
  const char *description
) {
  if (number) {
    char *string = usbGetString(device, number, 1000);
    if (string) {
      LogPrint(LOG_INFO, "USB: %s: %s", description, string);
      free(string);
    }
  }
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
usbGetConfiguration (
  UsbDevice *device,
  unsigned char *number
) {
  int size = usbControlTransfer(device, USB_DIRECTION_INPUT,
                                USB_RECIPIENT_DEVICE, USB_TYPE_STANDARD,
                                USB_REQ_GET_CONFIGURATION, 0, 0,
                                number, sizeof(*number), 1000);
  return size;
}

static int
usbReadConfiguration (UsbDevice *device) {
  if (device->configurationDescriptor) return 1;

  {
    unsigned char configuration;
    if (usbGetConfiguration(device, &configuration) != -1) {
      UsbDescriptor descriptor;
      int size = usbGetDescriptor(device, USB_DESCRIPTOR_TYPE_CONFIGURATION,
                                  configuration, 0, &descriptor, 1000);
      if (size != -1) {
        int length = getLittleEndian(descriptor.configuration.wTotalLength);
        UsbDescriptor *descriptors = malloc(length);
        if (descriptors) {
          size = usbControlTransfer(device, USB_DIRECTION_INPUT,
                                    USB_RECIPIENT_DEVICE, USB_TYPE_STANDARD,
                                    USB_REQ_GET_DESCRIPTOR,
                                    (USB_DESCRIPTOR_TYPE_CONFIGURATION << 8) | configuration,
                                    0, descriptors, length, 1000);
          if (size != -1) {
            device->configurationDescriptor = descriptors;
            device->configurationLength = length;
            return 1;
          }
          free(descriptors);
        }
      }
    }
  }
  return 0;
}

static void
usbDeallocateEndpoint (void *item, void *data) {
  UsbEndpoint *endpoint = item;

  switch (USB_ENDPOINT_DIRECTION(endpoint->descriptor)) {
    case USB_ENDPOINT_DIRECTION_INPUT:
      if (endpoint->direction.input.pending)
        deallocateQueue(endpoint->direction.input.pending);
      if (endpoint->direction.input.completed)
        free(endpoint->direction.input.completed);
      break;
  }

  usbCloseEndpoint(endpoint->system);
  free(endpoint);
}

static int
usbTestEndpoint (void *item, void *data) {
  UsbEndpoint *endpoint = item;
  unsigned char *endpointAddress = data;
  return endpoint->descriptor->bEndpointAddress == *endpointAddress;
}

UsbEndpoint *
usbGetEndpoint (UsbDevice *device, unsigned char endpointAddress) {
  UsbEndpoint *endpoint = findItem(device->endpoints, usbTestEndpoint, &endpointAddress);
  if (endpoint) return endpoint;

  if (usbReadConfiguration(device)) {
    int found = 0;
    int offset = 0;
    while (offset < device->configurationLength) {
      UsbEndpointDescriptor *descriptor = (UsbEndpointDescriptor *)((unsigned char *)device->configurationDescriptor + offset);
      offset += descriptor->bLength;
      if (descriptor->bDescriptorType != USB_DESCRIPTOR_TYPE_ENDPOINT) continue;
      if (descriptor->bEndpointAddress != endpointAddress) continue;
      found = 1;

      {
        const char *direction;
        const char *transfer;

        switch (USB_ENDPOINT_DIRECTION(descriptor)) {
          default:                            direction = "?";   break;
          case USB_ENDPOINT_DIRECTION_INPUT:  direction = "in";  break;
          case USB_ENDPOINT_DIRECTION_OUTPUT: direction = "out"; break;
        }

        switch (USB_ENDPOINT_TRANSFER(descriptor)) {
          default:                                transfer = "?";   break;
          case USB_ENDPOINT_TRANSFER_CONTROL:     transfer = "ctl"; break;
          case USB_ENDPOINT_TRANSFER_ISOCHRONOUS: transfer = "iso"; break;
          case USB_ENDPOINT_TRANSFER_BULK:        transfer = "blk"; break;
          case USB_ENDPOINT_TRANSFER_INTERRUPT:   transfer = "int"; break;
        }

        LogPrint(LOG_DEBUG, "USB: ept=%02X dir=%s xfr=%s pkt=%d ivl=%dms",
                 descriptor->bEndpointAddress, direction, transfer,
                 getLittleEndian(descriptor->wMaxPacketSize),
                 descriptor->bInterval);
      }

      if ((endpoint = malloc(sizeof(*endpoint)))) {
        memset(endpoint, 0, sizeof(*endpoint));
        endpoint->device = device;
        endpoint->descriptor = descriptor;
        endpoint->system = NULL;

        switch (USB_ENDPOINT_DIRECTION(endpoint->descriptor)) {
          case USB_ENDPOINT_DIRECTION_INPUT:
            endpoint->direction.input.pending = NULL;
            endpoint->direction.input.completed = NULL;
            endpoint->direction.input.buffer = NULL;
            endpoint->direction.input.length = 0;
            break;
        }

        if (usbOpenEndpoint(endpoint->descriptor, &endpoint->system)) {
          if (enqueueItem(device->endpoints, endpoint)) return endpoint;
          usbCloseEndpoint(endpoint->system);
        }

        free(endpoint);
      }
      break;
    }
    if (!found) errno = ENOENT;
  }

  LogPrint(LOG_WARNING, "USB: endpoint not found: %02X", endpointAddress);
  return NULL;
}

UsbEndpoint *
usbGetInputEndpoint (UsbDevice *device, unsigned char endpointNumber) {
  return usbGetEndpoint(device, endpointNumber|USB_ENDPOINT_DIRECTION_INPUT);
}

UsbEndpoint *
usbGetOutputEndpoint (UsbDevice *device, unsigned char endpointNumber) {
  return usbGetEndpoint(device, endpointNumber|USB_ENDPOINT_DIRECTION_OUTPUT);
}

void
usbCloseDevice (UsbDevice *device) {
  deallocateQueue(device->endpoints);
  close(device->file);
  if (device->configurationDescriptor) free(device->configurationDescriptor);
  free(device);
}

UsbDevice *
usbOpenDevice (const char *path) {
  UsbDevice *device;
  if ((device = malloc(sizeof(*device)))) {
    memset(device, 0, sizeof(*device));

    if ((device->endpoints = newQueue(usbDeallocateEndpoint, NULL))) {
      if ((device->file = open(path, O_RDWR)) != -1) {
        if (usbReadDeviceDescriptor(device))
          if (device->descriptor.bDescriptorType == USB_DESCRIPTOR_TYPE_DEVICE)
            if (device->descriptor.bLength == USB_DESCRIPTOR_SIZE_DEVICE)
              return device;
        close(device->file);
      }
      deallocateQueue(device->endpoints);
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
isUsbDevice (const char **path) {
  return isQualifiedDevice(path, "usb");
}

static void
usbDeallocatePendingInputRequest (void *item, void *data) {
  void *request = item;
  UsbEndpoint *endpoint = data;
  usbCancelRequest(endpoint->device, request);
}

static Element *
usbAddPendingInputRequest (
  UsbEndpoint *endpoint
) {
  void *request = usbSubmitRequest(endpoint->device,
                                   endpoint->descriptor->bEndpointAddress,
                                   NULL,
                                   getLittleEndian(endpoint->descriptor->wMaxPacketSize),
                                   endpoint);
  if (request) {
    Element *element = enqueueItem(endpoint->direction.input.pending, request);
    if (element) return element;
    usbCancelRequest(endpoint->device, request);
  }
  return NULL;
}

int
usbBeginInput (
  UsbDevice *device,
  unsigned char endpointNumber,
  int count
) {
  int actual = 0;
  UsbEndpoint *endpoint = usbGetInputEndpoint(device, endpointNumber);
  if (endpoint) {
    if ((endpoint->direction.input.pending = newQueue(usbDeallocatePendingInputRequest, NULL))) {
      setQueueData(endpoint->direction.input.pending, endpoint);
      while (actual < count) {
        if (!usbAddPendingInputRequest(endpoint)) break;
        actual++;
      }
    }
  }
  return actual;
}

int
usbAwaitInput (
  UsbDevice *device,
  unsigned char endpointNumber,
  int timeout
) {
  UsbEndpoint *endpoint = usbGetInputEndpoint(device, endpointNumber);
  if (!endpoint) return 0;
  if (!endpoint->direction.input.completed) {
    UsbResponse response;
    while (!(endpoint->direction.input.completed = usbReapResponse(device, &response, 0))) {
      if (errno != EAGAIN) return 0;
      if (timeout <= 0) return 0;

      {
        const int interval = 10;
        delay(interval);
        timeout -= interval;
      }
    }
    usbAddPendingInputRequest(endpoint);
    deleteItem(endpoint->direction.input.pending, endpoint->direction.input.completed);

    endpoint->direction.input.buffer = response.buffer;
    endpoint->direction.input.length = response.length;
  }
  return 1;
}

int
usbReapInput (
  UsbDevice *device,
  unsigned char endpointNumber,
  void *buffer,
  int length,
  int timeout
) {
  UsbEndpoint *endpoint = usbGetInputEndpoint(device, endpointNumber);
  if (endpoint) {
    unsigned char *bytes = buffer;
    unsigned char *target = bytes;
    while (length > 0) {
      if (!usbAwaitInput(device, endpointNumber,
                         (target == bytes)? 0: timeout)) return -1;

      {
        int count = endpoint->direction.input.length;
        if (length < count) count = length;
        memcpy(target, endpoint->direction.input.buffer, count);

        if ((endpoint->direction.input.length -= count)) {
          endpoint->direction.input.buffer += count;
        } else {
          endpoint->direction.input.buffer = NULL;
          free(endpoint->direction.input.completed);
          endpoint->direction.input.completed = NULL;
        }

        target += count;
        length -= count;
      }
    }
    return target - bytes;
  }
  return -1;
}

static int
usbSetBelkinAttribute (UsbDevice *device, unsigned char request, int value) {
  return usbControlTransfer(device, USB_DIRECTION_OUTPUT,
                            USB_RECIPIENT_DEVICE, USB_TYPE_VENDOR, 
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
#define BELKIN_FLOW(from,to) if ((flow & (from)) == (from)) flow &= ~(from), value |= (to)
  BELKIN_FLOW(USB_SERIAL_FLOW_OUTPUT_CTS, 0X0001);
  BELKIN_FLOW(USB_SERIAL_FLOW_OUTPUT_DSR, 0X0002);
  BELKIN_FLOW(USB_SERIAL_FLOW_INPUT_DSR , 0X0004);
  BELKIN_FLOW(USB_SERIAL_FLOW_INPUT_DTR , 0X0008);
  BELKIN_FLOW(USB_SERIAL_FLOW_INPUT_RTS , 0X0010);
  BELKIN_FLOW(USB_SERIAL_FLOW_OUTPUT_RTS, 0X0020);
  BELKIN_FLOW(USB_SERIAL_FLOW_OUTPUT_XON, 0X0080);
  BELKIN_FLOW(USB_SERIAL_FLOW_INPUT_XON , 0X0100);
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
  return usbControlTransfer(device, USB_DIRECTION_OUTPUT,
                            USB_RECIPIENT_DEVICE, USB_TYPE_VENDOR, 
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
#define FTDI_FLOW(from,to) if ((flow & (from)) == (from)) flow &= ~(from), index |= (to)
  FTDI_FLOW(USB_SERIAL_FLOW_OUTPUT_CTS|USB_SERIAL_FLOW_INPUT_RTS, 0X0100);
  FTDI_FLOW(USB_SERIAL_FLOW_OUTPUT_DSR|USB_SERIAL_FLOW_INPUT_DTR, 0X0200);
  FTDI_FLOW(USB_SERIAL_FLOW_OUTPUT_XON|USB_SERIAL_FLOW_INPUT_XON, 0X0400);
#undef FTDI_FLOW
  if (flow) {
    LogPrint(LOG_WARNING, "Unsupported FTDI flow control: %02X", flow);
  }
  return usbSetFtdiAttribute(device, 2, ((index & 0X0400)? 0X1311: 0), index);
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
    {0X0921, 0X1200, &usbBelkinOperations}, /* HandyTech GoHubs */
    {0X0403, 0X6001, &usbFtdiOperations_FT8U232AM}, /* HandyTech FTDI */
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
