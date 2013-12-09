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

#ifndef BRLTTY_INCLUDED_USB_CP2101
#define BRLTTY_INCLUDED_USB_CP2101

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_USB_CP2101 */
