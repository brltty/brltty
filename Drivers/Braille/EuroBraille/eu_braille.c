/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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

/**
 ** brl.c -- EuroBraille core driver file.
 ** Made by Yannick PLASSIARD and Olivier BERT
 */

#include "prologue.h"

typedef enum {
  PARAM_PROTOCOLTYPE,
}		DriverParameter;

#define BRLPARMS "protocol"


#include <stdio.h>
#include <string.h>

#include "message.h"
#include "log.h"

#define BRL_HAVE_PACKET_IO
#include "brl_driver.h"
#include "parse.h"
#include "timing.h"
#include "io_generic.h"

#include	"eu_protocol.h"


/*
** Externs, declared in protocol.c and io.c
*/

static GioEndpoint *gioEndpoint = NULL;
const t_eubrl_io *iop = NULL;
static const t_eubrl_protocol *protocolp = NULL;

static inline void
updateWriteDelay (BrailleDisplay *brl, size_t count) {
  brl->writeDelay += gioGetMillisecondsToTransfer(gioEndpoint, count);
}

static ssize_t
eubrl_genericRead (BrailleDisplay *brl, void *buffer, size_t length, int wait) {
  return gioReadData(gioEndpoint, buffer, length, wait);
}

static ssize_t
eubrl_genericWrite (BrailleDisplay *brl, const void *data, size_t length) {
  updateWriteDelay(brl, length);
  return gioWriteData(gioEndpoint, data, length);
}

static ssize_t
eubrl_usbWrite (BrailleDisplay *brl, const void *data, size_t length) {
  const unsigned int USB_PACKET_SIZE = 64;
  size_t pos = 0;
  while (pos < length) {
    char packetToSend[USB_PACKET_SIZE];
    size_t tosend = length - pos;
    if (tosend > USB_PACKET_SIZE) {
      tosend = USB_PACKET_SIZE;
    }
    memset(packetToSend,0x55,USB_PACKET_SIZE);
    memcpy(packetToSend,data+pos,tosend);
    updateWriteDelay(brl, USB_PACKET_SIZE);
    if (gioSetHidReport(gioEndpoint, 0, packetToSend, USB_PACKET_SIZE) < 0) {
      return -1;
    }
    pos += tosend;
  }
  return length;
}

static const t_eubrl_io	eubrl_serialIos = {
  .read = eubrl_genericRead,
  .write = eubrl_genericWrite
};

static const t_eubrl_io	eubrl_usbIos = {
  .protocol = &esysirisProtocol,
  .read = eubrl_genericRead,
  .write = eubrl_usbWrite
};

static const t_eubrl_io	eubrl_bluetoothIos = {
  .protocol = &esysirisProtocol,
  .read = eubrl_genericRead,
  .write = eubrl_genericWrite
};

static int
connectResource (const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 9600,
    .parity = SERIAL_PARITY_EVEN
  };

  static const UsbChannelDefinition usbChannelDefinitions[] = {
    { /* Esys, version < 3.0, without SD card */
      .vendor=0XC251, .product=0X1122,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* reserved */
      .vendor=0XC251, .product=0X1123,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* Esys, version < 3.0, with SD card */
      .vendor=0XC251, .product=0X1124,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* reserved */
      .vendor=0XC251, .product=0X1125,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* Esys, version >= 3.0, without SD card */
      .vendor=0XC251, .product=0X1126,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* reserved */
      .vendor=0XC251, .product=0X1127,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* Esys, version >= 3.0, with SD card */
      .vendor=0XC251, .product=0X1128,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* reserved */
      .vendor=0XC251, .product=0X1129,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* Esytime */
      .vendor=0XC251, .product=0X1130,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* reserved */
      .vendor=0XC251, .product=0X1131,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* reserved */
      .vendor=0XC251, .product=0X1132,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { .vendor=0 }
  };

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.serial.parameters = &serialParameters;
  descriptor.serial.options.applicationData = &eubrl_serialIos;

  descriptor.usb.channelDefinitions = usbChannelDefinitions;
  descriptor.usb.options.applicationData = &eubrl_usbIos;

  descriptor.bluetooth.channelNumber = 1;
  descriptor.bluetooth.options.applicationData = &eubrl_bluetoothIos;

  if ((gioEndpoint = gioConnectResource(identifier, &descriptor))) {
    iop = gioGetApplicationData(gioEndpoint);
    if (iop->protocol) protocolp = iop->protocol;
    return 1;
  }

  return 0;
}

