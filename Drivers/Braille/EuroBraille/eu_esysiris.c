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

/** EuroBraille/eu_esysiris.c 
 ** Implements the ESYS and IRIS rev >=1.71 protocol 
 ** Made by Yannick PLASSIARD <yan@mistigri.org>
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "log.h"
#include "ascii.h"
#include "eu_protocol.h"
#include "eu_keys.h"
#include "brldefs-eu.h"

#define KEY_ENTRY(s,t,k,n) {.value = {.set=EU_SET_##s, .key=EU_##t##_##k}, .name=n}
#define COMMAND_KEY_ENTRY(k,n) KEY_ENTRY(CommandKeys, CMD, k, n)
#define BRAILLE_KEY_ENTRY(k,n) KEY_ENTRY(BrailleKeys, BRL, k, n)

BEGIN_KEY_NAME_TABLE(linear)
  COMMAND_KEY_ENTRY(L1, "L1"),
  COMMAND_KEY_ENTRY(L2, "L2"),
  COMMAND_KEY_ENTRY(L3, "L3"),
  COMMAND_KEY_ENTRY(L4, "L4"),
  COMMAND_KEY_ENTRY(L5, "L5"),
  COMMAND_KEY_ENTRY(L6, "L6"),
  COMMAND_KEY_ENTRY(L7, "L7"),
  COMMAND_KEY_ENTRY(L8, "L8"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(arrow)
  COMMAND_KEY_ENTRY(LA, "LeftArrow"),
  COMMAND_KEY_ENTRY(RA, "RightArrow"),
  COMMAND_KEY_ENTRY(UA, "UpArrow"),
  COMMAND_KEY_ENTRY(DA, "DownArrow"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(function)
  COMMAND_KEY_ENTRY(F1, "L1"),
  COMMAND_KEY_ENTRY(F2, "L2"),
  COMMAND_KEY_ENTRY(F3, "L3"),
  COMMAND_KEY_ENTRY(F4, "L4"),
  COMMAND_KEY_ENTRY(F5, "L5"),
  COMMAND_KEY_ENTRY(F6, "L6"),
  COMMAND_KEY_ENTRY(F7, "L7"),
  COMMAND_KEY_ENTRY(F8, "L8"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(switch1)
  COMMAND_KEY_ENTRY(S1L, "S1L"),
  COMMAND_KEY_ENTRY(S1R, "S1R"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(switch2)
  COMMAND_KEY_ENTRY(S2L, "S2L"),
  COMMAND_KEY_ENTRY(S2R, "S2R"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(switch3)
  COMMAND_KEY_ENTRY(S3L, "S3L"),
  COMMAND_KEY_ENTRY(S3R, "S3R"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(switch4)
  COMMAND_KEY_ENTRY(S4L, "S4L"),
  COMMAND_KEY_ENTRY(S4R, "S4R"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(switch5)
  COMMAND_KEY_ENTRY(S5L, "S5L"),
  COMMAND_KEY_ENTRY(S5R, "S5R"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(switch6)
  COMMAND_KEY_ENTRY(S6L, "S6L"),
  COMMAND_KEY_ENTRY(S6R, "S6R"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(joystick1)
  COMMAND_KEY_ENTRY(J1L, "J1L"),
  COMMAND_KEY_ENTRY(J1R, "J1R"),
  COMMAND_KEY_ENTRY(J1U, "J1U"),
  COMMAND_KEY_ENTRY(J1D, "J1D"),
  COMMAND_KEY_ENTRY(J1P, "J1P"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(joystick2)
  COMMAND_KEY_ENTRY(J2L, "J2L"),
  COMMAND_KEY_ENTRY(J2R, "J2R"),
  COMMAND_KEY_ENTRY(J2U, "J2U"),
  COMMAND_KEY_ENTRY(J2D, "J2D"),
  COMMAND_KEY_ENTRY(J2P, "J2P"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(common)
  BRAILLE_KEY_ENTRY(Dot1, "Dot1"),
  BRAILLE_KEY_ENTRY(Dot2, "Dot2"),
  BRAILLE_KEY_ENTRY(Dot3, "Dot3"),
  BRAILLE_KEY_ENTRY(Dot4, "Dot4"),
  BRAILLE_KEY_ENTRY(Dot5, "Dot5"),
  BRAILLE_KEY_ENTRY(Dot6, "Dot6"),
  BRAILLE_KEY_ENTRY(Dot7, "Dot7"),
  BRAILLE_KEY_ENTRY(Dot8, "Dot8"),
  BRAILLE_KEY_ENTRY(Backspace, "Backspace"),
  BRAILLE_KEY_ENTRY(Space, "Space"),

  KEY_SET_ENTRY(EU_SET_RoutingKeys1, "RoutingKey1"),
  KEY_SET_ENTRY(EU_SET_RoutingKeys2, "RoutingKey2"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(iris)
  KEY_NAME_TABLE(linear),
  KEY_NAME_TABLE(arrow),
  KEY_NAME_TABLE(common),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(esys_small)
  KEY_NAME_TABLE(switch1),
  KEY_NAME_TABLE(switch2),
  KEY_NAME_TABLE(joystick1),
  KEY_NAME_TABLE(joystick2),
  KEY_NAME_TABLE(common),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(esys_medium)
  KEY_NAME_TABLE(switch1),
  KEY_NAME_TABLE(switch2),
  KEY_NAME_TABLE(switch3),
  KEY_NAME_TABLE(switch4),
  KEY_NAME_TABLE(joystick1),
  KEY_NAME_TABLE(joystick2),
  KEY_NAME_TABLE(common),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(esys_large)
  KEY_NAME_TABLE(switch1),
  KEY_NAME_TABLE(switch2),
  KEY_NAME_TABLE(switch3),
  KEY_NAME_TABLE(switch4),
  KEY_NAME_TABLE(switch5),
  KEY_NAME_TABLE(switch6),
  KEY_NAME_TABLE(joystick1),
  KEY_NAME_TABLE(joystick2),
  KEY_NAME_TABLE(common),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(esytime)
  KEY_NAME_TABLE(function),
  KEY_NAME_TABLE(joystick1),
  KEY_NAME_TABLE(joystick2),
  KEY_NAME_TABLE(common),
END_KEY_NAME_TABLES

PUBLIC_KEY_TABLE(iris)
PUBLIC_KEY_TABLE(esys_small)
PUBLIC_KEY_TABLE(esys_medium)
PUBLIC_KEY_TABLE(esys_large)
PUBLIC_KEY_TABLE(esytime)

typedef enum {
  IRIS_UNKNOWN        = 0X00,
  IRIS_20             = 0X01,
  IRIS_40             = 0X02,
  IRIS_S20            = 0X03,
  IRIS_S32            = 0X04,
  IRIS_KB20           = 0X05,
  IRIS_KB40           = 0X06,
  ESYS_12             = 0X07,
  ESYS_40             = 0X08,
  ESYS_LIGHT_40       = 0X09,
  ESYS_24             = 0X0A,
  ESYS_64             = 0X0B,
  ESYS_80             = 0X0C,
  ESYTIME_32          = 0X0E,
  ESYTIME_32_STANDARD = 0X0F
} ModelIdentifier;

typedef struct {
  const char *modelName;
  unsigned char cellCount;
  const KeyTableDefinition *keyTable;
  unsigned hasBrailleKeyboard:1;
  unsigned hasAzertyKeyboard:1;
  unsigned hasVisualDisplay:1;
  unsigned hasOpticalRouting:1;
  unsigned isIris:1;
  unsigned isEsys:1;
  unsigned isEsytime:1;
} ModelEntry;

static const ModelEntry modelTable[] = {
  [IRIS_UNKNOWN] = {
    .modelName = "Iris (unknown)",
    .isIris = 1,
    .keyTable = &KEY_TABLE_DEFINITION(iris)
  },

  [IRIS_20] = {
    .modelName = "Iris 20",
    .cellCount = 20,
    .hasBrailleKeyboard = 1,
    .hasVisualDisplay = 1,
    .isIris = 1,
    .keyTable = &KEY_TABLE_DEFINITION(iris)
  },

  [IRIS_40] = {
    .modelName = "Iris 40",
    .cellCount = 40,
    .hasBrailleKeyboard = 1,
    .hasVisualDisplay = 1,
    .isIris = 1,
    .keyTable = &KEY_TABLE_DEFINITION(iris)
  },

  [IRIS_S20] = {
    .modelName = "Iris S-20",
    .cellCount = 20,
    .hasBrailleKeyboard = 1,
    .isIris = 1,
    .keyTable = &KEY_TABLE_DEFINITION(iris)
  },

  [IRIS_S32] = {
    .modelName = "Iris S-32",
    .cellCount = 32,
    .hasBrailleKeyboard = 1,
    .isIris = 1,
    .keyTable = &KEY_TABLE_DEFINITION(iris)
  },

  [IRIS_KB20] = {
    .modelName = "Iris KB-20",
    .cellCount = 20,
    .hasAzertyKeyboard = 1,
    .hasVisualDisplay = 1,
    .isIris = 1,
    .keyTable = &KEY_TABLE_DEFINITION(iris)
  },

  [IRIS_KB40] = {
    .modelName = "Iris KB-40",
    .cellCount = 40,
    .hasAzertyKeyboard = 1,
    .hasVisualDisplay = 1,
    .isIris = 1,
    .keyTable = &KEY_TABLE_DEFINITION(iris)
  },

  [ESYS_12] = {
    .modelName = "Esys 12",
    .cellCount = 12,
    .hasBrailleKeyboard = 1,
    .isEsys = 1,
    .keyTable = &KEY_TABLE_DEFINITION(esys_small)
  },

  [ESYS_40] = {
    .modelName = "Esys 40",
    .cellCount = 40,
    .hasBrailleKeyboard = 1,
    .isEsys = 1,
    .keyTable = &KEY_TABLE_DEFINITION(esys_medium)
  },

  [ESYS_LIGHT_40] = {
    .modelName = "Esys Light 40",
    .cellCount = 40,
    .isEsys = 1,
    .keyTable = &KEY_TABLE_DEFINITION(esys_medium)
  },

  [ESYS_24] = {
    .modelName = "Esys 24",
    .cellCount = 24,
    .hasBrailleKeyboard = 1,
    .isEsys = 1,
    .keyTable = &KEY_TABLE_DEFINITION(esys_small)
  },

  [ESYS_64] = {
    .modelName = "Esys 64",
    .cellCount = 64,
    .hasBrailleKeyboard = 1,
    .isEsys = 1,
    .keyTable = &KEY_TABLE_DEFINITION(esys_medium)
  },

  [ESYS_80] = {
    .modelName = "Esys 80",
    .cellCount = 80,
    .hasBrailleKeyboard = 1,
    .isEsys = 1,
    .keyTable = &KEY_TABLE_DEFINITION(esys_large)
  },

  [ESYTIME_32] = {
    .modelName = "Esytime 32",
    .cellCount = 32,
    .hasBrailleKeyboard = 1,
    .hasOpticalRouting = 1,
    .isEsytime = 1,
    .keyTable = &KEY_TABLE_DEFINITION(esytime)
  },

  [ESYTIME_32_STANDARD] = {
    .modelName = "Esytime 32 Standard",
    .cellCount = 32,
    .hasBrailleKeyboard = 1,
    .isEsytime = 1,
    .keyTable = &KEY_TABLE_DEFINITION(esytime)
  },
};


/** Static Local Variables */

