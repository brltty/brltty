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

/** EuroBraille/eu_clio.c 
 ** Implements the NoteBraille/Clio/Scriba/Iris <= 1.70 protocol 
 ** Made by Olivier BER` <obert01@mistigri.org>
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "log.h"
#include "timing.h"
#include "ascii.h"
#include "brldefs-eu.h"
#include "eu_protocol.h"

#define BRAILLE_KEY_ENTRY(k,n) KEY_ENTRY(BrailleKeys, DOT, k, n)
#define NAVIGATION_KEY_ENTRY(k,n) KEY_ENTRY(NavigationKeys, NAV, k, n)

BEGIN_KEY_NAME_TABLE(braille)
  BRAILLE_KEY_ENTRY(1, "Dot1"),
  BRAILLE_KEY_ENTRY(2, "Dot2"),
  BRAILLE_KEY_ENTRY(3, "Dot3"),
  BRAILLE_KEY_ENTRY(4, "Dot4"),
  BRAILLE_KEY_ENTRY(5, "Dot5"),
  BRAILLE_KEY_ENTRY(6, "Dot6"),
  BRAILLE_KEY_ENTRY(7, "Dot7"),
  BRAILLE_KEY_ENTRY(8, "Dot8"),
  BRAILLE_KEY_ENTRY(B, "Backspace"),
  BRAILLE_KEY_ENTRY(S, "Space"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(function)
  NAVIGATION_KEY_ENTRY(E, "E"),
  NAVIGATION_KEY_ENTRY(F, "F"),
  NAVIGATION_KEY_ENTRY(G, "G"),
  NAVIGATION_KEY_ENTRY(H, "H"),
  NAVIGATION_KEY_ENTRY(I, "I"),
  NAVIGATION_KEY_ENTRY(J, "J"),
  NAVIGATION_KEY_ENTRY(K, "K"),
  NAVIGATION_KEY_ENTRY(L, "L"),
  NAVIGATION_KEY_ENTRY(M, "M"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(keypad)
  NAVIGATION_KEY_ENTRY(One, "One"),
  NAVIGATION_KEY_ENTRY(Two, "Up"),
  NAVIGATION_KEY_ENTRY(Three, "Three"),
  NAVIGATION_KEY_ENTRY(A, "A"),

  NAVIGATION_KEY_ENTRY(Four, "Left"),
  NAVIGATION_KEY_ENTRY(Five, "Five"),
  NAVIGATION_KEY_ENTRY(Six, "Right"),
  NAVIGATION_KEY_ENTRY(B, "B"),

  NAVIGATION_KEY_ENTRY(Seven, "Seven"),
  NAVIGATION_KEY_ENTRY(Eight, "Down"),
  NAVIGATION_KEY_ENTRY(Nine, "Nine"),
  NAVIGATION_KEY_ENTRY(C, "C"),

  NAVIGATION_KEY_ENTRY(Star, "Star"),
  NAVIGATION_KEY_ENTRY(Zero, "Zero"),
  NAVIGATION_KEY_ENTRY(Sharp, "Sharp"),
  NAVIGATION_KEY_ENTRY(D, "D"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(sets)
  KEY_SET_ENTRY(EU_SET_RoutingKeys1, "RoutingKey"),
  KEY_SET_ENTRY(EU_SET_SeparatorKeys, "SeparatorKey"),
  KEY_SET_ENTRY(EU_SET_StatusKeys, "StatusKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(clio)
  KEY_NAME_TABLE(braille),
  KEY_NAME_TABLE(function),
  KEY_NAME_TABLE(keypad),
  KEY_NAME_TABLE(sets),
END_KEY_NAME_TABLES

PUBLIC_KEY_TABLE(clio)

/* Communication codes */
# define PRT_E_PAR 0x01		/* parity error */
# define PRT_E_NUM 0x02		/* frame numver error */
# define PRT_E_ING 0x03		/* length error */
# define PRT_E_COM 0x04		/* command error */
# define PRT_E_DON 0x05		/* data error */
# define PRT_E_SYN 0x06		/* syntax error */

