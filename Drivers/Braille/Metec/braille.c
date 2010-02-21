/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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
#include "misc.h"

#include "brl_driver.h"
#include "brldefs-mt.h"

BEGIN_KEY_NAME_TABLE(all)
  KEY_NAME_ENTRY(MT_KEY_LeftRear, "LeftRear"),
  KEY_NAME_ENTRY(MT_KEY_LeftMiddle, "LeftMiddle"),
  KEY_NAME_ENTRY(MT_KEY_LeftFront, "LeftFront"),
  KEY_NAME_ENTRY(MT_KEY_RightRear, "RightRear"),
  KEY_NAME_ENTRY(MT_KEY_RightMiddle, "RightMiddle"),
  KEY_NAME_ENTRY(MT_KEY_RightFront, "RightFront"),

  KEY_NAME_ENTRY(MT_KEY_CursorLeft, "CursorLeft"),
  KEY_NAME_ENTRY(MT_KEY_CursorUp, "CursorUp"),
  KEY_NAME_ENTRY(MT_KEY_CursorRight, "CursorRight"),
  KEY_NAME_ENTRY(MT_KEY_CursorDown, "CursorDown"),

  KEY_SET_ENTRY(MT_SET_RoutingKeys1, "RoutingKey1"),
  KEY_SET_ENTRY(MT_SET_RoutingKeys2, "RoutingKey2"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(all),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
END_KEY_TABLE_LIST

#define MT_INPUT_PACKET_LENGTH 8

#define MT_ROUTING_KEYS_SECONDARY 100
#define MT_ROUTING_KEYS_NONE 0XFF

#define MT_MODULE_SIZE 8
#define MT_MODULES_MAXIMUM 10

static unsigned char textCells[MT_MODULES_MAXIMUM * MT_MODULE_SIZE];
static int forceWrite;
static uint16_t lastNavigationKeys;
static unsigned char lastRoutingKey;
static TranslationTable outputTable;

#ifdef ENABLE_USB_SUPPORT
#include "io_usb.h"

#define MT_REQUEST_RECIPIENT UsbControlRecipient_Device
#define MT_REQUEST_TYPE UsbControlType_Vendor
#define MT_REQUEST_TIMEOUT 1000

static UsbChannel *usbChannel = NULL;

static int
readDevice (unsigned char request, void *buffer, int length) {
  return usbControlRead(usbChannel->device, MT_REQUEST_RECIPIENT, MT_REQUEST_TYPE,
                        request, 0, 0, buffer, length, MT_REQUEST_TIMEOUT);
}

static int
writeDevice (unsigned char request, const void *buffer, int length) {
  return usbControlWrite(usbChannel->device, MT_REQUEST_RECIPIENT, MT_REQUEST_TYPE,
                         request, 0, 0, buffer, length, MT_REQUEST_TIMEOUT);
}

static int
getDeviceIdentity (char *buffer, int *length) {
  int result;

  {
    static const unsigned char data[1] = {0};
    result = writeDevice(0X04, data, sizeof(data));
    if (result == -1) return 0;
  }

  result = usbReadEndpoint(usbChannel->device, usbChannel->definition.inputEndpoint,
                           buffer, *length, MT_REQUEST_TIMEOUT);
  if (result == -1) return 0;

  LogPrint(LOG_INFO, "Device Identity: %.*s", result, buffer);
  *length = result;
  return 1;
}

static int
setHighVoltage (int on) {
  const unsigned char data[1] = {on? 0XFF: 0X7F};
  return writeDevice(0X01, data, sizeof(data)) != -1;
}

static int
getInputPacket (unsigned char *packet) {
  return readDevice(0X80, packet, MT_INPUT_PACKET_LENGTH);
}
#endif /* ENABLE_USB_SUPPORT */

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  forceWrite = 1;
  lastNavigationKeys = 0;
  lastRoutingKey = MT_ROUTING_KEYS_NONE;

  {
    static const DotsTable dots = {0X80, 0X40, 0X20, 0X10, 0X08, 0X04, 0X02, 0X01};
    makeOutputTable(dots, outputTable);
  }

#ifdef ENABLE_USB_SUPPORT
  if (isUsbDevice(&device)) {
    static const UsbChannelDefinition definitions[] = {
      {
        .vendor=0X0452, .product=0X0100,
        .configuration=1, .interface=0, .alternative=0,
        .inputEndpoint=1, .outputEndpoint=0
      }
      ,
      { .vendor=0 }
    };

    if ((usbChannel = usbFindChannel(definitions, (void *)device))) {
      char identity[100];
      int identityLength;
      int retries = -1;

      do {
        identityLength = sizeof(identity);
        if (getDeviceIdentity(identity, &identityLength)) break;
        identityLength = 0;

        if (errno != EAGAIN) {
          if (errno != EILSEQ) break;
          retries = MIN(retries, 1);
        }
      } while (retries-- > 0);

      if (identityLength || (retries < -1)) {
        if (setHighVoltage(1)) {
          unsigned char inputPacket[MT_INPUT_PACKET_LENGTH];

          if (getInputPacket(inputPacket)) {
            brl->textColumns = inputPacket[1];
            brl->textRows = 1;

            {
              const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(all);

              brl->keyBindings = ktd->bindings;
              brl->keyNameTables = ktd->names;
            }

            return 1;
          }

          setHighVoltage(0);
        }
      }

      usbCloseChannel(usbChannel);
      usbChannel = NULL;
    }
  } else
#endif /* ENABLE_USB_SUPPORT */

  {
    unsupportedDevice(device);
  }
  
  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
#ifdef ENABLE_USB_SUPPORT
  if (usbChannel) {
    setHighVoltage(0);
    usbCloseChannel(usbChannel);
    usbChannel = NULL;
  }
#endif /* ENABLE_USB_SUPPORT */
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  const unsigned char *source = brl->buffer;
  unsigned char *target = textCells;
  const int moduleCount = brl->textColumns / MT_MODULE_SIZE;
  int moduleNumber;

  for (moduleNumber=0; moduleNumber<moduleCount; moduleNumber+=1) {
    if (forceWrite || (memcmp(target, source, MT_MODULE_SIZE) != 0)) {
      unsigned char buffer[MT_MODULE_SIZE];

      {
        int i;
        for (i=0; i<MT_MODULE_SIZE; i+=1) buffer[i] = outputTable[source[i]];
      }

#ifdef ENABLE_USB_SUPPORT
      if (writeDevice(0X0A+moduleNumber, buffer, sizeof(buffer)) == -1) return 0;
#endif /* ENABLE_USB_SUPPORT */

      memcpy(target, source, MT_MODULE_SIZE);
    }

    source += MT_MODULE_SIZE;
    target += MT_MODULE_SIZE;
  }

  forceWrite = 0;
  return 1;
}

