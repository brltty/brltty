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
#include "io_usb.h"
#include "usb_internal.h"

static int
usbSkipInitialBytes (UsbInputFilterData *data, unsigned int count) {
  if (data->length > count) {
    unsigned char *buffer = data->buffer;
    memmove(buffer, buffer+count, data->length-=count);
  } else {
    data->length = 0;
  }

  return 1;
}


typedef enum {
  USB_CDC_ACM_CTL_SetLineCoding   = 0X20,
  USB_CDC_ACM_CTL_GetLineCoding   = 0X21,
  USB_CDC_ACM_CTL_SetControlLines = 0X22,
  USB_CDC_ACM_CTL_SendBreak       = 0X23
} USB_CDC_ACM_ControlRequest;

typedef enum {
  USB_CDC_ACM_LINE_DTR = 0X01,
  USB_CDC_ACM_LINE_RTS = 0X02
} USB_CDC_ACM_ControlLine;

typedef enum {
  USB_CDC_ACM_STOP_1,
  USB_CDC_ACM_STOP_1_5,
  USB_CDC_ACM_STOP_2
} USB_CDC_ACM_StopBits;

typedef enum {
  USB_CDC_ACM_PARITY_NONE,
  USB_CDC_ACM_PARITY_ODD,
  USB_CDC_ACM_PARITY_EVEN,
  USB_CDC_ACM_PARITY_MARK,
  USB_CDC_ACM_PARITY_SPACE
} USB_CDC_ACM_Parity;

typedef struct {
  uint32_t dwDTERate; /* transmission rate - bits per second */
  uint8_t bCharFormat; /* number of stop bits */
  uint8_t bParityType; /* type of parity */
  uint8_t bDataBits; /* number of data bits - 5,6,7,8,16 */
} PACKED USB_CDC_ACM_LineCoding;

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
    const UsbInterfaceDescriptor *interface = device->serialData;
    ssize_t result = usbControlWrite(device,
                                     UsbControlRecipient_Interface, UsbControlType_Class,
                                     USB_CDC_ACM_CTL_SetLineCoding,
                                     0,
                                     interface->bInterfaceNumber,
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
  const UsbInterfaceDescriptor *interface = device->serialData;
  ssize_t result = usbControlWrite(device,
                                   UsbControlRecipient_Interface, UsbControlType_Class,
                                   USB_CDC_ACM_CTL_SetControlLines,
                                   USB_CDC_ACM_LINE_DTR,
                                   interface->bInterfaceNumber,
                                   NULL, 0, 1000);

  if (result != -1) return 1;
  return 0;
}

static const UsbSerialOperations usbSerialOperations_CDC_ACM = {
  .setLineProperties = usbSetLineProperties_CDC_ACM,
  .setFlowControl = usbSetFlowControl_CDC_ACM,
  .enableAdapter = usbEnableAdapter_CDC_ACM
};


static int
usbSetAttribute_Belkin (UsbDevice *device, unsigned char request, unsigned int value, unsigned int index) {
  logMessage(LOG_DEBUG, "Belkin Request: %02X %04X %04X", request, value, index);
  return usbControlWrite(device, UsbControlRecipient_Device, UsbControlType_Vendor,
                         request, value, index, NULL, 0, 1000) != -1;
}

static int
usbSetBaud_Belkin (UsbDevice *device, unsigned int baud) {
  const unsigned int base = 230400;
  if (base % baud) {
    logMessage(LOG_WARNING, "unsupported Belkin baud: %u", baud);
    errno = EINVAL;
    return 0;
  }
  return usbSetAttribute_Belkin(device, 0, base/baud, 0);
}

static int
usbSetFlowControl_Belkin (UsbDevice *device, SerialFlowControl flow) {
  unsigned int value = 0;
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
    logMessage(LOG_WARNING, "unsupported Belkin flow control: %02X", flow);
  }
  return usbSetAttribute_Belkin(device, 16, value, 0);
}

static int
usbSetDataBits_Belkin (UsbDevice *device, unsigned int bits) {
  if ((bits < 5) || (bits > 8)) {
    logMessage(LOG_WARNING, "unsupported Belkin data bits: %u", bits);
    errno = EINVAL;
    return 0;
  }
  return usbSetAttribute_Belkin(device, 2, bits-5, 0);
}

