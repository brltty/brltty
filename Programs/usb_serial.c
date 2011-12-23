/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2011 by The BRLTTY Developers.
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

static const UsbInterfaceDescriptor *
usbCommunicationInterfaceDescriptor (UsbDevice *device) {
  if (device->descriptor.bDeviceClass == 0X02) {
    const UsbDescriptor *descriptor = NULL;

    while (usbNextDescriptor(device, &descriptor)) {
      if (descriptor->interface.bDescriptorType == UsbDescriptorType_Interface)
        if (descriptor->interface.bInterfaceClass == 0X02)
          if (descriptor->interface.bInterfaceSubClass == 0X02)
            return &descriptor->interface;
    }
  }

  logMessage(LOG_WARNING, "USB: communication interface descriptor not found");
  errno = ENOENT;
  return NULL;
}

static int
usbEnableAdapter_CDC_ACM (UsbDevice *device) {
  const UsbInterfaceDescriptor *interface = usbCommunicationInterfaceDescriptor(device);

  if (interface) {
    ssize_t result = usbControlWrite(device, UsbControlRecipient_Interface, UsbControlType_Class,
                                     USB_CDC_ACM_CTL_SetControlLines,
                                     USB_CDC_ACM_LINE_DTR,
                                     interface->bInterfaceNumber,
                                     NULL, 0, 1000);

    if (result != -1) return 1;
  }

  return 0;
}

static const UsbSerialOperations usbSerialOperations_CDC_ACM = {
  .enableAdapter = usbEnableAdapter_CDC_ACM
};


static int
usbSetAttribute_Belkin (UsbDevice *device, unsigned char request, unsigned int value, unsigned int index) {
  logMessage(LOG_DEBUG, "Belkin Request: %02X %04X %04X", request, value, index);
  return usbControlWrite(device, UsbControlRecipient_Device, UsbControlType_Vendor,
                         request, value, index, NULL, 0, 1000) != -1;
}

static int
usbSetBaud_Belkin (UsbDevice *device, unsigned int rate) {
  const unsigned int base = 230400;
  if (base % rate) {
    logMessage(LOG_WARNING, "unsupported Belkin baud: %u", rate);
    errno = EINVAL;
    return 0;
  }
  return usbSetAttribute_Belkin(device, 0, base/rate, 0);
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
  .setFlowControl = usbSetFlowControl_Belkin,
  .setDataFormat = usbSetDataFormat_Belkin,
  .setDtrState = usbSetDtrState_Belkin,
  .setRtsState = usbSetRtsState_Belkin
};


static int
usbInputFilter_FTDI (UsbInputFilterData *data) {
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
usbSetBaud_FTDI_SIO (UsbDevice *device, unsigned int rate) {
  unsigned int divisor;
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
      logMessage(LOG_WARNING, "unsupported FTDI SIO baud: %u", rate);
      errno = EINVAL;
      return 0;
  }
  return usbSetBaud_FTDI(device, divisor);
}

