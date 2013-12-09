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

#include <string.h>
#include <errno.h>

#include "log.h"
#include "usb_serial.h"
#include "bitfield.h"

typedef enum {
  USB_CP2101_CTL_EnableInterface        = 0x00,
  USB_CP2101_CTL_SetBaudDivisor         = 0x01,
  USB_CP2101_CTL_GetBaudDivisor         = 0x02,
  USB_CP2101_CTL_SetLineControl         = 0x03,
  USB_CP2101_CTL_GetLineControl         = 0x04,
  USB_CP2101_CTL_SetBreak               = 0x05,
  USB_CP2101_CTL_SendImmediateCharacter = 0x06,
  USB_CP2101_CTL_SetModemHandShaking    = 0x07,
  USB_CP2101_CTL_GetModemStatus         = 0x08,
  USB_CP2101_CTL_SetXon                 = 0x09,
  USB_CP2101_CTL_SetXoff                = 0x0A,
  USB_CP2101_CTL_SetEventMask           = 0x0B,
  USB_CP2101_CTL_GetEventMask           = 0x0C,
  USB_CP2101_CTL_SetSpecialCharacter    = 0x0D,
  USB_CP2101_CTL_GetSpecialCharacters   = 0x0E,
  USB_CP2101_CTL_GetProperties          = 0x0F,
  USB_CP2101_CTL_GetSerialStatus        = 0x10,
  USB_CP2101_CTL_Reset                  = 0x11,
  USB_CP2101_CTL_Purge                  = 0x12,
  USB_CP2101_CTL_SetFlowControl         = 0x13,
  USB_CP2101_CTL_GetFlowControl         = 0x14,
  USB_CP2101_CTL_EmbedEvents            = 0x15,
  USB_CP2101_CTL_GetEventState          = 0x16,
  USB_CP2101_CTL_SetSpecialCharacters   = 0x19,
  USB_CP2101_CTL_GetBaudRate            = 0x1D,
  USB_CP2101_CTL_SetBaudRate            = 0x1E,
  USB_CP2101_CTL_VendorSpecific         = 0xFF
} USB_CP2101_ControlRequest;

#define USB_CP2101_BAUD_BASE 0X384000

static ssize_t
usbGetAttributes_CP2101 (UsbDevice *device, uint8_t request, void *data, size_t length) {
  ssize_t result = usbControlRead(device, UsbControlRecipient_Interface, UsbControlType_Vendor,
                                  request, 0, 0, data, length, 1000);
  if (result == -1) return 0;

  if (result < length) {
    unsigned char *bytes = data;
    memset(&bytes[result], 0, length-result);
  }

  logMessage(LOG_DEBUG, "CP2101 Get: %02X", request);
  logBytes(LOG_DEBUG, "CP2101 Data", data, result);
  return result;
}

static int
usbSetAttributes_CP2101 (
  UsbDevice *device,
  uint8_t request, uint16_t value,
  const void *data, size_t length
) {
  logMessage(LOG_DEBUG, "CP2101 Set: %02X %04X", request, value);
  if (length) logBytes(LOG_DEBUG, "CP2101 Data", data, length);

  return usbControlWrite(device, UsbControlRecipient_Interface, UsbControlType_Vendor,
                         request, value, 0, data, length, 1000) != -1;
}

static int
usbSetAttribute_CP2101 (UsbDevice *device, uint8_t request, uint16_t value) {
  return usbSetAttributes_CP2101(device, request, value, NULL, 0);
}

static int
usbSetBaud_CP2101 (UsbDevice *device, unsigned int baud) {
  const unsigned int base = USB_CP2101_BAUD_BASE;
  unsigned int divisor = base / baud;

  if ((baud * divisor) != base) {
    logMessage(LOG_WARNING, "unsupported CP2101 baud: %u", baud);
    errno = EINVAL;
    return 0;
  }

  if (usbSetAttribute_CP2101(device, USB_CP2101_CTL_SetBaudDivisor, divisor)) {
    uint32_t data;
    putLittleEndian32(&data, baud);

    if (usbSetAttributes_CP2101(device, USB_CP2101_CTL_SetBaudRate, 0, &data, sizeof(data))) {
      return 1;
    }
  }

  return 0;
}

