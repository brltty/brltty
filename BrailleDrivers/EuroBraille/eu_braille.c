/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
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
#include "misc.h"

#define BRL_HAVE_PACKET_IO
#define BRL_HAVE_VISUAL_DISPLAY
#include "brl_driver.h"
#include "misc.h"
#ifdef ENABLE_USB_SUPPORT
#include "io_usb.h"
#endif /* ENABLE_USB_SUPPORT */

#ifdef ENABLE_BLUETOOTH_SUPPORT
#include "io_bluetooth.h"
#endif /* ENABLE_BLUETOOTH_SUPPORT */

#include "io_serial.h"

#include	"eu_protocol.h"


/*
** Externs, declared in protocol.c and io.c
*/

extern t_eubrl_protocol esysirisProtocol, clioProtocol;
extern t_eubrl_io eubrl_usbIos, eubrl_serialIos;
extern t_eubrl_io eubrl_bluetoothIos, eubrl_ethernetIos;


static TranslationTable outputTable;
static t_eubrl_io *iop = NULL;
static t_eubrl_protocol *protocolp = NULL;

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) 
{
  protocolp = NULL;
  brl->x = 0;
  iop = NULL;
  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(dots, outputTable);
  }
  if (parameters[PARAM_PROTOCOLTYPE])
    {
      unsigned int choice = 0;
      char* choices[3];
      LogPrint(LOG_DEBUG, "Detecting param %s", parameters[PARAM_PROTOCOLTYPE]);
      choices[0] = "clio";
      choices[1] = "esysiris";
      choices[2] = NULL;
      if (!validateChoice(&choice, parameters[PARAM_PROTOCOLTYPE], (const char **)choices))
	{
	  LogPrint(LOG_ERR, "%s: unknown protocol type.", 
		   parameters[PARAM_PROTOCOLTYPE]);
	  return 0;
	}
      else if (choice == 0)
	protocolp = &clioProtocol;
      else if (choice == 1)
	protocolp = &esysirisProtocol;
      if (protocolp == NULL)
	{
	  LogPrint(LOG_ERR, "eu: Undefined NULL protocol subsystem.");
	  return (0);
	}
    }
  if (strlen(parameters[PARAM_PROTOCOLTYPE]) == 0)
    protocolp = NULL;

  /*
  ** Now, let's select and initialize our IO subsystem 
  */ 
  if (isSerialDevice(&device))
    {
      iop = &eubrl_serialIos;
    }

#ifdef ENABLE_USB_SUPPORT
  else if (isUsbDevice(&device))
    {
      iop = &eubrl_usbIos;
      protocolp = &esysirisProtocol;
    }
#endif /* ENABLE_USB_SUPPORT */

#ifdef ENABLE_BLUETOOTH_SUPPORT
  else if (isBluetoothDevice(&device))
    {
      iop = &eubrl_bluetoothIos;
      protocolp = &esysirisProtocol;
    }
#endif /* ENABLE_BLUETOOTH_SUPPORT */

  else if (!strncasecmp(device, "net:", 4))
    {
      iop = &eubrl_ethernetIos;
      protocolp = &esysirisProtocol;
    }
  if (!iop)
    {
      unsupportedDevice(device);
      return (0);
    }
  if (iop->init(brl, parameters, device) == 0)
    {
      LogPrint(LOG_ERR, "eu: Failed to initialize IO subsystem.");
      return (0);
    }

  if (!protocolp) /* Autodetecting */
    { 
      protocolp = &esysirisProtocol;
      LogPrint(LOG_INFO, "eu: Starting auto-detection process...");
      if (protocolp->init(brl, iop) == 0)
	{
	  LogPrint(LOG_INFO, "eu: Esysiris detection failed.");
	  iop->close(brl);
	  approximateDelay(700);
	  if (iop->init(brl, parameters, device) == 0)
	    {
	      LogPrint(LOG_ERR, "Failed to initialize IO for second autodetection.");
	      return (0);
	    }
	  protocolp = &clioProtocol;
	  if (protocolp->init(brl, iop) == 0)
	    {
	      LogPrint(LOG_ERR, "eu: Autodetection failed.");
	      iop->close(brl);
	      return (0);
	    }
	}
    }
  else
    {
      if (protocolp->protocolType == CLIO_PROTOCOL)
	LogPrint(LOG_INFO, "Initializing clio protocol.");
      else
	LogPrint(LOG_INFO, "Initializing EsysIris protocol.");
      if (protocolp->init(brl, iop) == 0)
	{
	  LogPrint(LOG_ERR, "eu: Unable to connect to Braille display.");
	  iop->close(brl);
	  return (0);
	}      
    }
  LogPrint(LOG_INFO, "EuroBraille driver initialized: %d display length connected", brl->x);
  brl->helpPage = 0;
  return (1);
}

static void
brl_destruct (BrailleDisplay *brl) 
{
  if (protocolp)
    {
      protocolp = NULL;
    }
  if (iop)
    {
      iop->close(brl);
      iop = NULL;
    }
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
brl_keyToCommand (BrailleDisplay *brl, BRL_DriverCommandContext context, int key) 
{
  if (protocolp)
    return protocolp->keyToCommand(brl, key, context);
  return BRL_CMD_NOOP;
}
#endif /* BRL_HAVE_KEY_CODES */

static int
brl_writeWindow (BrailleDisplay *brl) 
{
  if (!protocolp)
    return 1;
  protocolp->writeWindow(brl);
  return 1;
}

#ifdef BRL_HAVE_VISUAL_DISPLAY
static int
brl_writeVisual (BrailleDisplay *brl) 
{
  if (!protocolp)
    return 1;
  protocolp->writeVisual(brl);  
  return 1;
}
#endif /* BRL_HAVE_VISUAL_DISPLAY */

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) 
{
  if (protocolp)
    return protocolp->readCommand(brl, context);
  return EOF;
}