#define	INPUT_BUFFER_SIZE 1024
#define MAXIMUM_DISPLAY_SIZE 80

typedef enum {
  UNKNOWN = 0x00,
  CE2,
  CE4,
  CE8,
  CN2,
  CN4,
  CN8,
  CP2,
  CP4,
  CP8,
  CZ4,
  JN2,
  NB2,
  NB4, 
  NB8, 
  SB2,
  SB4,
  SC2,
  SC4,
  IR2,
  IR4,
  IS2,
  IS3,
  TYPE_LAST
} clioModel;

typedef struct {
  unsigned char modelIdentifier;
  char modelCode[3];
  const char *modelName;
  unsigned char cellCount;
  unsigned isAzerBraille:1;
  unsigned isEuroBraille:1;
  unsigned isIris:1;
  unsigned isNoteBraille:1;
  unsigned isPupiBraille:1;
  unsigned isScriba:1;
  unsigned hasClioInteractive:1;
  unsigned hasVisualDisplay:1;
} ModelEntry;

static const ModelEntry modelTable[] = {
  { .modelIdentifier = UNKNOWN,
    .modelCode = "",
    .modelName = ""
  },

  { .modelIdentifier = CE2,
    .modelCode = "CE2",
    .modelName = "Clio-EuroBraille 20",
    .cellCount = 20,
    .hasClioInteractive = 1,
    .isEuroBraille = 1
  },

  { .modelIdentifier = CE4,
    .modelCode = "CE4",
    .modelName = "Clio-EuroBraille 40",
    .cellCount = 40,
    .hasClioInteractive = 1,
    .isEuroBraille = 1
  },

  { .modelIdentifier = CE8,
    .modelCode = "CE8",
    .modelName = "Clio-EuroBraille 80",
    .cellCount = 80,
    .hasClioInteractive = 1,
    .isEuroBraille = 1
  },

  { .modelIdentifier = CN2,
    .modelCode = "CN2",
    .modelName = "Clio-NoteBraille 20",
    .cellCount = 20,
    .hasClioInteractive = 1,
    .hasVisualDisplay = 1,
    .isNoteBraille = 1
  },

  { .modelIdentifier = CN4,
    .modelCode = "CN4",
    .modelName = "Clio-NoteBraille 40",
    .cellCount = 40,
    .hasClioInteractive = 1,
    .hasVisualDisplay = 1,
    .isNoteBraille = 1
  },

  { .modelIdentifier = CN8,
    .modelCode = "CN8",
    .modelName = "Clio-NoteBraille 80",
    .cellCount = 80,
    .hasClioInteractive = 1,
    .hasVisualDisplay = 1,
    .isNoteBraille = 1
  },

  { .modelIdentifier = CP2,
    .modelCode = "Cp2",
    .modelName = "Clio-PupiBraille 20",
    .cellCount = 20,
    .hasClioInteractive = 1,
    .isPupiBraille = 1
  },

  { .modelIdentifier = CP4,
    .modelCode = "Cp4",
    .modelName = "Clio-PupiBraille 40",
    .cellCount = 40,
    .hasClioInteractive = 1,
    .isPupiBraille = 1
  },

  { .modelIdentifier = CP8,
    .modelCode = "Cp8",
    .modelName = "Clio-PupiBraille 80",
    .cellCount = 80,
    .hasClioInteractive = 1,
    .isPupiBraille = 1
  },

  { .modelIdentifier = CZ4,
    .modelCode = "CZ4",
    .modelName = "Clio-AzerBraille 40",
    .cellCount = 40,
    .hasClioInteractive = 1,
    .hasVisualDisplay = 1,
    .isAzerBraille = 1
  },

  { .modelIdentifier = JN2,
    .modelCode = "JN2",
    .modelName = "",
    .cellCount = 20,
    .hasVisualDisplay = 1,
    .isNoteBraille = 1
  },

  { .modelIdentifier = NB2,
    .modelCode = "NB2",
    .modelName = "NoteBraille 20",
    .cellCount = 20,
    .hasVisualDisplay = 1,
    .isNoteBraille = 1
  },

  { .modelIdentifier = NB4,
    .modelCode = "NB4",
    .modelName = "NoteBraille 40",
    .cellCount = 40,
    .hasVisualDisplay = 1,
    .isNoteBraille = 1
  },

  { .modelIdentifier = NB8,
    .modelCode = "NB8",
    .modelName = "NpoteBraille 80",
    .cellCount = 80,
    .hasVisualDisplay = 1,
    .isNoteBraille = 1
  },

  { .modelIdentifier = SB2,
    .modelCode = "SB2",
    .modelName = "Scriba 20",
    .cellCount = 20,
    .hasClioInteractive = 1,
    .isScriba = 1
  },

  { .modelIdentifier = SB4,
    .modelCode = "SB4",
    .modelName = "Scriba 40",
    .cellCount = 40,
    .hasClioInteractive = 1,
    .isScriba = 1
  },

  { .modelIdentifier = SC2,
    .modelCode = "SC2",
    .modelName = "Scriba 20",
    .cellCount = 20
  },

  { .modelIdentifier = SC4,
    .modelCode = "SC4",
    .modelName = "Scriba 40",
    .cellCount = 40
  },

  { .modelIdentifier = IR2,
    .modelCode = "IR2",
    .modelName = "Iris 20",
    .cellCount = 20,
    .hasVisualDisplay = 1,
    .isIris = 1
  },

  { .modelIdentifier = IR4,
    .modelCode = "IR4",
    .modelName = "Iris 40",
    .cellCount = 40,
    .hasVisualDisplay = 1,
    .isIris = 1
  },

  { .modelIdentifier = IS2,
    .modelCode = "IS2",
    .modelName = "Iris S20",
    .cellCount = 20,
    .isIris = 1
  },

  { .modelIdentifier = IS3,
    .modelCode = "IS3",
    .modelName = "Iris S32",
    .cellCount = 32,
    .isIris = 1
  },

  { .modelIdentifier = TYPE_LAST,
    .modelCode = "",
    .modelName = ""
  },
};

