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
#include "message.h"
#include "ascii.h"
#include "brldefs-eu.h"
#include "eu_protocol.h"
#include "eu_keys.h"

#define NAVIGATION_KEY_ENTRY(k,n) KEY_ENTRY(NavigationKeys, NAV, k, n)

BEGIN_KEY_NAME_TABLE(navigation)
  NAVIGATION_KEY_ENTRY(Sharp, "Sharp"),
  NAVIGATION_KEY_ENTRY(Star, "Star"),

  NAVIGATION_KEY_ENTRY(0, "0"),
  NAVIGATION_KEY_ENTRY(1, "1"),
  NAVIGATION_KEY_ENTRY(2, "Up"),
  NAVIGATION_KEY_ENTRY(3, "3"),
  NAVIGATION_KEY_ENTRY(4, "Left"),
  NAVIGATION_KEY_ENTRY(5, "5"),
  NAVIGATION_KEY_ENTRY(6, "Right"),
  NAVIGATION_KEY_ENTRY(7, "7"),
  NAVIGATION_KEY_ENTRY(8, "Down"),
  NAVIGATION_KEY_ENTRY(9, "9"),

  NAVIGATION_KEY_ENTRY(A, "A"),
  NAVIGATION_KEY_ENTRY(B, "B"),
  NAVIGATION_KEY_ENTRY(C, "C"),
  NAVIGATION_KEY_ENTRY(D, "D"),
  NAVIGATION_KEY_ENTRY(E, "E"),
  NAVIGATION_KEY_ENTRY(F, "F"),
  NAVIGATION_KEY_ENTRY(G, "G"),
  NAVIGATION_KEY_ENTRY(H, "H"),
  NAVIGATION_KEY_ENTRY(I, "I"),
  NAVIGATION_KEY_ENTRY(J, "J"),
  NAVIGATION_KEY_ENTRY(K, "K"),
  NAVIGATION_KEY_ENTRY(L, "L"),
  NAVIGATION_KEY_ENTRY(M, "M"),

  KEY_SET_ENTRY(EU_SET_RoutingKeys1, "RoutingKey"),
  KEY_SET_ENTRY(EU_SET_SeparatorKeys, "SeparatorKey"),
  KEY_SET_ENTRY(EU_SET_StatusKeys, "StatusKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(clio)
  KEY_NAME_TABLE(navigation),
END_KEY_NAME_TABLES

PUBLIC_KEY_TABLE(clio)

/* Communication codes */
# define PRT_E_PAR 0x01		/* parity error */
# define PRT_E_NUM 0x02		/* frame numver error */
# define PRT_E_ING 0x03		/* length error */
# define PRT_E_COM 0x04		/* command error */
# define PRT_E_DON 0x05		/* data error */
# define PRT_E_SYN 0x06		/* syntax error */

#define	READ_BUFFER_LENGTH 1024

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
static unsigned char	brlFirmwareVersion[21];
static int		routingMode = BRL_BLK_ROUTE;
static int refreshDisplay = 0;
static int previousPacketNumber;

static int
needsEscape (unsigned char code) {
  switch (code) {
    case SOH:
    case EOT:
    case DLE:
    case ACK:
    case NAK:
      return 1;
  }

  return 0;
}

/* Local functions */
static int
sendByte (BrailleDisplay *brl, unsigned char c) {
  return io->writeData(brl, (char*)&c, 1);
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

            if (parity)
              {
                sendByte(brl, NAK);
                sendByte(brl, PRT_E_PAR);

                offset = 0;
                continue;
              }
          }

          offset -= 1; /* remove parity */
          sendByte(brl, ACK);

          if (buffer[--offset] == previousPacketNumber)
            {
              offset = 0;
              continue;
            }
          previousPacketNumber = buffer[offset];

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
  static int pktNbr = 127; /* packet number = 127 at first time */
  size_t packetSize;

  *q++ = SOH;
  while (size--)
    {
	if (needsEscape(*p)) *q++ = DLE;
        *q++ = *p;
	parity ^= *p++;
     }
   *q++ = pktNbr; /* Doesn't need to be prefixed since greater than 128 */
   parity ^= pktNbr;
   if (++pktNbr >= 256)
     pktNbr = 128;
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
handleSystemInformation (BrailleDisplay *brl, char* packet) {
  char *p;
  unsigned char ln = 0;
  int i;

  p = packet;
  while (1)
    {
      ln = *(p++);
      if (ln == 22 && 
	  (!strncmp(p, "SI", 2) || !strncmp(p, "si", 2 )))
	{
	  memcpy(brlFirmwareVersion, p + 2, 20);
	  break;
	}
      else
	p += ln;
    }
  switch (brlFirmwareVersion[2]) {
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
      if (!strncasecmp(modelTable[i].modelCode, (char*)brlFirmwareVersion, 3))
	break;
      i++;
    }
  brlModel = modelTable[i].modelIdentifier;
  brl->resizeRequired = 1;
}

/*
** Converts an old protocol dots model to a new protocol compatible one.
** The new model is also compatible with brltty, so no conversion s needed 
** after that.
*/
static int
convert (char *keys) {
  unsigned int res = 0;

  res = (keys[1] & 1 ? BRL_DOT7 : 0);
  res += (keys[1] & 2 ? BRL_DOT8 : 0);
  res += (keys[0] & 0x01 ? BRL_DOT1 : 0);
  res += (keys[0] & 0x02 ? BRL_DOT2 : 0);
  res += (keys[0] & 0x04 ? BRL_DOT3 : 0);
  res += (keys[0] & 0x08 ? BRL_DOT4 : 0);
  res += (keys[0] & 0x10 ? BRL_DOT5 : 0);
  res += (keys[0] & 0x20 ? BRL_DOT6 : 0);
  res += (keys[0] & 0x40 ? 0x0100 : 0);
  res += (keys[0] & 0x80 ? 0x0200 : 0);  
  return res;
}

static int
writeWindow (BrailleDisplay *brl) {
  static unsigned char previousBrailleWindow[80];
  int displaySize = brl->textColumns * brl->textRows;
  unsigned char buf[displaySize + 3];

  if ( displaySize > sizeof(previousBrailleWindow) ) {
    logMessage(LOG_WARNING, "[eu] Discarding too large braille window" );
    return 0;
  }
  if (cellsHaveChanged(previousBrailleWindow, brl->buffer, displaySize, NULL, NULL, &refreshDisplay)) {
    buf[0] = (unsigned char)(displaySize + 2);
    buf[1] = 'D';
    buf[2] = 'P';
    memcpy(buf + 3, brl->buffer, displaySize);
    writePacket(brl, buf, sizeof(buf));
  }
  return 1;
}

static int
writeVisual (BrailleDisplay *brl, const wchar_t *text) {
  static wchar_t previousVisualDisplay[80];
  int displaySize = brl->textColumns * brl->textRows;
  unsigned char buf[displaySize + 3];
  int i;

  if ( displaySize > sizeof(previousVisualDisplay) ) {
    logMessage(LOG_WARNING, "[eu] Discarding too large visual display" );
    return 0;
  }

  if (wmemcmp(previousVisualDisplay, text, displaySize) == 0)
    return 1;
  wmemcpy(previousVisualDisplay, text, displaySize);
  buf[0] = (unsigned char)(displaySize + 2);
  buf[1] = 'D';
  buf[2] = 'L';
  for (i = 0; i < displaySize; i++)
    {
      wchar_t wc = text[i];
      buf[i+3] = iswLatin1(wc)? wc: '?';
    }
  writePacket(brl, buf, sizeof(buf));
  return 1;
}

static int
hasVisualDisplay (BrailleDisplay *brl) {
  return (1);
}

static void
handleMode (BrailleDisplay *brl, char* packet) {
  if (*packet == 'B')
    {
      refreshDisplay = 1;
      writeWindow(brl);
    }
}

static int
handleKeyboard (BrailleDisplay *brl, char *packet) {
  unsigned int key = 0;
  switch (packet[0])
    {
    case 'B' :
      key = convert(packet + 1);
      key |= EUBRL_BRAILLE_KEY;
      break;
    case 'I' :
      key = packet[1];
      key |= EUBRL_ROUTING_KEY;
      break;
    case 'T' : 
      key = packet[1];
      key |= EUBRL_COMMAND_KEY;
      break;
    default :
      break;
    }
  return key;
}

static int
readKey (BrailleDisplay *brl) {
  static unsigned char	inPacket[READ_BUFFER_LENGTH];
  int res = 0;

  while (readPacket(brl, inPacket, READ_BUFFER_LENGTH) > 0)
    {
      switch (inPacket[1]) {
      case 'S' : 
	handleSystemInformation(brl, (char*)inPacket);
	break;
      case 'R' : 
	handleMode(brl, (char *)inPacket + 2);
	break;
      case 'K' : 
	res = handleKeyboard(brl, (char *)inPacket + 2);
	break;
      default: 
	break;
      }
    }
  return res;
}

static int
handleCommandKey (BrailleDisplay *brl, unsigned int key) {
  static char flagLevel1 = 0, flagLevel2 = 0;
  unsigned int res = EOF;
  unsigned int	subkey;
  
  if (key == CL_STAR && !flagLevel1)
    {
      flagLevel2 = !flagLevel2;
      if (flagLevel2)
	{
	  if (brlModel >= IR2)
	    message(NULL, gettext("layer 2 ..."), MSG_NODELAY);
	  else
	    message(NULL, gettext("programming on ..."), MSG_NODELAY);
	}
    }
  else if (key == CL_SHARP && !flagLevel2)
    {
      flagLevel1 = !flagLevel1;
      if (flagLevel1)
	{
	  if (brlModel >= IR2) 
	    message(NULL, gettext("layer 1 ..."), MSG_NODELAY);
	  else
	    message(NULL, gettext("view on ..."), MSG_NODELAY);
	}
    }
  if (flagLevel1)
    {
      while ((subkey = readKey(brl)) == 0) approximateDelay(20);
      flagLevel1 = 0;
      switch (subkey & 0x000000ff)
	{
	case CL_1 : res = BRL_CMD_LEARN; break;
	case CL_3 : res = BRL_CMD_TOP_LEFT; break;
	case CL_9 : res = BRL_CMD_BOT_LEFT; break;
	case CL_A : res = BRL_CMD_DISPMD; break;
	case CL_E : res = BRL_CMD_TOP_LEFT; break;
	case CL_G : res = BRL_CMD_PRSEARCH; break;
	case CL_H : res = BRL_CMD_HELP; break;
	case CL_K : res = BRL_CMD_NXSEARCH; break;
	case CL_L : res = BRL_CMD_LEARN; break;
	case CL_M : res = BRL_CMD_BOT_LEFT; break;
	case CL_FG: res = BRL_CMD_LNBEG; break;
	case CL_FD: res = BRL_CMD_LNEND; break;
	case CL_FH : res = BRL_CMD_HOME; break;
	case CL_FB : res = BRL_CMD_RETURN; break;
	default : res = BRL_CMD_NOOP; break;
	}
    }
  else if (flagLevel2)
    {
      while ((subkey = readKey(brl)) == 0) approximateDelay(20);
      flagLevel2 = 0;
      switch (subkey & 0x000000ff)
	{
	case CL_E : routingMode = BRL_BLK_CUTBEGIN; break;
	case CL_F : routingMode = BRL_BLK_CUTAPPEND; break;
	case CL_G : res = BRL_CMD_CSRVIS; break;
	case CL_K : routingMode = BRL_BLK_CUTRECT; break;
	case CL_L : res = BRL_CMD_PASTE; break;
	case CL_M : routingMode = BRL_BLK_CUTLINE; break;
	case CL_FH : res = BRL_CMD_PREFMENU; break;
	case CL_FB : res = BRL_CMD_CSRTRK; break;
	case CL_FD : res = BRL_CMD_TUNES; break;
	default : res = BRL_CMD_NOOP; break;
	}
    }
  else
    {
      switch (key)
	{
	case CL_NONE:	res = BRL_CMD_NOOP; break;
	case CL_0:	res = BRL_CMD_CSRTRK; break;
	case CL_1 :	res = BRL_CMD_TOP_LEFT; break;
	case CL_3 :	res = BRL_CMD_PRDIFLN; break;
	case CL_5:	res = BRL_CMD_HOME; break;
	case CL_9 :	res = BRL_CMD_NXDIFLN; break;
	case CL_7 :	res = BRL_CMD_BOT_LEFT; break;
	case CL_A:	res = BRL_CMD_FREEZE; break;
	case CL_E:	res = BRL_CMD_FWINLT; break;
	case CL_F:	res = BRL_CMD_LNUP; break;
	case CL_G:	res = BRL_CMD_PRPROMPT; break;
	case CL_H:	res = BRL_CMD_PREFMENU; break;
	case CL_I:	res = BRL_CMD_INFO; break;
	case CL_J:	res = BRL_CMD_INFO; break;
	case CL_K:	res = BRL_CMD_NXPROMPT; break;
	case CL_L:	res = BRL_CMD_LNDN; break;
	case CL_M:	res = BRL_CMD_FWINRT; break;
	case CL_FB:	res = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_DOWN; break;
	case CL_FH:	res = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_UP; break;
	case CL_FG:	res = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_LEFT; break;
	case CL_FD:	res = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_RIGHT; break;
	}
    }
  return res;
}

static int
keyToCommand (BrailleDisplay *brl, int key, KeyTableCommandContext ctx) {
  int res = EOF;

  if (key & EUBRL_BRAILLE_KEY)
    {
      res = eubrl_handleBrailleKey(key, ctx);
    }
  if (key & EUBRL_ROUTING_KEY)
    {
      res = routingMode | ((key - 1) & 0x0000007f);
      routingMode = BRL_BLK_ROUTE;
    }
  if (key & EUBRL_COMMAND_KEY)
    {
      res = handleCommandKey(brl, key & 0x000000ff);
    }
  return res;
}
 
static int
readCommand (BrailleDisplay *brl, KeyTableCommandContext ctx) {
  return keyToCommand(brl, readKey(brl), ctx);
}

static int
initializeDevice (BrailleDisplay *brl) {
  int	leftTries = 2;

  brlCols = 0;
  memset(brlFirmwareVersion, 0, 21);

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

      previousPacketNumber = -1;

      logMessage(LOG_INFO, "eu: %s connected.",
	         modelTable[brlModel].modelName);
      return (1);
    }
  return (0);
}

const t_eubrl_protocol clioProtocol = {
  .protocolName = "clio",

  .initializeDevice = initializeDevice,
  .resetDevice = resetDevice,

  .readPacket = readPacket,
  .writePacket = writePacket,

  .readKey = readKey,
  .readCommand = readCommand,
  .keyToCommand = keyToCommand,

  .writeWindow = writeWindow,
  .hasVisualDisplay = hasVisualDisplay,
  .writeVisual = writeVisual
};
