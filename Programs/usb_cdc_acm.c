/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>
#include <errno.h>

#include "log.h"
#include "usb_serial.h"
#include "usb_cdc_acm.h"
#include "usb_internal.h"
#include "bitfield.h"

struct UsbSerialDataStruct {
  UsbDevice *device;
  const UsbInterfaceDescriptor *interface;
  const UsbEndpointDescriptor *endpoint;
};

static const UsbInterfaceDescriptor *
usbFindCommunicationInterface (UsbDevice *device) {
  const UsbDescriptor *descriptor = NULL;

  while (usbNextDescriptor(device, &descriptor)) {
    if (descriptor->header.bDescriptorType == UsbDescriptorType_Interface) {
      if (descriptor->interface.bInterfaceClass == 0X02) {
        return &descriptor->interface;
      }
    }
  }

  logMessage(LOG_WARNING, "USB: communication interface descriptor not found");
  errno = ENOENT;
  return NULL;
}

static const UsbEndpointDescriptor *
usbFindInterruptInputEndpoint (UsbDevice *device, const UsbInterfaceDescriptor *interface) {
  const UsbDescriptor *descriptor = (const UsbDescriptor *)interface;

  while (usbNextDescriptor(device, &descriptor)) {
    if (descriptor->header.bDescriptorType == UsbDescriptorType_Interface) break;

    if (descriptor->header.bDescriptorType == UsbDescriptorType_Endpoint) {
      if (USB_ENDPOINT_DIRECTION(&descriptor->endpoint) == UsbEndpointDirection_Input) {
        if (USB_ENDPOINT_TRANSFER(&descriptor->endpoint) == UsbEndpointTransfer_Interrupt) {
          return &descriptor->endpoint;
        }
      }
    }
  }

  logMessage(LOG_WARNING, "USB: interrupt input endpoint descriptor not found");
  errno = ENOENT;
  return NULL;
}

static int
usbMakeData_CDC_ACM (UsbDevice *device, UsbSerialData **serialData) {
  UsbSerialData *usd;

  if ((usd = malloc(sizeof(*usd)))) {
    memset(usd, 0, sizeof(*usd));
    usd->device = device;

    if ((usd->interface = usbFindCommunicationInterface(device))) {
      unsigned char interfaceNumber = usd->interface->bInterfaceNumber;

      if (usbClaimInterface(device, interfaceNumber)) {
        if (usbSetAlternative(device, usd->interface->bInterfaceNumber, usd->interface->bAlternateSetting)) {
          if ((usd->endpoint = usbFindInterruptInputEndpoint(device, usd->interface))) {
            usbBeginInput(device, USB_ENDPOINT_NUMBER(usd->endpoint));
            *serialData = usd;
            return 1;
          }
        }

        usbReleaseInterface(device, interfaceNumber);
      }
    }

    free(usd);
  } else {
    logMallocError();
  }

  return 0;
}

static void
usbDestroyData_CDC_ACM (UsbSerialData *usd) {
  usbReleaseInterface(usd->device, usd->interface->bInterfaceNumber);
  free(usd);
}

static int
usbSetLineProperties_CDC_ACM (UsbDevice *device, unsigned int baud, unsigned int dataBits, SerialStopBits stopBits, SerialParity parity) {
  USB_CDC_ACM_LineCoding lineCoding;
  memset(&lineCoding, 0, sizeof(lineCoding));

  putLittleEndian32(&lineCoding.dwDTERate, baud);

  switch (dataBits) {
    case  5:
    case  6:
    case  7:
    case  8:
    case 16:
      lineCoding.bDataBits = dataBits;
      break;

    default:
      logMessage(LOG_WARNING, "unsupported CDC ACM data bits: %u", dataBits);
      errno = EINVAL;
      return 0;
  }

  switch (stopBits) {
    case SERIAL_STOP_1:
      lineCoding.bCharFormat = USB_CDC_ACM_STOP_1;
      break;

    case SERIAL_STOP_1_5:
      lineCoding.bCharFormat = USB_CDC_ACM_STOP_1_5;
      break;

    case SERIAL_STOP_2:
      lineCoding.bCharFormat = USB_CDC_ACM_STOP_2;
      break;

    default:
      logMessage(LOG_WARNING, "unsupported CDC ACM stop bits: %u", stopBits);
      errno = EINVAL;
      return 0;
  }

  switch (parity) {
    case SERIAL_PARITY_NONE:
      lineCoding.bParityType = USB_CDC_ACM_PARITY_NONE;
      break;

    case SERIAL_PARITY_ODD:
      lineCoding.bParityType = USB_CDC_ACM_PARITY_ODD;
      break;

    case SERIAL_PARITY_EVEN:
      lineCoding.bParityType = USB_CDC_ACM_PARITY_EVEN;
      break;

    case SERIAL_PARITY_MARK:
      lineCoding.bParityType = USB_CDC_ACM_PARITY_MARK;
      break;

    case SERIAL_PARITY_SPACE:
      lineCoding.bParityType = USB_CDC_ACM_PARITY_SPACE;
      break;

    default:
      logMessage(LOG_WARNING, "unsupported CDC ACM parity: %u", parity);
      errno = EINVAL;
      return 0;
  }

  {
    ssize_t result = usbControlWrite(device,
                                     UsbControlRecipient_Interface, UsbControlType_Class,
                                     USB_CDC_ACM_CTL_SetLineCoding,
                                     0,
                                     device->serial.data->interface->bInterfaceNumber,
                                     &lineCoding, sizeof(lineCoding), 1000);

    if (result == -1) return 0;
  }

  return 1;
}

static int
usbSetFlowControl_CDC_ACM (UsbDevice *device, SerialFlowControl flow) {
  if (flow) {
    logMessage(LOG_WARNING, "unsupported CDC ACM flow control: %02X", flow);
    errno = EINVAL;
    return 0;
  }

  return 1;
}

static int
usbEnableAdapter_CDC_ACM (UsbDevice *device) {
  ssize_t result = usbControlWrite(device,
                                   UsbControlRecipient_Interface, UsbControlType_Class,
                                   USB_CDC_ACM_CTL_SetControlLines,
                                   USB_CDC_ACM_LINE_DTR,
                                   device->serial.data->interface->bInterfaceNumber,
                                   NULL, 0, 1000);

  if (result != -1) return 1;
  return 0;
}

const UsbSerialOperations usbSerialOperations_CDC_ACM = {
  .name = "CDC_ACM",

  .makeData = usbMakeData_CDC_ACM,
  .destroyData = usbDestroyData_CDC_ACM,

  .setLineProperties = usbSetLineProperties_CDC_ACM,
  .setFlowControl = usbSetFlowControl_CDC_ACM,
  .enableAdapter = usbEnableAdapter_CDC_ACM
};
