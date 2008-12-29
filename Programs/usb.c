/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef HAVE_REGEX_H
#include <regex.h>
#endif /* HAVE_REGEX_H */

#include "misc.h"
#include "io_usb.h"
#include "usb_internal.h"

int
usbControlRead (
  UsbDevice *device,
  unsigned char recipient,
  unsigned char type,
  unsigned char request,
  unsigned short value,
  unsigned short index,
  void *buffer,
  int length,
  int timeout
) {
  return usbControlTransfer(device, UsbControlDirection_Input, recipient, type,
                            request, value, index, buffer, length, timeout);
}

int
usbControlWrite (
  UsbDevice *device,
  unsigned char recipient,
  unsigned char type,
  unsigned char request,
  unsigned short value,
  unsigned short index,
  const void *buffer,
  int length,
  int timeout
) {
  return usbControlTransfer(device, UsbControlDirection_Output, recipient, type,
                            request, value, index, (void *)buffer, length, timeout);
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
  return usbControlRead(device, UsbControlRecipient_Device, UsbControlType_Standard,
                        UsbStandardRequest_GetDescriptor, (type << 8) | number, index,
                        descriptor->bytes, sizeof(descriptor->bytes), timeout);
}

int
usbGetDeviceDescriptor (
  UsbDevice *device,
  UsbDeviceDescriptor *descriptor
) {
  UsbDescriptor desc;
  int size = usbGetDescriptor(device, UsbDescriptorType_Device, 0, 0, &desc, 1000);

  if (size != -1) {
    *descriptor = desc.device;
  }

  return size;
}

int
usbGetLanguage (
  UsbDevice *device,
  uint16_t *language,
  int timeout
) {
  UsbDescriptor descriptor;
  int size = usbGetDescriptor(device, UsbDescriptorType_String,
                              0, 0, &descriptor, timeout);
  if (size != -1) {
    if (size >= 4) {
      *language = getLittleEndian(descriptor.string.wData[0]);
      LogPrint(LOG_DEBUG, "USB Language: %02X", *language);
      return 1;
    }
    errno = EIO;
  }
  return 0;
}

char *
usbDecodeString (const UsbStringDescriptor *descriptor) {
  int count = (descriptor->bLength - 2) / sizeof(descriptor->wData[0]);
  char *string = malloc(count+1);
  if (string) {
    string[count] = 0;
    while (count--) {
      uint16_t character = getLittleEndian(descriptor->wData[count]);
      if (character & 0XFF00) character = '?';
      string[count] = character;
    }
  } else {
    LogError("USB string allocate");
  }
  return string;
}

