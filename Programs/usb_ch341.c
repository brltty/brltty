/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>
#include <errno.h>

#include "log.h"
#include "usb_serial.h"
#include "usb_ch341.h"

struct UsbSerialDataStruct {
  char version[2];
  char status[2];
  uint8_t lcr;
};

typedef struct {
  uint16_t factor;
  uint8_t bits;
} CH341_PrescalerEntry;

static const CH341_PrescalerEntry CH341_prescalerTable[] = {
  { .factor = 00001, // 1
    .bits = USB_CH341_PS_DISABLE_2x | USB_CH341_PS_DISABLE_8x | USB_CH341_PS_DISABLE_64x
  },

  { .factor = 00002, // 2
    .bits = USB_CH341_PS_DISABLE_8x | USB_CH341_PS_DISABLE_64x
  },

  { .factor = 00010, // 8
    .bits = USB_CH341_PS_DISABLE_2x | USB_CH341_PS_DISABLE_64x
  },

  { .factor = 00020, // 16
    .bits = USB_CH341_PS_DISABLE_64x
  },

  { .factor = 00100, // 64
    .bits = USB_CH341_PS_DISABLE_2x | USB_CH341_PS_DISABLE_8x
  },

  { .factor = 00200, // 128
    .bits = USB_CH341_PS_DISABLE_8x
  },

  { .factor = 01000, // 512
    .bits = USB_CH341_PS_DISABLE_2x
  },

  { .factor = 02000, // 1024
    .bits = 0
  },
};

static const uint8_t CH341_prescalerCOUNT = ARRAY_COUNT(CH341_prescalerTable);

static inline unsigned long
usbTransformValue_CH341 (uint16_t factor, unsigned long value) {
  return (((2UL * USB_CH341_FREQUENCY) / (factor * value)) + 1UL) / 2UL;
}

static int
usbGetBaudParameters (unsigned int baud, uint8_t *prescaler, uint16_t *divisor) {
  const CH341_PrescalerEntry *ps = CH341_prescalerTable;
  const CH341_PrescalerEntry *end = ps + CH341_prescalerCOUNT;

  int NOT_FOUND = -1;
  int nearest = NOT_FOUND;

  while (ps < end) {
    unsigned long psDivisor = usbTransformValue_CH341(ps->factor, baud);

    if (psDivisor < ((ps->factor == 1)? 9: USB_CH341_DIVISOR_MINIMUM)) {
      break;
    }

    if (psDivisor <= USB_CH341_DIVISOR_MAXIMUM) {
      unsigned int psBaud = usbTransformValue_CH341(ps->factor, psDivisor);
      int distance = psBaud - baud;
      if (distance < 0) distance = -distance;

      if ((nearest == NOT_FOUND) || (distance <= nearest)) {
        *divisor = 256 - psDivisor;
        *prescaler = ps->bits | USB_CH341_PS_IMMEDIATE;
        nearest = distance;
      }
    }

    ps += 1;
  }

  return nearest != NOT_FOUND;
}

static int
usbControlWrite_CH341 (
  UsbDevice *device, unsigned char request,
  unsigned int value, unsigned int index
) {
  logMessage(LOG_CATEGORY(USB_IO),
    "CH341 control write: %02X %04X %04X",
    request, value, index
  );

  ssize_t result = usbControlWrite(
    device, UsbControlRecipient_Device, UsbControlType_Vendor,
    request, value, index, NULL, 0, 1000
  );

  return result != -1;
}

static int
usbControlRead_CH341 (
  UsbDevice *device, unsigned char request,
  unsigned int value, unsigned int index,
  unsigned char *buffer, size_t size
) {
  logMessage(LOG_CATEGORY(USB_IO),
    "CH341 control read: %02X %04X %04X",
    request, value, index
  );

  ssize_t result = usbControlRead(
    device, UsbControlRecipient_Device, UsbControlType_Vendor,
    request, value, index, buffer, size, 1000
  );

  return result != -1;
}

static int
usbWriteRegisters_CH341 (
  UsbDevice *device,
  uint8_t register1, uint8_t value1,
  uint8_t register2, uint8_t value2
) {
  return usbControlWrite_CH341(
    device, USB_CH341_REQ_WRITE_REGISTERS,
    (register2 << 8) | register1,
    (value2 << 8) | value1
  );
}

static int
usbGetVersion_CH341 (UsbDevice *device) {
  UsbSerialData *usd = usbGetSerialData(device);
  size_t size = sizeof(usd->version);
  unsigned char version[size];

  if (usbControlRead_CH341(device, USB_CH341_REQ_READ_VERSION, 0, 0, version, size)) {
    memcpy(usd->version, version, size);

    logBytes(LOG_CATEGORY(USB_IO),
      "CH341 version", usd->version, size
    );

    return 1;
  }

  return 0;
}

static int
usbGetStatus_CH341 (UsbDevice *device) {
  UsbSerialData *usd = usbGetSerialData(device);
  size_t size = sizeof(usd->status);
  unsigned char status[size];

  if (usbControlRead_CH341(device, USB_CH341_REQ_READ_REGISTER, 0X0706, 0, status, size)) {
    for (unsigned int i=0; i<size; i+=1) {
      usd->status[i] = status[i] ^ 0XFF;
    }

    logBytes(LOG_CATEGORY(USB_IO),
      "CH341 status", usd->status, size
    );

    return 1;
  }

  return 0;
}