static int
usbSetBaud_FTDI_FT8U232AM (UsbDevice *device, unsigned int rate) {
  if (rate > 3000000) {
    logMessage(LOG_WARNING, "unsupported FTDI FT8U232AM baud: %u", rate);
    errno = EINVAL;
    return 0;
  }
  {
    const unsigned int base = 48000000;
    unsigned int eighths = base / 2 / rate;
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
usbSetBaud_FTDI_FT232BM (UsbDevice *device, unsigned int rate) {
  if (rate > 3000000) {
    logMessage(LOG_WARNING, "unsupported FTDI FT232BM baud: %u", rate);
    errno = EINVAL;
    return 0;
  }
  {
    static const unsigned char mask[8] = {00, 03, 02, 04, 01, 05, 06, 07};
    const unsigned int base = 48000000;
    const unsigned int eighths = base / 2 / rate;
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
  .setFlowControl = usbSetFlowControl_FTDI,
  .setDataFormat = usbSetDataFormat_FTDI,
  .setDtrState = usbSetDtrState_FTDI,
  .setRtsState = usbSetRtsState_FTDI
};

static const UsbSerialOperations usbSerialOperations_FTDI_FT8U232AM = {
  .setBaud = usbSetBaud_FTDI_FT8U232AM,
  .setFlowControl = usbSetFlowControl_FTDI,
  .setDataFormat = usbSetDataFormat_FTDI,
  .setDtrState = usbSetDtrState_FTDI,
  .setRtsState = usbSetRtsState_FTDI
};

static const UsbSerialOperations usbSerialOperations_FTDI_FT232BM = {
  .setBaud = usbSetBaud_FTDI_FT232BM,
  .setFlowControl = usbSetFlowControl_FTDI,
  .setDataFormat = usbSetDataFormat_FTDI,
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
usbSetBaud_CP2101 (UsbDevice *device, unsigned int rate) {
  const unsigned int base = 0X384000;
  unsigned int divisor = base / rate;
  if ((rate * divisor) != base) {
    logMessage(LOG_WARNING, "unsupported CP2101 baud: %u", rate);
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
  .setFlowControl = usbSetFlowControl_CP2101,
  .setDataFormat = usbSetDataFormat_CP2101,
  .setDtrState = usbSetDtrState_CP2101,
  .setRtsState = usbSetRtsState_CP2101,
  .enableAdapter = usbEnableAdapter_CP2101
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
  }
  ,
  { /* HandyTech FTDI */
    .vendor=0X0403, .product=0X6001,
    .operations = &usbSerialOperations_FTDI_FT8U232AM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Papenmeier FTDI */
    .vendor=0X0403, .product=0XF208,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum Vario40 (40 cells) */
    .vendor=0X0403, .product=0XFE70,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum PocketVario (24 cells) */
    .vendor=0X0403, .product=0XFE71,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum SuperVario 40 (40 cells) */
    .vendor=0X0403, .product=0XFE72,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum SuperVario 32 (32 cells) */
    .vendor=0X0403, .product=0XFE73,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum SuperVario 64 (64 cells) */
    .vendor=0X0403, .product=0XFE74,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum SuperVario 80 (80 cells) */
    .vendor=0X0403, .product=0XFE75,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum VarioPro 80 (80 cells) */
    .vendor=0X0403, .product=0XFE76,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum VarioPro 64 (64 cells) */
    .vendor=0X0403, .product=0XFE77,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum VarioPro 40 (40 cells) */
    .vendor=0X0904, .product=0X2000,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum EcoVario 24 (24 cells) */
    .vendor=0X0904, .product=0X2001,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum EcoVario 40 (40 cells) */
    .vendor=0X0904, .product=0X2002,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum VarioConnect 40 (40 cells) */
    .vendor=0X0904, .product=0X2007,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum VarioConnect 32 (32 cells) */
    .vendor=0X0904, .product=0X2008,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum VarioConnect 24 (24 cells) */
    .vendor=0X0904, .product=0X2009,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum VarioConnect 64 (64 cells) */
    .vendor=0X0904, .product=0X2010,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum VarioConnect 80 (80 cells) */
    .vendor=0X0904, .product=0X2011,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum EcoVario 32 (32 cells) */
    .vendor=0X0904, .product=0X2014,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum EcoVario 64 (64 cells) */
    .vendor=0X0904, .product=0X2015,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum EcoVario 80 (80 cells) */
    .vendor=0X0904, .product=0X2016,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Baum Refreshabraille 18 (18 cells) */
    .vendor=0X0904, .product=0X3000,
    .operations = &usbSerialOperations_FTDI_FT232BM,
    .inputFilter = usbInputFilter_FTDI
  }
  ,
  { /* Seika (40 cells) */
    .vendor=0X10C4, .product=0XEA60,
    .operations = &usbSerialOperations_CP2101
  }
  ,
  { /* HumanWare (serial protocol) */
    .vendor=0X1C71, .product=0XC005,
    .operations = &usbSerialOperations_CDC_ACM
  }
  ,
  {.vendor=0, .product=0}
};

int
usbSetSerialOperations (UsbDevice *device) {
  if (!device->serialOperations) {
    const UsbSerialAdapter *sa = usbSerialAdapters;

    while (sa->vendor) {
      if (sa->vendor == device->descriptor.idVendor) {
        if (!sa->product || (sa->product == device->descriptor.idProduct)) {
          if (sa->inputFilter && !usbAddInputFilter(device, sa->inputFilter)) return 0;
          device->serialOperations = sa->operations;
          break;
        }
      }

      sa += 1;
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
               device->descriptor.idVendor, device->descriptor.idProduct);
    errno = ENOSYS;
    return 0;
  }

  if (!serial->setBaud(device, parameters->baud)) return 0;
  if (!serial->setFlowControl(device, parameters->flowControl)) return 0;
  if (!serial->setDataFormat(device, parameters->dataBits, parameters->stopBits, parameters->parity)) return 0;

  return 1;
}