char *
usbGetString (
  UsbDevice *device,
  unsigned char number,
  int timeout
) {
  UsbDescriptor descriptor;

  if (!device->language)
    if (!usbGetLanguage(device, &device->language, timeout))
      return NULL;

  if (usbGetDescriptor(device, UsbDescriptorType_String,
                       number, device->language,
                       &descriptor, timeout) == -1)
    return NULL;

  return usbDecodeString(&descriptor.string);
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

#ifdef HAVE_REGEX_H
  regex_t expression;
  if (regcomp(&expression, value, REG_EXTENDED|REG_NOSUB) == 0) {
    if (regexec(&expression, reference, 0, NULL, 0) == 0) {
      ok = 1;
    }
    regfree(&expression);
  }
#endif /* HAVE_REGEX_H */

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

const UsbDeviceDescriptor *
usbDeviceDescriptor (UsbDevice *device) {
  return &device->descriptor;
}

int
usbGetConfiguration (
  UsbDevice *device,
  unsigned char *configuration
) {
  int size = usbControlRead(device, UsbControlRecipient_Device, UsbControlType_Standard,
                            UsbStandardRequest_GetConfiguration, 0, 0,
                            configuration, sizeof(*configuration), 1000);
  return size;
}

static void
usbDeallocateConfigurationDescriptor (UsbDevice *device) {
  if (device->configuration) {
    free(device->configuration);
    device->configuration = NULL;
  }
}

const UsbConfigurationDescriptor *
usbConfigurationDescriptor (
  UsbDevice *device
) {
  if (!device->configuration) {
    unsigned char configuration;

    if (usbGetConfiguration(device, &configuration) != -1) {
      UsbDescriptor descriptor;
      unsigned char number;

      for (number=0; number<device->descriptor.bNumConfigurations; number++) {
        int size = usbGetDescriptor(device, UsbDescriptorType_Configuration,
                                    number, 0, &descriptor, 1000);
        if (size == -1) {
          LogPrint(LOG_WARNING, "USB configuration descriptor not readable: %d", number);
        } else if (descriptor.configuration.bConfigurationValue == configuration) {
          break;
        }
      }

      if (number < device->descriptor.bNumConfigurations) {
        int length = getLittleEndian(descriptor.configuration.wTotalLength);
        UsbDescriptor *descriptors;

        if ((descriptors = malloc(length))) {
          int size;

          if (length > sizeof(descriptor)) {
            size = usbControlRead(device, UsbControlRecipient_Device, UsbControlType_Standard,
                                  UsbStandardRequest_GetDescriptor,
                                  (UsbDescriptorType_Configuration << 8) | number,
                                  0, descriptors, length, 1000);
          } else {
            memcpy(descriptors, &descriptor, (size = length));
          }

          if (size != -1) {
            device->configuration = &descriptors->configuration;
          } else {
            free(descriptors);
          }
        } else {
          LogError("USB configuration descriptor allocate");
        }
      } else {
        LogPrint(LOG_ERR, "USB configuration descriptor not found: %d", configuration);
      }
    }
  }

  return device->configuration;
}

int
usbConfigureDevice (
  UsbDevice *device,
  unsigned char configuration
) {
  usbCloseInterface(device);
  if (!usbSetConfiguration(device, configuration)) return 0;
  usbDeallocateConfigurationDescriptor(device);
  return 1;
}

int
usbNextDescriptor (
  UsbDevice *device,
  const UsbDescriptor **descriptor
) {
  if (*descriptor) {
    const UsbDescriptor *next = (UsbDescriptor *)&(*descriptor)->bytes[(*descriptor)->header.bLength];
    const UsbDescriptor *first = (UsbDescriptor *)device->configuration;
    unsigned int length = getLittleEndian(first->configuration.wTotalLength);
    if ((&next->bytes[0] - &first->bytes[0]) >= length) return 0;
    if ((&next->bytes[next->header.bLength] - &first->bytes[0]) > length) return 0;
    *descriptor = next;
  } else if (usbConfigurationDescriptor(device)) {
    *descriptor = (UsbDescriptor *)device->configuration;
  } else {
    return 0;
  }
  return 1;
}

const UsbInterfaceDescriptor *
usbInterfaceDescriptor (
  UsbDevice *device,
  unsigned char interface,
  unsigned char alternative
) {
  const UsbDescriptor *descriptor = NULL;

  while (usbNextDescriptor(device, &descriptor)) {
    if (descriptor->interface.bDescriptorType == UsbDescriptorType_Interface)
      if (descriptor->interface.bInterfaceNumber == interface)
        if (descriptor->interface.bAlternateSetting == alternative)
          return &descriptor->interface;
  }

  LogPrint(LOG_WARNING, "USB: interface descriptor not found: %d.%d", interface, alternative);
  errno = ENOENT;
  return NULL;
}

const UsbEndpointDescriptor *
usbEndpointDescriptor (
  UsbDevice *device,
  unsigned char endpointAddress
) {
  const UsbDescriptor *descriptor = NULL;

  while (usbNextDescriptor(device, &descriptor)) {
    if (descriptor->endpoint.bDescriptorType == UsbDescriptorType_Endpoint)
      if (descriptor->endpoint.bEndpointAddress == endpointAddress)
        return &descriptor->endpoint;
  }

  LogPrint(LOG_WARNING, "USB: endpoint descriptor not found: %02X", endpointAddress);
  errno = ENOENT;
  return NULL;
}

static void
usbDeallocateEndpoint (void *item, void *data) {
  UsbEndpoint *endpoint = item;

  switch (USB_ENDPOINT_DIRECTION(endpoint->descriptor)) {
    case UsbEndpointDirection_Input:
      if (endpoint->direction.input.pending) {
        deallocateQueue(endpoint->direction.input.pending);
        endpoint->direction.input.pending = NULL;
      }

      if (endpoint->direction.input.completed) {
        free(endpoint->direction.input.completed);
        endpoint->direction.input.completed = NULL;
      }

      break;
  }

  if (endpoint->extension) {
    usbDeallocateEndpointExtension(endpoint->extension);
    endpoint->extension = NULL;
  }

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
  UsbEndpoint *endpoint;
  const UsbEndpointDescriptor *descriptor;

  if ((endpoint = findItem(device->endpoints, usbTestEndpoint, &endpointAddress))) return endpoint;

  if ((descriptor = usbEndpointDescriptor(device, endpointAddress))) {
    {
      const char *direction;
      const char *transfer;

      switch (USB_ENDPOINT_DIRECTION(descriptor)) {
        default:                            direction = "?";   break;
        case UsbEndpointDirection_Input:  direction = "in";  break;
        case UsbEndpointDirection_Output: direction = "out"; break;
      }

      switch (USB_ENDPOINT_TRANSFER(descriptor)) {
        default:                                transfer = "?";   break;
        case UsbEndpointTransfer_Control:     transfer = "ctl"; break;
        case UsbEndpointTransfer_Isochronous: transfer = "iso"; break;
        case UsbEndpointTransfer_Bulk:        transfer = "blk"; break;
        case UsbEndpointTransfer_Interrupt:   transfer = "int"; break;
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

      switch (USB_ENDPOINT_DIRECTION(endpoint->descriptor)) {
        case UsbEndpointDirection_Input:
          endpoint->direction.input.pending = NULL;
          endpoint->direction.input.completed = NULL;
          endpoint->direction.input.buffer = NULL;
          endpoint->direction.input.length = 0;
          break;
      }

      endpoint->extension = NULL;
      if (usbAllocateEndpointExtension(endpoint)) {
        if (enqueueItem(device->endpoints, endpoint)) return endpoint;
        usbDeallocateEndpointExtension(endpoint->extension);
      }

      free(endpoint);
    }
  }

  return NULL;
}

UsbEndpoint *
usbGetInputEndpoint (UsbDevice *device, unsigned char endpointNumber) {
  return usbGetEndpoint(device, endpointNumber|UsbEndpointDirection_Input);
}

UsbEndpoint *
usbGetOutputEndpoint (UsbDevice *device, unsigned char endpointNumber) {
  return usbGetEndpoint(device, endpointNumber|UsbEndpointDirection_Output);
}

static void
usbDeallocateInputFilter (void *item, void *data) {
  UsbInputFilterEntry *entry = item;
  free(entry);
}

int
usbAddInputFilter (UsbDevice *device, UsbInputFilter filter) {
  UsbInputFilterEntry *entry;
  if ((entry = malloc(sizeof(*entry)))) {
    entry->filter = filter;
    if (enqueueItem(device->inputFilters, entry)) return 1;
    free(entry);
  }
  return 0;
}

static int
usbApplyInputFilter (void *item, void *data) {
  UsbInputFilterEntry *entry = item;
  return !entry->filter(data);
}

int
usbApplyInputFilters (UsbDevice *device, void *buffer, int size, int *length) {
  UsbInputFilterData data = {buffer, size, *length};
  if (processQueue(device->inputFilters, usbApplyInputFilter, &data)) {
    errno = EIO;
    return 0;
  }
  *length = data.length;
  return 1;
}

void
usbCloseInterface (
  UsbDevice *device
) {
  if (device->endpoints) deleteElements(device->endpoints);

  if (device->interface) {
    usbReleaseInterface(device, device->interface->bInterfaceNumber);
    device->interface = NULL;
  }
}

int
usbOpenInterface (
  UsbDevice *device,
  unsigned char interface,
  unsigned char alternative
) {
  const UsbInterfaceDescriptor *descriptor = usbInterfaceDescriptor(device, interface, alternative);
  if (!descriptor) return 0;
  if (descriptor == device->interface) return 1;

  if (device->interface)
    if (device->interface->bInterfaceNumber != interface)
      usbCloseInterface(device);

  if (!device->interface)
    if (!usbClaimInterface(device, interface))
      return 0;

  {
    unsigned char bytes[1];
    int size = usbControlRead(device, UsbControlRecipient_Interface, UsbControlType_Standard,
                              UsbStandardRequest_GetInterface, 0, interface,
                              bytes, sizeof(bytes), 1000);

    if ((size != -1) && (bytes[0] == alternative)) {
      device->interface = descriptor;
      return 1;
    }
  }

  if (usbSetAlternative(device, interface, alternative)) {
    device->interface = descriptor;
    return 1;
  }

  if (!device->interface) usbReleaseInterface(device, interface);
  return 0;
}

void
usbCloseDevice (UsbDevice *device) {
  usbCloseInterface(device);

  if (device->endpoints) {
    deallocateQueue(device->endpoints);
    device->endpoints = NULL;
  }

  if (device->inputFilters) {
    deallocateQueue(device->inputFilters);
    device->inputFilters = NULL;
  }

  if (device->extension) {
    usbDeallocateDeviceExtension(device->extension);
    device->extension = NULL;
  }

  usbDeallocateConfigurationDescriptor(device);
  free(device);
}

static UsbDevice *
usbOpenDevice (UsbDeviceExtension *extension) {
  UsbDevice *device;
  if ((device = malloc(sizeof(*device)))) {
    memset(device, 0, sizeof(*device));
    device->extension = extension;
    device->serial = NULL;

    if ((device->endpoints = newQueue(usbDeallocateEndpoint, NULL))) {
      if ((device->inputFilters = newQueue(usbDeallocateInputFilter, NULL))) {
        if (usbReadDeviceDescriptor(device))
          if (device->descriptor.bDescriptorType == UsbDescriptorType_Device)
            if (device->descriptor.bLength == UsbDescriptorSize_Device)
              return device;
        deallocateQueue(device->inputFilters);
      }
      deallocateQueue(device->endpoints);
    }
    free(device);
  }

  LogError("USB device open");
  return NULL;
}

UsbDevice *
usbTestDevice (UsbDeviceExtension *extension, UsbDeviceChooser chooser, void *data) {
  UsbDevice *device;
  if ((device = usbOpenDevice(extension))) {
    LogPrint(LOG_DEBUG, "USB: testing: vendor=%04X product=%04X",
             device->descriptor.idVendor,
             device->descriptor.idProduct);
    if (chooser(device, data)) {
      usbLogString(device, device->descriptor.iManufacturer, "Manufacturer Name");
      usbLogString(device, device->descriptor.iProduct, "Product Description");
      usbLogString(device, device->descriptor.iSerialNumber, "Serial Number");
      return device;
    }

    errno = ENOENT;
    device->extension = NULL;
    usbCloseDevice(device);
  }
  return NULL;
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
    if (!endpoint->direction.input.pending) {
      if ((endpoint->direction.input.pending = newQueue(usbDeallocatePendingInputRequest, NULL))) {
        setQueueData(endpoint->direction.input.pending, endpoint);
      }
    }

    if (endpoint->direction.input.pending) {
      while ((actual = getQueueSize(endpoint->direction.input.pending)) < count)
        if (!usbAddPendingInputRequest(endpoint))
          break;
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
  UsbEndpoint *endpoint;
  int interval;

  if (!(endpoint = usbGetInputEndpoint(device, endpointNumber))) return 0;
  if (endpoint->direction.input.completed) return 1;

  interval = endpoint->descriptor->bInterval;
  interval = MAX(20, interval);

  if (!(endpoint->direction.input.pending && getQueueSize(endpoint->direction.input.pending))) {
    int size = getLittleEndian(endpoint->descriptor->wMaxPacketSize);
    unsigned char *buffer = malloc(size);
    if (buffer) {
      int count;

      if (timeout) hasTimedOut(0);
      while (1) {
        while ((count = usbReadEndpoint(device, endpointNumber, buffer, size,
                                        MAX(interval, timeout))) != -1) {
          if (!count) {
            errno = EAGAIN;
            break;
          }

          endpoint->direction.input.buffer = buffer;
          endpoint->direction.input.length = count;
          endpoint->direction.input.completed = buffer;
          return 1;
        }
#ifdef ETIMEDOUT
        if (errno == ETIMEDOUT) errno = EAGAIN;
#endif /* ETIMEDOUT */

        if (errno != EAGAIN) break;
        if (!timeout) break;
        if (hasTimedOut(timeout)) break;
        approximateDelay(interval);
      }

      free(buffer);
    }
    return 0;
  }

  if (timeout) hasTimedOut(0);
  while (1) {
    UsbResponse response;
    void *request;

    while (!(request = usbReapResponse(device,
                                       endpointNumber | UsbEndpointDirection_Input,
                                       &response, 0))) {
      if (errno != EAGAIN) return 0;
      if (!timeout) return 0;
      if (hasTimedOut(timeout)) return 0;
      approximateDelay(interval);
    }

    usbAddPendingInputRequest(endpoint);
    deleteItem(endpoint->direction.input.pending, request);

    if (response.count > 0) {
      endpoint->direction.input.buffer = response.buffer;
      endpoint->direction.input.length = response.count;
      endpoint->direction.input.completed = request;
      return 1;
    }

    free(request);
  }
}

int
usbReapInput (
  UsbDevice *device,
  unsigned char endpointNumber,
  void *buffer,
  int length,
  int initialTimeout,
  int subsequentTimeout
) {
  UsbEndpoint *endpoint = usbGetInputEndpoint(device, endpointNumber);
  if (endpoint) {
    unsigned char *bytes = buffer;
    unsigned char *target = bytes;
    while (length > 0) {
      if (!usbAwaitInput(device, endpointNumber,
                         (target == bytes)? initialTimeout: subsequentTimeout)) return -1;

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

int
usbHidSetReport (
  UsbDevice *device,
  unsigned char interface,
  unsigned char report,
  const void *buffer,
  int length,
  int timeout
) {
  return usbControlWrite(device,
                         UsbControlRecipient_Interface, UsbControlType_Class,
                         0X09, (0X02 << 8) | report, interface,
                         buffer, length, timeout);
}

static int
usbSetBelkinAttribute (UsbDevice *device, unsigned char request, int value) {
  return usbControlWrite(device, UsbControlRecipient_Device, UsbControlType_Vendor,
                         request, value, 0, NULL, 0, 1000) != -1;
}
static int
usbSetBelkinBaud (UsbDevice *device, int rate) {
  const int base = 230400;
  if (base % rate) {
    LogPrint(LOG_WARNING, "Unsupported Belkin baud: %d", rate);
    errno = EINVAL;
    return 0;
  }
  return usbSetBelkinAttribute(device, 0, base/rate);
}
static int
usbSetBelkinStopBits (UsbDevice *device, int bits) {
  if ((bits < 1) || (bits > 2)) {
    LogPrint(LOG_WARNING, "Unsupported Belkin stop bits: %d", bits);
    errno = EINVAL;
    return 0;
  }
  return usbSetBelkinAttribute(device, 1, bits-1);
}
static int
usbSetBelkinDataBits (UsbDevice *device, int bits) {
  if ((bits < 5) || (bits > 8)) {
    LogPrint(LOG_WARNING, "Unsupported Belkin data bits: %d", bits);
    errno = EINVAL;
    return 0;
  }
  return usbSetBelkinAttribute(device, 2, bits-5);
}
static int
usbSetBelkinParity (UsbDevice *device, SerialParity parity) {
  int value;
  switch (parity) {
    case SERIAL_PARITY_SPACE: value = 4; break;
    case SERIAL_PARITY_ODD:   value = 2; break;
    case SERIAL_PARITY_EVEN:  value = 1; break;
    case SERIAL_PARITY_MARK:  value = 3; break;
    case SERIAL_PARITY_NONE:  value = 0; break;
    default:
      LogPrint(LOG_WARNING, "Unsupported Belkin parity: %d", parity);
      errno = EINVAL;
      return 0;
  }
  return usbSetBelkinAttribute(device, 3, value);
}
static int
usbSetBelkinDtrState (UsbDevice *device, int state) {
  if ((state < 0) || (state > 1)) {
    LogPrint(LOG_WARNING, "Unsupported Belkin DTR state: %d", state);
    errno = EINVAL;
    return 0;
  }
  return usbSetBelkinAttribute(device, 10, state);
}
static int
usbSetBelkinRtsState (UsbDevice *device, int state) {
  if ((state < 0) || (state > 1)) {
    LogPrint(LOG_WARNING, "Unsupported Belkin RTS state: %d", state);
    errno = EINVAL;
    return 0;
  }
  return usbSetBelkinAttribute(device, 11, state);
}
static int
usbSetBelkinFlowControl (UsbDevice *device, SerialFlowControl flow) {
  int value = 0;
#define BELKIN_FLOW(from,to) if ((flow & (from)) == (from)) flow &= ~(from), value |= (to)
  BELKIN_FLOW(SERIAL_FLOW_OUTPUT_CTS, 0X0001);
  BELKIN_FLOW(SERIAL_FLOW_OUTPUT_DSR, 0X0002);
  BELKIN_FLOW(SERIAL_FLOW_INPUT_DSR , 0X0004);
  BELKIN_FLOW(SERIAL_FLOW_INPUT_DTR , 0X0008);
  BELKIN_FLOW(SERIAL_FLOW_INPUT_RTS , 0X0010);
  BELKIN_FLOW(SERIAL_FLOW_OUTPUT_RTS, 0X0020);
  BELKIN_FLOW(SERIAL_FLOW_OUTPUT_XON, 0X0080);
  BELKIN_FLOW(SERIAL_FLOW_INPUT_XON , 0X0100);
#undef BELKIN_FLOW
  if (flow) {
    LogPrint(LOG_WARNING, "Unsupported Belkin flow control: %02X", flow);
  }
  return usbSetBelkinAttribute(device, 16, value);
}
static int
usbSetBelkinDataFormat (UsbDevice *device, int dataBits, int stopBits, SerialParity parity) {
  if (usbSetBelkinDataBits(device, dataBits))
    if (usbSetBelkinStopBits(device, stopBits))
      if (usbSetBelkinParity(device, parity))
        return 1;
  return 0;
}
static const UsbSerialOperations usbBelkinOperations = {
  usbSetBelkinBaud,
  usbSetBelkinFlowControl,
  usbSetBelkinDataFormat,
  usbSetBelkinDtrState,
  usbSetBelkinRtsState
};

static int
usbFtdiInputFilter (UsbInputFilterData *data) {
  const int count = 2;
  if (data->length > count) {
    unsigned char *buffer = data->buffer;
    memmove(buffer, buffer+count, data->length-=count);
  } else {
    data->length = 0;
  }
  return 1;
}
static int
usbSetFtdiAttribute (UsbDevice *device, unsigned char request, int value, int index) {
  return usbControlWrite(device, UsbControlRecipient_Device, UsbControlType_Vendor,
                         request, value, index, NULL, 0, 1000) != -1;
}
static int
usbSetFtdiModemState (UsbDevice *device, int state, int shift, const char *name) {
  if ((state < 0) || (state > 1)) {
    LogPrint(LOG_WARNING, "Unsupported FTDI %s state: %d", name, state);
    errno = EINVAL;
    return 0;
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
usbSetFtdiFlowControl (UsbDevice *device, SerialFlowControl flow) {
  int index = 0;
#define FTDI_FLOW(from,to) if ((flow & (from)) == (from)) flow &= ~(from), index |= (to)
  FTDI_FLOW(SERIAL_FLOW_OUTPUT_CTS|SERIAL_FLOW_INPUT_RTS, 0X0100);
  FTDI_FLOW(SERIAL_FLOW_OUTPUT_DSR|SERIAL_FLOW_INPUT_DTR, 0X0200);
  FTDI_FLOW(SERIAL_FLOW_OUTPUT_XON|SERIAL_FLOW_INPUT_XON, 0X0400);
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
      return 0;
  }
  return usbSetFtdiBaud(device, divisor);
}
static int
usbSetFtdiBaud_FT8U232AM (UsbDevice *device, int rate) {
  if (rate > 3000000) {
    LogPrint(LOG_WARNING, "Unsupported FTDI FT8U232AM baud: %d", rate);
    errno = EINVAL;
    return 0;
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
    return 0;
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
usbSetFtdiDataFormat (UsbDevice *device, int dataBits, int stopBits, SerialParity parity) {
  int ok = 1;
  int value = dataBits & 0XFF;
  if (dataBits != value) {
    LogPrint(LOG_WARNING, "Unsupported FTDI data bits: %d", dataBits);
    ok = 0;
  }
  switch (parity) {
    case SERIAL_PARITY_NONE:  value |= 0X000; break;
    case SERIAL_PARITY_ODD:   value |= 0X100; break;
    case SERIAL_PARITY_EVEN:  value |= 0X200; break;
    case SERIAL_PARITY_MARK:  value |= 0X300; break;
    case SERIAL_PARITY_SPACE: value |= 0X400; break;
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
    return 0;
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
usbGetSerialOperations (UsbDevice *device) {
  if (!device->serial) {
    typedef struct {
      uint16_t vendor;
      uint16_t product;
      const UsbSerialOperations *operations;
      UsbInputFilter inputFilter;
    } UsbSerialAdapter;

    static const UsbSerialAdapter usbSerialAdapters[] = {
      { /* HandyTech GoHubs */
        0X0921, 0X1200,
        &usbBelkinOperations,
        NULL
      }
      ,
      { /* HandyTech FTDI */
        0X0403, 0X6001,
        &usbFtdiOperations_FT8U232AM,
        usbFtdiInputFilter
      }
      ,
      { /* Papenmeier FTDI */
        0X0403, 0XF208,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum Vario40 (40 cells) */
        0X0403, 0XFE70,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum PocketVario (24 cells) */
        0X0403, 0XFE71,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum SuperVario 40 (40 cells) */
        0X0403, 0XFE72,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum SuperVario 32 (32 cells) */
        0X0403, 0XFE73,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum SuperVario 64 (64 cells) */
        0X0403, 0XFE74,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum SuperVario 80 (80 cells) */
        0X0403, 0XFE75,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum VarioPro 80 (80 cells) */
        0X0403, 0XFE76,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum VarioPro 64 (64 cells) */
        0X0403, 0XFE77,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum VarioPro 40 (40 cells) */
        0X0904, 0X2000,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum EcoVario 24 (24 cells) */
        0X0904, 0X2001,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum EcoVario 40 (40 cells) */
        0X0904, 0X2002,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum VarioConnect 40 (40 cells) */
        0X0904, 0X2007,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum VarioConnect 32 (32 cells) */
        0X0904, 0X2008,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum VarioConnect 24 (24 cells) */
        0X0904, 0X2009,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum VarioConnect 64 (64 cells) */
        0X0904, 0X2010,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum VarioConnect 80 (80 cells) */
        0X0904, 0X2011,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum VarioPro 40 (40 cells) */
        0X0904, 0X2012,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum EcoVario 32 (32 cells) */
        0X0904, 0X2014,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum EcoVario 64 (64 cells) */
        0X0904, 0X2015,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      { /* Baum EcoVario 80 (80 cells) */
        0X0904, 0X2016,
        &usbFtdiOperations_FT232BM,
        usbFtdiInputFilter
      }
      ,
      {0, 0}
    };
    const UsbSerialAdapter *sa = usbSerialAdapters;

    while (sa->vendor) {
      if (sa->vendor == device->descriptor.idVendor) {
        if (!sa->product || (sa->product == device->descriptor.idProduct)) {
          if (sa->inputFilter && !usbAddInputFilter(device, sa->inputFilter)) goto error;
          device->serial = sa->operations;
          break;
        }
      }

      ++sa;
    }

    if (!device->serial)
      LogPrint(LOG_WARNING, "USB: unsupported serial adapter: vendor=%04X product=%04X",
               device->descriptor.idVendor, device->descriptor.idProduct);
  }

error:
  return device->serial;
}

int
usbSetSerialParameters (UsbDevice *device, const SerialParameters *parameters) {
  const UsbSerialOperations *serial = usbGetSerialOperations(device);
  if (!serial) return 0;
  if (!serial->setBaud(device, parameters->baud)) return 0;
  if (!serial->setFlowControl(device, parameters->flow)) return 0;
  if (!serial->setDataFormat(device, parameters->data, parameters->stop, parameters->parity)) return 0;
  return 1;
}

typedef struct {
  const UsbChannelDefinition *definition;
  const char *serialNumber;
} UsbChooseChannelData;

static int
usbChooseChannel (UsbDevice *device, void *data) {
  const UsbDeviceDescriptor *descriptor = usbDeviceDescriptor(device);
  UsbChooseChannelData *choose = data;
  const UsbChannelDefinition *definition = choose->definition;

  while (definition->vendor) {
    if (USB_IS_PRODUCT(descriptor, definition->vendor, definition->product)) {
      if (!usbVerifySerialNumber(device, choose->serialNumber)) break;

      if (definition->disableAutosuspend) usbDisableAutosuspend(device);

      if (usbConfigureDevice(device, definition->configuration)) {
        if (usbOpenInterface(device, definition->interface, definition->alternative)) {
          int ok = 1;

          if (definition->serial)
            if (!usbSetSerialParameters(device, definition->serial))
              ok = 0;

          if (ok) {
            choose->definition = definition;
            return 1;
          }
          usbCloseInterface(device);
        }
      }
    }

    ++definition;
  }
  return 0;
}

UsbChannel *
usbFindChannel (const UsbChannelDefinition *definitions, const char *device) {
  UsbChooseChannelData choose;
  UsbChannel *channel;

  choose.definition = definitions;
  choose.serialNumber = device;

  if ((channel = malloc(sizeof(*channel)))) {
    memset(channel, 0, sizeof(*channel));

    if ((channel->device = usbFindDevice(usbChooseChannel, &choose))) {
      channel->definition = *choose.definition;
      return channel;
    } else {
      LogPrint(LOG_DEBUG, "USB device not found%s%s",
               (*device? ": ": ""),
               device);
    }

    free(channel);
  }
  return NULL;
}

void
usbCloseChannel (UsbChannel *channel) {
  usbCloseDevice(channel->device);
  free(channel);
}

int
isUsbDevice (const char **path) {
  return isQualifiedDevice(path, "usb");
}