static int
usbSetBaud_CH341 (UsbDevice *device, unsigned int baud) {
  if (baud < USB_CH341_BAUD_MINIMUM) return 0;
  if (baud > USB_CH341_BAUD_MAXIMUM) return 0;

  uint8_t prescaler;
  uint16_t divisor;
  if (!usbGetBaudParameters(baud, &prescaler, &divisor)) return 0;

  return usbWriteRegisters_CH341(device,
    USB_CH341_REG_PRESCALER, prescaler,
    USB_CH341_REG_DIVISOR, divisor
  );
}

static int
usbWriteLCR_CH341 (UsbDevice *device) {
  UsbSerialData *usd = usbGetSerialData(device);

  return usbWriteRegisters_CH341(device,
    USB_CH341_REG_LCR1, usd->lcr,
    USB_CH341_REG_LCR2, 0
  );
}

static void
usbUpdateLCR_CH341 (UsbDevice *device, uint8_t clear, uint8_t set) {
  UsbSerialData *usd = usbGetSerialData(device);
  usd->lcr &= ~clear;
  usd->lcr |= set;
}

static int
usbUpdateDataBits_CH341 (UsbDevice *device, unsigned int dataBits) {
  uint8_t lcrBits = 0;

  switch (dataBits) {
    case 5: lcrBits |= USB_CH341_LCR_DATA_BITS_5; break;
    case 6: lcrBits |= USB_CH341_LCR_DATA_BITS_6; break;
    case 7: lcrBits |= USB_CH341_LCR_DATA_BITS_7; break;
    case 8: lcrBits |= USB_CH341_LCR_DATA_BITS_8; break;

    default:
      logMessage(LOG_WARNING, "unsupported CH341 data bits: %u", dataBits);
      return 0;
  }

  usbUpdateLCR_CH341(device, USB_CH341_LCR_DATA_BITS_MASK, lcrBits);
  return 1;
}

static int
usbUpdateStopBits_CH341 (UsbDevice *device, SerialStopBits stopBits) {
  uint8_t lcrBits = 0;

  switch (stopBits) {
    case SERIAL_STOP_1:
      lcrBits |= USB_CH341_LCR_STOP_BITS_1;
      break;

    case SERIAL_STOP_2:
      lcrBits |= USB_CH341_LCR_STOP_BITS_2;
      break;

    default:
      logMessage(LOG_WARNING, "unsupported CH341 stop bits: %u", stopBits);
      return 0;
  }

  usbUpdateLCR_CH341(device, USB_CH341_LCR_STOP_BITS_MASK, lcrBits);
  return 1;
}

static int
usbUpdateParity_CH341 (UsbDevice *device, SerialParity parity) {
  uint8_t lcrBits = 0;

  if (parity != SERIAL_PARITY_NONE) {
    lcrBits |= USB_CH341_LCR_PARITY_ENABLE;

    switch (parity) {
      case SERIAL_PARITY_EVEN:
        lcrBits |= USB_CH341_LCR_PARITY_EVEN;
        break;

      case SERIAL_PARITY_ODD:
        lcrBits |= USB_CH341_LCR_PARITY_ODD;
        break;

      case SERIAL_PARITY_SPACE:
        lcrBits |= USB_CH341_LCR_PARITY_SPACE;
        break;

      case SERIAL_PARITY_MARK:
        lcrBits |= USB_CH341_LCR_PARITY_MARK;
        break;

      default:
        logMessage(LOG_WARNING, "unsupported CH341 parity: %u", parity);
        return 0;
    }
  }

  usbUpdateLCR_CH341(device, USB_CH341_LCR_PARITY_MASK, lcrBits);
  return 1;
}

static int
usbSetDataFormat_CH341 (UsbDevice *device, unsigned int dataBits, SerialStopBits stopBits, SerialParity parity) {
  return usbUpdateParity_CH341(device, parity)
      && usbUpdateDataBits_CH341(device, dataBits)
      && usbUpdateStopBits_CH341(device, stopBits)
      && usbWriteLCR_CH341(device);
}

static int
usbEnableAdapter_CH341 (UsbDevice *device) {
  usbGetVersion_CH341(device);

  if (!usbControlWrite_CH341(device, USB_CH341_REQ_INITIALIZE, 0, 0)) {
    return 0;
  }

  if (!usbWriteLCR_CH341(device)) return 0;
  usbGetStatus_CH341(device);
  return 1;
}

static int
usbMakeData_CH341 (UsbDevice *device, UsbSerialData **serialData) {
  UsbSerialData *usd;

  if ((usd = malloc(sizeof(*usd)))) {
    memset(usd, 0, sizeof(*usd));

    usd->lcr = USB_CH341_LCR_DATA_BITS_8 
             | USB_CH341_LCR_TRANSMIT_ENABLE
             | USB_CH341_LCR_RECEIVE_ENABLE
             ;

    *serialData = usd;
    return 1;
  } else {
    logMallocError();
  }

  return 0;
}

static void
usbDestroyData_CH341 (UsbSerialData *usd) {
  free(usd);
}

const UsbSerialOperations usbSerialOperations_CH341 = {
  .name = "CH341",     

  .enableAdapter = usbEnableAdapter_CH341,     
  .makeData = usbMakeData_CH341,     
  .destroyData = usbDestroyData_CH341,     

  .setBaud = usbSetBaud_CH341,
  .setDataFormat = usbSetDataFormat_CH341,
};
