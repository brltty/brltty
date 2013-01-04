/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#include "log.h"
#include "device.h"
#include "timing.h"
#include "io_usb.h"
#include "usb_internal.h"

ssize_t
usbControlRead (
  UsbDevice *device,
  uint8_t recipient,
  uint8_t type,
  uint8_t request,
  uint16_t value,
  uint16_t index,
  void *buffer,
  uint16_t length,
  int timeout
) {
  return usbControlTransfer(device, UsbControlDirection_Input, recipient, type,
                            request, value, index, buffer, length, timeout);
}

ssize_t
usbControlWrite (
  UsbDevice *device,
  uint8_t recipient,
  uint8_t type,
  uint8_t request,
  uint16_t value,
  uint16_t index,
  const void *buffer,
  uint16_t length,
  int timeout
) {
  return usbControlTransfer(device, UsbControlDirection_Output, recipient, type,
                            request, value, index, (void *)buffer, length, timeout);
}

ssize_t
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
      *language = getLittleEndian16(descriptor.string.wData[0]);
      logMessage(LOG_DEBUG, "USB Language: %02X", *language);
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
      uint16_t character = getLittleEndian16(descriptor->wData[count]);
      if (character & 0XFF00) character = '?';
      string[count] = character;
    }
  } else {
    logSystemError("USB string allocate");
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

char *
usbGetManufacturer (UsbDevice *device, int timeout) {
  return usbGetString(device, device->descriptor.iManufacturer, timeout);
}

char *
usbGetProduct (UsbDevice *device, int timeout) {
  return usbGetString(device, device->descriptor.iProduct, timeout);
}

char *
usbGetSerialNumber (UsbDevice *device, int timeout) {
  return usbGetString(device, device->descriptor.iSerialNumber, timeout);
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
      logMessage(LOG_INFO, "USB: %s: %s", description, string);
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
  ssize_t size = usbControlRead(device, UsbControlRecipient_Device, UsbControlType_Standard,
                                UsbStandardRequest_GetConfiguration, 0, 0,
                                configuration, sizeof(*configuration), 1000);
  if (size != -1) return 1;
  logMessage(LOG_WARNING, "USB standard request not supported: get configuration");
  return 0;
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
    unsigned char current;

    if (device->descriptor.bNumConfigurations < 2) {
      current = 1;
    } else if (!usbGetConfiguration(device, &current)) {
      current = 0;
    }

    if (current) {
      UsbDescriptor descriptor;
      unsigned char number;

      for (number=0; number<device->descriptor.bNumConfigurations; number++) {
        int size = usbGetDescriptor(device, UsbDescriptorType_Configuration,
                                    number, 0, &descriptor, 1000);
        if (size == -1) {
          logMessage(LOG_WARNING, "USB configuration descriptor not readable: %d", number);
        } else if (descriptor.configuration.bConfigurationValue == current) {
          break;
        }
      }

      if (number < device->descriptor.bNumConfigurations) {
        int length = getLittleEndian16(descriptor.configuration.wTotalLength);
        UsbDescriptor *descriptors;

        if ((descriptors = malloc(length))) {
          ssize_t size;

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
          logSystemError("USB configuration descriptor allocate");
        }
      } else {
        logMessage(LOG_ERR, "USB configuration descriptor not found: %d", current);
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

  if (usbSetConfiguration(device, configuration)) {
    usbDeallocateConfigurationDescriptor(device);
    return 1;
  }

  {
    const UsbConfigurationDescriptor *descriptor = usbConfigurationDescriptor(device);

    if (descriptor)
      if (descriptor->bConfigurationValue == configuration)
        return 1;
  }

  return 0;
}

int
usbNextDescriptor (
  UsbDevice *device,
  const UsbDescriptor **descriptor
) {
  if (*descriptor) {
    const UsbDescriptor *next = (UsbDescriptor *)&(*descriptor)->bytes[(*descriptor)->header.bLength];
    const UsbDescriptor *first = (UsbDescriptor *)device->configuration;
    unsigned int length = getLittleEndian16(first->configuration.wTotalLength);
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

  logMessage(LOG_WARNING, "USB: interface descriptor not found: %d.%d", interface, alternative);
  errno = ENOENT;
  return NULL;
}

unsigned int
usbAlternativeCount (
  UsbDevice *device,
  unsigned char interface
) {
  unsigned int count = 0;
  const UsbDescriptor *descriptor = NULL;

  while (usbNextDescriptor(device, &descriptor)) {
    if (descriptor->interface.bDescriptorType == UsbDescriptorType_Interface)
      if (descriptor->interface.bInterfaceNumber == interface)
        count += 1;
  }

  return count;
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

  logMessage(LOG_WARNING, "USB: endpoint descriptor not found: %02X", endpointAddress);
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
usbTestEndpoint (const void *item, const void *data) {
  const UsbEndpoint *endpoint = item;
  const unsigned char *endpointAddress = data;
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

      logMessage(LOG_DEBUG, "USB: ept=%02X dir=%s xfr=%s pkt=%d ivl=%dms",
                 descriptor->bEndpointAddress, direction, transfer,
                 getLittleEndian16(descriptor->wMaxPacketSize),
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
usbApplyInputFilters (UsbDevice *device, void *buffer, size_t size, ssize_t *length) {
  UsbInputFilterData data = {
    .buffer = buffer,
    .size = size,
    .length = *length
  };

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

  if (usbAlternativeCount(device, interface) == 1) goto done;

  {
    unsigned char response[1];
    ssize_t size = usbControlRead(device, UsbControlRecipient_Interface, UsbControlType_Standard,
                                  UsbStandardRequest_GetInterface, 0, interface,
                                  response, sizeof(response), 1000);

    if (size != -1) {
      if (response[0] == alternative) goto done;
    } else {
      logMessage(LOG_WARNING, "USB standard request not supported: get interface");
    }
  }

  if (usbSetAlternative(device, interface, alternative)) goto done;
  if (!device->interface) usbReleaseInterface(device, interface);
  return 0;

done:
  device->interface = descriptor;
  return 1;
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
    device->serialOperations = NULL;
    device->serialData = NULL;

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

  logSystemError("USB device open");
  return NULL;
}

UsbDevice *
usbTestDevice (UsbDeviceExtension *extension, UsbDeviceChooser chooser, void *data) {
  UsbDevice *device;
  if ((device = usbOpenDevice(extension))) {
    logMessage(LOG_DEBUG, "USB: testing: vendor=%04X product=%04X",
               getLittleEndian16(device->descriptor.idVendor),
               getLittleEndian16(device->descriptor.idProduct));
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
                                   getLittleEndian16(endpoint->descriptor->wMaxPacketSize),
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
    int size = getLittleEndian16(endpoint->descriptor->wMaxPacketSize);
    unsigned char *buffer = malloc(size);

    if (buffer) {
      TimePeriod period;
      if (timeout) startTimePeriod(&period, timeout);

      while (1) {
        ssize_t count = usbReadEndpoint(device, endpointNumber, buffer, size, 20);

        if (count != -1) {
          if (count) {
            endpoint->direction.input.buffer = buffer;
            endpoint->direction.input.length = count;
            endpoint->direction.input.completed = buffer;
            return 1;
          }

          errno = EAGAIN;
        }

#ifdef ETIMEDOUT
        if (errno == ETIMEDOUT) errno = EAGAIN;
#endif /* ETIMEDOUT */

        if (errno != EAGAIN) break;
        if (!timeout) break;
        if (afterTimePeriod(&period, NULL)) break;
        approximateDelay(interval);
      }

      free(buffer);
    } else {
      logMallocError();
    }

    return 0;
  }

  {
    TimePeriod period;

    if (timeout) startTimePeriod(&period, timeout);

    while (1) {
      UsbResponse response;
      void *request;

      while (!(request = usbReapResponse(device,
                                         endpointNumber | UsbEndpointDirection_Input,
                                         &response, 0))) {
        if (errno != EAGAIN) return 0;
        if (!timeout) return 0;
        if (afterTimePeriod(&period, NULL)) return 0;
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
}

ssize_t
usbReapInput (
  UsbDevice *device,
  unsigned char endpointNumber,
  void *buffer,
  size_t length,
  int initialTimeout,
  int subsequentTimeout
) {
  UsbEndpoint *endpoint = usbGetInputEndpoint(device, endpointNumber);

  if (endpoint) {
    unsigned char *bytes = buffer;
    unsigned char *target = bytes;

    while (length > 0) {
      if (!usbAwaitInput(device, endpointNumber,
                         (target == bytes)? initialTimeout: subsequentTimeout)) {
        if (errno == EAGAIN) break;
        return -1;
      }

      {
        size_t count = endpoint->direction.input.length;
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
    if (!definition->version || (definition->version == getLittleEndian16(descriptor->bcdUSB))) {
      if (USB_IS_PRODUCT(descriptor, definition->vendor, definition->product)) {
        if (!(descriptor->iManufacturer ||
              descriptor->iProduct ||
              descriptor->iSerialNumber)) {
          UsbDeviceDescriptor actualDescriptor;
          ssize_t result = usbGetDeviceDescriptor(device, &actualDescriptor);

          if (result == UsbDescriptorSize_Device) {
            logMessage(LOG_DEBUG, "USB: using actual device descriptor");
            device->descriptor = actualDescriptor;
          }
        }

        if (!usbVerifySerialNumber(device, choose->serialNumber)) break;

        if (definition->disableAutosuspend) usbDisableAutosuspend(device);

        if (usbConfigureDevice(device, definition->configuration)) {
          if (usbOpenInterface(device, definition->interface, definition->alternative)) {
            int ok = 1;

            if (ok)
              if (!usbSetSerialOperations(device))
                ok = 0;

            if (ok)
              if (device->serialOperations)
                if (device->serialOperations->enableAdapter)
                  if (!device->serialOperations->enableAdapter(device))
                    ok = 0;

            if (ok)
              if (definition->serial)
                if (!usbSetSerialParameters(device, definition->serial))
                  ok = 0;

            if (ok) {
              if (definition->inputEndpoint) {
                UsbEndpoint *endpoint = usbGetInputEndpoint(device, definition->inputEndpoint);

                if (!endpoint) {
                  ok = 0;
                } else if (USB_ENDPOINT_TRANSFER(endpoint->descriptor) == UsbEndpointTransfer_Interrupt) {
                  usbBeginInput(device, definition->inputEndpoint, 8);
                }
              }
            }

            if (ok) {
              if (definition->outputEndpoint) {
                UsbEndpoint *endpoint = usbGetOutputEndpoint(device, definition->outputEndpoint);

                if (!endpoint) {
                  ok = 0;
                }
              }
            }

            if (ok) {
              choose->definition = definition;
              return 1;
            }
            usbCloseInterface(device);
          }
        }
      }
    }

    definition += 1;
  }
  return 0;
}

UsbChannel *
usbFindChannel (const UsbChannelDefinition *definitions, const char *serialNumber) {
  UsbChooseChannelData choose = {
    .definition = definitions,
    .serialNumber = serialNumber
  };
  UsbDevice *device = usbFindDevice(usbChooseChannel, &choose);

  if (device) {
    UsbChannel *channel = malloc(sizeof(*channel));

    if (channel) {
      memset(channel, 0, sizeof(*channel));
      channel->device = device;
      channel->definition = *choose.definition;
      return channel;
    } else {
      logMallocError();
    }

    usbCloseDevice(device);
  } else {
    logMessage(LOG_DEBUG, "USB device not found%s%s",
               (*serialNumber? ": ": ""), serialNumber);
  }

  return NULL;
}

void
usbCloseChannel (UsbChannel *channel) {
  usbCloseDevice(channel->device);
  free(channel);
}

int
isUsbDevice (const char **identifier) {
  return isQualifiedDevice(identifier, "usb");
}