static unsigned char modelIdentifier;
static const ModelEntry *model;

static int haveSystemInformation;
static uint32_t firmwareVersion;
static uint32_t protocolVersion;
static uint32_t deviceOptions;
static uint16_t maximumFrameLength;

static int forceRewrite;
static int keyReadError;

static unsigned char sequenceCheck;
static unsigned char sequenceKnown;
static unsigned char sequenceNumber;

static uint32_t commandKeys;


/*** Local functions */


static inline void
LogUnknownProtocolKey(const char *function, unsigned char key) {
  logMessage(LOG_NOTICE,"[eu] %s: unknown protocol key %c (%x)",function,key,key);
}

static ssize_t esysiris_readPacket(BrailleDisplay *brl, void *packet, size_t size)
{
  unsigned char *buffer = packet;
  const unsigned char pad = 0X55;
  unsigned int offset = 0;
  unsigned int length = 3;

  while (1)
    {
      int started = offset > 0;
      unsigned char byte;

      if (!io->readByte(brl, &byte, started))
        {
          if (started) logPartialPacket(buffer, offset);
          return (errno == EAGAIN)? 0: -1;
        }

      switch (offset)
        {
          case 0: {
            unsigned char sequence = sequenceCheck;
            sequenceCheck = 0;

            if (sequence && sequenceKnown) {
              if (byte == ++sequenceNumber) continue;
              logInputProblem("Unexpected Sequence Number", &byte, 1);
              sequenceKnown = 0;
            }

            if (byte == pad) continue;
            if (byte == STX) break;

            if (sequence && !sequenceKnown) {
              sequenceNumber = byte;
              sequenceKnown = 1;
            } else {
              logIgnoredByte(byte);
            }

            continue;
          }

          case 1:
            if ((byte == pad) && !sequenceKnown) {
              sequenceNumber = buffer[0];
              sequenceKnown = 1;
              offset = 0;
              continue;
            }
            break;

          case 2:
            length = ((buffer[1] << 8) | byte) + 2;
            break;

          default:
            break;
        }

      if (offset < size)
        {
          buffer[offset] = byte;
        }
      else
        {
          if (offset == length) logTruncatedPacket(buffer, offset);
          logDiscardedByte(byte);
        }

      if (++offset == length)
        {
          if (byte != ETX)
            {
              logCorruptPacket(buffer, offset);
              offset = 0;
              length = 3;
              continue;
            }

          sequenceCheck = 1;
          logInputPacket(buffer, offset);
          return offset;
        }
    }
}