static int
usbSetStopBits_Belkin (UsbDevice *device, SerialStopBits bits) {
  unsigned int value;
  switch (bits) {
    case SERIAL_STOP_1: value = 0; break;
    case SERIAL_STOP_2: value = 1; break;
    default:
      logMessage(LOG_WARNING, "unsupported Belkin stop bits: %u", bits);
      errno = EINVAL;
      return 0;
  }
  return usbSetAttribute_Belkin(device, 1, value, 0);
}

static int
usbSetParity_Belkin (UsbDevice *device, SerialParity parity) {
  unsigned int value;
  switch (parity) {
    case SERIAL_PARITY_SPACE: value = 4; break;
    case SERIAL_PARITY_ODD:   value = 2; break;
    case SERIAL_PARITY_EVEN:  value = 1; break;
    case SERIAL_PARITY_MARK:  value = 3; break;
    case SERIAL_PARITY_NONE:  value = 0; break;
    default:
      logMessage(LOG_WARNING, "unsupported Belkin parity: %u", parity);
      errno = EINVAL;
      return 0;
  }
  return usbSetAttribute_Belkin(device, 3, value, 0);
}

static int
usbSetDataFormat_Belkin (UsbDevice *device, unsigned int dataBits, SerialStopBits stopBits, SerialParity parity) {
  if (usbSetDataBits_Belkin(device, dataBits))
    if (usbSetStopBits_Belkin(device, stopBits))
      if (usbSetParity_Belkin(device, parity))
        return 1;
  return 0;
}

static int
usbSetDtrState_Belkin (UsbDevice *device, int state) {
  if ((state < 0) || (state > 1)) {
    logMessage(LOG_WARNING, "Unsupported Belkin DTR state: %d", state);
    errno = EINVAL;
    return 0;
  }
  return usbSetAttribute_Belkin(device, 10, state, 0);
}

static int
usbSetRtsState_Belkin (UsbDevice *device, int state) {
  if ((state < 0) || (state > 1)) {
    logMessage(LOG_WARNING, "Unsupported Belkin RTS state: %d", state);
    errno = EINVAL;
    return 0;
  }
  return usbSetAttribute_Belkin(device, 11, state, 0);
}

static const UsbSerialOperations usbSerialOperations_Belkin = {
  .setBaud = usbSetBaud_Belkin,
  .setDataFormat = usbSetDataFormat_Belkin,
  .setFlowControl = usbSetFlowControl_Belkin,
  .setDtrState = usbSetDtrState_Belkin,
  .setRtsState = usbSetRtsState_Belkin
};


static int
usbInputFilter_FTDI (UsbInputFilterData *data) {
  return usbSkipInitialBytes(data, 2);
}

static int
usbSetAttribute_FTDI (UsbDevice *device, unsigned char request, unsigned int value, unsigned int index) {
  logMessage(LOG_DEBUG, "FTDI Request: %02X %04X %04X", request, value, index);
  return usbControlWrite(device, UsbControlRecipient_Device, UsbControlType_Vendor,
                         request, value, index, NULL, 0, 1000) != -1;
}

static int
usbSetBaud_FTDI (UsbDevice *device, unsigned int divisor) {
  return usbSetAttribute_FTDI(device, 3, divisor&0XFFFF, divisor>>0X10);
}

static int
usbSetBaud_FTDI_SIO (UsbDevice *device, unsigned int baud) {
  unsigned int divisor;
  switch (baud) {
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
      logMessage(LOG_WARNING, "unsupported FTDI SIO baud: %u", baud);
      errno = EINVAL;
      return 0;
  }
  return usbSetBaud_FTDI(device, divisor);
}

static int
usbSetBaud_FTDI_FT8U232AM (UsbDevice *device, unsigned int baud) {
  if (baud > 3000000) {
    logMessage(LOG_WARNING, "unsupported FTDI FT8U232AM baud: %u", baud);
    errno = EINVAL;
    return 0;
  }
  {
    const unsigned int base = 48000000;
    unsigned int eighths = base / 2 / baud;
    unsigned int divisor;
    if ((eighths & 07) == 7) eighths++;
    divisor = eighths >> 3;
    divisor |= (eighths & 04)? 0X4000:
               (eighths & 02)? 0X8000:
               (eighths & 01)? 0XC000:
                               0X0000;
    if (divisor == 1) divisor = 0;
    return usbSetBaud_FTDI(device, divisor);
  }
}

