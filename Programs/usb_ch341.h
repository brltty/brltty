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

#ifndef BRLTTY_INCLUDED_USB_CH341
#define BRLTTY_INCLUDED_USB_CH341

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  USB_CH341_REQ_READ_VERSION   = 0X5F,
  USB_CH341_REQ_READ_REGISTER  = 0X95,
  USB_CH341_REQ_WRITE_REGISTER = 0X9A,
  USB_CH341_REQ_INITIALIZE     = 0XA1,
  USB_CH341_REQ_MODEM_CCONTROL = 0XA4,
} USB_CH341_Request;

typedef enum {
  USB_CH341_REG_BREAK     = 0X05,
  USB_CH341_REG_PRESCALER = 0X12,
  USB_CH341_REG_DIVISOR   = 0X13,
  USB_CH341_REG_BPS_MOD   = 0X14,
  USB_CH341_REG_LCR1      = 0X18,
  USB_CH341_REG_LCR2      = 0X25,
} USB_CH341_Register;

typedef enum {
  USB_CH341_LCR_RECEIVE_ENABLE  = 0X80,
  USB_CH341_LCR_TRANSMIT_ENABLE = 0X40,
  USB_CH341_LCR_MARK_SPACE      = 0X20,
  USB_CH341_LCR_PARITY_EVEN     = 0X10,
  USB_CH341_LCR_PARITY_ENABLE   = 0X08,
  USB_CH341_LCR_STOP_BITS_2     = 0X04,
  USB_CH341_LCR_DATA_BITS_MASK  = 0X03,

  USB_CH341_LCR_DATA_BITS_5     = 0X00,
  USB_CH341_LCR_DATA_BITS_6     = 0X01,
  USB_CH341_LCR_DATA_BITS_7     = 0X02,
  USB_CH341_LCR_DATA_BITS_8     = 0X03,
} USB_CH341_LCRBits;

typedef enum {
  USB_CH341_MODEM_CTS = 0X01, // clear to send
  USB_CH341_MODEM_DSR = 0X02, // data set ready
  USB_CH341_MODEM_RI  = 0X04, // ring indicator
  USB_CH341_MODEM_DCD = 0X08, // data carrier detect
} USB_CH341_ModemBits;

typedef enum {
  USB_CH341_PS_DISABLE_8x  = 0X01,
  USB_CH341_PS_DISABLE_64x = 0X02,
  USB_CH341_PS_DISABLE_2x  = 0X04,
  USB_CH341_PS_IMMEDIATE   = 0X80, // don't wait till there are 32 bytes
} USB_CH341_PrescalerBits;

#define USB_CH341_FREQUENCY 12000000

#define USB_CH341_BAUD_MINIMUM 46
#define USB_CH341_BAUD_MAXIMUM 2000000

#define USB_CH341_DIVISOR_MINIMUM 2
#define USB_CH341_DIVISOR_MAXIMUM 256

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_USB_CH341 */