static ssize_t esysiris_writePacket(BrailleDisplay *brl, const void *packet, size_t size)
{
  int packetSize = size + 2;
  unsigned char buf[packetSize + 2];
  if (!io || !packet || !size)
    return (-1);
  buf[0] = STX;
  buf[1] = (packetSize >> 8) & 0x00FF;
  buf[2] = packetSize & 0x00FF;
  memcpy(buf + 3, packet, size);
  buf[sizeof(buf)-1] = ETX;
  logOutputPacket(buf, sizeof(buf));
  return io->writeData(brl, buf, sizeof(buf));
}

static int
handleSystemInformation(BrailleDisplay *brl, unsigned char *packet) {
  int logLevel = LOG_INFO;
  const char *infoDescription;
  enum {Unknown, End, String, Dec8, Dec16, Hex32} infoType;

  switch(packet[0]) {
    case 'H': 
      infoType = String;
      infoDescription = "Short Name";
      break;

    case 'I': 
      infoType = End;
      break;

    case 'G': 
      brl->textColumns = packet[1];

      infoType = Dec8;
      infoDescription = "Cell Count";
      break;

    case 'L': 
      infoType = String;
      infoDescription = "Country Code";
      break;

    case 'M': 
      maximumFrameLength = (packet[1] << 8)
                         | (packet[2] << 0)
                         ;

      infoType = Dec16;
      infoDescription = "Maximum Frame Length";
      break;

    case 'N': 
      infoType = String;
      infoDescription = "Long Name";
      break;

    case 'O': 
      deviceOptions = (packet[1] << 24)
                    | (packet[2] << 16)
                    | (packet[3] <<  8)
                    | (packet[4] <<  0)
                    ;

      infoType = Hex32;
      infoDescription = "Device Options";
      break;

    case 'P': 
      protocolVersion = ((packet[1] - '0') << 16)
                      | ((packet[3] - '0') <<  8)
                      | ((packet[4] - '0') <<  0)
                      ;

      infoType = String;
      infoDescription = "Protocol Version";
      break;

    case 'S': 
      infoType = String;
      infoDescription = "Serial Number";
      break;

    case 'T':
      modelIdentifier = packet[1];
      if ((modelIdentifier >= ARRAY_COUNT(modelTable)) ||
          !modelTable[modelIdentifier].modelName) {
        logMessage(LOG_WARNING, "unknown Esysiris model: 0X%02X", modelIdentifier);
        modelIdentifier = IRIS_UNKNOWN;
      }

      infoType = Dec8;
      infoDescription = "Model Identifier";
      break;

    case 'W': 
      firmwareVersion = ((packet[1] - '0') << 16)
                      | ((packet[3] - '0') <<  8)
                      | ((packet[4] - '0') <<  0)
                      ;

      infoType = String;
      infoDescription = "Firmware Version";
      break;

    default:
      infoType = Unknown;
      break;
  }

  switch (infoType) {
    case Unknown:
      logMessage(LOG_WARNING, "unknown Esysiris system information subcode: 0X%02X", packet[0]);
      break;

    case End:
      logMessage(LOG_DEBUG, "end of Esysiris system information");
      return 1;

    case String:
      logMessage(logLevel, "Esysiris %s: %s", infoDescription, &packet[1]);
      break;

    case Dec8:
      logMessage(logLevel, "Esysiris %s: %u", infoDescription, packet[1]);
      break;

    case Dec16:
      logMessage(logLevel, "Esysiris %s: %u", infoDescription, (packet[1] << 8) | packet[2]);
      break;

    case Hex32:
      logMessage(logLevel, "Esysiris %s: 0X%02X%02X%02X%02X",
                 infoDescription, packet[1], packet[2], packet[3], packet[4]);
      break;

    default:
      logMessage(LOG_WARNING, "unimplemented Esysiris system information subcode type: 0X%02X", infoType);
      break;
  }

  return 0;
}

