/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

#include <string.h>
#include <errno.h>

#include "log.h"
#include "io_usb.h"
#include "usb_internal.h"
#include "usb_serial.h"

int
usbSkipInitialBytes (UsbInputFilterData *data, unsigned int count) {
  if (data->length > count) {
    unsigned char *buffer = data->buffer;
    memmove(buffer, buffer+count, data->length-=count);
  } else {
    data->length = 0;
  }

  return 1;
}

static const UsbSerialAdapter usbSerialAdapterTable[] = {
  { /* Albatross, Cebra, HIMS, HandyTech FTDI */
    .vendor=0X0403, .product=0X6001,
    .generic = 1,
    .operations = &usbSerialOperations_FTDI_FT8U232AM
  },

  { /* Papenmeier FTDI */
    .vendor=0X0403, .product=0XF208,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum Vario40 (40 cells) */
    .vendor=0X0403, .product=0XFE70,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum PocketVario (24 cells) */
    .vendor=0X0403, .product=0XFE71,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum SuperVario 40 (40 cells) */
    .vendor=0X0403, .product=0XFE72,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum SuperVario 32 (32 cells) */
    .vendor=0X0403, .product=0XFE73,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum SuperVario 64 (64 cells) */
    .vendor=0X0403, .product=0XFE74,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum SuperVario 80 (80 cells) */
    .vendor=0X0403, .product=0XFE75,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum VarioPro 80 (80 cells) */
    .vendor=0X0403, .product=0XFE76,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum VarioPro 64 (64 cells) */
    .vendor=0X0403, .product=0XFE77,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum VarioPro 40 (40 cells) */
    .vendor=0X0904, .product=0X2000,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum EcoVario 24 (24 cells) */
    .vendor=0X0904, .product=0X2001,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum EcoVario 40 (40 cells) */
    .vendor=0X0904, .product=0X2002,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum VarioConnect 40 (40 cells) */
    .vendor=0X0904, .product=0X2007,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum VarioConnect 32 (32 cells) */
    .vendor=0X0904, .product=0X2008,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum VarioConnect 24 (24 cells) */
    .vendor=0X0904, .product=0X2009,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum VarioConnect 64 (64 cells) */
    .vendor=0X0904, .product=0X2010,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum VarioConnect 80 (80 cells) */
    .vendor=0X0904, .product=0X2011,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum EcoVario 32 (32 cells) */
    .vendor=0X0904, .product=0X2014,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum EcoVario 64 (64 cells) */
    .vendor=0X0904, .product=0X2015,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum EcoVario 80 (80 cells) */
    .vendor=0X0904, .product=0X2016,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* Baum Refreshabraille 18 (18 cells) */
    .vendor=0X0904, .product=0X3000,
    .operations = &usbSerialOperations_FTDI_FT232BM
  },

  { /* HandyTech GoHubs */
    .vendor=0X0921, .product=0X1200,
    .operations = &usbSerialOperations_Belkin
  },

  { /* Braille Memo, Seika Braille Display */
    .vendor=0X10C4, .product=0XEA60,
    .generic = 1,
    .operations = &usbSerialOperations_CP2101
  },

  { /* Seika Note Taker */
    .vendor=0X10C4, .product=0XEA80,
    .generic = 1,
    .operations = &usbSerialOperations_CP2110
  }
};

static size_t usbSerialAdapterCount = ARRAY_COUNT(usbSerialAdapterTable);
static const UsbSerialAdapter **usbSerialAdapters = NULL;

static const UsbInterfaceDescriptor *
usbFindCommunicationInterface (UsbDevice *device) {
  const UsbDescriptor *descriptor = NULL;

  while (usbNextDescriptor(device, &descriptor))
    if (descriptor->header.bDescriptorType == UsbDescriptorType_Interface)
      if (descriptor->interface.bInterfaceClass == 0X02)
        return &descriptor->interface;

  logMessage(LOG_WARNING, "USB: communication interface descriptor not found");
  errno = ENOENT;
  return NULL;
}

static const UsbEndpointDescriptor *
usbFindInterruptInputEndpoint (UsbDevice *device, const UsbInterfaceDescriptor *interface) {
  const UsbDescriptor *descriptor = (const UsbDescriptor *)interface;

  while (usbNextDescriptor(device, &descriptor)) {
    if (descriptor->header.bDescriptorType == UsbDescriptorType_Interface) break;

    if (descriptor->header.bDescriptorType == UsbDescriptorType_Endpoint)
      if (USB_ENDPOINT_DIRECTION(&descriptor->endpoint) == UsbEndpointDirection_Input)
        if (USB_ENDPOINT_TRANSFER(&descriptor->endpoint) == UsbEndpointTransfer_Interrupt)
          return &descriptor->endpoint;
  }

  logMessage(LOG_WARNING, "USB: interrupt input endpoint descriptor not found");
  errno = ENOENT;
  return NULL;
}