/** Static Local Variables */

static int brlCols = 0; /* Number of braille cells */
static clioModel brlModel = 0; /* brl display model currently used */
static unsigned char firmwareVersion[21];
static int forceWindowRewrite;
static int forceVisualRewrite;
static int inputPacketNumber;
static int outputPacketNumber;

static int
needsEscape (unsigned char byte) {
  switch (byte) {
    case SOH:
    case EOT:
    case DLE:
    case ACK:
    case NAK:
      return 1;
  }

  return 0;
}

static ssize_t
readPacket (BrailleDisplay *brl, void *packet, size_t size) {
  unsigned char buffer[size + 4];
  int offset = 0;
  int escape = 0;

  while (1)
    {
      int started = offset > 0;
      int escaped = 0;
      unsigned char byte;

      if (!io->readByte(brl, &byte, (started || escape)))
        {
          if (started) logPartialPacket(buffer, offset);
          return (errno == EAGAIN)? 0: -1;
        }

      if (escape)
        {
          escape = 0;
          escaped = 1;
        }
      else if (byte == DLE)
        {
          escape = 1;
          continue;
        }

      if (!escaped)
        {
          switch (byte)
            {
            case SOH:
              if (started)
                {
                  logShortPacket(buffer, offset);
                  offset = 1;
                  continue;
                }
              goto addByte;

            case EOT:
              break;

            default:
              if (needsEscape(byte))
                {
                  if (started) logShortPacket(buffer, offset);
                  offset = 0;
                  continue;
                }
              break;
            }
        }

      if (!started)
        {
          logIgnoredByte(byte);
          continue;
        }

    addByte:
      if (offset < sizeof(buffer))
        {
          buffer[offset] = byte;
        }
      else
        {
          if (offset == sizeof(buffer)) logTruncatedPacket(buffer, offset);
          logDiscardedByte(byte);
        }
      offset += 1;

      if (!escaped && (byte == EOT))
        {
          if (offset > sizeof(buffer))
            {
              offset = 0;
              continue;
            }

          logInputPacket(buffer, offset);
          offset -= 1; /* remove EOT */

          {
            unsigned char parity = 0;

            {
              int i;

              for (i=1; i<offset; i+=1)
                {
                  parity ^= buffer[i];
                }
            }

            if (parity) {
              const unsigned char message[] = {NAK, PRT_E_PAR};

              io->writeData(brl, message, sizeof(message));
              offset = 0;
              continue;
            }
          }

          offset -= 1; /* remove parity */

          {
            static const unsigned char message[] = {ACK};
            io->writeData(brl, message, sizeof(message));
          }

          if (buffer[--offset] == inputPacketNumber)
            {
              offset = 0;
              continue;
            }
          inputPacketNumber = buffer[offset];

          memcpy(packet, &buffer[1], offset-1);
          return offset;
        }
    }
}