static int
usbSetBaud_FTDI_FT232BM (UsbDevice *device, unsigned int baud) {
  if (baud > 3000000) {
    logMessage(LOG_WARNING, "unsupported FTDI FT232BM baud: %u", baud);
    errno = EINVAL;
    return 0;
  }
  {
    static const unsigned char mask[8] = {00, 03, 02, 04, 01, 05, 06, 07};
    const unsigned int base = 48000000;
    const unsigned int eighths = base / 2 / baud;
    unsigned int divisor = (eighths >> 3) | (mask[eighths & 07] << 14);
    if (divisor == 1) {
      divisor = 0;
    } else if (divisor == 0X4001) {
      divisor = 1;
    }
    return usbSetBaud_FTDI(device, divisor);
  }
}

static int
usbSetFlowControl_FTDI (UsbDevice *device, SerialFlowControl flow) {
  unsigned int index = 0;
#define FTDI_FLOW(from,to) if ((flow & (from)) == (from)) flow &= ~(from), index |= (to)
  FTDI_FLOW(SERIAL_FLOW_OUTPUT_CTS|SERIAL_FLOW_INPUT_RTS, 0X0100);
  FTDI_FLOW(SERIAL_FLOW_OUTPUT_DSR|SERIAL_FLOW_INPUT_DTR, 0X0200);
  FTDI_FLOW(SERIAL_FLOW_OUTPUT_XON|SERIAL_FLOW_INPUT_XON, 0X0400);
#undef FTDI_FLOW
  if (flow) {
    logMessage(LOG_WARNING, "unsupported FTDI flow control: %02X", flow);
  }
  return usbSetAttribute_FTDI(device, 2, ((index & 0X0400)? 0X1311: 0), index);
}

static int
usbSetDataFormat_FTDI (UsbDevice *device, unsigned int dataBits, SerialStopBits stopBits, SerialParity parity) {
  int ok = 1;
  unsigned int value = dataBits & 0XFF;
  if (dataBits != value) {
    logMessage(LOG_WARNING, "unsupported FTDI data bits: %u", dataBits);
    ok = 0;
  }
  switch (parity) {
    case SERIAL_PARITY_NONE:  value |= 0X000; break;
    case SERIAL_PARITY_ODD:   value |= 0X100; break;
    case SERIAL_PARITY_EVEN:  value |= 0X200; break;
    case SERIAL_PARITY_MARK:  value |= 0X300; break;
    case SERIAL_PARITY_SPACE: value |= 0X400; break;
    default:
      logMessage(LOG_WARNING, "unsupported FTDI parity: %u", parity);
      ok = 0;
      break;
  }
  switch (stopBits) {
    case SERIAL_STOP_1: value |= 0X0000; break;
    case SERIAL_STOP_2: value |= 0X1000; break;
    default:
      logMessage(LOG_WARNING, "unsupported FTDI stop bits: %u", stopBits);
      ok = 0;
      break;
  }
  if (!ok) {
    errno = EINVAL;
    return 0;
  }
  return usbSetAttribute_FTDI(device, 4, value, 0);
}

static int
usbSetModemState_FTDI (UsbDevice *device, int state, int shift, const char *name) {
  if ((state < 0) || (state > 1)) {
    logMessage(LOG_WARNING, "Unsupported FTDI %s state: %d", name, state);
    errno = EINVAL;
    return 0;
  }
  return usbSetAttribute_FTDI(device, 1, ((1 << (shift + 8)) | (state << shift)), 0);
}

static int
usbSetDtrState_FTDI (UsbDevice *device, int state) {
  return usbSetModemState_FTDI(device, state, 0, "DTR");
}

static int
usbSetRtsState_FTDI (UsbDevice *device, int state) {
  return usbSetModemState_FTDI(device, state, 1, "RTS");
}

static const UsbSerialOperations usbSerialOperations_FTDI_SIO = {
  .setBaud = usbSetBaud_FTDI_SIO,
  .setDataFormat = usbSetDataFormat_FTDI,
  .setFlowControl = usbSetFlowControl_FTDI,
  .setDtrState = usbSetDtrState_FTDI,
  .setRtsState = usbSetRtsState_FTDI
};

static const UsbSerialOperations usbSerialOperations_FTDI_FT8U232AM = {
  .setBaud = usbSetBaud_FTDI_FT8U232AM,
  .setDataFormat = usbSetDataFormat_FTDI,
  .setFlowControl = usbSetFlowControl_FTDI,
  .setDtrState = usbSetDtrState_FTDI,
  .setRtsState = usbSetRtsState_FTDI
};