static int esysiris_KeyboardHandling(BrailleDisplay *brl, unsigned char *packet)
{
  int key = EOF;

  switch(packet[0])
    {
    case 'B': {
      uint32_t keys = ((packet[1] << 8) | packet[2]) & 0X3Ff;
      enqueueKeys(keys, EU_SET_BrailleKeys, 0);
      break;
    }

    case 'I': {
      unsigned char key = packet[2];

      if ((key > 0) && (key <= brl->textColumns)) {
        key -= 1;

        switch (packet[1]) {
          case 1:
            enqueueKey(EU_SET_RoutingKeys1, key);
            break;

          case 3:
            enqueueKey(EU_SET_RoutingKeys2, key);
            break;

          default:
            break;
        }
      }

      break;
    }

    case 'C': {
      uint32_t keys;

      if (model->isIris) {
        keys = ((packet[1] << 8) | packet[2]) & 0XFFF;
      } else {
        keys = (packet[1] << 24) + (packet[2] << 16) + (packet[3] << 8) + packet[4];
      }

      if (model->isIris) {
        enqueueKeys(keys, EU_SET_CommandKeys, 0);
      } else {
        enqueueUpdatedKeys(keys, &commandKeys, EU_SET_CommandKeys, 0);
      }
      break;
    }

    case 'Z':
      {
        unsigned char a = packet[1];
        unsigned char b = packet[2];
        unsigned char c = packet[3];
        unsigned char d = packet[4];
        key = 0;
        logMessage(LOG_DEBUG, "PC key %x %x %x %x", a, b, c, d);
        if (!a) {
          if (d)
            key = EUBRL_PC_KEY | BRL_BLK_PASSCHAR | d;
          else if (b == 0x08)
            key = EUBRL_PC_KEY | BRL_BLK_PASSKEY | BRL_KEY_BACKSPACE;
          else if (b >= 0x70 && b <= 0x7b) {
            int functionKey = b - 0x70;
            key = EUBRL_PC_KEY | BRL_BLK_PASSKEY | (BRL_KEY_FUNCTION + functionKey);
          } else if (b)
            key = EUBRL_PC_KEY | BRL_BLK_PASSCHAR | b;
          if (c & 0x02)
            key |= BRL_FLG_CHAR_CONTROL;
          if (c & 0x04)
            key |= BRL_FLG_CHAR_META;
        } else if (a == 1) {
          switch (b)
            {
            case 0x07:
              key = EUBRL_PC_KEY | BRL_BLK_PASSKEY | BRL_KEY_HOME;
              break;
            case 0x08:
              key = EUBRL_PC_KEY | BRL_BLK_PASSKEY | BRL_KEY_END;
              break;
            case 0x09:
              key = EUBRL_PC_KEY | BRL_BLK_PASSKEY | BRL_KEY_PAGE_UP;
              break;
            case 0x0a:
              key = EUBRL_PC_KEY | BRL_BLK_PASSKEY | BRL_KEY_PAGE_DOWN;
              break;
            case 0x0b:
              key = EUBRL_PC_KEY | BRL_BLK_PASSKEY | BRL_KEY_CURSOR_LEFT;
              break;
            case 0x0c:
              key = EUBRL_PC_KEY | BRL_BLK_PASSKEY | BRL_KEY_CURSOR_RIGHT;
              break;
            case 0x0d:
              key = EUBRL_PC_KEY | BRL_BLK_PASSKEY | BRL_KEY_CURSOR_UP;
              break;
            case 0x0e:
              key = EUBRL_PC_KEY | BRL_BLK_PASSKEY | BRL_KEY_CURSOR_DOWN;
              break;
            case 0x10:
              key = EUBRL_PC_KEY | BRL_BLK_PASSKEY | BRL_KEY_DELETE;
              break;
            default:
              break;
            }
        }
    }
    break;
    }
  return key;
}

