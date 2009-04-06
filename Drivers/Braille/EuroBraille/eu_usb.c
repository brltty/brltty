/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
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

/* EuroBraille -- Input/Output handling
** USB Specific low-level IO routines.
*/

#include <string.h>

#include "prologue.h"

#include <errno.h>

#include "misc.h"

#include "eu_io.h"

#define USB_PACKET_SIZE 64


#ifdef ENABLE_USB_SUPPORT
#include "io_usb.h"

static UsbChannel *usb = NULL;
int
eubrl_usbInit (BrailleDisplay *brl, char **parameters, const char *device) {
  static const UsbChannelDefinition definitions[] = {
    { /* ESYS 12/40 (with SD-Card inserted) */
      .vendor=0XC251, .product=0X1122,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0,
      .disableAutosuspend=0
    }
    ,
    { /* ESYS 12/40 (with SD-Card not inserted) */
      .vendor=0XC251, .product=0X1124,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0,
      .disableAutosuspend=0
    }
    ,
    { .vendor=0 }
  };

  if ((usb = usbFindChannel(definitions, (void *)device))) 
    {
      return 1;
    }
  return 0;
}

int
eubrl_usbClose (BrailleDisplay *brl) 
{
  if (usb) {
    usbCloseChannel(usb);
    usb = NULL;
  }
  return 0;
}


ssize_t
eubrl_usbRead (BrailleDisplay *brl, void *buffer, size_t length) 
{
  ssize_t count=0;

  if(length>=USB_PACKET_SIZE) count = usbReapInput(usb->device, usb->definition.inputEndpoint, buffer, USB_PACKET_SIZE, 0,0);
  if(count>0 && count<USB_PACKET_SIZE)
    {
      LogPrint(LOG_DEBUG,"eu: We recieved a too small packet");
      return (-1);
    }
  return count;
}

ssize_t
eubrl_usbWrite(BrailleDisplay *brl, const void *buffer, size_t length)
{
  if(length>USB_PACKET_SIZE) return(-1);
  char packetToSend[USB_PACKET_SIZE];
  memset(packetToSend,0x55,USB_PACKET_SIZE);
  memcpy(packetToSend,buffer,length);
  return usbHidSetReport(usb->device, usb->definition.interface, 0, packetToSend, USB_PACKET_SIZE, 10);
}


#else /* No usb support - empty functions */


int
eubrl_usbInit (BrailleDisplay *brl, char **parameters, const char *device)
{
  return 0;
}

int
eubrl_usbClose (BrailleDisplay *brl) 
{
  return (0);
}


ssize_t
eubrl_usbRead (BrailleDisplay *brl, void *buffer, size_t length) 
{
  return -1;
}

ssize_t
eubrl_usbWrite(BrailleDisplay *brl, const void *buffer, size_t length)
{
  return -1;
}

#endif /* ENABLE_USB_SUPPORT */