static const UsbSerialOperations usbSerialOperations_FTDI_FT232BM = {
  .setBaud = usbSetBaud_FTDI_FT232BM,
  .setDataFormat = usbSetDataFormat_FTDI,
  .setFlowControl = usbSetFlowControl_FTDI,
  .setDtrState = usbSetDtrState_FTDI,
  .setRtsState = usbSetRtsState_FTDI
};


static ssize_t
usbGetAttributes_CP2101 (UsbDevice *device, unsigned char request, void *data, size_t length) {
  ssize_t result = usbControlRead(device, UsbControlRecipient_Interface, UsbControlType_Vendor,
                                  request, 0, 0, data, length, 1000);
  if (result == -1) return 0;

  if (result < length) {
    unsigned char *bytes = data;
    memset(&bytes[result], 0, length-result);
  }

  logBytes(LOG_DEBUG, "CP2101 Attributes", data, result);
  return result;
}

static ssize_t
usbSetAttributes_CP2101 (UsbDevice *device, unsigned char request, const void *data, size_t length) {
  logBytes(LOG_DEBUG, "CP2101 Attributes", data, length);
  return usbControlWrite(device, UsbControlRecipient_Interface, UsbControlType_Vendor,
                         request, 0, 0, data, length, 1000) != -1;
}

static int
usbSetAttribute_CP2101 (UsbDevice *device, unsigned char request, unsigned int value, unsigned int index) {
  logMessage(LOG_DEBUG, "CP2101 Request: %02X %04X %04X", request, value, index);
  return usbControlWrite(device, UsbControlRecipient_Interface, UsbControlType_Vendor,
                         request, value, index, NULL, 0, 1000) != -1;
}

static int
usbSetBaud_CP2101 (UsbDevice *device, unsigned int baud) {
  const unsigned int base = 0X384000;
  unsigned int divisor = base / baud;
  if ((baud * divisor) != base) {
    logMessage(LOG_WARNING, "unsupported CP2101 baud: %u", baud);
    errno = EINVAL;
    return 0;
  }
  return usbSetAttribute_CP2101(device, 1, divisor, 0);
}

static int
usbSetFlowControl_CP2101 (UsbDevice *device, SerialFlowControl flow) {
  unsigned char bytes[16];
  ssize_t count = usbGetAttributes_CP2101(device, 20, bytes, sizeof(bytes));
  if (!count) return 0;

  if (flow) {
    logMessage(LOG_WARNING, "unsupported CP2101 flow control: %02X", flow);
  }

  return usbSetAttributes_CP2101(device, 19, bytes, count) != -1;
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
    case SERIAL_STOP_1: value |= 0X0; break;
    case SERIAL_STOP_2: value |= 0X2; break;
    default:
      logMessage(LOG_WARNING, "unsupported CP2101 stop bits: %u", stopBits);
      ok = 0;
      break;
  }
  if (!ok) {
    errno = EINVAL;
    return 0;
  }
  return usbSetAttribute_CP2101(device, 3, value, 0);
}

static int
usbSetModemState_CP2101 (UsbDevice *device, int state, int shift, const char *name) {
  if ((state < 0) || (state > 1)) {
    logMessage(LOG_WARNING, "Unsupported CP2101 %s state: %d", name, state);
    errno = EINVAL;
    return 0;
  }
  return usbSetAttribute_CP2101(device, 7, ((1 << (shift + 8)) | (state << shift)), 0);
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
  return usbSetAttribute_CP2101(device, 0, state, 0);
}

static int
usbEnableAdapter_CP2101 (UsbDevice *device) {
  if (!usbSetUartState_CP2101(device, 0)) return 0;
  if (!usbSetUartState_CP2101(device, 1)) return 0;
  if (!usbSetDtrState_CP2101(device, 1)) return 0;
  if (!usbSetRtsState_CP2101(device, 1)) return 0;
  return 1;
}

static const UsbSerialOperations usbSerialOperations_CP2101 = {
  .setBaud = usbSetBaud_CP2101,
  .setDataFormat = usbSetDataFormat_CP2101,
  .setFlowControl = usbSetFlowControl_CP2101,
  .setDtrState = usbSetDtrState_CP2101,
  .setRtsState = usbSetRtsState_CP2101,
  .enableAdapter = usbEnableAdapter_CP2101
};


typedef enum {
  USB_CP2110_PARITY_NONE,
  USB_CP2110_PARITY_EVEN,
  USB_CP2110_PARITY_ODD,
  USB_CP2110_PARITY_MARK,
  USB_CP2110_PARITY_SPACE
} USB_CP2110_Parity;

