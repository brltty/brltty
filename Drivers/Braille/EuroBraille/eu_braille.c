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
  PARM_PROTOCOL
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
const t_eubrl_io *io = NULL;
static const t_eubrl_protocol *protocol = NULL;

static inline void
updateWriteDelay (BrailleDisplay *brl, size_t count) {
  brl->writeDelay += gioGetMillisecondsToTransfer(gioEndpoint, count);
}
static int
eubrl_genericAwaitInput (int timeout) {
  return gioAwaitInput(gioEndpoint, timeout);
}

static int
eubrl_genericReadByte (BrailleDisplay *brl, unsigned char *byte, int wait) {
  return gioReadByte(gioEndpoint, byte, wait);
}

static ssize_t
eubrl_genericWriteData (BrailleDisplay *brl, const void *data, size_t length) {
  updateWriteDelay(brl, length);
  return gioWriteData(gioEndpoint, data, length);
}

static ssize_t
eubrl_usbWriteData (BrailleDisplay *brl, const void *data, size_t length) {
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
  .awaitInput = eubrl_genericAwaitInput,
  .readByte = eubrl_genericReadByte,
  .writeData = eubrl_genericWriteData
};

static const t_eubrl_io	eubrl_usbIos = {
  .protocol = &esysirisProtocol,
  .awaitInput = eubrl_genericAwaitInput,
  .readByte = eubrl_genericReadByte,
  .writeData = eubrl_usbWriteData
};

static const t_eubrl_io	eubrl_bluetoothIos = {
  .protocol = &esysirisProtocol,
  .awaitInput = eubrl_genericAwaitInput,
  .readByte = eubrl_genericReadByte,
  .writeData = eubrl_genericWriteData
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
    io = gioGetApplicationData(gioEndpoint);
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
  io = NULL;
  protocol = NULL;
  makeOutputTable(dotsTable_ISO11548_1);

  if (parameters[PARM_PROTOCOL]) {
    static const char *const choices[] = {
      "auto", "clio", "esysiris",
      NULL
    };

    static const t_eubrl_protocol *const protocols[] = {
      NULL, &clioProtocol, &esysirisProtocol
    };

    unsigned int choice;

    if (!validateChoice(&choice, parameters[PARM_PROTOCOL], choices)) {
      logMessage(LOG_ERR, "unknown EuroBraille protocol: %s", 
                 parameters[PARM_PROTOCOL]);
      return 0;
    }

    protocol = protocols[choice];
  }

  if (connectResource(device)) {
    if (protocol) {
      if (!io->protocol || (io->protocol == protocol)) {
        if (protocol->init(brl)) return 1;
      } else {
        logMessage(LOG_ERR, "protocol not supported by device: %s", protocol->name);
      }
    } else if (io->protocol) {
      protocol = io->protocol;
      if (protocol->init(brl)) return 1;
    } else {
      static const t_eubrl_protocol *const protocols[] = {
        &esysirisProtocol, &clioProtocol,
        NULL
      };
      const t_eubrl_protocol *const *p = protocols;

      while (*p) {
        const t_eubrl_protocol *protocol = *p++;

        logMessage(LOG_NOTICE, "trying protocol: %s", protocol->name);
        if (protocol->init(brl)) return 1;
	approximateDelay(700);
      }
    }

    disconnectResource();
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) 
{
  if (protocol)
    {
      protocol = NULL;
    }
  disconnectResource();
}

#ifdef BRL_HAVE_PACKET_IO
static ssize_t
brl_readPacket (BrailleDisplay *brl, void *buffer, size_t size) 
{
  if (!protocol || !io)
    return (-1);
  return protocol->readPacket(brl, buffer, size);
}

static ssize_t
brl_writePacket (BrailleDisplay *brl, const void *packet, size_t length) 
{
  if (!protocol || !io)
    return (-1);
  return protocol->writePacket(brl, packet, length);
}

static int
brl_reset (BrailleDisplay *brl) 
{
  if (!protocol || !io)
    return (-1);
  return protocol->resetDevice(brl);
}
#endif /* BRL_HAVE_PACKET_IO */

#ifdef BRL_HAVE_KEY_CODES
static int
brl_readKey (BrailleDisplay *brl) 
{
  if (protocol)
    return protocol->readKey(brl);
  return EOF;
}

static int
brl_keyToCommand (BrailleDisplay *brl, KeyTableCommandContext context, int key) 
{
  if (protocol)
    return protocol->keyToCommand(brl, key, context);
  return BRL_CMD_NOOP;
}
#endif /* BRL_HAVE_KEY_CODES */

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  if (!protocol) return 1;

  if (text)
    if (!protocol->writeVisual(brl, text))
      return 0;

  return protocol->writeWindow(brl);
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) 
{
  if (protocol)
    return protocol->readCommand(brl, context);
  return EOF;
}
