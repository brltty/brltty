/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/** EuroBraille/esysiris_protocol.c 
 ** Implements the ESYS and IRIS rev >=1.71 protocol 
 ** Made by Yannick PLASSIARD <yan@mistigri.org>
 */
#include	"eu_protocol.h"
#include	"misc.h"
#include	"message.h"
#include	"eu_keys.h"


# define	STX	0x02
# define	ETX	0x03

# define	READ_BUFFER_LENGTH 2048

enum hardwareType {
  UNKNOWN = 0x00,
  IRIS_20,
  IRIS_40,
  IRIS_S20,
  IRIS_S32,
  IRIS_KB20,
  IRIS_KB40,
  ESYS_12,
  ESYS_40,
  TYPE_LAST
};

static
const char	modelTable[TYPE_LAST][20] = {
  "Unknown hardware",
  "IRIS 20",
  "IRIS 40",
  "IRIS S-20",
  "IRIS S-32",
  "IRIS KB-20",
  "IRIS KB-40",
  "ESYS 12",
  "ESYS 40",
};


/** Static Local Variables */

static int brlCols = 0;
static enum hardwareType brlType = UNKNOWN;
static t_eubrl_io*	iop = NULL;
static char* prevData = NULL, *prevLcdData = NULL;
static unsigned char	brlFirmwareVersion[21];
static unsigned int	chars_per_sec = 0;
static int		routingMode = BRL_BLK_ROUTE;



/*** Local functions */



static int esysiris_handleCommandKey(BrailleDisplay *brl, unsigned int key)
{
  int res = EOF;
  unsigned int subkey = 0;
  static char flagLevel1 = 0, flagLevel2 = 0;

  if (brlType == IRIS_20 || brlType == IRIS_S20 || brlType == IRIS_KB20
      || brlType == IRIS_S32
      || brlType == IRIS_40 || brlType == IRIS_KB40)
    { /** Iris models keys */
      if (key == VK_FDB && !flagLevel1)
	{
	  flagLevel2 = !flagLevel2;
	  if (flagLevel2) message("Level 2 ...", MSG_NODELAY);
	}
      else if (key == VK_FGB && !flagLevel2)
	{
	  flagLevel1 = !flagLevel1;
	  if (flagLevel1) message("Level 1 ...", MSG_NODELAY);
	}
      if (flagLevel1)
	{
	  while ((subkey = esysiris_readKey(brl)) == 0) approximateDelay(20);
	  flagLevel1 = 0;
	  switch (subkey & 0x00000fff)
	    {
	    case VK_L1 : res = BRL_CMD_TOP_LEFT; break;
	    case VK_L4: res = BRL_CMD_HELP; break;
	    case VK_L5: res = BRL_CMD_LEARN; break;
	    case VK_L8 : res = BRL_CMD_BOT_LEFT; break;
	    case VK_FG: res = BRL_CMD_LNBEG; break;
	    case VK_FD: res = BRL_CMD_LNEND; break;
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
	    case VK_L3 : res = BRL_CMD_CSRVIS; break;
	    case VK_L6: res = BRL_CMD_SIXDOTS; break;
	    case VK_L7: res = BRL_CMD_PASTE; break;
	    case VK_L8: routingMode = BRL_BLK_CUTLINE; break;
	    case VK_FB: res = BRL_CMD_CSRTRK; break;
	    case VK_FH : res = BRL_CMD_TUNES; break;
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
	    case VK_L78:	res = BRL_CMD_BOT_LEFT; break;
	    case VK_L1234:	res = BRL_CMD_RESTARTBRL; break;
	    case VK_L5678:	res = BRL_CMD_RESTARTSPEECH; break;
	    }
	}
    }
  return res;
}

static int esysiris_SysIdentity(BrailleDisplay *brl, char *packet)
{
  switch(packet[0])
    {
    case 'G': 
      brlCols = packet[1];
      break;
    case 'T':
      brlType = packet[1];
      break;
    }
  return 0;
}

static int esysiris_KeyboardHandling(BrailleDisplay *brl, char *packet)
{
  unsigned int key = EOF;
  switch(packet[0])
    {
    case 'B':
      key = (packet[1] * 256 + (unsigned char)packet[2]) & 0x000003ff;
      key |= EUBRL_BRAILLE_KEY;
      break;
    case 'I':
      key = packet[2] & 0xbf;
      key |= EUBRL_ROUTING_KEY;
      break;
    case 'C':
      {
	if (brlType == ESYS_12 || brlType == ESYS_40)
	  {
	    unsigned char *buf = (unsigned char *)packet;
	    unsigned int scrolls = buf[1] * 1024 + buf[2] * 512 + buf[3] * 256 + buf[4];
	    LogPrint(LOG_DEBUG, "eu: EsysIris: Scrolls: %x", scrolls);
	    
	  }
	else
	  {
	    key = ((unsigned char)packet[1] * 256 + (unsigned char)packet[2]) & 0x00000fff;
	  }
      }
      key |= EUBRL_COMMAND_KEY;
      break;
    }
  return key;
}



/**
 ** Functions that must be implemented, according to t_eubrl_protocol.
 */

