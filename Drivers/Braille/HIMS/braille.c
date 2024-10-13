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

#include "brl_driver.h"
#include "brldefs-hm.h"

#define MAXIMUM_CELL_COUNT 40

BEGIN_KEY_NAME_TABLE(common)
  KEY_GROUP_ENTRY(HM_GRP_RoutingKeys, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(braille)
  KEY_NAME_ENTRY(HM_KEY_Dot1, "Dot1"),
  KEY_NAME_ENTRY(HM_KEY_Dot2, "Dot2"),
  KEY_NAME_ENTRY(HM_KEY_Dot3, "Dot3"),
  KEY_NAME_ENTRY(HM_KEY_Dot4, "Dot4"),
  KEY_NAME_ENTRY(HM_KEY_Dot5, "Dot5"),
  KEY_NAME_ENTRY(HM_KEY_Dot6, "Dot6"),
  KEY_NAME_ENTRY(HM_KEY_Dot7, "Dot7"),
  KEY_NAME_ENTRY(HM_KEY_Dot8, "Dot8"),
  KEY_NAME_ENTRY(HM_KEY_Space, "Space"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(pan)
  KEY_NAME_ENTRY(HM_KEY_Backward, "Backward"),
  KEY_NAME_ENTRY(HM_KEY_Forward, "Forward"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(BS_scroll)
  KEY_NAME_ENTRY(HM_KEY_BS_LeftScrollUp, "LeftScrollUp"),
  KEY_NAME_ENTRY(HM_KEY_BS_LeftScrollDown, "LeftScrollDown"),
  KEY_NAME_ENTRY(HM_KEY_BS_RightScrollUp, "RightScrollUp"),
  KEY_NAME_ENTRY(HM_KEY_BS_RightScrollDown, "RightScrollDown"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(BE_scroll)
  KEY_NAME_ENTRY(HM_KEY_BE_LeftScrollUp, "LeftScrollUp"),
  KEY_NAME_ENTRY(HM_KEY_BE_LeftScrollDown, "LeftScrollDown"),
  KEY_NAME_ENTRY(HM_KEY_BE_RightScrollUp, "RightScrollUp"),
  KEY_NAME_ENTRY(HM_KEY_BE_RightScrollDown, "RightScrollDown"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(f14)
  KEY_NAME_ENTRY(HM_KEY_F1, "F1"),
  KEY_NAME_ENTRY(HM_KEY_F2, "F2"),
  KEY_NAME_ENTRY(HM_KEY_F3, "F3"),
  KEY_NAME_ENTRY(HM_KEY_F4, "F4"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(f58)
  KEY_NAME_ENTRY(HM_KEY_F5, "F5"),
  KEY_NAME_ENTRY(HM_KEY_F6, "F6"),
  KEY_NAME_ENTRY(HM_KEY_F7, "F7"),
  KEY_NAME_ENTRY(HM_KEY_F8, "F8"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(lp)
  KEY_NAME_ENTRY(HM_KEY_LeftPadUp, "LeftPadUp"),
  KEY_NAME_ENTRY(HM_KEY_LeftPadDown, "LeftPadDown"),
  KEY_NAME_ENTRY(HM_KEY_LeftPadLeft, "LeftPadLeft"),
  KEY_NAME_ENTRY(HM_KEY_LeftPadRight, "LeftPadRight"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(rp)
  KEY_NAME_ENTRY(HM_KEY_RightPadUp, "RightPadUp"),
  KEY_NAME_ENTRY(HM_KEY_RightPadDown, "RightPadDown"),
  KEY_NAME_ENTRY(HM_KEY_RightPadLeft, "RightPadLeft"),
  KEY_NAME_ENTRY(HM_KEY_RightPadRight, "RightPadRight"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(emotion)
  KEY_NAME_ENTRY(HM_KEY_EM_Control, "Control"),
  KEY_NAME_ENTRY(HM_KEY_EM_Alt, "Alt"),

  KEY_NAME_ENTRY(HM_KEY_EM_F1, "F1"),
  KEY_NAME_ENTRY(HM_KEY_EM_F2, "F2"),
  KEY_NAME_ENTRY(HM_KEY_EM_F3, "F3"),
  KEY_NAME_ENTRY(HM_KEY_EM_F4, "F4"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(pan)
  KEY_NAME_TABLE(common),
  KEY_NAME_TABLE(braille),
  KEY_NAME_TABLE(f14),
  KEY_NAME_TABLE(pan),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(scroll)
  KEY_NAME_TABLE(common),
  KEY_NAME_TABLE(braille),
  KEY_NAME_TABLE(f14),
  KEY_NAME_TABLE(BS_scroll),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(qwerty)
  KEY_NAME_TABLE(common),
  KEY_NAME_TABLE(braille),
  KEY_NAME_TABLE(f14),
  KEY_NAME_TABLE(BS_scroll),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(edge)
  KEY_NAME_TABLE(common),
  KEY_NAME_TABLE(braille),
  KEY_NAME_TABLE(f14),
  KEY_NAME_TABLE(f58),
  KEY_NAME_TABLE(BE_scroll),
  KEY_NAME_TABLE(lp),
  KEY_NAME_TABLE(rp),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLE(SB_scroll)
  KEY_NAME_ENTRY(HM_KEY_SB_LeftScrollUp, "LeftScrollUp"),
  KEY_NAME_ENTRY(HM_KEY_SB_LeftScrollDown, "LeftScrollDown"),
  KEY_NAME_ENTRY(HM_KEY_SB_RightScrollUp, "RightScrollUp"),
  KEY_NAME_ENTRY(HM_KEY_SB_RightScrollDown, "RightScrollDown"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(sync)
  KEY_NAME_TABLE(common),
  KEY_NAME_TABLE(SB_scroll),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLE(beetle)
  KEY_NAME_ENTRY(HM_KEY_BS_RightScrollUp, "Backward"),
  KEY_NAME_ENTRY(HM_KEY_BS_RightScrollDown, "Forward"),

  KEY_NAME_ENTRY(HM_KEY_F1, "F1"),
  KEY_NAME_ENTRY(HM_KEY_F4, "F2"),
  KEY_NAME_ENTRY(HM_KEY_F3, "F3"),
  KEY_NAME_ENTRY(HM_KEY_F2, "F4"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(beetle)
  KEY_NAME_TABLE(common),
  KEY_NAME_TABLE(braille),
  KEY_NAME_TABLE(beetle),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(emotion)
  KEY_NAME_TABLE(common),
  KEY_NAME_TABLE(braille),
  KEY_NAME_TABLE(BE_scroll),
  KEY_NAME_TABLE(emotion),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(pan)
DEFINE_KEY_TABLE(scroll)
DEFINE_KEY_TABLE(qwerty)
DEFINE_KEY_TABLE(edge)
DEFINE_KEY_TABLE(sync)
DEFINE_KEY_TABLE(beetle)
DEFINE_KEY_TABLE(emotion)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(pan),
  &KEY_TABLE_DEFINITION(scroll),
  &KEY_TABLE_DEFINITION(qwerty),
  &KEY_TABLE_DEFINITION(edge),
  &KEY_TABLE_DEFINITION(sync),
  &KEY_TABLE_DEFINITION(beetle),
  &KEY_TABLE_DEFINITION(emotion),
END_KEY_TABLE_LIST

typedef enum {
  IPT_CURSOR  = 0X00,
  IPT_KEYS    = 0X01,
  IPT_CELLS   = 0X02
} InputPacketType;

typedef union {
  unsigned char bytes[10];

  struct {
    unsigned char start;
    unsigned char type;
    unsigned char count;
    unsigned char data;
    unsigned char reserved[4];
    unsigned char checksum;
    unsigned char end;
  } PACKED data;
} PACKED InputPacket;

typedef struct {
  const char *description;
  const KeyTableDefinition *keyTable;
  unsigned char id1;
  unsigned char id2;
} IdentityEntry;

static const IdentityEntry assumePanIdentity = {
  .keyTable = &KEY_TABLE_DEFINITION(pan)
};

static const IdentityEntry assumeScrollIdentity = {
  .keyTable = &KEY_TABLE_DEFINITION(scroll)
};

static const IdentityEntry panIdentity = {
  .description = "Braille Sense PLUS (pan)",
  .id1 = 0X42, .id2 = 0X53,
  .keyTable = &KEY_TABLE_DEFINITION(pan)
};

static const IdentityEntry scrollIdentity = {
  .description = "Braille Sense PLUS & OnHand (scroll)",
  .id1 = 0X4C, .id2 = 0X58,
  .keyTable = &KEY_TABLE_DEFINITION(scroll)
};

static const IdentityEntry qwerty2Identity = {
  .description = "Braille Sense PLUS (qwerty)",
  .id1 = 0X53, .id2 = 0X58,
  .keyTable = &KEY_TABLE_DEFINITION(qwerty)
};

static const IdentityEntry qwerty1Identity = {
  .description = "Braille Sense PLUS (qwerty)",
  .id1 = 0X51, .id2 = 0X58,
  .keyTable = &KEY_TABLE_DEFINITION(qwerty)
};

static const IdentityEntry edgeIdentity = {
  .description = "Braille EDGE",
  .id1 = 0X42, .id2 = 0X45,
  .keyTable = &KEY_TABLE_DEFINITION(edge)
};

typedef struct {
  int (*getCellCount) (BrailleDisplay *brl, unsigned int *count);
  int (*readCommands) (BrailleDisplay *brl);
  int (*writeCells) (BrailleDisplay *brl, const unsigned char *cells, size_t count);
} InputOutputOperations;

typedef struct {
  const char *modelName;
  const char *resourceNamePrefix;
  const KeyTableDefinition *keyTable;
  const KeyTableDefinition * (*testIdentities) (BrailleDisplay *brl);
  int (*getDefaultCellCount) (BrailleDisplay *brl, unsigned int *count);
  const InputOutputOperations *io;
} ProtocolEntry;

struct BrailleDataStruct {
  const ProtocolEntry *protocol;
  unsigned char previousCells[MAXIMUM_CELL_COUNT];

  struct {
    HidReportSize reportSize;
    unsigned char pressedKeys[0X20];
  } hid;
};

static BraillePacketVerifierResult
verifyPacket (
  BrailleDisplay *brl,
  unsigned char *bytes, size_t size,
  size_t *length, void *data
) {
  unsigned char byte = bytes[size-1];

  switch (size) {
    case 1: {
      switch (byte) {
        case 0X1C:
          *length = 4;
          break;

        case 0XFA:
          *length = 10;
          break;

        default:
          return BRL_PVR_INVALID;
      }

      break;
    }

    default:
      break;
  }

  if (size == *length) {
    switch (bytes[0]) {
      case 0X1C: {
        if (byte != 0X1F) return BRL_PVR_INVALID;
        break;
      }

      case 0XFA: {
        if (byte != 0XFB) return BRL_PVR_INVALID;

        const InputPacket *packet = (const void *)bytes;
        int checksum = -packet->data.checksum;
        for (size_t i=0; i<size; i+=1) checksum += packet->bytes[i];

        if ((checksum & 0XFF) != packet->data.checksum) {
          logInputProblem("incorrect input checksum", packet->bytes, size);
          return BRL_PVR_INVALID;
        }

        break;
      }

      default:
        break;
    }
  }

  return BRL_PVR_INCLUDE;
}

static size_t
readPacket (BrailleDisplay *brl, InputPacket *packet) {
  return readBraillePacket(brl, NULL, packet, sizeof(*packet), verifyPacket, NULL);
}

static size_t
readBytes (BrailleDisplay *brl, void *packet, size_t size) {
  return readPacket(brl, packet);
}

static int
writeBytes (BrailleDisplay *brl, const unsigned char *bytes, size_t count) {
  return writeBraillePacket(brl, NULL, bytes, count);
}

static int
testIdentity (BrailleDisplay *brl, unsigned char id1, unsigned char id2) {
  const unsigned char sequence[] = {0X1C, id1, id2, 0X1F};
  const size_t length = sizeof(sequence);

  if (writeBytes(brl, sequence, length)) {
    while (awaitBrailleInput(brl, 200)) {
      InputPacket response;
      size_t size = readPacket(brl, &response);
      if (!size) break;

      if (response.bytes[0] == sequence[0]) {
        return memcmp(response.bytes, sequence, length) == 0;
      }
    }
  }

  return 0;
}

static const KeyTableDefinition *
testIdentities (BrailleDisplay *brl, const IdentityEntry *const *identities) {
  while (*identities) {
    const IdentityEntry *identity = *identities;
    const char *name = identity->keyTable->bindings;

    if (!identity->id1 && !identity->id2) {
      logMessage(LOG_CATEGORY(BRAILLE_DRIVER), "assuming identity: %s", name);
    } else {
      logMessage(LOG_CATEGORY(BRAILLE_DRIVER), "testing identity: %s", name);

      if (!testIdentity(brl, identity->id1, identity->id2)) {
        identities += 1;
        continue;
      }
    }

    return identity->keyTable;
  }

  return NULL;
}

static const KeyTableDefinition *
testIdentities_BrailleSense (BrailleDisplay *brl) {
  static const IdentityEntry *const identities[] = {
    &qwerty2Identity,
    &qwerty1Identity,
    &scrollIdentity,
    &panIdentity,
    &assumePanIdentity,
    NULL
  };

  return testIdentities(brl, identities);
}

static const KeyTableDefinition *
testIdentities_BrailleSense6 (BrailleDisplay *brl) {
  static const IdentityEntry *const identities[] = {
    &assumeScrollIdentity,
    NULL
  };

  return testIdentities(brl, identities);
}

static const KeyTableDefinition *
testIdentities_BrailleEdge (BrailleDisplay *brl) {
  static const IdentityEntry *const identities[] = {
    &edgeIdentity,
    NULL
  };

  return testIdentities(brl, identities);
}

static int
writePacket (
  BrailleDisplay *brl,
  unsigned char type, unsigned char mode,
  const unsigned char *data1, size_t length1,
  const unsigned char *data2, size_t length2
) {
  unsigned char packet[2 + 1 + 1 + 2 + length1 + 1 + 1 + 2 + length2 + 1 + 4 + 1 + 2];
  unsigned char *byte = packet;
  unsigned char *checksum;

  /* DS */
  *byte++ = type;
  *byte++ = type;

  /* M */
  *byte++ = mode;

  /* DS1 */
  *byte++ = 0XF0;

  /* Cnt1 */
  *byte++ = (length1 >> 0) & 0XFF;
  *byte++ = (length1 >> 8) & 0XFF;

  /* D1 */
  byte = mempcpy(byte, data1, length1);

  /* DE1 */
  *byte++ = 0XF1;

  /* DS2 */
  *byte++ = 0XF2;

  /* Cnt2 */
  *byte++ = (length2 >> 0) & 0XFF;
  *byte++ = (length2 >> 8) & 0XFF;

  /* D2 */
  if (data2) byte = mempcpy(byte, data2, length2);

  /* DE2 */
  *byte++ = 0XF3;

  /* Reserved */
  {
    int count = 4;
    while (count--) *byte++ = 0;
  }

  /* Chk */
  *(checksum = byte++) = 0;

  /* DE */
  *byte++ = 0XFD;
  *byte++ = 0XFD;

  {
    unsigned char sum = 0;
    const unsigned char *ptr = packet;
    while (ptr != byte) sum += *ptr++;
    *checksum = sum;
  }

  return writeBytes(brl, packet, byte - packet);
}


static int
writeCellCountRequest_serial (BrailleDisplay *brl) {
  static const unsigned char data[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };

  return writePacket(brl, 0XFB, 0X01, data, sizeof(data), NULL, 0);
}

static BrailleResponseResult
isCellCountResponse_serial (BrailleDisplay *brl, const void *packet, size_t size) {
  const InputPacket *response = packet;

  return (response->data.type == IPT_CELLS)? BRL_RSP_DONE: BRL_RSP_UNEXPECTED;
}

static int
getCellCount_serial (BrailleDisplay *brl, unsigned int *count) {
  InputPacket response;

  int probed = probeBrailleDisplay(
    brl, 2, NULL, 1000, writeCellCountRequest_serial,
    readBytes, &response, sizeof(response.bytes),
    isCellCountResponse_serial
  );

  if (!probed) return 0;
  *count = response.data.data;
  return 1;
}

static int
readCommands_serial (BrailleDisplay *brl) {
  InputPacket packet;
  int length;

  while ((length = readPacket(brl, &packet))) {
    switch (packet.data.type) {
      case IPT_CURSOR: {
        unsigned char key = packet.data.data;

        enqueueKey(brl, HM_GRP_RoutingKeys, key);
        continue;
      }

      case IPT_KEYS: {
        KeyNumberSet bits = (packet.data.reserved[0] << 0X00)
                          | (packet.data.reserved[1] << 0X08)
                          | (packet.data.reserved[2] << 0X10)
                          | (packet.data.reserved[3] << 0X18);

        enqueueKeys(brl, bits, HM_GRP_NavigationKeys, 0);
        continue;
      }

      default:
        break;
    }

    logUnexpectedPacket(&packet, length);
  }

  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
}

static int
writeCells_serial (BrailleDisplay *brl, const unsigned char *cells, size_t count) {
  return writePacket(brl, 0XFC, 0X01, cells, count, NULL, 0);
}

static const InputOutputOperations serialOperations = {
  .getCellCount = getCellCount_serial,
  .readCommands = readCommands_serial,
  .writeCells = writeCells_serial,
};


static int
getCellCount_HID (BrailleDisplay *brl, unsigned int *count) {
  HidReportSize *reportSize = &brl->data->hid.reportSize;
  if (!gioGetHidReportSize(brl->gioEndpoint, 0, reportSize)) return 0;

  size_t cellCount = reportSize->output;
  if (!cellCount) return 0;

  *count = cellCount;
  return 1;
}

typedef struct {
  unsigned char mask;
  KeyValue key;
} HidInputBit;

typedef struct {
  unsigned char offset;
  HidInputBit bits[8];
} HidInputByte;

static const HidInputByte hidInputBytes[11] = {
  { .offset = 0,
    .bits = {
      {.mask=0X01, .key={.group=HM_GRP_NavigationKeys, .number=HM_KEY_Dot1}},
      {.mask=0X02, .key={.group=HM_GRP_NavigationKeys, .number=HM_KEY_Dot2}},
      {.mask=0X04, .key={.group=HM_GRP_NavigationKeys, .number=HM_KEY_Dot3}},
      {.mask=0X08, .key={.group=HM_GRP_NavigationKeys, .number=HM_KEY_Dot4}},
      {.mask=0X10, .key={.group=HM_GRP_NavigationKeys, .number=HM_KEY_Dot5}},
      {.mask=0X20, .key={.group=HM_GRP_NavigationKeys, .number=HM_KEY_Dot6}},
      {.mask=0X40, .key={.group=HM_GRP_NavigationKeys, .number=HM_KEY_Dot7}},
      {.mask=0X80, .key={.group=HM_GRP_NavigationKeys, .number=HM_KEY_Dot8}},
    }
  },

  { .offset = 1,
    .bits = {
      {.mask=0X01, .key={.group=HM_GRP_RoutingKeys, .number= 0}},
      {.mask=0X02, .key={.group=HM_GRP_RoutingKeys, .number= 1}},
      {.mask=0X04, .key={.group=HM_GRP_RoutingKeys, .number= 2}},
      {.mask=0X08, .key={.group=HM_GRP_RoutingKeys, .number= 3}},
      {.mask=0X10, .key={.group=HM_GRP_RoutingKeys, .number= 4}},
      {.mask=0X20, .key={.group=HM_GRP_RoutingKeys, .number= 5}},
      {.mask=0X40, .key={.group=HM_GRP_RoutingKeys, .number= 6}},
      {.mask=0X80, .key={.group=HM_GRP_RoutingKeys, .number= 7}},
    }
  },

  { .offset = 2,
    .bits = {
      {.mask=0X01, .key={.group=HM_GRP_RoutingKeys, .number= 8}},
      {.mask=0X02, .key={.group=HM_GRP_RoutingKeys, .number= 9}},
      {.mask=0X04, .key={.group=HM_GRP_RoutingKeys, .number=10}},
      {.mask=0X08, .key={.group=HM_GRP_RoutingKeys, .number=11}},
      {.mask=0X10, .key={.group=HM_GRP_RoutingKeys, .number=12}},
      {.mask=0X20, .key={.group=HM_GRP_RoutingKeys, .number=13}},
      {.mask=0X40, .key={.group=HM_GRP_RoutingKeys, .number=14}},
      {.mask=0X80, .key={.group=HM_GRP_RoutingKeys, .number=15}},
    }
  },

  { .offset = 3,
    .bits = {
      {.mask=0X01, .key={.group=HM_GRP_RoutingKeys, .number=16}},
      {.mask=0X02, .key={.group=HM_GRP_RoutingKeys, .number=17}},
      {.mask=0X04, .key={.group=HM_GRP_RoutingKeys, .number=18}},
      {.mask=0X08, .key={.group=HM_GRP_RoutingKeys, .number=19}},
      {.mask=0X10, .key={.group=HM_GRP_RoutingKeys, .number=20}},
      {.mask=0X20, .key={.group=HM_GRP_RoutingKeys, .number=21}},
      {.mask=0X40, .key={.group=HM_GRP_RoutingKeys, .number=22}},
      {.mask=0X80, .key={.group=HM_GRP_RoutingKeys, .number=23}},
    }
  },

  { .offset = 4,
    .bits = {
      {.mask=0X01, .key={.group=HM_GRP_RoutingKeys, .number=24}},
      {.mask=0X02, .key={.group=HM_GRP_RoutingKeys, .number=25}},
      {.mask=0X04, .key={.group=HM_GRP_RoutingKeys, .number=26}},
      {.mask=0X08, .key={.group=HM_GRP_RoutingKeys, .number=27}},
      {.mask=0X10, .key={.group=HM_GRP_RoutingKeys, .number=28}},
      {.mask=0X20, .key={.group=HM_GRP_RoutingKeys, .number=29}},
      {.mask=0X40, .key={.group=HM_GRP_RoutingKeys, .number=30}},
      {.mask=0X80, .key={.group=HM_GRP_RoutingKeys, .number=31}},
    }
  },

  { .offset = 5,
    .bits = {
      {.mask=0X01, .key={.group=HM_GRP_RoutingKeys, .number=32}},
      {.mask=0X02, .key={.group=HM_GRP_RoutingKeys, .number=33}},
      {.mask=0X04, .key={.group=HM_GRP_RoutingKeys, .number=34}},
      {.mask=0X08, .key={.group=HM_GRP_RoutingKeys, .number=35}},
      {.mask=0X10, .key={.group=HM_GRP_RoutingKeys, .number=36}},
      {.mask=0X20, .key={.group=HM_GRP_RoutingKeys, .number=37}},
      {.mask=0X40, .key={.group=HM_GRP_RoutingKeys, .number=38}},
      {.mask=0X80, .key={.group=HM_GRP_RoutingKeys, .number=39}},
    }
  },

  { .offset = 7,
    .bits = {
      {.mask=0X01, .key={.group=HM_GRP_NavigationKeys, .number=HM_KEY_EM_F1}},
      {.mask=0X04, .key={.group=HM_GRP_NavigationKeys, .number=HM_KEY_EM_F2}},
      {.mask=0X08, .key={.group=HM_GRP_NavigationKeys, .number=HM_KEY_BE_RightScrollUp}},
      {.mask=0X10, .key={.group=HM_GRP_NavigationKeys, .number=HM_KEY_EM_F3}},
      {.mask=0X40, .key={.group=HM_GRP_NavigationKeys, .number=HM_KEY_EM_F4}},
      {.mask=0X80, .key={.group=HM_GRP_NavigationKeys, .number=HM_KEY_BE_RightScrollDown}},
    }
  },

  { .offset = 8,
    .bits = {
      {.mask=0X01, .key={.group=HM_GRP_NavigationKeys, .number=HM_KEY_EM_Control}},
      {.mask=0X02, .key={.group=HM_GRP_NavigationKeys, .number=HM_KEY_EM_Alt}},
    }
  },

  { .offset = 10,
    .bits = {
      {.mask=0X01, .key={.group=HM_GRP_NavigationKeys, HM_KEY_Space}},
    }
  },
};

static int
readCommands_HID (BrailleDisplay *brl) {
  while (1) {
    const size_t reportSize = brl->data->hid.reportSize.input;
    unsigned char *oldReport = brl->data->hid.pressedKeys;
    unsigned char newReport[reportSize];

    const ssize_t length = gioReadData(brl->gioEndpoint, newReport, reportSize, 1);
    if (!length) return EOF;
    if (length == -1) return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
    logInputPacket(newReport, length);

    if (length < reportSize) {
      memset(&newReport[length], 0, (reportSize - length));
    }

    const HidInputByte *byte = hidInputBytes;
    const HidInputByte *byteEnd = byte + ARRAY_COUNT(hidInputBytes);

    while (byte < byteEnd) {
      unsigned char *newByte = newReport + byte->offset;
      unsigned char *oldByte = oldReport + byte->offset;

      if (*newByte != *oldByte) {
        const HidInputBit *bit = byte->bits;
        const HidInputBit *bitEnd = bit + ARRAY_COUNT(byte->bits);

        while (bit < bitEnd) {
          if (!bit->mask) break;
          unsigned char newBit = *newByte & bit->mask;
          unsigned char oldBit = *oldByte & bit->mask;

          if (newBit != oldBit) {
            KeyValue key = bit->key;

            if (newBit) {
              enqueueKeyEvent(brl, key.group, key.number, 1);
              *oldByte |= newBit;
            } else {
              enqueueKeyEvent(brl, key.group, key.number, 0);
              *oldByte &= ~oldBit;
            }

            if (*oldByte == *newByte) break;
          }

          bit += 1;
        }
      }

      byte += 1;
    }
  }
}

static int
writeCells_HID (BrailleDisplay *brl, const unsigned char *cells, size_t count) {
  return writeBytes(brl, cells, count);
}

static const InputOutputOperations hidOperations = {
  .getCellCount = getCellCount_HID,
  .readCommands = readCommands_HID,
  .writeCells = writeCells_HID,
};


static int
getDefaultCellCount_BrailleSense (BrailleDisplay *brl, unsigned int *count) {
  *count = 32;
  return 1;
}

static const ProtocolEntry protocol_BrailleSense = {
  .modelName = "Braille Sense",
  .keyTable = &KEY_TABLE_DEFINITION(pan),
  .testIdentities = testIdentities_BrailleSense,
  .getDefaultCellCount = getDefaultCellCount_BrailleSense,
  .io = &serialOperations
};

static const ProtocolEntry protocol_BrailleSense6 = {
  .modelName = "BrailleSense 6",
  .resourceNamePrefix = "H632B",
  .keyTable = &KEY_TABLE_DEFINITION(scroll),
  .testIdentities = testIdentities_BrailleSense6,
  .getDefaultCellCount = getDefaultCellCount_BrailleSense,
  .io = &serialOperations
};


static int
getDefaultCellCount_SyncBraille (BrailleDisplay *brl, unsigned int *count) {
  return 0;
}

static const ProtocolEntry protocol_SyncBraille = {
  .modelName = "SyncBraille",
  .keyTable = &KEY_TABLE_DEFINITION(sync),
  .getDefaultCellCount = getDefaultCellCount_SyncBraille,
  .io = &serialOperations
};


static int
getDefaultCellCount_BrailleEdge (BrailleDisplay *brl, unsigned int *count) {
  *count = 40;
  return 1;
}

static const ProtocolEntry protocol_BrailleEdge = {
  .modelName = "Braille Edge",
  .resourceNamePrefix = "BrailleEDGE",
  .keyTable = &KEY_TABLE_DEFINITION(edge),
  .testIdentities = testIdentities_BrailleEdge,
  .getDefaultCellCount = getDefaultCellCount_BrailleEdge,
  .io = &serialOperations
};


static int
getDefaultCellCount_eMotion (BrailleDisplay *brl, unsigned int *count) {
  *count = 40;
  return 1;
}

static const ProtocolEntry protocol_eMotion = {
  .modelName = "eMotion (legacy)",
  .keyTable = &KEY_TABLE_DEFINITION(emotion),
  .getDefaultCellCount = getDefaultCellCount_eMotion,
  .io = &serialOperations
};

static const ProtocolEntry protocol_eMotionHID = {
  .modelName = "eMotion (HID)",
  .keyTable = &KEY_TABLE_DEFINITION(emotion),
  .getDefaultCellCount = getDefaultCellCount_eMotion,
  .io = &hidOperations
};


static const ProtocolEntry *protocolTable[] = {
  &protocol_BrailleSense,
  &protocol_SyncBraille,
  &protocol_BrailleEdge,
  &protocol_BrailleSense6,
  NULL
};


static int
writeCells (BrailleDisplay *brl) {
  const size_t count = MIN(brl->textColumns*brl->textRows, MAXIMUM_CELL_COUNT);
  unsigned char cells[count];
  translateOutputCells(cells, brl->data->previousCells, count);

  return brl->data->protocol->io->writeCells(brl, cells, count);
}

static int
clearCells (BrailleDisplay *brl) {
  memset(brl->data->previousCells, 0, MIN(brl->textColumns*brl->textRows, MAXIMUM_CELL_COUNT));
  return writeCells(brl);
}

static int
getCellCount (BrailleDisplay *brl, unsigned int *count) {
  if (brl->data->protocol->io->getCellCount(brl, count)) {
    logMessage(LOG_CATEGORY(BRAILLE_DRIVER), "explicit cell count: %u", *count);
  } else if (brl->data->protocol->getDefaultCellCount(brl, count)) {
    logMessage(LOG_CATEGORY(BRAILLE_DRIVER), "default cell count: %u", *count);
  } else {
    logMessage(LOG_CATEGORY(BRAILLE_DRIVER), "unknown cell count");
    return 0;
  }

  return 1;
}

static void
setKeyTable (BrailleDisplay *brl, const KeyTableDefinition *ktd) {
  if (!ktd) ktd = brl->data->protocol->keyTable;

  switch (brl->textColumns) {
    case 14:
      if (ktd == &KEY_TABLE_DEFINITION(scroll)) {
        ktd = &KEY_TABLE_DEFINITION(beetle);
      }
      break;
  }

  setBrailleKeyTable(brl, ktd);
}

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 115200
  };

  BEGIN_USB_STRING_LIST(usbManufacturers_0403_6001)
    "FTDI",
  END_USB_STRING_LIST

  BEGIN_USB_CHANNEL_DEFINITIONS
    { /* Braille Sense (USB 1.1) */
      .version = UsbSpecificationVersion_1_1,
      .vendor=0X045E, .product=0X930A,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1,
      .data=&protocol_BrailleSense
    },

    { /* Braille Sense (USB 2.0) */
      .version = UsbSpecificationVersion_2_0,
      .vendor=0X045E, .product=0X930A,
      .configuration=1, .interface=1, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .verifyInterface=1,
      .disableAutosuspend=1,
      .data=&protocol_BrailleSense
    },

    { /* Braille Sense U2 (USB 2.0) */
      .version = UsbSpecificationVersion_2_0,
      .vendor=0X045E, .product=0X930A,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .verifyInterface=1,
      .disableAutosuspend=1,
      .data=&protocol_BrailleSense
    },

    { /* BrailleSense 6 (USB 2.1) */
      .version = UsbSpecificationVersion_2_1,
      .vendor=0X045E, .product=0X930A,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=1,
      .verifyInterface=1,
      .resetDevice=1,
      .disableAutosuspend=1,
      .data=&protocol_BrailleSense6
    },

    { /* Sync Braille */
      .vendor=0X0403, .product=0X6001,
      .manufacturers = usbManufacturers_0403_6001,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .data=&protocol_SyncBraille
    },

    { /* Braille Edge and QBrailleXL */
      .vendor=0X045E, .product=0X930B,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .disableAutosuspend=1,
      .data=&protocol_BrailleEdge
    },

    { /* eMotion (legacy) */
      .vendor=0X1A86, .product=0X55D3,
      .configuration=1, .interface=1, .alternative=0,
      .inputEndpoint=2, .outputEndpoint=2,
      .serial = &serialParameters,
      .disableAutosuspend=1,
      .data=&protocol_eMotion
    },

    { /* eMotion (HID) */
      .vendor=0X045E, .product=0X940A,
      .configuration=1, .interface=2, .alternative=0,
      .inputEndpoint=4, .outputEndpoint=3,
      .disableAutosuspend=1,
      .data=&protocol_eMotionHID
    },
  END_USB_CHANNEL_DEFINITIONS

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.serial.parameters = &serialParameters;
  descriptor.serial.options.applicationData = &protocol_BrailleSense;

  descriptor.usb.channelDefinitions = usbChannelDefinitions;

  descriptor.bluetooth.channelNumber = 4;
  descriptor.bluetooth.discoverChannel = 1;

  if (connectBrailleResource(brl, identifier, &descriptor, NULL)) {
    return 1;
  }

  return 0;
}

static const ProtocolEntry *
getProtocol (BrailleDisplay *brl) {
  char *name = gioGetResourceName(brl->gioEndpoint);

  if (name) {
    const ProtocolEntry *const *protocolAddress = protocolTable;

    while (*protocolAddress) {
      const ProtocolEntry *protocol = *protocolAddress;
      const char *prefix = protocol->resourceNamePrefix;

      if (prefix) {
        if (strncasecmp(name, prefix, strlen(prefix)) == 0) {
          return protocol;
        }
      }

      protocolAddress += 1;
    }

    free(name);
  }

  return &protocol_BrailleSense;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));

    if (connectResource(brl, device)) {
      if (!(brl->data->protocol = gioGetApplicationData(brl->gioEndpoint))) {
        brl->data->protocol = getProtocol(brl);
      }

      logMessage(LOG_INFO, "detected: %s", brl->data->protocol->modelName);

      if (getCellCount(brl, &brl->textColumns)) {
        brl->textRows = 1;

        {
          const KeyTableDefinition *ktd =
            brl->data->protocol->testIdentities?
            brl->data->protocol->testIdentities(brl):
            NULL;

          setKeyTable(brl, ktd);
        }

        makeOutputTable(dotsTable_ISO11548_1);
        if (clearCells(brl)) return 1;
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
    free(brl->data);
    brl->data = NULL;
  }
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  size_t count = brl->textColumns * brl->textRows;

  if (cellsHaveChanged(brl->data->previousCells, brl->buffer, count, NULL, NULL, NULL)) {
    if (!writeCells(brl)) return 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  return brl->data->protocol->io->readCommands(brl);
}