static ssize_t
writePacket (BrailleDisplay *brl, const void *packet, size_t size) {
  /* limit case, every char is escaped */
  unsigned char		buf[(size + 3) * 2]; 
  unsigned char		*q = buf;
  const unsigned char *p = packet;
  unsigned char		parity = 0;
  size_t packetSize;

  *q++ = SOH;
  while (size--)
    {
	if (needsEscape(*p)) *q++ = DLE;
        *q++ = *p;
	parity ^= *p++;
     }
   *q++ = outputPacketNumber; /* Doesn't need to be prefixed since greater than 128 */
   parity ^= outputPacketNumber;
   if (++outputPacketNumber >= 256)
     outputPacketNumber = 128;
   if (needsEscape(parity)) *q++ = DLE;
   *q++ = parity;
   *q++ = EOT;
   packetSize = q - buf;
   logOutputPacket(buf, packetSize);
   return io->writeData(brl, buf, packetSize);
}

static int
resetDevice (BrailleDisplay *brl) {
  static const unsigned char packet[] = {0X02, 'S', 'I'};

  logMessage(LOG_INFO, "eu Clio hardware reset requested");
  if (writePacket(brl, packet, sizeof(packet)) == -1)
    {
      logMessage(LOG_WARNING, "Clio: Failed to send ident request.");
      return -1;
    }
  return 1;
}

static void
handleSystemInformation (BrailleDisplay *brl, const void *packet) {
  const char *p = packet;
  unsigned char ln = 0;
  int i;

  while (1)
    {
      ln = *(p++);
      if (ln == 22 && 
	  (!strncmp(p, "SI", 2) || !strncmp(p, "si", 2 )))
	{
	  memcpy(firmwareVersion, p+2, 20);
	  break;
	}
      else
	p += ln;
    }
  switch (firmwareVersion[2]) {
  case '2' : 
    brlCols = 20;
    break;
  case '4' : 
    brlCols = 40;
    break;
  case '3' : 
    brlCols = 32;
    break;
  case '8' : 
    brlCols = 80;
    break;
  default : 
    brlCols = 20;
    break;
  }
  i = 0;
  while (modelTable[i].modelIdentifier != TYPE_LAST)
    {
      if (!strncasecmp(modelTable[i].modelCode, (char*)firmwareVersion, 3))
	break;
      i++;
    }
  brlModel = modelTable[i].modelIdentifier;
  brl->resizeRequired = 1;
}

static int
writeWindow (BrailleDisplay *brl) {
  static unsigned char previousCells[MAXIMUM_DISPLAY_SIZE];
  size_t size = brl->textColumns * brl->textRows;
  unsigned char buffer[size + 3];

  if (cellsHaveChanged(previousCells, brl->buffer, size, NULL, NULL, &forceWindowRewrite)) {
    buffer[0] = size + 2;
    buffer[1] = 'D';
    buffer[2] = 'P';
    translateOutputCells(buffer+3, brl->buffer, size);
    writePacket(brl, buffer, sizeof(buffer));
  }

  return 1;
}