int	esysiris_init(BrailleDisplay *brl, t_eubrl_io *io)
{
  chars_per_sec = BAUD_RATE / 9;
  if (!io)
    {
      LogPrint(LOG_ERR, "eu: EsysIris: Invalid IO Subsystem driver.");
      return (-1);
    }
  memset(brlFirmwareVersion, 0, 21);
  char outPacket[2] = {'S', 'I'};
  int	leftTries = 2;
  iop = io;
      
  while (leftTries-- && brlCols == 0)
    {
      if (esysiris_writePacket(brl, (unsigned char *)outPacket, 2) == -1)
	{
	  LogPrint(LOG_WARNING, "eu: EsysIris: Failed to send ident request.");
	  leftTries = 0;
	  continue;
	}
      approximateDelay(500);
      esysiris_readCommand(brl, BRL_CTX_SCREEN);
    }
  if (brlCols > 0)
    { /* Succesfully identified hardware. */
      brl->y = 1;
      brl->x = brlCols;
      LogPrint(LOG_INFO, "eu: %s connected.",
	       modelTable[brlType]);
      return (1);
    }
  return (0);
}

int	esysiris_reset(BrailleDisplay *brl)
{
  (void)brl;
  return 1;
}


unsigned int	esysiris_readKey(BrailleDisplay *brl)
{
  static unsigned char	inPacket[2048];
  unsigned int res = 0;

  while (esysiris_readPacket(brl, inPacket, 2048) == 1)
    { /* We got a packet */
      switch (inPacket[3])
	{
	case 'S':
	  esysiris_SysIdentity(brl, (char *)inPacket + 4);
	  break;
	case 'K':
	  res = esysiris_KeyboardHandling(brl, (char *)inPacket + 4);
	  break;
	default:
	  break;
	}
    }
  return res;
}

int	esysiris_readCommand(BrailleDisplay *brl, BRL_DriverCommandContext ctx)
{
  return esysiris_keyToCommand(brl, esysiris_readKey(brl), ctx);
}

int	esysiris_keyToCommand(BrailleDisplay *brl, unsigned int key, BRL_DriverCommandContext ctx)
{
  unsigned int res = EOF;

  if (key & EUBRL_BRAILLE_KEY)
    {
      res = protocol_handleBrailleKey(key);
    }
  if (key & EUBRL_ROUTING_KEY)
    {
      res = routingMode | ((key - 1) & 0x0000007f);
      routingMode = BRL_BLK_ROUTE;
    }
  if (key & EUBRL_COMMAND_KEY)
    {
      res = esysiris_handleCommandKey(brl, key & 0x00000fff);
    }
  return res;
}

int	esysiris_writeBraille(BrailleDisplay *brl, unsigned char *data, int len)
{
  unsigned char buf[len + 2];

  if (prevData == NULL)
    {
      prevData = malloc(brlCols);
      if (prevData == NULL)
	return -1;
      memset(prevData, 0, brlCols);
    }
  if (!memcmp(prevData, data, brlCols))
    return (1);
  memcpy(prevData, data, len);
  buf[0] = 'B';
  buf[1] = 'S';
  memcpy(buf + 2, data, len);
  return (esysiris_writePacket(brl, buf, len + 2));
}

int	esysiris_hasLcdSupport(BrailleDisplay *brl)
{
  return (0);
}

int	esysiris_writeLcd(BrailleDisplay *brl, unsigned char *str, int len)
{
  return (-1);
}


int	esysiris_readPacket(BrailleDisplay *brl, unsigned char *packet, int size)
{
  static char* buffer = NULL;
  static int pos = 0;
  int	ret, i, start, end, framelen = 0;

  if (!iop || !packet || size < 4)
    return (-1);
  if (buffer == NULL)
    {
      if (!(buffer = malloc(READ_BUFFER_LENGTH)))
	{
	  LogPrint(LOG_ERR, "esysiris: Failed to allocate memory.\n");
	  return (-1);
	}
      pos = 0;
    }
  ret = iop->read(brl, buffer + pos, READ_BUFFER_LENGTH - pos);
  if (ret < 0)
    return (-1);
  for (i = 0, start = -1, end = -1; i < pos + ret && (start == -1 || end == -1); i++)
    {
      if (buffer[i] == STX && start == -1)
	{
	  start = i;
	  framelen = 0;
	}
      if (start > -1 && start + 2 < i)
	{ /* Catch our packet length */
	  framelen = buffer[start + 1] * 256 + buffer[start + 2];
	}
      if (start != -1 && buffer[i] == ETX)
	if (i == (start + framelen + 1))
	  {
	    end = i;
	  }
    }
  pos += ret;
  if (start != -1 && end != -1
      && size > framelen + 2)
    {
      memcpy(packet, buffer + start, framelen + 2);
      memmove(buffer, buffer + end + 1, pos - framelen - 2);
      pos -= (framelen + 2);
      return (1);
    }
  return (0);
}

int	esysiris_writePacket(BrailleDisplay *brl, 
			     const unsigned char *packet, int size)
{
  unsigned char buf[size + 4];
  if (!iop || !packet || !size)
    return (-1);;
  buf[0] = STX;
  buf[1] = ((size + 2) & 0xFF00);
  buf[2] = ((size + 2) & 0x00FF);
  memcpy(buf + 3, packet, size);
  buf[size + 3] = ETX;
  if (iop->ioType == IO_SERIAL)
    brl->writeDelay += size * 1000 / chars_per_sec + 1;
  return (iop->write(brl, (char *)buf, size + 4));
}
