/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#include "log.h"
#include "misc.h"
#include "message.h"
#include "eu_protocol.h"
#include "eu_keys.h"

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
static unsigned char	brlFirmwareVersion[21];
static int		routingMode = BRL_BLK_ROUTE;



/*** Local functions */


static inline void
LogUnknownProtocolKey(const char *function, unsigned char key) {
  LogPrint(LOG_NOTICE,"[eu] %s: unknown protocol key %c (%x)",function,key,key);
}

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
	  if (flagLevel2) message(NULL, "Level 2 ...", MSG_NODELAY);
	}
      else if (key == VK_FGB && !flagLevel2)
	{
	  flagLevel1 = !flagLevel1;
	  if (flagLevel1) message(NULL, "Level 1 ...", MSG_NODELAY);
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
  if (brlType == ESYS_12 || brlType == ESYS_40)
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
    default:
      LogUnknownProtocolKey("esysiris_SysIdentity", packet[0]);
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
            key = (packet[1] << 24) + (packet[2] << 16) + (packet[3] << 8) + packet[4];
	  }
	else
	  {
	    key = ((unsigned char)packet[1] * 256 + (unsigned char)packet[2]) & 0x00000fff;
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
        LogPrint(LOG_DEBUG, "PC key %x %x %x %x", a, b, c, d);
        if (!a) {
          if (d)
            key = EUBRL_PC_KEY | BRL_BLK_PASSCHAR | d;
          else if (b == 0x08)
            key = EUBRL_PC_KEY | BRL_BLK_PASSKEY | BRL_KEY_BACKSPACE;
          else if (b >= 0x70 && b <= 0x7b) {
            int functionKey = b - 0x70;
            if (c & 0x04)
              key = EUBRL_PC_KEY | BRL_BLK_SWITCHVT | functionKey;
            else
              key = EUBRL_PC_KEY | BRL_BLK_PASSKEY | (BRL_KEY_FUNCTION + b-0x70);
            goto end;
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
end:
  return key;
}



/**
 ** Functions that must be implemented, according to t_eubrl_protocol.
 */

int	esysiris_init(BrailleDisplay *brl, t_eubrl_io *io)
{
  if (!io)
    {
      LogPrint(LOG_ERR, "eu: EsysIris: Invalid IO Subsystem driver.");
      return (-1);
    }
  memset(brlFirmwareVersion, 0, 21);
  char outPacket[2] = {'S', 'I'};
  int	leftTries = 24;
  iop = io;
      
  while (leftTries-- && brlCols == 0)
    {
      if (esysiris_writePacket(brl, (unsigned char *)outPacket, 2) == -1)
	{
	  LogPrint(LOG_WARNING, "eu: EsysIris: Failed to send ident request.");
	  leftTries = 0;
	  continue;
	}
      int i=60;
      while(i-- && brlCols==0)
        {
          esysiris_readCommand(brl, BRL_CTX_DEFAULT);
          approximateDelay(10);
        }
      approximateDelay(100);
    }
  if (brlCols > 0)
    { /* Succesfully identified hardware. */
      brl->textRows = 1;
      brl->textColumns = brlCols;
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

  if (esysiris_readPacket(brl, inPacket, 2048) == 1)
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
	  LogUnknownProtocolKey("esysiris_readKey", inPacket[3]);
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

  if (key == EOF) 
    return EOF;
  if (key == 0) 
    return EOF;

  if (key & EUBRL_BRAILLE_KEY)
    {
      res = protocol_handleBrailleKey(key, ctx);
    }
  if (key & EUBRL_ROUTING_KEY)
    {
      res = routingMode | ((key - 1) & 0x0000007f);
      routingMode = BRL_BLK_ROUTE;
    }
  if (key & EUBRL_COMMAND_KEY)
    {
      if (brlType == ESYS_12 || brlType == ESYS_40) res = esysiris_handleCommandKey(brl, key & 0x7fffffff);
       else res = esysiris_handleCommandKey(brl, key & 0x00000fff);
    }
  if (key & EUBRL_PC_KEY)
    {
      res = key & 0xFFFFFF;
    }
  return res;
}

void	esysiris_writeWindow(BrailleDisplay *brl)
{
  static unsigned char previousBrailleWindow[80];
  int displaySize = brl->textColumns * brl->textRows;
  unsigned char buf[displaySize + 2];
  
  if (displaySize > sizeof(previousBrailleWindow)) {
    LogPrint(LOG_WARNING, "[eu] Discarding too large braille window");
    return;
  }

  if (!memcmp(previousBrailleWindow, brl->buffer, displaySize))
    return;
  memcpy(previousBrailleWindow, brl->buffer, displaySize);
  buf[0] = 'B';
  buf[1] = 'S';
  memcpy(buf + 2, brl->buffer, displaySize);
  esysiris_writePacket(brl, buf, sizeof(buf));
}

int	esysiris_hasLcdSupport(BrailleDisplay *brl)
{
  return (0);
}

void	esysiris_writeVisual(BrailleDisplay *brl, const wchar_t *text)
{
  return;
}

ssize_t esysiris_readPacket(BrailleDisplay *brl, void *packet, size_t size)
{
  static char buffer[READ_BUFFER_LENGTH];
  static int pos = 0;

  int	i; 
  int end=-1; int start=-1; 
  int framelen = 0;
  int ret = 0;
  int bytesRead = 0;

  if (!iop || !packet || size < 4)
    return (-1);

  while( ((READ_BUFFER_LENGTH - pos)>0) ? ((ret = iop->read(brl, buffer + pos, READ_BUFFER_LENGTH - pos))>0) : 0)
//  if ((READ_BUFFER_LENGTH - pos)>0) ret = iop->read(brl, buffer + pos, READ_BUFFER_LENGTH - pos);
  {
    bytesRead+=ret;
    pos+=ret;
  }


  if (ret < 0 && pos==0)
    return (-1);

  if(pos==0) {
    return (0);
  }

  for (i = 0; i < pos && start==-1; i++)
    {
      if (buffer[i] == STX) start = i;
    }
  if (start!=-1 && start+2<pos)
    {
      framelen = buffer[start + 1] * 256 + buffer[start + 2];
      if (start+framelen+1<pos) 
        if (buffer[start+framelen+1] == ETX) end = start+framelen+1;
    }

  if (start != -1 && end != -1 && size > framelen + 2)
    {
      memcpy(packet, buffer + start, framelen + 2);
      memmove(buffer, buffer + end + 1, pos - framelen - 2);
      pos -= (framelen + 2);
      return (1);
    }
      else if(pos==bytesRead)
    {
      pos=0;
      return (0);
    }
      else if(pos>bytesRead)
    {
      memmove(buffer,buffer+(pos-bytesRead),bytesRead);
      pos = bytesRead;
      return (0);
    }
  return (0);
}


ssize_t esysiris_writePacket(BrailleDisplay *brl, const void *packet, size_t size)
{
  int packetSize = size + 2;
  unsigned char buf[packetSize + 2];
  if (!iop || !packet || !size)
    return (-1);;
  buf[0] = STX;
  buf[1] = (packetSize >> 8) & 0x00FF;
  buf[2] = packetSize & 0x00FF;
  memcpy(buf + 3, packet, size);
  buf[sizeof(buf)-1] = ETX;
  updateWriteDelay(brl, sizeof(buf) );
  return iop->write(brl, buf, sizeof(buf));
}
