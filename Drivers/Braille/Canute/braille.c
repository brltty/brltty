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

#include "brl_driver.h"
#include "brldefs-cn.h"

#define PROBE_RETRY_LIMIT 2
#define PROBE_INPUT_TIMEOUT 1000
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
  unsigned char write;
  unsigned char cells[];
} RowEntry;

typedef BrailleResponseResult IdentityResponseHandler (
  BrailleDisplay *brl,
  const unsigned char *response, size_t size
);

struct BrailleDataStruct {
  IdentityResponseHandler *handleIdentityResponse;
  RowEntry **rowEntries;
};

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

static int
writePacket (BrailleDisplay *brl, const unsigned char *packet, size_t count) {
  return writeBraillePacket(brl, NULL, packet, count);
}

static int
writeCommand (BrailleDisplay *brl, unsigned char command) {
  unsigned char packet[] = {command};
  return writePacket(brl, packet, sizeof(packet));
}

static BrailleResponseResult
isIdentityResponse (BrailleDisplay *brl, const void *packet, size_t size) {
  IdentityResponseHandler *handler = brl->data->handleIdentityResponse;
  brl->data->handleIdentityResponse = NULL;
  return handler(brl, packet, size);
}

static BrailleResponseResult
writeNextIdentifyRequest (BrailleDisplay *brl, unsigned char command, IdentityResponseHandler *handler) {
  if (!writeCommand(brl, command)) return BRL_RSP_FAIL;
  brl->data->handleIdentityResponse = handler;
  return BRL_RSP_CONTINUE;
}

static BrailleResponseResult
handleRowCount (BrailleDisplay *brl, const unsigned char *response, size_t size) {
  if (response[0] != CN_CMD_ROW_COUNT) return BRL_RSP_UNEXPECTED;
  brl->textRows = response[1];
  return BRL_RSP_DONE;
}

static BrailleResponseResult
handleColumnCount (BrailleDisplay *brl, const unsigned char *response, size_t size) {
  if (response[0] != CN_CMD_COLUMN_COUNT) return BRL_RSP_UNEXPECTED;
  brl->textColumns = response[1];
  return writeNextIdentifyRequest(brl, CN_CMD_ROW_COUNT, handleRowCount);
}

static int
writeIdentifyRequest (BrailleDisplay *brl) {
  if (!writeCommand(brl, CN_CMD_COLUMN_COUNT)) return 0;
  brl->data->handleIdentityResponse = handleColumnCount;
  return 1;
}

static void
deallocateRowEntries (BrailleDisplay *brl, unsigned int count) {
  if (brl->data->rowEntries) {
    while (count > 0) free(brl->data->rowEntries[--count]);
    free(brl->data->rowEntries);
    brl->data->rowEntries = NULL;
  }
}

static int
allocateRowEntries (BrailleDisplay *brl) {
  if ((brl->data->rowEntries = malloc(ARRAY_SIZE(brl->data->rowEntries, brl->textRows)))) {
    for (unsigned int index=0; index<brl->textRows; index+=1) {
      RowEntry **row = &brl->data->rowEntries[index];
      size_t size = sizeof(**row) + brl->textColumns;

      if (!(*row = malloc(size))) {
        logMallocError();
        deallocateRowEntries(brl, (index + 1));
        return 0;
      }

      memset(*row, 0, size);
      (*row)->write = 1;
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
    brl->data->rowEntries = NULL;

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
          return 1;
        }
      }

      disconnectBrailleResource(brl, NULL);
    }

    free(brl->data);
  } else {
    logMallocError();
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  disconnectBrailleResource(brl, NULL);

  if (brl->data) {
    deallocateRowEntries(brl, brl->textRows);
    free(brl->data);
    brl->data = NULL;
  }
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  unsigned int length = brl->textColumns;

  for (unsigned int index=0; index<brl->textRows; index+=1) {
    RowEntry *row = brl->data->rowEntries[index];

    if (cellsHaveChanged(row->cells, &brl->buffer[index * length], length, NULL, NULL, &row->write)) {
    }
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  unsigned char packet[MAXIMUM_RESPONSE_SIZE];
  size_t size;

  while ((size = readPacket(brl, packet, sizeof(packet)))) {
    logUnexpectedPacket(packet, size);
  }

  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
}