typedef enum {
  USB_CP2110_FLOW_NONE,
  USB_CP2110_FLOW_HARDWARE
} USB_CP2110_FlowControl;

typedef enum {
  USB_CP2110_DATA_5,
  USB_CP2110_DATA_6,
  USB_CP2110_DATA_7,
  USB_CP2110_DATA_8
} USB_CP2110_DataBits;

typedef enum {
  USB_CP2110_STOP_SHORT,
  USB_CP2110_STOP_LONG
} USB_CP2110_StopBits;

typedef struct {
  uint8_t reportIdentifier;
  uint32_t baudRate;
  uint8_t parity;
  uint8_t flowControl;
  uint8_t dataBits;
  uint8_t stopBits;
} PACKED USB_CP2110_UartConfigurationReport;

static int
usbInputFilter_CP2110 (UsbInputFilterData *data) {
  return usbSkipInitialBytes(data, 1);
}

static int
usbSetReport_CP2110 (UsbDevice *device, const void *report, size_t size) {
  const unsigned char *bytes = report;
  ssize_t result = usbHidSetReport(device, 0, bytes[0], report, size, 1000);
  return result != -1;
}

static int
usbSetLineConfiguration_CP2110 (UsbDevice *device, unsigned int baud, unsigned int dataBits, SerialStopBits stopBits, SerialParity parity, SerialFlowControl flowControl) {
  USB_CP2110_UartConfigurationReport report;

  memset(&report, 0, sizeof(report));
  report.reportIdentifier = 0X50;

  if ((baud >= 300) && (baud <= 500000)) {
    putBigEndian32(&report.baudRate, baud);
  } else {
    logMessage(LOG_WARNING, "unsupported CP2110 baud: %u", baud);
    errno = EINVAL;
    return 0;
  }

  switch (dataBits) {
    case 5:
      report.dataBits = USB_CP2110_DATA_5;
      break;

    case 6:
      report.dataBits = USB_CP2110_DATA_6;
      break;

    case 7:
      report.dataBits = USB_CP2110_DATA_7;
      break;

    case 8:
      report.dataBits = USB_CP2110_DATA_8;
      break;

    default:
      logMessage(LOG_WARNING, "unsupported CP2110 data bits: %u", dataBits);
      errno = EINVAL;
      return 0;
  }

  if (stopBits == SERIAL_STOP_1) {
    report.stopBits = USB_CP2110_STOP_SHORT;
  } else if (stopBits == ((dataBits > 5)? SERIAL_STOP_2: SERIAL_STOP_1_5)) {
    report.stopBits = USB_CP2110_STOP_LONG;
  } else {
    logMessage(LOG_WARNING, "unsupported CP2110 stop bits: %u", stopBits);
    errno = EINVAL;
    return 0;
  }

  switch (parity) {
    case SERIAL_PARITY_NONE:
      report.parity = USB_CP2110_PARITY_NONE;
      break;

    case SERIAL_PARITY_ODD:
      report.parity = USB_CP2110_PARITY_ODD;
      break;

    case SERIAL_PARITY_EVEN:
      report.parity = USB_CP2110_PARITY_EVEN;
      break;

    case SERIAL_PARITY_MARK:
      report.parity = USB_CP2110_PARITY_MARK;
      break;

    case SERIAL_PARITY_SPACE:
      report.parity = USB_CP2110_PARITY_SPACE;
      break;

    default:
      logMessage(LOG_WARNING, "unsupported CP2110 parity: %u", parity);
      errno = EINVAL;
      return 0;
  }

  switch (flowControl) {
    case SERIAL_FLOW_NONE:
      report.flowControl = USB_CP2110_FLOW_NONE;
      break;

    case SERIAL_FLOW_HARDWARE:
      report.flowControl = USB_CP2110_FLOW_HARDWARE;
      break;

    default:
      logMessage(LOG_WARNING, "unsupported CP2110 flow control: %u", flowControl);
      errno = EINVAL;
      return 0;
  }

  return usbSetReport_CP2110(device, &report, sizeof(report));
}

static int
usbSetUartStatus_CP2110 (UsbDevice *device, unsigned char status) {
  const unsigned char report[] = {0X41, status};
  return usbSetReport_CP2110(device, report, sizeof(report));
}

static int
usbEnableAdapter_CP2110 (UsbDevice *device) {
  if (usbSetUartStatus_CP2110(device, 0X01)) {
    return 1;
  }

  return 0;
}