static int
usbSetFlowControl_CP2101 (UsbDevice *device, SerialFlowControl flow) {
  unsigned char bytes[16];
  ssize_t count = usbGetAttributes_CP2101(device, USB_CP2101_CTL_GetFlowControl, bytes, sizeof(bytes));
  if (!count) return 0;

  if (flow) {
    logMessage(LOG_WARNING, "unsupported CP2101 flow control: %02X", flow);
  }

  return usbSetAttributes_CP2101(device, USB_CP2101_CTL_SetFlowControl, 0, bytes, count);
}

static int
usbSetDataFormat_CP2101 (UsbDevice *device, unsigned int dataBits, SerialStopBits stopBits, SerialParity parity) {
  int ok = 1;
  unsigned int value = dataBits & 0XF;

  if (dataBits != value) {
    logMessage(LOG_WARNING, "unsupported CP2101 data bits: %u", dataBits);
    ok = 0;
  }
  value <<= 8;

  switch (parity) {
    case SERIAL_PARITY_NONE:  value |= 0X00; break;
    case SERIAL_PARITY_ODD:   value |= 0X10; break;
    case SERIAL_PARITY_EVEN:  value |= 0X20; break;
    case SERIAL_PARITY_MARK:  value |= 0X30; break;
    case SERIAL_PARITY_SPACE: value |= 0X40; break;

    default:
      logMessage(LOG_WARNING, "unsupported CP2101 parity: %u", parity);
      ok = 0;
      break;
  }

  switch (stopBits) {
    case SERIAL_STOP_1:   value |= 0X0; break;
    case SERIAL_STOP_1_5: value |= 0X1; break;
    case SERIAL_STOP_2:   value |= 0X2; break;

    default:
      logMessage(LOG_WARNING, "unsupported CP2101 stop bits: %u", stopBits);
      ok = 0;
      break;
  }

  if (!ok) {
    errno = EINVAL;
    return 0;
  }

  return usbSetAttribute_CP2101(device, USB_CP2101_CTL_SetLineControl, value);
}

static int
usbSetModemState_CP2101 (UsbDevice *device, int state, int shift, const char *name) {
  if ((state < 0) || (state > 1)) {
    logMessage(LOG_WARNING, "Unsupported CP2101 %s state: %d", name, state);
    errno = EINVAL;
    return 0;
  }

  return usbSetAttribute_CP2101(device, USB_CP2101_CTL_SetModemHandShaking, ((1 << (shift + 8)) | (state << shift)));
}

static int
usbSetDtrState_CP2101 (UsbDevice *device, int state) {
  return usbSetModemState_CP2101(device, state, 0, "DTR");
}

static int
usbSetRtsState_CP2101 (UsbDevice *device, int state) {
  return usbSetModemState_CP2101(device, state, 1, "RTS");
}

static int
usbSetUartState_CP2101 (UsbDevice *device, int state) {
  return usbSetAttribute_CP2101(device, USB_CP2101_CTL_EnableInterface, state);
}

static int
usbEnableAdapter_CP2101 (UsbDevice *device) {
  if (!usbSetUartState_CP2101(device, 0)) return 0;
  if (!usbSetUartState_CP2101(device, 1)) return 0;
  if (!usbSetDtrState_CP2101(device, 1)) return 0;
  if (!usbSetRtsState_CP2101(device, 1)) return 0;
  return 1;
}

const UsbSerialOperations usbSerialOperations_CP2101 = {
  .name = "CP2101",     
  .setBaud = usbSetBaud_CP2101,
  .setDataFormat = usbSetDataFormat_CP2101,
  .setFlowControl = usbSetFlowControl_CP2101,
  .setDtrState = usbSetDtrState_CP2101,
  .setRtsState = usbSetRtsState_CP2101,
  .enableAdapter = usbEnableAdapter_CP2101
};