static int
usbCompareSerialAdapters (const UsbSerialAdapter *adapter1, const UsbSerialAdapter *adapter2) {
  if (adapter1->vendor < adapter2->vendor) return -1;
  if (adapter1->vendor > adapter2->vendor) return 1;

  if (adapter1->product < adapter2->product) return -1;
  if (adapter1->product > adapter2->product) return 1;

  return 0;
}

static int
usbSortSerialAdapters (const void *element1, const void *element2) {
  const UsbSerialAdapter *const *adapter1 = element1;
  const UsbSerialAdapter *const *adapter2 = element2;

  return usbCompareSerialAdapters(*adapter1, *adapter2);
}

static int
usbSearchSerialAdapter (const void *target, const void *element) {
  const UsbSerialAdapter *const *adapter = element;

  return usbCompareSerialAdapters(target, *adapter);
}

static const UsbSerialAdapter *
usbGetSerialAdapter (uint16_t vendor, uint16_t product) {
  const UsbSerialAdapter target = {
    .vendor = vendor,
    .product = product
  };

  const UsbSerialAdapter *const *adapter = bsearch(&target, usbSerialAdapters, usbSerialAdapterCount, sizeof(*usbSerialAdapters), usbSearchSerialAdapter);

  return adapter? *adapter: NULL;
}

const UsbSerialAdapter *
usbFindSerialAdapter (const UsbDeviceDescriptor *descriptor) {
  if (!usbSerialAdapters) {
    const UsbSerialAdapter **adapters;

    if (!(adapters = malloc(usbSerialAdapterCount * sizeof(*adapters)))) {
      logMallocError();
      return NULL;
    }

    {
      const UsbSerialAdapter *source = usbSerialAdapterTable;
      const UsbSerialAdapter *end = source + usbSerialAdapterCount;
      const UsbSerialAdapter **target = adapters;

      while (source < end) *target++ = source++;
      qsort(adapters, usbSerialAdapterCount, sizeof(*adapters), usbSortSerialAdapters);
    }

    usbSerialAdapters = adapters;
  }

  {
    uint16_t vendor = getLittleEndian16(descriptor->idVendor);
    uint16_t product = getLittleEndian16(descriptor->idProduct);
    const UsbSerialAdapter *adapter = usbGetSerialAdapter(vendor, product);

    if (!adapter) adapter = usbGetSerialAdapter(vendor, 0);
    return adapter;
  }
}

int
usbSetSerialOperations (UsbDevice *device) {
  if (device->serialOperations) return 1;

  {
    const UsbSerialAdapter *adapter = usbFindSerialAdapter(&device->descriptor);

    if (adapter) {
      const UsbSerialOperations *operations = adapter->operations;

      if (operations) {
        UsbInputFilter *filter = operations->inputFilter;
       
        if (filter && !usbAddInputFilter(device, filter)) return 0;
      }

      device->serialOperations = operations;
      return 1;
    }
  }

  if (device->descriptor.bDeviceClass == 0X02) {
    const UsbInterfaceDescriptor *interface = usbFindCommunicationInterface(device);

    if (interface) {
      switch (interface->bInterfaceSubClass) {
        case 0X02:
          device->serialOperations = &usbSerialOperations_CDC_ACM;
          break;

        default:
          break;
      }

      if (device->serialOperations) {
        device->serialData = interface;

        if (usbClaimInterface(device, interface->bInterfaceNumber)) {
          if (usbSetAlternative(device, interface->bInterfaceNumber, interface->bAlternateSetting)) {
            {
              const UsbEndpointDescriptor *endpoint = usbFindInterruptInputEndpoint(device, interface);

              if (endpoint) {
                usbBeginInput(device, USB_ENDPOINT_NUMBER(endpoint), 8);
              }
            }

            return 1;
          }
        }
      }
    }
  }

  return 1;
}

const UsbSerialOperations *
usbGetSerialOperations (UsbDevice *device) {
  return device->serialOperations;
}

int
usbSetSerialParameters (UsbDevice *device, const SerialParameters *parameters) {
  const UsbSerialOperations *serial = usbGetSerialOperations(device);

  if (!serial) {
    logMessage(LOG_CATEGORY(USB_IO), "no serial operations: vendor=%04X product=%04X",
               getLittleEndian16(device->descriptor.idVendor),
               getLittleEndian16(device->descriptor.idProduct));
    errno = ENOSYS;
    return 0;
  }

  if (serial->setLineConfiguration) {
    if (!serial->setLineConfiguration(device, parameters->baud, parameters->dataBits, parameters->stopBits, parameters->parity, parameters->flowControl)) return 0;
  } else {
    if (serial->setLineProperties) {
      if (!serial->setLineProperties(device, parameters->baud, parameters->dataBits, parameters->stopBits, parameters->parity)) return 0;
    } else {
      if (!serial->setBaud(device, parameters->baud)) return 0;
      if (!serial->setDataFormat(device, parameters->dataBits, parameters->stopBits, parameters->parity)) return 0;
    }

    if (!serial->setFlowControl(device, parameters->flowControl)) return 0;
  }

  return 1;
}
