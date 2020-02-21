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
#include "timing.h"

#include "brl_driver.h"
#include "brldefs-cn.h"

#define PROBE_RETRY_LIMIT 0
#define PROBE_RESPONSE_TIMEOUT 1000
#define POLL_ALARM_INTERVAL 100
#define REQUEST_RESPONSE_TIMEOUT 5000
#define MAXIMUM_RESPONSE_SIZE 0X100

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

typedef BrailleResponseResult ProbeResponseHandler (
  BrailleDisplay *brl,
  const unsigned char *response, size_t size
);

struct BrailleDataStruct {
  CRCGenerator *crcGenerator;

  struct {
    ProbeResponseHandler *responseHandler;
    unsigned int protocolVersion;
  } probe;

  struct {
    AsyncHandle alarmHandle;
    CN_PacketInteger deviceState;
    unsigned char deviceStateCounter;
    TimeValue lastWriteTime;
    unsigned waitingForResponse:1;
  } poll;

  struct {
    KeyNumberSet keys;
  } input;

  struct {
    RowEntry **rowEntries;
    unsigned int firstNewRow;
  } output;
};

static crc_t
makePacketChecksum (BrailleDisplay *brl, const void *packet, size_t size) {
  CRCGenerator *crc = brl->data->crcGenerator;
  crcResetGenerator(crc);
  crcAddData(crc, packet, size);
  return crcGetChecksum(crc);
}

typedef enum {
  PVS_WAITING,
  PVS_STARTED,
  PVS_DONE
} PacketVerificationState;

typedef struct {
  PacketVerificationState state;
  unsigned escaped:1;
} PacketVerificationData;

static BraillePacketVerifierResult
verifyPacket (
  BrailleDisplay *brl,
  unsigned char *bytes, size_t size,
  size_t *length, void *data
) {
  PacketVerificationData *pvd = data;
  unsigned char *byte = &bytes[size-1];

  if (*byte == CN_PACKET_FRAMING_BYTE) {
    if ((pvd->state += 1) == PVS_DONE) {
      if (pvd->escaped) return BRL_PVR_INVALID;
      *length = size - 1;
    } else {
      *length = MAXIMUM_RESPONSE_SIZE;
    }

    return BRL_PVR_EXCLUDE;
  } else if (pvd->state == PVS_WAITING) {
    return BRL_PVR_INVALID;
  }

  if (*byte == CN_PACKET_ESCAPE_BYTE) {
    if (pvd->escaped) return BRL_PVR_INVALID;
    pvd->escaped = 1;
    return BRL_PVR_EXCLUDE;
  }

  if (pvd->escaped) {
    pvd->escaped = 0;
    *byte ^= CN_PACKET_ESCAPE_BIT;
  }

  return BRL_PVR_INCLUDE;
}

static size_t
readPacket (BrailleDisplay *brl, void *packet, size_t size) {
  while (1) {
    PacketVerificationData pvd = {
      .state = PVS_WAITING
    };

    size_t length = readBraillePacket(brl, NULL, packet, size, verifyPacket, &pvd);

    if (length > 0) {
      if (length < 3) {
        logShortPacket(packet, length);
        continue;
      }

      {
        crc_t expected = CN_getResponseInteger(packet, (length -= 2));
        crc_t actual = makePacketChecksum(brl, packet, length);

        if (actual != expected) {
          logMessage(LOG_WARNING,
            "input packet checksum mismatch: Actual:%X Expected:%X",
            actual, expected
          );

          continue;
        }
      }

      {
        const unsigned char *bytes = packet;
        unsigned char type = bytes[0];
        size_t expected = 0;

        switch (type) {
          case CN_CMD_COLUMN_COUNT:
          case CN_CMD_ROW_COUNT:
          case CN_CMD_PROTOCOL_VERSION:
          case CN_CMD_FIRMWARE_VERSION:
          case CN_CMD_DEVICE_STATE:
          case CN_CMD_KEYS_STATE:
          case CN_CMD_SEND_ROW:
          case CN_CMD_RESET_CELLS:
            expected = 3;
            break;

          default:
            logUnexpectedPacket(packet, length);
            continue;
        }

        if (length < expected) {
          logTruncatedPacket(packet, length);
          continue;
        }
      }
    }

    return length;
  }
}