static int	esysiris_readKey(BrailleDisplay *brl)
{
  unsigned char	packet[2048];
  ssize_t size = esysiris_readPacket(brl, packet, sizeof(packet));
  int res = 0;

  if (size == 0) return EOF;

  if (size == -1) {
    keyReadError = 1;
    return EOF;
  }

  switch (packet[3])
    {
    case 'S':
      if (handleSystemInformation(brl, packet+4)) haveSystemInformation = 1;
      break;

    case 'K':
      res = esysiris_KeyboardHandling(brl, packet + 4);
      break;

    case 'R':
      if (packet[4] == 'P') {
        /* return from internal menu */
        forceRewrite = 1;
      }
      break;

    case 'V':
      /* ignore visualization */
      break;

    default:
      LogUnknownProtocolKey("esysiris_readKey", packet[3]);
      break;
    }

  return res;
}

static int	esysiris_keyToCommand(BrailleDisplay *brl, int key, KeyTableCommandContext ctx)
{
  int res = EOF;

  if (key == EOF) return EOF;
  if (key == 0) return EOF;

  if (key & EUBRL_PC_KEY)
    {
      res = key & 0xFFFFFF;
    }

  return res;
}

static int	esysiris_readCommand(BrailleDisplay *brl, KeyTableCommandContext ctx)
{
  int key = esysiris_readKey(brl);
  if (keyReadError) return BRL_CMD_RESTARTBRL;
  return esysiris_keyToCommand(brl, key, ctx);
}

