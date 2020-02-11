/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2020 by The BRLTTY Developers.
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

#include "prologue.h"

#include <string.h>
#include <errno.h>

#include "log.h"
#include "crc.h"
#include "async.h"
#include "async_alarm.h"

#include "brl_driver.h"
#include "brldefs-cn.h"

#define PROBE_RETRY_LIMIT 0
#define PROBE_INPUT_TIMEOUT 1000
#define POLL_ALARM_INTERVAL 100
#define MAXIMUM_RESPONSE_SIZE 3

BEGIN_KEY_NAME_TABLE(navigation)
  KEY_NAME_ENTRY(CN_KEY_Help, "Help"),
  KEY_NAME_ENTRY(CN_KEY_Refresh, "Refresh"),

  KEY_NAME_ENTRY(CN_KEY_Line1, "Line1"),
  KEY_NAME_ENTRY(CN_KEY_Line2, "Line2"),
  KEY_NAME_ENTRY(CN_KEY_Line3, "Line3"),
  KEY_NAME_ENTRY(CN_KEY_Line4, "Line4"),
  KEY_NAME_ENTRY(CN_KEY_Line5, "Line5"),
  KEY_NAME_ENTRY(CN_KEY_Line6, "Line6"),
  KEY_NAME_ENTRY(CN_KEY_Line7, "Line7"),
  KEY_NAME_ENTRY(CN_KEY_Line8, "Line8"),
  KEY_NAME_ENTRY(CN_KEY_Line9, "Line9"),

  KEY_NAME_ENTRY(CN_KEY_Back, "Back"),
  KEY_NAME_ENTRY(CN_KEY_Menu, "Menu"),
  KEY_NAME_ENTRY(CN_KEY_Forward, "Forward"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(navigation),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
END_KEY_TABLE_LIST

typedef struct {
  unsigned char isNew:1;
  unsigned char force;
  unsigned char cells[];
} RowEntry;

typedef BrailleResponseResult IdentityResponseHandler (
  BrailleDisplay *brl,
  const unsigned char *response, size_t size
);

struct BrailleDataStruct {
  IdentityResponseHandler *handleIdentityResponse;
  CRCGenerator *crcGenerator;
  AsyncHandle pollAlarm;

  struct {
    KeyNumberSet keys;
  } input;

  struct {
    RowEntry **rowEntries;
  } output;
};

#define PACKET_ESCAPE_BYTE 0X7D
#define PACKET_FRAMING_BYTE 0X7E

static uint16_t
makePacketChecksum (BrailleDisplay *brl, const void *packet, size_t size) {
  CRCGenerator *crc = brl->data->crcGenerator;
  crcResetGenerator(crc);
  crcAddData(crc, packet, size);
  return crcGetChecksum(crc);
}

static BraillePacketVerifierResult
verifyPacket (
  BrailleDisplay *brl,
  const unsigned char *bytes, size_t size,
  size_t *length, void *data
) {
  unsigned char byte = bytes[size-1];

  switch (size) {
    case 1:
      switch (byte) {
        case CN_CMD_COLUMN_COUNT:
        case CN_CMD_ROW_COUNT:
        case CN_CMD_SEND_ROW:
        case CN_CMD_KEYS_STATE:
        case CN_CMD_DEVICE_STATE:
          *length = 3;
          break;

        default:
          return BRL_PVR_INVALID;
      }
      break;

    default:
      break;
  }

  return BRL_PVR_INCLUDE;
}

static size_t
readPacket (BrailleDisplay *brl, void *packet, size_t size) {
  return readBraillePacket(brl, NULL, packet, size, verifyPacket, NULL);
}

typedef uint16_t ResponseResult;

static ResponseResult
getResponseResult (const unsigned char *response) {
  return response[1] | (response[2] << 8);
}

static int
writePacket (BrailleDisplay *brl, const unsigned char *packet, size_t size) {
  logBytes(LOG_CATEGORY(OUTPUT_PACKETS), "raw", packet, size);

  unsigned char buffer[1 + ((size + 2) * 2) + 1];
  unsigned char *target = buffer;
  *target++ = PACKET_FRAMING_BYTE;

  const unsigned char *source = packet;
  const unsigned char *end = source + size;

  while (source < end) {
    unsigned char byte = *source++;

    if ((byte == PACKET_ESCAPE_BYTE) || (byte == PACKET_FRAMING_BYTE)) {
      *target++ = PACKET_ESCAPE_BYTE;
      byte ^= 0X20;
    }

    *target++ = byte;
  }

  {
    uint16_t checksum = makePacketChecksum(brl, packet, size);
    *target++ = checksum & UINT8_MAX;
    *target++ = checksum >> 8;
  }

  *target++ = PACKET_FRAMING_BYTE;
  size = target - buffer;
  return writeBraillePacket(brl, NULL, buffer, size);
}

static int
writeCommand (BrailleDisplay *brl, unsigned char command) {
  unsigned char packet[] = {command};
  return writePacket(brl, packet, sizeof(packet));
}

ASYNC_ALARM_CALLBACK(CN_handlePollAlarm) {
}

static void
stopPollAlarm (BrailleDisplay *brl) {
  AsyncHandle *alarm = &brl->data->pollAlarm;

  if (*alarm) {
    asyncDiscardHandle(*alarm);
    *alarm = NULL;
  }
}

static int
startPollAlarm (BrailleDisplay *brl) {
  AsyncHandle alarm = brl->data->pollAlarm;
  if (alarm) return 1;

  if (asyncNewRelativeAlarm(&alarm, 0, CN_handlePollAlarm, brl)) {
    if (asyncResetAlarmEvery(alarm, POLL_ALARM_INTERVAL)) {
      brl->data->pollAlarm = alarm;
      return 1;
    }

    asyncDiscardHandle(alarm);
  }

  return 0;
}

static BrailleResponseResult
isIdentityResponse (BrailleDisplay *brl, const void *packet, size_t size) {
  IdentityResponseHandler *handler = brl->data->handleIdentityResponse;
  brl->data->handleIdentityResponse = NULL;
  return handler(brl, packet, size);
}

static BrailleResponseResult
writeIdentifyCommand (BrailleDisplay *brl, unsigned char command, IdentityResponseHandler *handler) {
  if (!writeCommand(brl, command)) return 0;
  brl->data->handleIdentityResponse = handler;
  return 1;
}

static BrailleResponseResult
writeNextIdentifyCommand (BrailleDisplay *brl, unsigned char command, IdentityResponseHandler *handler) {
  return writeIdentifyCommand(brl, command, handler)? BRL_RSP_CONTINUE: BRL_RSP_FAIL;
}

static BrailleResponseResult
handleRowCount (BrailleDisplay *brl, const unsigned char *response, size_t size) {
  if (response[0] != CN_CMD_ROW_COUNT) return BRL_RSP_UNEXPECTED;
  brl->textRows = getResponseResult(response);
  return BRL_RSP_DONE;
}

static BrailleResponseResult
handleColumnCount (BrailleDisplay *brl, const unsigned char *response, size_t size) {
  if (response[0] != CN_CMD_COLUMN_COUNT) return BRL_RSP_UNEXPECTED;
  brl->textColumns = getResponseResult(response);
  return writeNextIdentifyCommand(brl, CN_CMD_ROW_COUNT, handleRowCount);
}

static int
writeIdentifyRequest (BrailleDisplay *brl) {
  return writeIdentifyCommand(brl, CN_CMD_COLUMN_COUNT, handleColumnCount);
}

static RowEntry *
getRowEntry (BrailleDisplay *brl, unsigned int index) {
  return brl->data->output.rowEntries[index];
}

static void
deallocateRowEntries (BrailleDisplay *brl, unsigned int count) {
  if (brl->data->output.rowEntries) {
    while (count > 0) free(getRowEntry(brl, --count));
    free(brl->data->output.rowEntries);
    brl->data->output.rowEntries = NULL;
  }
}

static int
allocateRowEntries (BrailleDisplay *brl) {
  if ((brl->data->output.rowEntries = malloc(ARRAY_SIZE(brl->data->output.rowEntries, brl->textRows)))) {
    for (unsigned int index=0; index<brl->textRows; index+=1) {
      RowEntry **row = &brl->data->output.rowEntries[index];
      size_t size = sizeof(**row) + brl->textColumns;

      if (!(*row = malloc(size))) {
        logMallocError();
        deallocateRowEntries(brl, (index + 1));
        return 0;
      }

      memset(*row, 0, size);
      (*row)->force = 1;
    }
  }

  return 1;
}

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 9600
  };

  BEGIN_USB_STRING_LIST(usbManufacturers_16C0_05E1)
    "bristolbraille.co.uk",
  END_USB_STRING_LIST

  BEGIN_USB_STRING_LIST(usbProducts_16C0_05E1)
    "Canute 360",
  END_USB_STRING_LIST

  BEGIN_USB_CHANNEL_DEFINITIONS
    { /* all models */
      .vendor=0X16C0, .product=0X05E1,
      .manufacturers = usbManufacturers_16C0_05E1,
      .products = usbProducts_16C0_05E1,
      .configuration=1, .interface=1, .alternative=0,
      .inputEndpoint=3, .outputEndpoint=2,
      .serial = &serialParameters,
      .resetDevice = 1
    },
  END_USB_CHANNEL_DEFINITIONS

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.serial.parameters = &serialParameters;

  descriptor.usb.channelDefinitions = usbChannelDefinitions;

  if (connectBrailleResource(brl, identifier, &descriptor, NULL)) {
    return 1;
  }

  return 0;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));

    brl->data->handleIdentityResponse = NULL;
    brl->data->crcGenerator = NULL;
    brl->data->pollAlarm = NULL;

    brl->data->input.keys = 0;

    brl->data->output.rowEntries = NULL;

    if ((brl->data->crcGenerator = crcNewGenerator(&crcAlgorithmParameters_HDLC))) {
      if (connectResource(brl, device)) {
        unsigned char response[MAXIMUM_RESPONSE_SIZE];

        if (probeBrailleDisplay(brl, PROBE_RETRY_LIMIT,
                                NULL, PROBE_INPUT_TIMEOUT,
                                writeIdentifyRequest,
                                readPacket, &response, sizeof(response),
                                isIdentityResponse)) {
          if (allocateRowEntries(brl)) {
            setBrailleKeyTable(brl, &KEY_TABLE_DEFINITION(all));
            makeOutputTable(dotsTable_ISO11548_1);
            brl->cellSize = 6;

            if (startPollAlarm(brl)) {
              return 1;
            }

            deallocateRowEntries(brl, brl->textRows);
          }
        }

        disconnectBrailleResource(brl, NULL);
      }

      crcDestroyGenerator(brl->data->crcGenerator);
    }

    free(brl->data);
  } else {
    logMallocError();
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  stopPollAlarm(brl);
  disconnectBrailleResource(brl, NULL);

  if (brl->data) {
    deallocateRowEntries(brl, brl->textRows);

    {
      CRCGenerator *crc = brl->data->crcGenerator;
      if (crc) crcDestroyGenerator(crc);
    }

    free(brl->data);
    brl->data = NULL;
  }
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  unsigned int length = brl->textColumns;

  for (unsigned int index=0; index<brl->textRows; index+=1) {
    RowEntry *row = getRowEntry(brl, index);

    if (cellsHaveChanged(row->cells, &brl->buffer[index * length], length, NULL, NULL, &row->force)) {
      row->isNew = 1;
    }
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  unsigned char packet[MAXIMUM_RESPONSE_SIZE];
  size_t size;

  while ((size = readPacket(brl, packet, sizeof(packet)))) {
    ResponseResult result = getResponseResult(packet);

    switch (packet[0]) {
      case CN_CMD_SEND_ROW:
        continue;

      case CN_CMD_KEYS_STATE:
        enqueueUpdatedKeys(brl, result, &brl->data->input.keys, CN_GRP_NavigationKeys, 0);
        continue;

      case CN_CMD_DEVICE_STATE:
        continue;

      default:
        break;
    }

    logUnexpectedPacket(packet, size);
  }

  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
}