static void
disconnectResource (void) {
  if (gioEndpoint) {
    gioDisconnectResource(gioEndpoint);
    gioEndpoint = NULL;
  }
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) 
{
  protocolp = NULL;
  brl->textColumns = 0;
  iop = NULL;
  makeOutputTable(dotsTable_ISO11548_1);
  if (parameters[PARAM_PROTOCOLTYPE])
    {
      unsigned int choice = 0;
      char* choices[3];
      logMessage(LOG_DEBUG, "Detecting param %s", parameters[PARAM_PROTOCOLTYPE]);
      choices[0] = "clio";
      choices[1] = "esysiris";
      choices[2] = NULL;
      if (!validateChoice(&choice, parameters[PARAM_PROTOCOLTYPE], (const char **)choices))
	{
	  logMessage(LOG_ERR, "%s: unknown protocol type.", 
		     parameters[PARAM_PROTOCOLTYPE]);
	  return 0;
	}
      else if (choice == 0)
	protocolp = &clioProtocol;
      else if (choice == 1)
	protocolp = &esysirisProtocol;
      if (protocolp == NULL)
	{
	  logMessage(LOG_ERR, "eu: Undefined NULL protocol subsystem.");
	  return (0);
	}
    }
  if (strlen(parameters[PARAM_PROTOCOLTYPE]) == 0)
    protocolp = NULL;

  if (!connectResource(device))
    {
      logMessage(LOG_DEBUG, "eu: Failed to initialize IO subsystem.");
      return (0);
    }

  if (!protocolp) /* Autodetecting */
    { 
      protocolp = &esysirisProtocol;
      logMessage(LOG_INFO, "eu: Starting auto-detection process...");
      if (!protocolp->init(brl))
	{
	  logMessage(LOG_INFO, "eu: Esysiris detection failed.");
	  disconnectResource();
	  approximateDelay(700);
	  if (!connectResource(device))
	    {
	      logMessage(LOG_ERR, "Failed to initialize IO for second autodetection.");
	      return (0);
	    }
	  protocolp = &clioProtocol;
	  if (!protocolp->init(brl))
	    {
	      logMessage(LOG_ERR, "eu: Autodetection failed.");
	      disconnectResource();
	      return (0);
	    }
	}
    }
  else
    {
      logMessage(LOG_INFO, "initializing %s protocol", protocolp->name);
      if (!protocolp->init(brl))
	{
	  logMessage(LOG_ERR, "eu: Unable to connect to Braille display.");
	  disconnectResource();
	  return (0);
	}      
    }
  logMessage(LOG_INFO, "EuroBraille driver initialized: %d display length connected", brl->textColumns);
  return (1);
}

static void
brl_destruct (BrailleDisplay *brl) 
{
  if (protocolp)
    {
      protocolp = NULL;
    }
  disconnectResource();
}

#ifdef BRL_HAVE_PACKET_IO
static ssize_t
brl_readPacket (BrailleDisplay *brl, void *buffer, size_t size) 
{
  if (!protocolp || !iop)
    return (-1);
  return protocolp->readPacket(brl, buffer, size);
}

static ssize_t
brl_writePacket (BrailleDisplay *brl, const void *packet, size_t length) 
{
  if (!protocolp || !iop)
    return (-1);
  return protocolp->writePacket(brl, packet, length);
}

static int
brl_reset (BrailleDisplay *brl) 
{
  if (!protocolp || !iop)
    return (-1);
  return protocolp->reset(brl);
}
#endif /* BRL_HAVE_PACKET_IO */

#ifdef BRL_HAVE_KEY_CODES
static int
brl_readKey (BrailleDisplay *brl) 
{
  if (protocolp)
    return protocolp->readKey(brl);
  return EOF;
}

static int
brl_keyToCommand (BrailleDisplay *brl, KeyTableCommandContext context, int key) 
{
  if (protocolp)
    return protocolp->keyToCommand(brl, key, context);
  return BRL_CMD_NOOP;
}
#endif /* BRL_HAVE_KEY_CODES */

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) 
{
  if (!protocolp)
    return 1;
  if (text)
    protocolp->writeVisual(brl, text);  
  protocolp->writeWindow(brl);
  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) 
{
  if (protocolp)
    return protocolp->readCommand(brl, context);
  return EOF;
}
