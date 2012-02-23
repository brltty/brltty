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
#include "timing.h"
#include "message.h"
#include "ascii.h"
#include "eu_protocol.h"
#include "eu_keys.h"
#include "brldefs-eu.h"
#include "ktbdefs.h"

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
  COMMAND_KEY_ENTRY(LA, "LA"),
  COMMAND_KEY_ENTRY(RA, "RA"),
  COMMAND_KEY_ENTRY(UA, "UA"),
  COMMAND_KEY_ENTRY(DA, "DA"),
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

DEFINE_KEY_TABLE(iris)
DEFINE_KEY_TABLE(esys_small)
DEFINE_KEY_TABLE(esys_medium)
DEFINE_KEY_TABLE(esys_large)
DEFINE_KEY_TABLE(esytime)

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
  const KeyTableDefinition *keyTable;
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
    .isIris = 1,
    .keyTable = &KEY_TABLE_DEFINITION(iris)
  },

  [IRIS_40] = {
    .modelName = "Iris 40",
    .isIris = 1,
    .keyTable = &KEY_TABLE_DEFINITION(iris)
  },

  [IRIS_S20] = {
    .modelName = "Iris S-20",
    .isIris = 1,
    .keyTable = &KEY_TABLE_DEFINITION(iris)
  },

  [IRIS_S32] = {
    .modelName = "Iris S-32",
    .isIris = 1,
    .keyTable = &KEY_TABLE_DEFINITION(iris)
  },

  [IRIS_KB20] = {
    .modelName = "Iris KB-20",
    .isIris = 1,
    .keyTable = &KEY_TABLE_DEFINITION(iris)
  },

  [IRIS_KB40] = {
    .modelName = "Iris KB-40",
    .isIris = 1,
    .keyTable = &KEY_TABLE_DEFINITION(iris)
  },

  [ESYS_12] = {
    .modelName = "Esys 12",
    .isEsys = 1,
    .keyTable = &KEY_TABLE_DEFINITION(esys_small)
  },

  [ESYS_40] = {
    .modelName = "Esys 40",
    .isEsys = 1,
    .keyTable = &KEY_TABLE_DEFINITION(esys_medium)
  },

  [ESYS_LIGHT_40] = {
    .modelName = "Esys Light 40",
    .isEsys = 1,
    .keyTable = &KEY_TABLE_DEFINITION(esys_medium)
  },

  [ESYS_24] = {
    .modelName = "Esys 24",
    .isEsys = 1,
    .keyTable = &KEY_TABLE_DEFINITION(esys_small)
  },

  [ESYS_64] = {
    .modelName = "Esys 64",
    .isEsys = 1,
    .keyTable = &KEY_TABLE_DEFINITION(esys_medium)
  },

  [ESYS_80] = {
    .modelName = "Esys 80",
    .isEsys = 1,
    .keyTable = &KEY_TABLE_DEFINITION(esys_large)
  },

  [ESYTIME_32] = {
    .modelName = "Esytime 32",
    .isEsytime = 1,
    .keyTable = &KEY_TABLE_DEFINITION(esytime)
  },

  [ESYTIME_32_STANDARD] = {
    .modelName = "Esytime 32 Standard",
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

static int routingMode;
static int forceRewrite;
static int keyReadError;

static unsigned char sequenceCheck;
static unsigned char sequenceKnown;
static unsigned char sequenceNumber;


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
  unsigned int key = EOF;
  switch(packet[0])
    {
    case 'B':
      key = (packet[1] * 256 + packet[2]) & 0x000003ff;
      key |= EUBRL_BRAILLE_KEY;
      break;
    case 'I':
      key = packet[2] & 0xbf;
      key |= EUBRL_ROUTING_KEY;
      break;
    case 'C':
      {
	if (model->isEsys)
	  {
            key = (packet[1] << 24) + (packet[2] << 16) + (packet[3] << 8) + packet[4];
	  }
	else
	  {
	    key = (packet[1] * 256 + packet[2]) & 0x00000fff;
	  }
      }
      key |= EUBRL_COMMAND_KEY;
      break;
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

static int esysiris_handleCommandKey(BrailleDisplay *brl, unsigned int key)
{
  int res = EOF;
  unsigned int subkey = 0;
  static char flagLevel1 = 0, flagLevel2 = 0;

  if (model->isIris)
    { /** Iris models keys */
      if (key == VK_FDB && !flagLevel1)
	{
	  flagLevel2 = !flagLevel2;
	  if (flagLevel2) message(NULL, gettext("level 2 ..."), MSG_NODELAY);
	}
      else if (key == VK_FGB && !flagLevel2)
	{
	  flagLevel1 = !flagLevel1;
	  if (flagLevel1) message(NULL, gettext("level 1 ..."), MSG_NODELAY);
	}
      if (flagLevel1)
	{
	  while ((subkey = esysiris_readKey(brl)) == 0) approximateDelay(20);
	  flagLevel1 = 0;
	  switch (subkey & 0x00000fff)
	    {
	    case VK_L1 : res = BRL_CMD_TOP_LEFT; break;
	    case VK_L3 : res = BRL_CMD_PRSEARCH; break;
	    case VK_L4 : res = BRL_CMD_HELP; break;
	    case VK_L5 : res = BRL_CMD_LEARN; break;
	    case VK_L6 : res = BRL_CMD_NXSEARCH; break;
	    case VK_L8 : res = BRL_CMD_BOT_LEFT; break;
	    case VK_FG : res = BRL_CMD_LNBEG; break;
	    case VK_FD : res = BRL_CMD_LNEND; break;
	    case VK_FH : res = BRL_CMD_HOME; break;
	    case VK_FB : res = BRL_CMD_RETURN; break;
	    default: res = BRL_CMD_NOOP; break;
	    }
	}
      else if (flagLevel2)
	{
	  while ((subkey = esysiris_readKey(brl)) == 0) approximateDelay(20);
	  flagLevel2 = 0;
	  switch (subkey & 0x00000fff)
	    {
	    case VK_L1: routingMode = BRL_BLK_CUTBEGIN; break;
	    case VK_L2: routingMode = BRL_BLK_CUTAPPEND; break;
	    case VK_L3 : res = BRL_CMD_CSRVIS; break;
	    case VK_L6: routingMode = BRL_BLK_CUTRECT; break;
	    case VK_L7: res = BRL_CMD_PASTE; break;
	    case VK_L8: routingMode = BRL_BLK_CUTLINE; break;
	    case VK_FH: res = BRL_CMD_PREFMENU; break;
	    case VK_FB: res = BRL_CMD_CSRTRK; break;
	    case VK_FD : res = BRL_CMD_TUNES; break;
	    default: res = BRL_CMD_NOOP; break;
	    }
	}
      else
	{
	  switch (key)
	    {
	    case VK_NONE:     res = BRL_CMD_NOOP; break;
	    case VK_L1:	res = BRL_CMD_FWINLT; break;
	    case VK_L2:	res = BRL_CMD_LNUP; break;
	    case VK_L3:	res = BRL_CMD_PRPROMPT; break;
	    case VK_L4:	res = BRL_CMD_PREFMENU; break;
	    case VK_L5:	res = BRL_CMD_INFO; break;
	    case VK_L6:	res = BRL_CMD_NXPROMPT; break;
	    case VK_L7:	res = BRL_CMD_LNDN; break;
	    case VK_L8:	res = BRL_CMD_FWINRT; break;
	    case VK_FB:	res = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_DOWN; break;
	    case VK_FH:	res = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_UP; break;
	    case VK_FG:	res = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_LEFT; break;
	    case VK_FD:	res = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_RIGHT; break;
	    case VK_L12:	res = BRL_CMD_TOP_LEFT; break;
	    case VK_L34:        res = BRL_CMD_FREEZE; break;
	    case VK_L67:	res = BRL_CMD_HOME; break;
	    case VK_L78:	res = BRL_CMD_BOT_LEFT; break;
	    case VK_L1234:	res = BRL_CMD_RESTARTBRL; break;
	    case VK_L5678:	res = BRL_CMD_RESTARTSPEECH; break;
	    }
	}
    }
  if (model->isEsys)
    { /** Esys models keys */
      if (key == VK_M1G || key == VK_M2G || key == VK_M3G || key == VK_M4G) res = BRL_CMD_FWINLT;
      if (key == VK_M1D || key == VK_M2D || key == VK_M3D || key == VK_M4D) res = BRL_CMD_FWINRT;
      if (key == VK_JDG) res = BRL_CMD_FWINLT;
      if (key == VK_JDH) res = BRL_CMD_LNUP;
      if (key == VK_JDD) res = BRL_CMD_FWINRT;
      if (key == VK_JDB) res = BRL_CMD_LNDN;

      if (key == VK_JDM) res = BRL_CMD_HOME;

      if (key == VK_M1M || key == VK_M2M || key == VK_M3M || key == VK_M4M) res = BRL_CMD_FREEZE;

      if (key == VK_JGH) res = BRL_CMD_TOP_LEFT;
      if (key == VK_JGB) res = BRL_CMD_BOT_LEFT;

      if (key == (VK_JGD | VK_JDG)) routingMode = BRL_BLK_CUTBEGIN;
      if (key == (VK_JGD | VK_JDD)) routingMode = BRL_BLK_CUTLINE;
      if (key == (VK_JGD | VK_JDM)) res = BRL_CMD_PASTE;

      if (key == (VK_JGG | VK_JDG)) res = BRL_CMD_SAY_SOFTER;
      if (key == (VK_JGG | VK_JDH)) res = BRL_CMD_SAY_ABOVE;
      if (key == (VK_JGG | VK_JDD)) res = BRL_CMD_SAY_LOUDER;
      if (key == (VK_JGG | VK_JDB)) res = BRL_CMD_SAY_BELOW;
      if (key == (VK_JGG | VK_JDM)) res = BRL_CMD_SAY_LINE;
      if ((key == (VK_JGG | VK_M1G)) || (key == (VK_JGG | VK_M2G)) || (key == (VK_JGG | VK_M3G)) || (key == (VK_JGG | VK_M4G))) res = BRL_CMD_SAY_SLOWER;
      if ((key == (VK_JGG | VK_M1D)) || (key == (VK_JGG | VK_M2D)) || (key == (VK_JGG | VK_M3D)) || (key == (VK_JGG | VK_M4D))) res = BRL_CMD_SAY_FASTER;
      if ((key == (VK_JGG | VK_M1M)) || (key == (VK_JGG | VK_M2M)) || (key == (VK_JGG | VK_M3M)) || (key == (VK_JGG | VK_M4M))) res = BRL_CMD_MUTE;

      if ((key == (VK_JGD | VK_M1G)) || (key == (VK_JGD | VK_M2G)) || (key == (VK_JGD | VK_M3G)) || (key == (VK_JGD | VK_M4G))) res = BRL_CMD_LNBEG;
      if ((key == (VK_JGD | VK_M1D)) || (key == (VK_JGD | VK_M2D)) || (key == (VK_JGD | VK_M3D)) || (key == (VK_JGD | VK_M4D))) res = BRL_CMD_LNEND;

      if (key == (VK_JGD | VK_JDH)) res = BRL_CMD_LEARN;
      if (key == (VK_JGD | VK_JDB)) res = BRL_CMD_HELP;
      if (key == (VK_JGD | VK_M1M)) res = BRL_CMD_PREFMENU;
      if (key == (VK_JGD | VK_M4M)) res = BRL_CMD_CSRTRK;
    }
  return res;
}

static int	esysiris_keyToCommand(BrailleDisplay *brl, int key, KeyTableCommandContext ctx)
{
  int res = EOF;

  if (key == EOF) return EOF;
  if (key == 0) return EOF;

  if (key & EUBRL_BRAILLE_KEY)
    {
      res = eubrl_handleBrailleKey(key, ctx);
    }
  else if (key & EUBRL_ROUTING_KEY)
    {
      res = routingMode | ((key - 1) & 0x0000007f);
      routingMode = BRL_BLK_ROUTE;
    }
  else if (key & EUBRL_COMMAND_KEY)
    {
      if (model->isEsys) {
        res = esysiris_handleCommandKey(brl, key & 0x7fffffff);
      } else {
        res = esysiris_handleCommandKey(brl, key & 0x00000fff);
      }
    }
  else if (key & EUBRL_PC_KEY)
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

static int	esysiris_init(BrailleDisplay *brl)
{
  char outPacket[2] = {'S', 'I'};
  int leftTries = 24;
      
  modelIdentifier = IRIS_UNKNOWN;
  model = NULL;

  haveSystemInformation = 0;
  firmwareVersion = 0;
  protocolVersion = 0;
  deviceOptions = 0;
  maximumFrameLength = 0;

  routingMode = BRL_BLK_ROUTE;
  forceRewrite = 1;
  keyReadError = 0;

  sequenceCheck = 0;
  sequenceKnown = 0;

  while (leftTries-- && !haveSystemInformation)
    {
      if (esysiris_writePacket(brl, (unsigned char *)outPacket, 2) == -1)
	{
	  logMessage(LOG_WARNING, "eu: EsysIris: Failed to send ident request.");
	  leftTries = 0;
	  continue;
	}
      int i=60;
      while(i-- && !haveSystemInformation)
        {
          esysiris_readCommand(brl, KTB_CTX_DEFAULT);
          approximateDelay(10);
        }
      approximateDelay(100);
    }
  if (haveSystemInformation)
    { /* Succesfully identified model. */
      model = &modelTable[modelIdentifier];

      if (!maximumFrameLength) {
        if (model->isIris) maximumFrameLength = 2048;
        if (model->isEsys) maximumFrameLength = 128;
      }

      logMessage(LOG_INFO, "Model Detected: %s (%u cells)",
	         model->modelName, brl->textColumns);
      return (1);
    }
  return (0);
}

static int	esysiris_resetDevice(BrailleDisplay *brl)
{
  (void)brl;
  return 1;
}


static void	esysiris_writeWindow(BrailleDisplay *brl)
{
  static unsigned char previousBrailleWindow[80];
  int displaySize = brl->textColumns * brl->textRows;
  unsigned char buf[displaySize + 2];
  
  if (displaySize > sizeof(previousBrailleWindow)) {
    logMessage(LOG_WARNING, "[eu] Discarding too large braille window");
    return;
  }

  if (cellsHaveChanged(previousBrailleWindow, brl->buffer, displaySize, NULL, NULL, &forceRewrite)) {
    buf[0] = 'B';
    buf[1] = 'S';
    memcpy(buf + 2, brl->buffer, displaySize);
    esysiris_writePacket(brl, buf, sizeof(buf));
  }
}

static int	esysiris_hasLcdSupport(BrailleDisplay *brl)
{
  return (0);
}

static void	esysiris_writeVisual(BrailleDisplay *brl, const wchar_t *text)
{
  return;
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
  .hasLcdSupport = esysiris_hasLcdSupport,
  .writeVisual = esysiris_writeVisual
};
