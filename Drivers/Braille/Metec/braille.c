/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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
#include "parameters.h"
#include "async_alarm.h"

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

typedef struct {
  int (*beginProtocol) (BrailleDisplay *brl);
  void (*endProtocol) (BrailleDisplay *brl);

  int (*getDeviceIdentity) (BrailleDisplay *brl, char *buffer, size_t *length);
  int (*setHighVoltage) (BrailleDisplay *brl, int on);
} ProtocolOperations;

struct BrailleDataStruct {
  const ProtocolOperations *protocol;

  unsigned char textCells[MT_MODULES_MAXIMUM * MT_MODULE_SIZE];
  int forceWrite[MT_MODULES_MAXIMUM];
  unsigned int moduleCount;

  uint32_t lastNavigationKeys;
  unsigned char lastRoutingKey;

  union {
    struct {
      AsyncHandle inputAlarm;
    } usb;
  } proto;
};

#define MT_REQUEST_RECIPIENT UsbControlRecipient_Device
#define MT_REQUEST_TYPE UsbControlType_Vendor

static void
handleRoutingKeyEvent (BrailleDisplay *brl, unsigned char key, int press) {
  if (key != MT_ROUTING_KEYS_NONE) {
    MT_KeySet set;

    if (key < brl->textColumns) {
      set = MT_SET_RoutingKeys1;
    } else if ((key >= MT_ROUTING_KEYS_SECONDARY) &&
        (key < (MT_ROUTING_KEYS_SECONDARY + brl->textColumns))) {
      key -= MT_ROUTING_KEYS_SECONDARY;
      set = MT_SET_RoutingKeys2;
    } else {
      logMessage(LOG_WARNING, "unexpected routing key: %u", key);
      return;
    }

    enqueueKeyEvent(brl, set, key, press);
  }
}

static void
handleRoutingKey (BrailleDisplay *brl, unsigned char key) {
  if (key != brl->data->lastRoutingKey) {
    handleRoutingKeyEvent(brl, brl->data->lastRoutingKey, 0);
    handleRoutingKeyEvent(brl, key, 1);
    brl->data->lastRoutingKey = key;
  }
}

static ssize_t
tellUsbDevice (BrailleDisplay *brl, unsigned char request, const void *buffer, int length) {
  return gioTellResource(brl->gioEndpoint, MT_REQUEST_RECIPIENT, MT_REQUEST_TYPE,
                         request, 0, 0, buffer, length);
}

static ssize_t
askUsbDevice (BrailleDisplay *brl, unsigned char request, void *buffer, int length) {
  return gioAskResource(brl->gioEndpoint, MT_REQUEST_RECIPIENT, MT_REQUEST_TYPE,
                        request, 0, 0, buffer, length);
}

static int
getUsbInputPacket (BrailleDisplay *brl, unsigned char *packet) {
  return askUsbDevice(brl, 0X80, packet, MT_INPUT_PACKET_LENGTH);
}

static int setUsbInputAlarm (BrailleDisplay *brl);

ASYNC_ALARM_CALLBACK(handleUsbInputAlarm) {
  BrailleDisplay *brl = parameters->data;
  unsigned char packet[MT_INPUT_PACKET_LENGTH];

  asyncDiscardHandle(brl->data->proto.usb.inputAlarm);
  brl->data->proto.usb.inputAlarm = NULL;

  memset(packet, 0, sizeof(packet));

  if (getUsbInputPacket(brl, packet))  {
    logInputPacket(packet, sizeof(packet));
    handleRoutingKey(brl, packet[0]);

    {
      uint32_t keys = packet[2] | (packet[3] << 8);

      enqueueUpdatedKeys(brl, keys, &brl->data->lastNavigationKeys, MT_SET_NavigationKeys, 0);
    }

    setUsbInputAlarm(brl);
  } else {
    enqueueCommand(BRL_CMD_RESTARTBRL);
  }
}

static int
setUsbInputAlarm (BrailleDisplay *brl) {
  return asyncSetAlarmIn(&brl->data->proto.usb.inputAlarm,
                         BRAILLE_INPUT_POLL_INTERVAL,
                         handleUsbInputAlarm, brl);
}

static int
beginUsbProtocol (BrailleDisplay *brl) {
  brl->data->proto.usb.inputAlarm = NULL;
  setUsbInputAlarm(brl);

  return 1;
}