static ssize_t
usbWriteData_CP2110 (UsbDevice *device, const void *data, size_t size) {
  const unsigned char *first = data;
  const unsigned char *next = first;

  while (size) {
    unsigned char report[0X40];
    size_t count = sizeof(report) - 1;

    if (count > size) count = size;
    report[0] = count;
    memcpy(&report[1], next, count);

    {
      ssize_t result = usbWriteEndpoint(device, 2, report, count+1, 1000);
      if (result == -1) return result;
    }

    next += count;
    size -= count;
  }

  return next - first;
}

static const UsbSerialOperations usbSerialOperations_CP2110 = {
  .setLineConfiguration = &usbSetLineConfiguration_CP2110,
  .enableAdapter = usbEnableAdapter_CP2110,
  .writeData = usbWriteData_CP2110
};


typedef struct {
  uint16_t vendor;
  uint16_t product;
  const UsbSerialOperations *operations;
  UsbInputFilter inputFilter;
} UsbSerialAdapter;

static const UsbSerialAdapter usbSerialAdapters[] = {
  { /* HandyTech GoHubs */
    .vendor=0X0921, .product=0X1200,
    .operations = &usbSerialOperations_Belkin
  },

  { /* HandyTech FTDI */
    .vendor=0X0403, .product=0X6001,
    .operations = &usbSerialOperations_FTDI_FT8U232AM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Papenmeier FTDI */
    .vendor=0X0403, .product=0XF208,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum Vario40 (40 cells) */
    .vendor=0X0403, .product=0XFE70,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum PocketVario (24 cells) */
    .vendor=0X0403, .product=0XFE71,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum SuperVario 40 (40 cells) */
    .vendor=0X0403, .product=0XFE72,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum SuperVario 32 (32 cells) */
    .vendor=0X0403, .product=0XFE73,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum SuperVario 64 (64 cells) */
    .vendor=0X0403, .product=0XFE74,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum SuperVario 80 (80 cells) */
    .vendor=0X0403, .product=0XFE75,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum VarioPro 80 (80 cells) */
    .vendor=0X0403, .product=0XFE76,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum VarioPro 64 (64 cells) */
    .vendor=0X0403, .product=0XFE77,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum VarioPro 40 (40 cells) */
    .vendor=0X0904, .product=0X2000,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum EcoVario 24 (24 cells) */
    .vendor=0X0904, .product=0X2001,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum EcoVario 40 (40 cells) */
    .vendor=0X0904, .product=0X2002,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum VarioConnect 40 (40 cells) */
    .vendor=0X0904, .product=0X2007,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum VarioConnect 32 (32 cells) */
    .vendor=0X0904, .product=0X2008,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum VarioConnect 24 (24 cells) */
    .vendor=0X0904, .product=0X2009,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum VarioConnect 64 (64 cells) */
    .vendor=0X0904, .product=0X2010,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum VarioConnect 80 (80 cells) */
    .vendor=0X0904, .product=0X2011,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum EcoVario 32 (32 cells) */
    .vendor=0X0904, .product=0X2014,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum EcoVario 64 (64 cells) */
    .vendor=0X0904, .product=0X2015,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum EcoVario 80 (80 cells) */
    .vendor=0X0904, .product=0X2016,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Baum Refreshabraille 18 (18 cells) */
    .vendor=0X0904, .product=0X3000,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  },

  { /* Seika (40 cells) */
    .vendor=0X10C4, .product=0XEA60,
    .operations = &usbSerialOperations_CP2101
  },

  { /* Seika NoteTaker */
    .vendor=0X10C4, .product=0XEA80,
    .operations = &usbSerialOperations_CP2110,
    .inputFilter = usbInputFilter_CP2110
  },

  {.vendor=0, .product=0}
};

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

int
usbSetSerialOperations (UsbDevice *device) {
  if (device->serialOperations) return 1;

  {
    const UsbSerialAdapter *sa = usbSerialAdapters;

    while (sa->vendor) {
      if (sa->vendor == getLittleEndian16(device->descriptor.idVendor)) {
        if (!sa->product || (sa->product == getLittleEndian16(device->descriptor.idProduct))) {
          if (sa->inputFilter && !usbAddInputFilter(device, sa->inputFilter)) return 0;
          device->serialOperations = sa->operations;
          return 1;
        }
      }

      sa += 1;
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
    logMessage(LOG_DEBUG, "USB: no serial operations: vendor=%04X product=%04X",
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