static int
writeVisual (BrailleDisplay *brl, const wchar_t *text) {
  static wchar_t previousText[MAXIMUM_DISPLAY_SIZE];
  size_t size = brl->textColumns * brl->textRows;
  unsigned char buffer[size + 3];

  if (textHasChanged(previousText, text, size, NULL, NULL, &forceVisualRewrite)) {
    int i;

    buffer[0] = size + 2;
    buffer[1] = 'D';
    buffer[2] = 'L';

    for (i=0; i<size; i+=1) {
      wchar_t wc = text[i];
      buffer[i+3] = iswLatin1(wc)? wc: '?';
    }

    writePacket(brl, buffer, sizeof(buffer));
  }

  return 1;
}

static int
hasVisualDisplay (BrailleDisplay *brl) {
  return (1);
}

static int
handleMode (BrailleDisplay *brl, const unsigned char *packet) {
  if (*packet == 'B') {
    forceWindowRewrite = 1;
    forceVisualRewrite = 1;
    return 1;
  }

  return 0;
}

static int
handleKeyEvent (BrailleDisplay *brl, const unsigned char *packet) {
  switch (packet[0]) {
    case 'B': {
      unsigned int keys = ((packet[2] << 8) | packet[1]) & 0X3FF;
      enqueueKeys(keys, EU_SET_BrailleKeys, 0);
      return 1;
    }

    case 'I': {
      unsigned char key = packet[1];

      if (key >= 0X88) {
        enqueueKey(EU_SET_StatusKeys, key-0X88);
        return 1;
      }

      if (key >= 0X81) {
        enqueueKey(EU_SET_SeparatorKeys, key-0X81);
        return 1;
      }

      if (key >= 1) {
        enqueueKey(EU_SET_RoutingKeys1, key-1);
        return 1;
      }

      break;
    }

    case 'T': 
      enqueueKey(EU_SET_NavigationKeys, packet[1]);
      return 1;

    default :
      break;
  }

  return 0;
}

static int
readCommand (BrailleDisplay *brl, KeyTableCommandContext ctx) {
  unsigned char	packet[INPUT_BUFFER_SIZE];
  ssize_t length;

  while ((length = readPacket(brl, packet, sizeof(packet))) > 0) {
    switch (packet[1]) {
      case 'S': 
        handleSystemInformation(brl, packet);
        continue;

      case 'R': 
        if (handleMode(brl, packet+2)) continue;
        break;

      case 'K': 
        if (handleKeyEvent(brl, packet+2)) continue;
        break;

      default: 
        break;
    }

    logUnexpectedPacket(packet, length);
  }

  return (length == -1)? BRL_CMD_RESTARTBRL: EOF;
}

static int
initializeDevice (BrailleDisplay *brl) {
  int leftTries = 2;

  brlCols = 0;
  memset(firmwareVersion, 0, sizeof(firmwareVersion));
  forceWindowRewrite = 1;
  forceVisualRewrite = 1;
  inputPacketNumber = -1;
  outputPacketNumber = 127;

  while (leftTries-- && brlCols == 0)
    {
      resetDevice(brl);      
      approximateDelay(500);
      readCommand(brl, KTB_CTX_DEFAULT);
    }
  if (brlCols > 0)
    { /* Succesfully identified hardware. */
      brl->textRows = 1;
      brl->textColumns = brlCols;

      {
        const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(clio);
        brl->keyBindings = ktd->bindings;
        brl->keyNameTables = ktd->names;
      }

      logMessage(LOG_INFO, "eu: %s connected.",
	         modelTable[brlModel].modelName);
      return (1);
    }
  return (0);
}

const ProtocolOperations clioProtocolOperations = {
  .protocolName = "clio",

  .initializeDevice = initializeDevice,
  .resetDevice = resetDevice,

  .readPacket = readPacket,
  .writePacket = writePacket,

  .readCommand = readCommand,
  .writeWindow = writeWindow,

  .hasVisualDisplay = hasVisualDisplay,
  .writeVisual = writeVisual
};