static void
endUsbProtocol (BrailleDisplay *brl) {
  if (brl->data->proto.usb.inputAlarm) {
    asyncCancelRequest(brl->data->proto.usb.inputAlarm);
    brl->data->proto.usb.inputAlarm = NULL;
  }
}

static int
getUsbDeviceIdentity (BrailleDisplay *brl, char *buffer, size_t *length) {
  ssize_t result;

  {
    static const unsigned char data[1] = {0};

    result = tellUsbDevice(brl, 0X04, data, sizeof(data));
    if (result == -1) return 0;
  }

  result = gioReadData(brl->gioEndpoint, buffer, *length, 0);
  if (result == -1) return 0;
  logMessage(LOG_INFO, "Device Identity: %.*s", (int)result, buffer);

  *length = result;
  return 1;
}

static int
setUsbHighVoltage (BrailleDisplay *brl, int on) {
  const unsigned char data[1] = {on? 0XFF: 0X7F};

  return tellUsbDevice(brl, 0X01, data, sizeof(data)) != -1;
}

static const ProtocolOperations usbProtocolOperations = {
  .beginProtocol = beginUsbProtocol,
  .endProtocol = endUsbProtocol,

  .getDeviceIdentity = getUsbDeviceIdentity,
  .setHighVoltage = setUsbHighVoltage
};

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  static const UsbChannelDefinition usbChannelDefinitions[] = {
    { /* all models */
      .vendor=0X0452, .product=0X0100,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    },

    { .vendor=0 }
  };

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.usb.channelDefinitions = usbChannelDefinitions;
  descriptor.usb.options.applicationData = &usbProtocolOperations;

  if (connectBrailleResource(brl, identifier, &descriptor, NULL)) {
    brl->data->protocol = gioGetApplicationData(brl->gioEndpoint);
    return 1;
  }

  return 0;
}

static void
disconnectResource (BrailleDisplay *brl) {
  disconnectBrailleResource(brl, NULL);
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));
    brl->data->lastNavigationKeys = 0;
    brl->data->lastRoutingKey = MT_ROUTING_KEYS_NONE;

    if (connectResource(brl, device)) {
      {
        char identity[100];
        size_t length = sizeof(identity);

        if (brl->data->protocol->getDeviceIdentity(brl, identity, &length)) {
        }
      }

      if (brl->data->protocol->setHighVoltage(brl, 1)) {
        unsigned char inputPacket[MT_INPUT_PACKET_LENGTH];

        if (getUsbInputPacket(brl, inputPacket)) {
          brl->textColumns = inputPacket[1];
          brl->data->moduleCount = brl->textColumns / MT_MODULE_SIZE;

          {
            unsigned int moduleNumber;

            for (moduleNumber=0; moduleNumber<brl->data->moduleCount; moduleNumber+=1) {
              brl->data->forceWrite[moduleNumber] = 1;
            }
          }

          {
            static const DotsTable dots = {
              0X80, 0X40, 0X20, 0X10, 0X08, 0X04, 0X02, 0X01
            };

            makeOutputTable(dots);
          }

          {
            const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(all);

            brl->keyBindings = ktd->bindings;
            brl->keyNameTables = ktd->names;
          }

          if (brl->data->protocol->beginProtocol(brl)) return 1;
        }

        brl->data->protocol->setHighVoltage(brl, 0);
      }

      disconnectResource(brl);
    }

    free(brl->data);
  } else {
    logMallocError();
  }
  
  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  brl->data->protocol->endProtocol(brl);
  brl->data->protocol->setHighVoltage(brl, 0);
  disconnectResource(brl);
  free(brl->data);
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  const unsigned char *source = brl->buffer;
  unsigned char *target = brl->data->textCells;
  unsigned int moduleNumber;

  for (moduleNumber=0; moduleNumber<brl->data->moduleCount; moduleNumber+=1) {
    if (cellsHaveChanged(target, source, MT_MODULE_SIZE, NULL, NULL, &brl->data->forceWrite[moduleNumber])) {
      unsigned char buffer[MT_MODULE_SIZE];

      translateOutputCells(buffer, source, MT_MODULE_SIZE);
      if (tellUsbDevice(brl, 0X0A+moduleNumber, buffer, sizeof(buffer)) == -1) return 0;
    }

    source += MT_MODULE_SIZE;
    target += MT_MODULE_SIZE;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  return EOF;
}