static void
routingKeyEvent (BrailleDisplay *brl, unsigned char key, int press) {
  if (key != MT_ROUTING_KEYS_NONE) {
    MT_KeySet set;

    if (key < brl->textColumns) {
      set = MT_SET_RoutingKeys1;
    } else if ((key >= MT_ROUTING_KEYS_SECONDARY) &&
        (key < (MT_ROUTING_KEYS_SECONDARY + brl->textColumns))) {
      key -= MT_ROUTING_KEYS_SECONDARY;
      set = MT_SET_RoutingKeys2;
    } else {
      LogPrint(LOG_WARNING, "unexpected routing key: %u", key);
      return;
    }

    enqueueKeyEvent(set, key, press);
  }
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  unsigned char packet[MT_INPUT_PACKET_LENGTH];
  memset(packet, 0, sizeof(packet));

#ifdef ENABLE_USB_SUPPORT
  if (!getInputPacket(packet)) return BRL_CMD_RESTARTBRL;
#endif /* ENABLE_USB_SUPPORT */

  logInputPacket(packet, sizeof(packet));

  {
    unsigned char key = packet[0];

    if (key != lastRoutingKey) {
      routingKeyEvent(brl, lastRoutingKey, 0);
      routingKeyEvent(brl, key, 1);
      lastRoutingKey = key;
    }
  }

  {
    uint16_t keys = packet[2] | (packet[3] << 8);
    uint16_t bit = 0X1;
    unsigned char key = 0;
    const unsigned char set = MT_SET_NavigationKeys;
    unsigned char pressTable[0X10];
    unsigned char pressCount = 0;

    while (bit) {
      if ((lastNavigationKeys & bit) && !(keys & bit)) {
        enqueueKeyEvent(set, key, 0);
        lastNavigationKeys &= ~bit;
      } else if (!(lastNavigationKeys & bit) && (keys & bit)) {
        pressTable[pressCount++] = key;
        lastNavigationKeys |= bit;
      }
      while (pressCount) enqueueKeyEvent(set, pressTable[--pressCount], 1);

      key += 1;
      bit <<= 1;
    }
  }

  return EOF;
}
