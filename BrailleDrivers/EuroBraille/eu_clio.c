/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#include "misc.h"
#include "message.h"
#include "eu_protocol.h"
#include "eu_keys.h"

/* Communication codes */
# define SOH	0x01
# define EOT	0x04
# define ACK	0x06
# define DLE	0x10
# define NAK	0x15

# define PRT_E_PAR 0x01		/* parity error */
# define PRT_E_NUM 0x02		/* frame numver error */
# define PRT_E_ING 0x03		/* length error */
# define PRT_E_COM 0x04		/* command error */
# define PRT_E_DON 0x05		/* data error */
# define PRT_E_SYN 0x06		/* syntax error */

/* Codes which need to be escaped */
static const unsigned char needsEscape[0x100] = {
  [SOH] = 1, [EOT] = 1, [DLE] = 1, [ACK] = 1, [NAK] = 1
};

# define	READ_BUFFER_LENGTH 1024

enum	clioModelType {
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
  IR2,
  IR4,
  IS2,
  IS3,
  JN2,
  NB2,
  NB4, 
  NB8, 
  SB2,
  SB4,
  SC2,
  SC4,
  TYPE_LAST
}clioModelType;

struct		s_clioModelType
{
  enum clioModelType	type;
  char		modelCode[3];
  char		modelDesc[30];
}	t_clioModelType;

/** Static Local Variables */

static int brlCols = 0; /* Number of braille cells */
static enum clioModelType brlModel = 0; /* brl display model currently used */
static t_eubrl_io*	iop = NULL; /* I/O methods */
static unsigned char	brlFirmwareVersion[21];
static int		routingMode = BRL_BLK_ROUTE;
static int refreshDisplay = 0;
static struct s_clioModelType		clioModels[] =
  {
    {UNKNOWN, "", ""},
    {CE2, "CE2", "Clio-EuroBraille 20"},
    {CE4, "CE4", "Clio-EuroBraille 40"},
    {CE8, "CE8", "Clio-EuroBraille 80"},
    {CN2, "CN2", "Clio-NoteBraille 20"},
    {CN4, "CN4", "Clio-NoteBraille 40"},
    {CN8, "CN8", "Clio-NoteBraille 80"},
    {CP2, "Cp2", "Clio-PupiBraille 20"},
    {CP4, "Cp4", "Clio-PupiBraille 40"},
    {CP8, "Cp8", "Clio-PupiBraille 80"},
    {CZ4, "CZ4", "Clio-AzerBraille 40"},
    {IR2, "IR2", "Iris 20"},
    {IR4, "IR4", "Iris 40"},
    {IS2, "IS2", "Iris S20"},
    {IS3, "IS3", "Iris S32"},
    {JN2, "JN2", ""},
    {NB2, "NB2", "NoteBraille 20"}, 
    {NB4, "NB4", "NoteBraille 40"}, 
    {NB8, "NB8", "NoteBraille 80"}, 
    {SB2, "SB2", "Scriba 20"},
    {SB4, "SB4", "Scriba 40"},
    {SC2, "SC2", "Scriba 20"},
    {SC4, "SC4", "Scriba 40"},
    {TYPE_LAST, "", ""},
  };

/* Local functions */
static int sendbyte(BrailleDisplay *brl, unsigned char c)
{
  return iop->write(brl, (char*)&c, 1);
}

static void		clio_sysIdentify(BrailleDisplay *brl, char* packet)
{
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
  while (clioModels[i].type != TYPE_LAST)
    {
      if (!strncasecmp(clioModels[i].modelCode, (char*)brlFirmwareVersion, 3))
	break;
      i++;
    }
  brlModel = clioModels[i].type;
  brl->resizeRequired = 1;
}