static int
esysiris_init(BrailleDisplay *brl) {
  static const unsigned char packet[] = {'S', 'I'};
  int leftTries = 4;
      
  modelIdentifier = IRIS_UNKNOWN;
  model = NULL;

  haveSystemInformation = 0;
  firmwareVersion = 0;
  protocolVersion = 0;
  deviceOptions = 0;
  maximumFrameLength = 0;

  forceRewrite = 1;
  keyReadError = 0;

  sequenceCheck = 0;
  sequenceKnown = 0;

  commandKeys = 0;

  while (leftTries-- && !haveSystemInformation) {
    if (esysiris_writePacket(brl, packet, sizeof(packet)) != -1) {
      while (io->awaitInput(500)) {
        esysiris_readCommand(brl, KTB_CTX_DEFAULT);

        if (haveSystemInformation) {
          model = &modelTable[modelIdentifier];

          {
            const KeyTableDefinition *ktd = model->keyTable;

            brl->keyBindings = ktd->bindings;
            brl->keyNameTables = ktd->names;
          }

          if (!maximumFrameLength) {
            if (model->isIris) maximumFrameLength = 2048;
            if (model->isEsys) maximumFrameLength = 128;
          }

          logMessage(LOG_INFO, "Model Detected: %s (%u cells)",
                     model->modelName, brl->textColumns);
          return 1;
        }
      }

      if (errno != EAGAIN) leftTries = 0;
    } else {
      logMessage(LOG_WARNING, "eu: EsysIris: Failed to send ident request.");
      leftTries = 0;
    }
  }

  return 0;
}

static int	esysiris_resetDevice(BrailleDisplay *brl)
{
  (void)brl;
  return 1;
}


static int	esysiris_writeWindow(BrailleDisplay *brl)
{
  static unsigned char previousBrailleWindow[80];
  int displaySize = brl->textColumns * brl->textRows;
  
  if (displaySize > sizeof(previousBrailleWindow)) {
    logMessage(LOG_WARNING, "[eu] Discarding too large braille window");
    return 0;
  }

  if (cellsHaveChanged(previousBrailleWindow, brl->buffer, displaySize, NULL, NULL, &forceRewrite)) {
    unsigned char buf[displaySize + 2];

    buf[0] = 'B';
    buf[1] = 'S';
    memcpy(buf + 2, brl->buffer, displaySize);

    if (esysiris_writePacket(brl, buf, sizeof(buf)) == -1) return 0;
  }

  return 1;
}

static int	esysiris_hasVisualDisplay(BrailleDisplay *brl)
{
  return model->hasVisualDisplay;
}

static int	esysiris_writeVisual(BrailleDisplay *brl, const wchar_t *text)
{
  if (model->hasVisualDisplay) {
  }

  return 1;
}

const t_eubrl_protocol esysirisProtocol = {
  .name = "esysiris",

  .init = esysiris_init,
  .resetDevice = esysiris_resetDevice,

  .readPacket = esysiris_readPacket,
  .writePacket = esysiris_writePacket,

  .readKey = esysiris_readKey,
  .readCommand = esysiris_readCommand,
  .keyToCommand = esysiris_keyToCommand,

  .writeWindow = esysiris_writeWindow,
  .hasVisualDisplay = esysiris_hasVisualDisplay,
  .writeVisual = esysiris_writeVisual
};