static inline void
addByteToPacket (unsigned char **target, unsigned char byte) {
  if ((byte == CN_PACKET_ESCAPE_BYTE) || (byte == CN_PACKET_FRAMING_BYTE)) {
    *(*target)++ = CN_PACKET_ESCAPE_BYTE;
    byte ^= CN_PACKET_ESCAPE_BIT;
  }

  *(*target)++ = byte;
}

static int
writePacket (BrailleDisplay *brl, const unsigned char *packet, size_t size) {
  logBytes(LOG_CATEGORY(OUTPUT_PACKETS), "raw", packet, size);

  unsigned char buffer[1 + ((size + 2) * 2) + 1];
  unsigned char *target = buffer;
  *target++ = CN_PACKET_FRAMING_BYTE;

  {
    const unsigned char *source = packet;
    const unsigned char *end = source + size;

    while (source < end) {
      addByteToPacket(&target, *source++);
    }
  }

  {
    uint16_t checksum = makePacketChecksum(brl, packet, size);
    addByteToPacket(&target, (checksum & UINT8_MAX));
    addByteToPacket(&target, (checksum >> 8));
  }

  *target++ = CN_PACKET_FRAMING_BYTE;
  size = target - buffer;
  if (!writeBraillePacket(brl, NULL, buffer, size)) return 0;

  brl->data->poll.waitingForResponse = 1;
  getMonotonicTime(&brl->data->poll.lastWriteTime);
  return 1;
}

static int
writeCommand (BrailleDisplay *brl, unsigned char command) {
  unsigned char packet[] = {command};
  return writePacket(brl, packet, sizeof(packet));
}

static int
testMotorsActive (BrailleDisplay *brl) {
  return !!(brl->data->poll.deviceState & CN_DEV_MOTORS_ACTIVE);
}

static void
setMotorsActive (BrailleDisplay *brl) {
  brl->data->poll.deviceState |= CN_DEV_MOTORS_ACTIVE;
  brl->data->poll.deviceStateCounter = 0;
}

static int
writeMotorsCommand (BrailleDisplay *brl, unsigned char command) {
  int ok = writeCommand(brl, command);
  if (ok) setMotorsActive(brl);
  return ok;
}

static RowEntry *
getRowEntry (BrailleDisplay *brl, unsigned int index) {
  return brl->data->output.rowEntries[index];
}

static void
deallocateRowEntries (BrailleDisplay *brl, unsigned int count) {
  RowEntry ***rowEntries = &brl->data->output.rowEntries;

  if (*rowEntries) {
    while (count > 0) free(getRowEntry(brl, --count));
    free(*rowEntries);
    *rowEntries = NULL;
  }
}

static int
allocateRowEntries (BrailleDisplay *brl) {
  RowEntry ***rowEntries = &brl->data->output.rowEntries;

  if (!(*rowEntries = malloc(ARRAY_SIZE(*rowEntries, brl->textRows)))) {
    logMallocError();
    return 0;
  }

  for (unsigned int index=0; index<brl->textRows; index+=1) {
    RowEntry **row = &(*rowEntries)[index];
    size_t size = sizeof(**row) + brl->textColumns;

    if (!(*row = malloc(size))) {
      logMallocError();
      deallocateRowEntries(brl, (index + 1));
      return 0;
    }

    memset(*row, 0, size);
    (*row)->force = 1;
  }

  return 1;
}

static int
sendRow (BrailleDisplay *brl) {
  while (brl->data->output.firstNewRow < brl->textRows) {
    RowEntry *row = getRowEntry(brl, brl->data->output.firstNewRow);

    if (row->isNew) {
      unsigned int length = brl->textColumns;
      unsigned char packet[2 + length];
      unsigned char *byte = packet;

      *byte++ = CN_CMD_SEND_ROW;
      *byte++ = brl->data->output.firstNewRow;
      byte = translateOutputCells(byte, row->cells, length);

      if (writePacket(brl, packet, (byte - packet))) {
        setMotorsActive(brl);
        row->isNew = 0;
        brl->data->output.firstNewRow += 1;
      } else {
        brl->hasFailed = 1;
      }

      return 1;
    }

    brl->data->output.firstNewRow += 1;
  }

  return 0;
}