/*
** Converts an old protocol dots model to a new protocol compatible one.
** The new model is also compatible with brltty, so no conversion s needed 
** after that.
*/
static int		convert(char *keys)
{
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

static int clio_handleCommandKey(BrailleDisplay *brl, unsigned int key)
{
  static char flagLevel1 = 0, flagLevel2 = 0;
  unsigned int res = EOF;
  unsigned int	subkey;
  
  if (key == CL_STAR && !flagLevel1)
    {
      flagLevel2 = !flagLevel2;
      if (flagLevel2) message("Level 2 ...", MSG_NODELAY);
    }
  else if (key == CL_SHARP && !flagLevel2)
    {
      flagLevel1 = !flagLevel1;
      if (flagLevel1) message("Level 1 ...", MSG_NODELAY);
    }
  if (flagLevel1)
    {
      while ((subkey = clio_readKey(brl)) == 0) approximateDelay(20);
      flagLevel1 = 0;
      switch (subkey & 0x000000ff)
	{
	case CL_E : res = BRL_CMD_TOP_LEFT; break;
	case CL_H : res = BRL_CMD_HELP; break;
	case CL_J : res = BRL_CMD_LEARN; break;
	case CL_M : res = BRL_CMD_BOT_LEFT; break;
	case CL_FG: res = BRL_CMD_LNBEG; break;
	case CL_FD: res = BRL_CMD_LNEND; break;
	case CL_FH : res = BRL_CMD_TOP_LEFT; break;
	case CL_FB : res = BRL_CMD_BOT_LEFT; break;
	default : res = BRL_CMD_NOOP; break;
	}
    }
  else if (flagLevel2)
    {
      while ((subkey = clio_readKey(brl)) == 0) approximateDelay(20);
      flagLevel2 = 0;
      switch (subkey & 0x000000ff)
	{
	case CL_E : routingMode = BRL_BLK_CUTBEGIN; break;
	case CL_G : res = BRL_CMD_CSRVIS; break;
	case CL_K : res = BRL_CMD_SIXDOTS; break;
	case CL_L : res = BRL_CMD_PASTE; break;
	case CL_M : routingMode = BRL_BLK_CUTLINE; break;
	case CL_FB : res = BRL_CMD_CSRTRK; break;
	case CL_FH : res = BRL_CMD_TUNES; break;
	default : res = BRL_CMD_NOOP; break;
	}
    }
  else
    {
      switch (key)
	{
	case CL_NONE:	res = BRL_CMD_NOOP; break;
	case CL_E:	res = BRL_CMD_FWINLT; break;
	case CL_F:	res = BRL_CMD_LNUP; break;
	case CL_G:	res = BRL_CMD_PRPROMPT; break;
	case CL_H:	res = BRL_CMD_PREFMENU; break;
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

static void	clio_ModeHandling(BrailleDisplay *brl, char* packet)
{
  if (*packet == 'B')
    {
      refreshDisplay = 1;
      clio_writeWindow(brl);
    }
}

static int clio_KeyboardHandling(BrailleDisplay *brl, char *packet)
{
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

/*
** Functions that must be implemented, according to t_eubrl_protocol.
*/

int     clio_init(BrailleDisplay *brl, t_eubrl_io *io)
{
  int	leftTries = 2;
  iop = io;

  brlCols = 0;
  if (!io)
    {
      LogPrint(LOG_ERR, "eu: Clio : Invalid IO Subsystem driver.\n");
      return (-1);
    }
  memset(brlFirmwareVersion, 0, 21);

  while (leftTries-- && brlCols == 0)
    {
      clio_reset(brl);      
      approximateDelay(500);
      clio_readCommand(brl, BRL_CTX_SCREEN);
    }
  if (brlCols > 0)
    { /* Succesfully identified hardware. */
      brl->y = 1;
      brl->x = brlCols;
      LogPrint(LOG_INFO, "eu: %s connected.",
	       clioModels[brlModel].modelDesc);
      return (1);
    }
  return (0);
}

int     clio_reset(BrailleDisplay *brl)
{
  static const unsigned char packet[] = {0X02, 'S', 'I'};

  LogPrint(LOG_INFO, "eu Clio hardware reset requested");
  if (clio_writePacket(brl, packet, sizeof(packet)) == -1)
    {
      LogPrint(LOG_WARNING, "Clio: Failed to send ident request.\n");
      return -1;
    }
  return 1;
}


unsigned int	clio_readKey(BrailleDisplay *brl)
{
  static unsigned char	inPacket[READ_BUFFER_LENGTH];
  unsigned int res = 0;

  while (clio_readPacket(brl, inPacket, READ_BUFFER_LENGTH) > 0)
    {
      switch (inPacket[1]) {
      case 'S' : 
	clio_sysIdentify(brl, (char*)inPacket);
	break;
      case 'R' : 
	clio_ModeHandling(brl, (char *)inPacket + 2);
	break;
      case 'K' : 
	res = clio_KeyboardHandling(brl, (char *)inPacket + 2);
	break;
      default: 
	break;
      }
    }
  return res;
}

int	clio_readCommand(BrailleDisplay *brl, BRL_DriverCommandContext ctx)
{
  return clio_keyToCommand(brl, clio_readKey(brl), ctx);
}

int	clio_keyToCommand(BrailleDisplay *brl, unsigned int key, BRL_DriverCommandContext ctx)
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
      res = clio_handleCommandKey(brl, key & 0x000000ff);
    }
  return res;
}
 
void     clio_writeWindow(BrailleDisplay *brl)
{
  static char previousBrailleWindow[80];
  int displaySize = brl->x * brl->y;
  unsigned char buf[displaySize + 3];

  if ( displaySize > sizeof(previousBrailleWindow) ) {
    LogPrint(LOG_WARNING, "[eu] Discarding too large braille window" );
    return;
  }
  if (!memcmp(previousBrailleWindow, brl->buffer, displaySize) && !refreshDisplay)
    return;
  refreshDisplay = 0;
  memcpy(previousBrailleWindow, brl->buffer, displaySize);
  buf[0] = (unsigned char)(displaySize + 2);
  buf[1] = 'D';
  buf[2] = 'P';
  memcpy(buf + 3, brl->buffer, displaySize);
  clio_writePacket(brl, buf, sizeof(buf));
}

void     clio_writeVisual(BrailleDisplay *brl, const wchar_t *text)
{
  static wchar_t previousVisualDisplay[80];
  int displaySize = brl->x * brl->y;
  unsigned char buf[displaySize + 3];
  int i;

  if ( displaySize > sizeof(previousVisualDisplay) ) {
    LogPrint(LOG_WARNING, "[eu] Discarding too large visual display" );
    return;
  }

  if (wmemcmp(previousVisualDisplay, text, displaySize) == 0)
    return;
  wmemcpy(previousVisualDisplay, text, displaySize);
  buf[0] = (unsigned char)(displaySize + 2);
  buf[1] = 'D';
  buf[2] = 'L';
  for (i = 0; i < displaySize; i++)
    {
      wchar_t wc = text[i];
      buf[i+3] = iswLatin1(wc)? wc: '?';
    }
  clio_writePacket(brl, buf, sizeof(buf));
}

int	clio_hasLcdSupport(BrailleDisplay *brl)
{
  return (1);
}

ssize_t	clio_readPacket(BrailleDisplay *brl, void *packet, size_t size)
{
  static char 		buffer[READ_BUFFER_LENGTH];
  static int		pos = 0;
  static char		prevPktNbr = 0;
  int		parity = 0;
  int		ret, i, j, start, end, framelen = 0, otherchars;
  char		*tmpres = NULL;
  
  if (!iop || !packet || size < 3)
    return (-1);
  ret = iop->read(brl, buffer + pos, READ_BUFFER_LENGTH - pos);
  if (ret < 0)
    return (-1);
  for (i = 0, start = -1, end = -1, framelen = 0, otherchars = 0;
       i < pos + ret && (start == -1 || end == -1); 
       i++)
    {
      if (buffer[i] == SOH && start == -1) /* packet start detection */
	start = i;
      /* Packet end detection */
      if (start != -1 && end == -1 && buffer[i] == EOT &&
	  (buffer[i - 1] != DLE || 
	   (buffer[i - 1] == DLE && buffer[i - 2] == DLE)))
	end = i;
      /* Frame length count */
      if (start != -1 || end != -1)
	framelen++;
      if ((start == -1 && end == -1) || (start != -1 && end != -1))
	otherchars++;
    }
  if (end != -1)
    otherchars--;
  pos += ret;
  /* Skipping trailing chars if no packet has been read */
  if (start == -1 && end == -1)
    {
      pos -= otherchars;
      return 0;
    }
  /* If we found beginning of the packet but not the end */
  if (end == -1)
    return 0;

  /* ignoring packets received twice **/
  if ((needsEscape[((unsigned char)buffer[end - 1])] != 1
       && buffer[end - 2] == prevPktNbr)
      || (needsEscape[((unsigned char)buffer[end - 1])] == 1 
	  && buffer[end - 3] == prevPktNbr))
    {
      memmove(buffer, buffer + end + 1, pos - framelen);
      pos -= framelen + otherchars;
      return 0;
    }
  /* Updating pprevPktNbr */
  if (needsEscape[((unsigned char)buffer[end - 1])] != 1)
    prevPktNbr = buffer[end - 2];
  else
    prevPktNbr = buffer[end - 3];

  if ((tmpres = malloc(size + 1)) == NULL)
    {
      LogPrint(LOG_ERR, "clio: Failed to allocate memory.\n");
      return (-1);
    }
  if (start != -1 && end != -1
      && size >= framelen - 2)
    {
      /* parity calculation and preparing resultin buffer */
      for (parity = 0, i = start + 1, j = 0; 
	   i < end - 1 && j < size; 
	   i++)
	{
	  if (buffer[i] != DLE || buffer[i - 1] == DLE)
	    {
	      tmpres[j] = buffer[i];
	      j++;
	      parity ^= buffer[i];
	    }
	}
      /* Parity check */
      if (parity != buffer[end - 1])
	{
	  sendbyte(brl, NAK);
	  sendbyte(brl, PRT_E_PAR);
	  prevPktNbr = 0;
	  pos = 0;
	  free(tmpres);
	  return (0);
	}
      else
	{
	  memcpy(packet, tmpres, j - 1);
	  memmove(buffer, buffer + end + 1, pos - framelen);
	  pos -= framelen + otherchars;
	  sendbyte(brl, ACK);
	  free(tmpres);
	  return (1); /* success */
	}
    }
  return (0);
}

ssize_t	clio_writePacket(BrailleDisplay *brl, const void *packet, size_t size)
{
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
	if (needsEscape[*p]) *q++ = DLE;
        *q++ = *p;
	parity ^= *p++;
     }
   *q++ = pktNbr; /* Doesn't need to be prefixed since greater than 128 */
   parity ^= pktNbr;
   if (++pktNbr >= 256)
     pktNbr = 128;
   if (needsEscape[parity]) *q++ = DLE;
   *q++ = parity;
   *q++ = EOT;
   packetSize = q - buf;
   updateWriteDelay(brl, packetSize);
   return iop->write(brl, buf, packetSize);
}
