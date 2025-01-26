/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2024 by The BRLTTY Developers.
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
#include "strfmt.h"

#include "brl_driver.h"
#include "brldefs-nh.h"

#define PROBE_RETRY_LIMIT 2
#define PROBE_INPUT_TIMEOUT 1000
#define MAXIMUM_RESPONSE_SIZE (0XFF + 4)
#define MAXIMUM_TEXT_CELLS 0XFF

BEGIN_KEY_NAME_TABLE(navigation)
  KEY_NAME_ENTRY(NH_KEY_Dot1, "Dot1"),
  KEY_NAME_ENTRY(NH_KEY_Dot2, "Dot2"),
  KEY_NAME_ENTRY(NH_KEY_Dot3, "Dot3"),
  KEY_NAME_ENTRY(NH_KEY_Dot4, "Dot4"),
  KEY_NAME_ENTRY(NH_KEY_Dot5, "Dot5"),
  KEY_NAME_ENTRY(NH_KEY_Dot6, "Dot6"),
  KEY_NAME_ENTRY(NH_KEY_Dot7, "Dot7"),
  KEY_NAME_ENTRY(NH_KEY_Dot8, "Dot8"),
  KEY_NAME_ENTRY(NH_KEY_Space, "Space"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(routing)
  KEY_GROUP_ENTRY(NH_GRP_RoutingKeys, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(navigation),
  KEY_NAME_TABLE(routing),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
END_KEY_TABLE_LIST

struct BrailleDataStruct {
  struct {
    unsigned char rewrite;
    unsigned char cells[MAXIMUM_TEXT_CELLS];
  } text;
};

static int
writeBytes (BrailleDisplay *brl, const char *bytes, size_t count) {
  return writeBraillePacket(brl, NULL, bytes, count);
}

static BraillePacketVerifierResult
verifyPacket (
  BrailleDisplay *brl,
  unsigned char *bytes, size_t size,
  size_t *length, void *data
) {
  unsigned char byte = bytes[size-1];

  switch (size) {
    case 1:
      switch (byte) {
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
connectResource (BrailleDisplay *brl, const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS
  };

  BEGIN_USB_CHANNEL_DEFINITIONS
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

    if (connectResource(brl, device)) {
      if (1) {
        setBrailleKeyTable(brl, &KEY_TABLE_DEFINITION(all));
        makeOutputTable(dotsTable_ISO11548_1);

        brl->textColumns = 80;
        brl->data->text.rewrite = 1;
        return 1;
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
  free(brl->data);
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  unsigned int cellCount = brl->textColumns * brl->textRows;

  if (cellsHaveChanged(brl->data->text.cells, brl->buffer, cellCount,
                       NULL, NULL, &brl->data->text.rewrite)) {
    unsigned char cells[cellCount];
    translateOutputCells(cells, brl->data->text.cells, cellCount);

    {
      char packet[4 + (4 * cellCount) + 4 + 1];
      STR_BEGIN(packet, sizeof(packet));
      STR_PRINTF("INT ");

      for (unsigned int cellIndex=0; cellIndex<cellCount; cellIndex+=1) {
        STR_PRINTF("%u ", cells[cellIndex]);
      }

      STR_PRINTF("END ");
      if (!writeBytes(brl, packet, STR_LENGTH)) return 0;

      STR_END;
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