static void
setFirstNewRow (BrailleDisplay *brl, unsigned int index) {
  if (index < brl->data->output.firstNewRow) {
    brl->data->output.firstNewRow = index;
  }
}

static int
refreshAllRows (BrailleDisplay *brl) {
  for (unsigned int index=0; index<brl->textRows; index+=1) {
    getRowEntry(brl, index)->isNew = 1;
  }

  brl->data->output.firstNewRow = 0;
  return writeMotorsCommand(brl, CN_CMD_RESET_CELLS);
}

static int
refreshRow (BrailleDisplay *brl, int row) {
  return refreshAllRows(brl); // for now
}

ASYNC_ALARM_CALLBACK(CN_handlePollAlarm) {
  BrailleDisplay *brl = parameters->data;

  if (brl->data->poll.waitingForResponse) {
    {
      long int elapsed = millisecondsBetween(
        &brl->data->poll.lastWriteTime,
        parameters->now
      );

      if (elapsed <= REQUEST_RESPONSE_TIMEOUT) return;
    }

    logMessage(LOG_WARNING, "Canute response timeout");
    return;
  }

  unsigned char command = CN_CMD_KEYS_STATE;

  if (!testMotorsActive(brl)) {
    if (sendRow(brl)) return;
  } else if (brl->data->poll.deviceStateCounter) {
    brl->data->poll.deviceStateCounter -= 1;
  } else {
    brl->data->poll.deviceStateCounter = 10;
    command = CN_CMD_DEVICE_STATE;
  }

  if (!writeCommand(brl, command)) brl->hasFailed = 1;
}

static void
stopPollAlarm (BrailleDisplay *brl) {
  AsyncHandle *alarm = &brl->data->poll.alarmHandle;

  if (*alarm) {
    asyncCancelRequest(*alarm);
    *alarm = NULL;
  }
}

static int
startPollAlarm (BrailleDisplay *brl) {
  AsyncHandle alarm = brl->data->poll.alarmHandle;
  if (alarm) return 1;

  if (asyncNewRelativeAlarm(&alarm, 0, CN_handlePollAlarm, brl)) {
    if (asyncResetAlarmEvery(alarm, POLL_ALARM_INTERVAL)) {
      brl->data->poll.alarmHandle = alarm;
      return 1;
    }

    asyncDiscardHandle(alarm);
  }

  return 0;
}

static BrailleResponseResult
isIdentityResponse (BrailleDisplay *brl, const void *packet, size_t size) {
  brl->data->poll.waitingForResponse = 0;
  ProbeResponseHandler *handler = brl->data->probe.responseHandler;
  brl->data->probe.responseHandler = NULL;
  return handler(brl, packet, size);
}

static BrailleResponseResult
writeProbeCommand (BrailleDisplay *brl, unsigned char command, ProbeResponseHandler *handler) {
  if (!writeCommand(brl, command)) return 0;
  brl->data->probe.responseHandler = handler;
  return 1;
}

static BrailleResponseResult
writeNextProbeCommand (BrailleDisplay *brl, unsigned char command, ProbeResponseHandler *handler) {
  return writeProbeCommand(brl, command, handler)? BRL_RSP_CONTINUE: BRL_RSP_FAIL;
}

static BrailleResponseResult
handleDeviceState (BrailleDisplay *brl, const unsigned char *response, size_t size) {
  if (response[0] != CN_CMD_DEVICE_STATE) return BRL_RSP_UNEXPECTED;
  brl->data->poll.deviceState = CN_getResponseResult(response);
  return BRL_RSP_DONE;
}

static BrailleResponseResult
handleFirmwareVersion (BrailleDisplay *brl, const unsigned char *response, size_t size) {
  if (response[0] != CN_CMD_FIRMWARE_VERSION) return BRL_RSP_UNEXPECTED;

  response += 1;
  size -= 1;
  logMessage(LOG_INFO, "Firmware Version: %.*s", (int)size, response);

  return writeNextProbeCommand(brl, CN_CMD_DEVICE_STATE, handleDeviceState);
}

static BrailleResponseResult
handleProtocolVersion (BrailleDisplay *brl, const unsigned char *response, size_t size) {
  if (response[0] != CN_CMD_PROTOCOL_VERSION) return BRL_RSP_UNEXPECTED;
  brl->data->probe.protocolVersion = CN_getResponseResult(response);
  logMessage(LOG_INFO, "Protocol Version: %u", brl->data->probe.protocolVersion);
  return writeNextProbeCommand(brl, CN_CMD_FIRMWARE_VERSION, handleFirmwareVersion);
}

static BrailleResponseResult
handleRowCount (BrailleDisplay *brl, const unsigned char *response, size_t size) {
  if (response[0] != CN_CMD_ROW_COUNT) return BRL_RSP_UNEXPECTED;
  brl->textRows = CN_getResponseResult(response);
  return writeNextProbeCommand(brl, CN_CMD_PROTOCOL_VERSION, handleProtocolVersion);
}

static BrailleResponseResult
handleColumnCount (BrailleDisplay *brl, const unsigned char *response, size_t size) {
  if (response[0] != CN_CMD_COLUMN_COUNT) return BRL_RSP_UNEXPECTED;
  brl->textColumns = CN_getResponseResult(response);
  return writeNextProbeCommand(brl, CN_CMD_ROW_COUNT, handleRowCount);
}

static int
writeIdentifyRequest (BrailleDisplay *brl) {
  return writeProbeCommand(brl, CN_CMD_COLUMN_COUNT, handleColumnCount);
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
    brl->data->probe.responseHandler = NULL;
    brl->data->poll.alarmHandle = NULL;
    brl->data->input.keys = 0;
    brl->data->output.rowEntries = NULL;

    if ((brl->data->crcGenerator = crcNewGenerator(crcGetProvidedAlgorithm(CN_PACKET_CHECKSUM_ALGORITHM)))) {
      if (connectResource(brl, device)) {
        unsigned char response[MAXIMUM_RESPONSE_SIZE];

        if (probeBrailleDisplay(brl, PROBE_RETRY_LIMIT,
                                NULL, PROBE_RESPONSE_TIMEOUT,
                                writeIdentifyRequest,
                                readPacket, &response, sizeof(response),
                                isIdentityResponse)) {
          if (allocateRowEntries(brl)) {
            brl->refreshBrailleDisplay = refreshAllRows;
            brl->refreshBrailleRow = refreshRow;

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
  deallocateRowEntries(brl, brl->textRows);
  crcDestroyGenerator(brl->data->crcGenerator);

  free(brl->data);
  brl->data = NULL;
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  unsigned int length = brl->textColumns;
  const unsigned char *cells = brl->buffer;

  for (unsigned int index=0; index<brl->textRows; index+=1) {
    RowEntry *row = getRowEntry(brl, index);

    if (cellsHaveChanged(row->cells, cells, length, NULL, NULL, &row->force)) {
      row->isNew = 1;
      setFirstNewRow(brl, index);
    }

    cells += length;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  unsigned char packet[MAXIMUM_RESPONSE_SIZE];
  size_t size;

  while ((size = readPacket(brl, packet, sizeof(packet)))) {
    brl->data->poll.waitingForResponse = 0;
    brl->writeDelay = 0;
    CN_PacketInteger result = CN_getResponseResult(packet);

    switch (packet[0]) {
      case CN_CMD_DEVICE_STATE:
        brl->data->poll.deviceState = result;
        continue;

      case CN_CMD_KEYS_STATE:
        enqueueUpdatedKeys(brl, result, &brl->data->input.keys, CN_GRP_NavigationKeys, 0);
        continue;

      case CN_CMD_SEND_ROW:
        continue;

      case CN_CMD_RESET_CELLS:
        continue;

      default:
        break;
    }

    logUnexpectedPacket(packet, size);
  }

  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
}
