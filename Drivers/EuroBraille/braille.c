/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

/* EuroBraille/braille.c - Braille display library for the EuroBraille family.
 * Copyright (C) 1997-2003 by Yannick Plassiard <plassi_y@epitech.net>
 *                        and Nicolas Pitre <nico@cam.org>
 * See the GNU General Public License for details in the ../COPYING file
 * See the README file for details about copyrights and version informations
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */


/*
** the next define controls whichever you want the LCD support or not.
** Please do not use this define if you use an IRIS, this terminal doesn't have
** any LCD screen.
*/

#define		BRL_HAVE_VISUAL_DISPLAY
/*
** headers
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/termios.h>
#include <string.h>

#include "Programs/brl.h"
#include "Programs/message.h"
#include "Programs/misc.h"

#define BRLNAME	"EuroBraille"
#define PREFSTYLE ST_None

#include "Programs/brl_driver.h"
#include "brlconf.h"


t_key		num_keys[27] = {
  {'1', 0, 1},
  {'2', 0, 2},
  {'3', 0, 3},
  {'4', 0, 4},
  {'5', 0, 5},
  {'6', 0, 6},
  {'7', 0, 7},
  {'8', 0, 8},
  {'9', 0, 9},
  {'*', 0, 10},
  {'0', 0, 30},
  {'#', 0, 11},
  {'A', 0, 12},
  {'B', 0, 13},
  {'C', 0, 14},
  {'D', 0, 15},
  {'E', 0, 16},
  {'F', 0, 17},
  {'G', 0, 18},
  {'H', 0, 19},
  {'I', 0, 20},
  {'J', 0, 21},
  {'K', 0, 22},
  {'L', 0, 23},
  {'M', 0, 24},
  {'Z', 0, 25},
  {0, 0, 0}
};

/*
** next tables define assigmenss bitween keys and HLLTTY commands depending on
** the terminal type (Scriba, Clio)NoteBraille, or AzerBraille).
**
** If you make changes to those tables I'd like to be informed in order to
** update the driver for next releases.
*/

t_key		scriba_keys[17] = {		
  {2, 0, VAL_PASSKEY | VPK_CURSOR_UP},
  {4, 0, VAL_PASSKEY | VPK_CURSOR_LEFT},
  {6, 0, VAL_PASSKEY | VPK_CURSOR_RIGHT},
  {8, 0, VAL_PASSKEY | VPK_CURSOR_DOWN},
  {10, Program, 0},
  {11, ViewOn, 0},
  {13, 0, 0},
  {16, 0, CMD_FWINLT},
  {17, 0, CMD_LNUP},
  {18, 0, CMD_PRPROMPT},
  {19, 0, CMD_PRSEARCH},
  {20, 0, CMD_INFO},
  {21, 0, CMD_NXSEARCH},
  {22, 0, CMD_NXPROMPT},
  {23, 0, CMD_LNDN},
  {24, 0, CMD_FWINRT},
  {0, 0, 0}
};

t_key		pscriba_keys[7] = {
  {2, 0, CMD_PREFMENU},
  {6, 0, CMD_TUNES},
  {8, 0, CMD_CSRTRK},
  {16, begblk, 0},
  {23, 0, CMD_PASTE},
  {24, endblk, 0},
  {0, 0, 0}
};

t_key		azer40_keys[8] = {
  {16, 0, CMD_FWINLT},
  {17, 0, CMD_LNUP},
  {18, 0, CMD_PRPROMPT},
  {20, 0, CMD_INFO},
  {22, 0, CMD_NXPROMPT},
  {23, 0, CMD_LNDN},
  {24, 0, CMD_FWINRT},
  {0, 0, 0}
};

t_key		*pazer40_keys = pscriba_keys;

t_key		azer80_keys[23] = {
  {1, 0, CMD_TOP_LEFT},
  {2, 0, VAL_PASSKEY | VPK_CURSOR_UP},
  {3, 0, CMD_PRDIFLN},
  {4, 0, VAL_PASSKEY | VPK_CURSOR_LEFT},
  {5, 0, CMD_HOME},
  {6, 0, VAL_PASSKEY | VPK_CURSOR_RIGHT},
  {7, 0, CMD_BOT_LEFT},
  {8, 0, VAL_PASSKEY | VPK_CURSOR_DOWN},
  {9, 0, CMD_NXDIFLN},
  {10, Program, 0},
  {30, 0, CMD_CSRTRK},
  {11, ViewOn, 0},
  {12, 0, CMD_FREEZE},
  {16, 0, CMD_FWINLT},
  {17, 0, CMD_LNUP},
  {18, 0, CMD_PRPROMPT},
  {19, 0, CMD_PRSEARCH},
  {20, 0, CMD_INFO},
  {21, 0, CMD_NXSEARCH},
  {22, 0, CMD_NXPROMPT},
  {23, 0, CMD_LNDN},
  {24, 0, CMD_FWINRT},
  {0, 0, 0}
};
t_key		*pazer80_keys = pscriba_keys;

t_key		*clio_keys = azer80_keys;
t_key		*pclio_keys = pscriba_keys;

t_key		*nb_keys = azer80_keys;
t_key		*pnb_keys = pscriba_keys;

t_key		iris_keys[14] = {
  {2, 0, VAL_PASSKEY | VPK_CURSOR_UP},
  {4, 0, VAL_PASSKEY | VPK_CURSOR_LEFT},
  {6, 0, VAL_PASSKEY | VPK_CURSOR_RIGHT},
  {8, 0, VAL_PASSKEY | VPK_CURSOR_DOWN},
  {16, 0, CMD_FWINLT},
  {17, 0, CMD_LNUP},
  {18, 0, CMD_CSRVIS},
  {19, 0, CMD_PREFMENU},
  {20, 0, CMD_NOOP},
  {21, 0, CMD_NOOP},
  {22, 0, CMD_TUNES},
  {23, 0, CMD_LNDN},
  {24, 0, CMD_FWINRT},
  {0, 0, 0}
};

/*
** Next bindings are keys that must be explicitly listed to allow function
** keys, escape, page-down, enter, backspace ...
** The first part is a 10-bits code representing the key. Format is :
**
** Braille -> decimal value
** 1		1
** 2		2
** 3		4
** 4		8
** 5		16
** 6		32
** 7		64
** 8		128
** bkspace	256
** space	512
*/

t_alias		brl_key[] = {
  {0x100,	VAL_PASSKEY + VPK_BACKSPACE},
  {0x300,	VAL_PASSKEY + VPK_RETURN},
  {0x232,	VAL_PASSKEY + VPK_TAB},
  {0x208,	VAL_PASSKEY + VPK_CURSOR_UP},
  {0x220,	VAL_PASSKEY + VPK_CURSOR_DOWN},
  {0x210,	VAL_PASSKEY + VPK_CURSOR_RIGHT},
  {0x202,	VAL_PASSKEY + VPK_CURSOR_LEFT},
  {0x205,	VAL_PASSKEY + VPK_PAGE_UP},
  {0x228,	VAL_PASSKEY + VPK_PAGE_DOWN},
  {0x207,	VAL_PASSKEY + VPK_HOME},
  {0x238,	VAL_PASSKEY + VPK_END},
  {0x224,	VAL_PASSKEY + VPK_DELETE},
  {0x21b,	VAL_PASSKEY + VPK_ESCAPE},
  {0x215,	VAL_PASSKEY + VPK_INSERT},
  {0x101,	VAL_PASSKEY + VPK_FUNCTION},
  {0x103,	VAL_PASSKEY + VPK_FUNCTION + 1},
  {0x109,	VAL_PASSKEY + VPK_FUNCTION + 2},
  {0x119,	VAL_PASSKEY + VPK_FUNCTION + 3},
  {0x111,	VAL_PASSKEY + VPK_FUNCTION + 4},
  {0x10b,	VAL_PASSKEY + VPK_FUNCTION + 5},
  {0x11b,	VAL_PASSKEY + VPK_FUNCTION + 6},
  {0x113,	VAL_PASSKEY + VPK_FUNCTION + 7},
  {0x10a,	VAL_PASSKEY + VPK_FUNCTION + 8},
  {0x11a,	VAL_PASSKEY + VPK_FUNCTION + 9},
  {0x105,	VAL_PASSKEY + VPK_FUNCTION + 10},
  {0x107,	VAL_PASSKEY + VPK_FUNCTION + 11},
  {0,		0}
};

static int model_ID = 0;
/* model identification string. possible values :
** 0 - Unknown model name
** 1 - NoteBRAILLE
** 2 - Clio-NoteBRAILLE
** 3 - SCRIBA
** 4 - AzerBRAILLE
** 5 - Iris (not defined yet, will come soon...)
*/

static char version_ID[21] = "Unknown";
/* This is where the driver will store the internal ROM version of the
** terminal. It is used when the user asks the version of BRLTTY and his
** terminal.
*/

#define BRLROWS		1
#define MAX_STCELLS	5	/* hiest number of status cells */

/* This is the brltty braille mapping standard to Eurobraille's mapping table.
 */
char TransTable[256] =
{
   0x00, 0x01, 0x08, 0x09, 0x02, 0x03, 0x0A, 0x0B,
     0x10, 0x11, 0x18, 0x19, 0x12, 0x13, 0x1A, 0x1B,
     0x04, 0x05, 0x0C, 0x0D, 0x06, 0x07, 0x0E, 0x0F,
     0x14, 0x15, 0x1C, 0x1D, 0x16, 0x17, 0x1E, 0x1F,
     0x20, 0x21, 0x28, 0x29, 0x22, 0x23, 0x2A, 0x2B,
     0x30, 0x31, 0x38, 0x39, 0x32, 0x33, 0x3A, 0x3B,
     0x24, 0x25, 0x2C, 0x2D, 0x26, 0x27, 0x2E, 0x2F,
     0x34, 0x35, 0x3C, 0x3D, 0x36, 0x37, 0x3E, 0x3F,
     0x40, 0x41, 0x48, 0x49, 0x42, 0x43, 0x4A, 0x4B,
     0x50, 0x51, 0x58, 0x59, 0x52, 0x53, 0x5A, 0x5B,
     0x44, 0x45, 0x4C, 0x4D, 0x46, 0x47, 0x4E, 0x4F,
     0x54, 0x55, 0x5C, 0x5D, 0x56, 0x57, 0x5E, 0x5F,
     0x60, 0x61, 0x68, 0x69, 0x62, 0x63, 0x6A, 0x6B,
     0x70, 0x71, 0x78, 0x79, 0x72, 0x73, 0x7A, 0x7B,
     0x64, 0x65, 0x6C, 0x6D, 0x66, 0x67, 0x6E, 0x6F,
     0x74, 0x75, 0x7C, 0x7D, 0x76, 0x77, 0x7E, 0x7F,
     0x80, 0x81, 0x88, 0x89, 0x82, 0x83, 0x8A, 0x8B,
     0x90, 0x91, 0x98, 0x99, 0x92, 0x93, 0x9A, 0x9B,
     0x84, 0x85, 0x8C, 0x8D, 0x86, 0x87, 0x8E, 0x8F,
     0x94, 0x95, 0x9C, 0x9D, 0x96, 0x97, 0x9E, 0x9F,
     0xA0, 0xA1, 0xA8, 0xA9, 0xA2, 0xA3, 0xAA, 0xAB,
     0xB0, 0xB1, 0xB8, 0xB9, 0xB2, 0xB3, 0xBA, 0xBB,
     0xA4, 0xA5, 0xAC, 0xAD, 0xA6, 0xA7, 0xAE, 0xAF,
     0xB4, 0xB5, 0xBC, 0xBD, 0xB6, 0xB7, 0xBE, 0xBF,
     0xC0, 0xC1, 0xC8, 0xC9, 0xC2, 0xC3, 0xCA, 0xCB,
     0xD0, 0xD1, 0xD8, 0xD9, 0xD2, 0xD3, 0xDA, 0xDB,
     0xC4, 0xC5, 0xCC, 0xCD, 0xC6, 0xC7, 0xCE, 0xCF,
     0xD4, 0xD5, 0xDC, 0xDD, 0xD6, 0xD7, 0xDE, 0xDF,
     0xE0, 0xE1, 0xE8, 0xE9, 0xE2, 0xE3, 0xEA, 0xEB,
     0xF0, 0xF1, 0xF8, 0xF9, 0xF2, 0xF3, 0xFA, 0xFB,
     0xE4, 0xE5, 0xEC, 0xED, 0xE6, 0xE7, 0xEE, 0xEF,
     0xF4, 0xF5, 0xFC, 0xFD, 0xF6, 0xF7, 0xFE, 0xFF
};

/* Global variables */

int		InDate = 0;
int		chars_per_sec;
int		brl_fd;			/* file descriptor for Braille display */
struct termios	oldtio;		/* old terminal settings */
unsigned char	*rawdata = NULL;		/* translated data to send to Braille */
unsigned char	*prevdata = NULL;	/* previously sent raw data */
unsigned char	*lcd_data = NULL;	/* previously sent to LCD */
int		NbCols = 0;			/* number of cells available */
static short	ReWrite = 0;		/* 1 if display need to be rewritten */
static short	ReWrite_LCD = 0;		/* same as rewrite, for LCD */
static int	OffsetType = CR_ROUTE;
static int	PktNbr = 127; /* 127 at first time */
static int	context = 0;   /* Input type test-value */
static int	control = 0;
static int	alt = 0;

/* Communication codes */

#define SOH     0x01
#define EOT     0x04
#define ACK     0x06
#define DLE     0x10
#define NACK    0x15

#define PRT_E_PAR 0x01		/* parity error */
#define PRT_E_NUM 0x02		/* frame numver error */
#define PRT_E_ING 0x03		/* length error */
#define PRT_E_COM 0x04		/* command error */
#define PRT_E_DON 0x05		/* data error */
#define PRT_E_SYN 0x06		/* syntax error */

#define DIM_INBUFSZ 256

static int readbrlkey(BrailleDisplay *brl);

static int begblk(BrailleDisplay *brl)
{
  OffsetType = CR_CUTBEGIN;
  return (EOF);
}

static int endblk(BrailleDisplay *brl)
{
  OffsetType = CR_CUTLINE;
  return (EOF);
}

static int sendbyte(unsigned char c)
{
  return (write(brl_fd, &c, 1));
}

static int WriteToBrlDisplay (BrailleDisplay *brl, int len, char *data)
{
  unsigned char	buf[1024];
  unsigned char		*p = buf;
  unsigned char parity = 0;

   if (!len)
     return (1);

   *p++ = SOH;
   while (len--)
     {
	switch (*data)
	  {
	   case SOH:
	   case EOT:
	   case ACK:
	   case DLE:
	   case NACK:
	     *p++ = DLE;
	  /* no break */
	   default:
	     *p++ = *data;
	     parity ^= *data++;
	  }
     }
   *p++ = PktNbr;
   parity ^= PktNbr;
   if (++PktNbr >= 256)
     PktNbr = 128;
   switch (parity)
     {
      case SOH:
      case EOT:
      case ACK:
      case DLE:
      case NACK:
	*p++ = DLE;
      /* no break */
      default:
	*p++ = parity;
     }
   *p++ = EOT;
   brl->writeDelay += (p - buf) * 1000 / chars_per_sec;
   return (write(brl_fd, buf, p - buf));
}

static void brl_identify (void)
{
   LogPrint(LOG_NOTICE, "EuroBraille driver, version 1.2");
   LogPrint(LOG_INFO, "  Copyright (C) 1997-2003");
   LogPrint(LOG_INFO, "      - Yannick PLASSIARD <plassi_y@epitech.net>");
   LogPrint(LOG_INFO, "      - Nicolas PITRE <nico@cam.org>");
}

static int brl_open (BrailleDisplay *brl, char **parameters, const char *dev)
{
   struct termios newtio;	/* new terminal settings */

   rawdata = prevdata = lcd_data = NULL;		/* clear pointers */

  /* Open the Braille display device for random access */
   brl_fd = open (dev, O_RDWR | O_NOCTTY);
   if (brl_fd < 0)
     {
	LogPrint( LOG_ERR, "%s: %s", dev, strerror(errno) );
        return 0;
     }
   tcgetattr (brl_fd, &oldtio);	/* save current settings */

  /* Set 8E1, enable reading, parity generation, etc. */
   newtio.c_cflag = CS8 | CLOCAL | CREAD | PARENB;
   newtio.c_iflag = INPCK;
   newtio.c_oflag = 0;		/* raw output */
   newtio.c_lflag = 0;		/* don't echo or generate signals */
   newtio.c_cc[VMIN] = 0;	/* set nonblocking read */
   newtio.c_cc[VTIME] = 0;

  /* set speed */
   chars_per_sec = baud2integer(BAUDRATE) / 10;
   cfsetispeed (&newtio, BAUDRATE);
   cfsetospeed (&newtio, BAUDRATE);
   tcsetattr (brl_fd, TCSANOW, &newtio);	   /* activate new settings */

  /* Set model params... */
   brl->helpPage = 0;
   brl->y = BRLROWS;
   while (!NbCols)
     {
	int i = 0;
	unsigned char AskIdent[] =
	  {2, 'S', 'I',3,'M','P','\x37'};
	WriteToBrlDisplay (brl, 7, AskIdent);
	while (!NbCols)
	  {
	     drainBrailleOutput (brl, 100);
	     brl_readCommand (brl, CMDS_SCREEN);       /* to get the answer */
	     if (++i >= 10)
	       break;
	  }
     }

   ReWrite = 1;  /* To write whole display at first time */
   ReWrite_LCD = 1;
   return 1;
}

static void brl_close (BrailleDisplay *brl)
{
   if (rawdata)
     {
       free (rawdata);
       rawdata = NULL;
     }
   if (prevdata)
     {
       free (prevdata);
       prevdata = NULL;
     }
   if (lcd_data)
     {
       free (lcd_data);
       lcd_data = NULL;
     }
   if (brl_fd >= 0)
     {
       tcsetattr (brl_fd, TCSADRAIN, &oldtio);	/* restore terminal settings */
       close (brl_fd);
       brl_fd = -1;
     }
}

static void brl_writeWindow (BrailleDisplay *brl)
{
   int i, j;

   if (context) 
     return ;
   if (!ReWrite)
     {
      /* We update the display only if it has changed */
	if (memcmp(brl->buffer, prevdata, NbCols) != 0)
	  ReWrite = 1;
     }
   if (ReWrite)
     {
      /* right end cells don't have to be transmitted if all dots down */
	i = NbCols;

	  {
	     char OutBuf[2 * i + 6];
	     char *p = OutBuf;

	     *p++ = i + 2;
	     *p++ = 'D';
	     *p++ = 'P';
	     for (j = 0;
		  j < i;
		  *p++ = TransTable[(prevdata[j++] = brl->buffer[j])]);
	     WriteToBrlDisplay (brl, p - OutBuf, OutBuf);
	     ReWrite = 0;
	  }
     }
}

/*
** If defined, the next routine will print informations on the LCD screen
*/

#ifdef		BRL_HAVE_VISUAL_DISPLAY

static void	brl_writeVisual(BrailleDisplay *brl)
{
  int		i = NbCols;
  char		OutBuf[2 * NbCols + 6];
  char	        *p = OutBuf;
  int		j;

  if (ReWrite_LCD == 0)
    if (memcmp(brl->buffer, lcd_data, NbCols) != 0)
      {
	ReWrite_LCD = 1;
	memcpy(lcd_data, brl->buffer, NbCols);
      }
  if (ReWrite_LCD)
    {
      memset(OutBuf, 0, NbCols + 2);
      *p++ = i + 2;
      *p++ = 'D';
      *p++ = 'L';
      for (j = 0; j < i; j++)
	*p++ = brl->buffer[j];
      WriteToBrlDisplay(brl, p - OutBuf, OutBuf);
      ReWrite_LCD = 0;
    }
}

#endif

static void brl_writeStatus (BrailleDisplay *brl, const unsigned char *st)
{
  
}

static int Program(BrailleDisplay *brl)
{
  int		key;
  t_key		*p = 0;
  short		i;

  switch (model_ID)
    {
    case 1:
      message("P PROGRAMMING      x", MSG_NODELAY);
      p = pnb_keys;
      break;
    case 2:
      message("P PROGRAMMING      x", MSG_NODELAY);
      p = pclio_keys;
      break;
    case 3:
      message("Beta level ...",MSG_NODELAY);
      p = pscriba_keys;
      break;
    case 4:
      message("P PROGRAMMING      x", MSG_NODELAY);
      p = pazer80_keys;
      break;
    default:
      message("P Unimplemented yet!", MSG_WAITKEY);
      break;
    }
  if (p)
    {
      while ((key = readbrlkey(brl)) != 10)
	{
	  for (i = 0; p[i].brl_key; i++)
	    if (key == p[i].brl_key)
	      {
		if (p[i].f)
		  return (p[i].f(brl));
		else
		  return (p[i].res);
	      }
	}
    }
  return (CMD_NOOP);
}


int ViewOn(BrailleDisplay *brl)
{
   int touche;
   static int res2 = 0;
   static int exitviewon = 0;
   if (InDate == 1)
     {
	InDate = 0;
	ReWrite = 1;
	return (0);
     }
   switch (model_ID)
     {
      case 3:
	message("Alpha level ...", MSG_NODELAY);
	break;
      default:
	message("V VIEW ON          x", MSG_NODELAY);
	break;
     }
   exitviewon=0;
   while (!exitviewon)
     {
	touche=readbrlkey(0);
	switch(touche)
	  {
	   case 1:
	     exitviewon=1;
	     break;
	   case 3:
	     res2 = CMD_TOP_LEFT;
	     exitviewon=1;
	     break;
	   case 9:
	     res2 = CMD_BOT_LEFT;
	     exitviewon=1;
	     break;
	   case 11:
	     exitviewon=1;
	     break;
	   case 12:
	     res2 = CMD_DISPMD;
	     exitviewon=1;
	     break;
	   case 19:
	     res2 = CMD_LEARN;
	     exitviewon=1;
	     break;
	   case 24:
	     res2 = CMD_NOOP;
	     break;
	  }
     }
   return res2;
}

static int brl_readCommand(BrailleDisplay *brl, DriverCommandContext cmds)
{
  int		res;
  int		i;
  t_key		*p;
  int		key;

  res = 0;
  key = readbrlkey(brl);
  p = 0;
  if (model_ID == 1)
    p = nb_keys;
  else if (model_ID == 2)
    p = clio_keys;
  else if (model_ID == 3)
    p = scriba_keys;
  else if (model_ID == 4 && NbCols == 40)
    p = azer40_keys;
  else if (model_ID == 4 && NbCols == 80)
    p = azer80_keys;
  else if (model_ID == 5)
    p = iris_keys;
  if (p)
    {
      for (i = 0; p[i].brl_key; i++)
	if (p[i].brl_key == key)
	  {
	    if (p[i].f)
	      res = p[i].f(brl);
	    else
	      res = p[i].res;
	  }
    }
  if (res)
    return (res);
  else
    return (key);
}

static int routing(int routekey)
{
   int res = EOF;

   switch (context)
     {
      case 1:
	switch (routekey)
	  {
	   case 0x02: /* exit menu */
	     ReWrite = 1;
	     context = 0;
	     res = CMD_NOOP;
	     break;
	   case 0x06: /* Console Switching */
	     context = 0;
	     message("switch:1 2 3 4 5 6 t", MSG_NODELAY);
	     context = 2;
	     ReWrite = 0;
	     res = CMD_NOOP;
	     break;
	   case 0x0B: /* Help */
	     context = 0;
	     ReWrite = 1;
	     res = CMD_LEARN;
	     break;
	  case 0x13: /* version information */
	    context = 0;
	    message(version_ID, MSG_WAITKEY);
	    res = CMD_NOOP;
	    break;
	  }
	break;
      case 2:
	switch(routekey)
	  {
	   case 0x07: /* exit */
	     context = 0;
	     ReWrite = 1;
	     res = CMD_NOOP;
	     break;
	   case 0x09: /* switch to console 1 */
	     res = CR_SWITCHVT;   /* CR_WITCHVT + 0 */
	     context = 0;
	     ReWrite = 1;
	     break;
	   case 0x0B: /* switch to console 2 */
	     res = CR_SWITCHVT + 1;
	     ReWrite = 1;
	     context = 0;
	     break;
	   case 0x0D: /* switch to console 3 */
	     res = CR_SWITCHVT + 2;
	     context = 0;
	     ReWrite = 1;
	     break;
	   case 0x0F: /* switch to console 4 */
	     res = CR_SWITCHVT + 3;
	     ReWrite = 1;
	     context = 0;
	     break;
	   case 0x11: /* switch to console 5 */
	     res = CR_SWITCHVT + 4;
	     context = 0;
	     ReWrite = 1;
	     break;
	   case 0x13: /* switch to console 6 */
	     res = CR_SWITCHVT + 5;
	     context = 0;
	     ReWrite = 1;
	     break;
	  }
	break;
      case 0:
	switch (routekey)
	  {
	  case 0x57:
	    /* no break */
	   case 0x83: /* Entering in menu-mode */
	     message("-:tty help version t", MSG_NODELAY);
	     context = 1;
	     res = CMD_NOOP;
	     break;
	   default:
	     res = OffsetType + routekey - 1;
	     OffsetType = CR_ROUTE;
	     break;
	  }
	break;
     }
   return res;
}

static int	convert(int keys)
{
  int		res;

  res = 0;
  res = (keys & 128) ? 128 : 0;
  res += (keys & 64) ? 64 : 0;
  res += (keys & 32) ? 32 : 0;
  res += (keys & 16) ? 8 : 0;
  res += (keys & 8) ? 2 : 0;
  res += (keys & 4) ? 16 : 0;
  res += (keys & 2) ? 4 : 0;
  res += (keys & 1) ? 1 : 0;
  return (res);
}


static int	key_handle(BrailleDisplay *brl, char *buf)
{
  int	res = EOF;
  /* here the braille keys are bitmapped into an int with
   * dots 1 through 8, left thumb and right thumb
   * respectively.  It makes up to 1023 possible
   * combinations! Here's some of them.
   */
  unsigned int keys = (buf[0] & 0x3F) |
    ((buf[1] & 0x03) << 6) |
    ((int) (buf[0] & 0xC0) << 2);
  if (keys >= 0xff && keys != 0x280 && keys != 0x2c0
      && keys != 0x200)
    {
      /*
      ** keys that must be explicitly listed
      */
      int	h;
      for (h = 0; brl_key[h].brl; h++)
	if (brl_key[h].brl == keys)
	  res = brl_key[h].key;
      if (control || alt)
	{
	  control = 0;
	  alt = 0;
	  context = 0;
	  ReWrite = 1;
	}
    }
  if (keys == 0x280 && alt)
    {
      context = 0;
      alt = 0;
      ReWrite = 1;
      res = CMD_NOOP;
    }
  if (keys == 0x280 && !alt && !control) /* alt */
    {
      message("! alt", MSG_NODELAY);
      context = 4;
      ReWrite = 0;
      alt = 1;
      res = CMD_NOOP;
    }
  if (alt && control)
    {
      context = 0;
      message("! alt control", MSG_NODELAY);
      context = 4;
    }
  if (keys == 0x2c0 && control)
    {
      context = 0;
      ReWrite = 1;
      res = CMD_NOOP;
      control = 0;
    }
  if (keys == 0x2c0 && !control) /* control */
    {
      control = 1;
      message("! control ", MSG_NODELAY);
      context = 4;
      ReWrite = 0;
      res = CMD_NOOP;
    }
  if (keys <= 0xff || keys == 0x200)
    {
      /*
      ** we pass a char
      */
      res = (VAL_PASSDOTS | convert(keys));
      if (control)
	{
	  res |= VPC_CONTROL;
	  control = 0;
	  context = 0;
	}
      if (alt)
	{
	  res |= VPC_META;
	  context = 0;
	  alt = 0;
	}
    }
  return (res);
}

static int readbrlkey(BrailleDisplay *brl)
{
  int res = EOF;
  unsigned char c;
  static unsigned char buf[DIM_INBUFSZ];
  static int DLEflag = 0, ErrFlag = 0;
  static int pos = 0, p = 0, pktready = 0;
  
  /* here we process incoming data */
  while (!pktready && read (brl_fd, &c, 1))
    {
      if (DLEflag)
	{
	  DLEflag = 0;
	  if (pos < DIM_INBUFSZ) buf[pos++] = c;
	}
      else if( ErrFlag )
	{
	  ErrFlag = 0;
	  /* Maybe should we check error code in c here? */
	  ReWrite = 1;
	}
      else
	switch (c)
	  {
	  case NACK:
	    ErrFlag = 1;
	    /* no break */
	  case ACK:
	  case SOH:
	    pos = 0;
	    break;
	  case DLE:
	    DLEflag = 1;
	    break;
	  case EOT:
	    {
	      /* end of packet, let's read it */
	      int i;
	      unsigned char parity = 0;
	      if (pos < 4)
		break;		/* packets can't be shorter */
	      for( i = 0; i < pos-1; i++ )
		parity ^= buf[i];
	      if ( parity != buf[pos - 1])
		{
		  sendbyte (NACK);
		  sendbyte (PRT_E_PAR);
		}
	      else if (buf[pos - 2] < 0x80)
		{
		  sendbyte (NACK);
		  sendbyte (PRT_E_NUM);
		}
	      else
		{
		  /* packet is OK */
		  sendbyte (ACK);
		  pos -= 2;	/* now forget about packet number and parity */
		  p = 0;
		  pktready = 1;
		}
	      break;
	    }
	  default:
	    if (pos < DIM_INBUFSZ)
	      buf[pos++] = c;
	    break;
	  }
    }
  
  /* Packet is OK, we go inside */
  if (pktready)
    {
      int lg;
      for (lg = 0; res == EOF; )
	{
	  /* let's look at the next message */
	  lg = buf[p++];
	  if (lg >= 0x80 || p + lg > pos)
	    {
	      pktready = 0;	/* we are done with this packet */
	      break;
	    }
	  if (buf[p] == 'K' && buf[p + 1] != 'B' && context == 4)
	    {
	      context = 0;
	      alt = 0;
	      control = 0;
	    }
	  switch (buf[p])
	    {
	    case 'R': /* Mode checking */
	      switch(buf[p + 1])
		{
		case 'B': /* PC-BRAILLE mode */
		  InDate = 0;
		  ReWrite = 1; /* to refresh display */
		  context = 0;
		  res = CMD_NOOP;
		  break;
		case 'V': /* Braille and Speech mode */
		  message("! Speech unavailable", MSG_WAITKEY);
		  break;
		}
	      break;
	    case 'K':      /* Keyboard -- here are the key bindings */
	      switch (buf[p + 1])
		{
		case 'T':
		  {
		    int	p2;
		    
		    for (p2 = 0; num_keys[p2].brl_key; p2++)
		      if (buf[p + 2] == num_keys[p2].brl_key)
			res = num_keys[p2].res;
		  }
		  break;
		case 'I':	/* Routing Key */
		  res = routing(buf[p + 2]);
		  break;
		case 'B':	/* Braille keyboard */
		  res = key_handle(brl, buf + p + 2);
		  break;
		}
	      break;
	    case 'S':		/* status information */
	      if (buf[p + 1] == 'I')
		{
		  /* we take the third letter from the model identification
		   * string as the amount of cells available and the two first letters
		   * for the model name in order to optimize the key configuration.
		   */
		  if (buf[p + 2] == 'N' && buf[p + 3] == 'B')
		    model_ID = 1;
		  else if (buf[p + 2] == 'C' && buf[p + 3] == 'N')
		    model_ID = 2;
		  else if (buf[p + 2] == 'S' && 
			   ((buf[p + 3] == 'C') || (buf[p + 3] == 'B')))
		    model_ID = 3;
		  else if (buf[p + 2] == 'C' && (buf[p + 3] == 'Z' || buf[p + 3] == 'P'))
		    model_ID = 4;
		  else if (buf[p + 2] == 'I' && buf[p + 3] == 'R')
		    model_ID = 5;
		  else
		    model_ID = 0;
		  if (strncmp(version_ID, buf + p + 2, 3))
		    {
		      strncpy(version_ID, buf + p + 2, 20);
		      NbCols = (buf[p + 4] - '0') * 10;
		      LogPrint(LOG_INFO, "Detected EuroBraille version %s: %d columns",
			       version_ID, NbCols);
		      brl->x = NbCols;
		      rawdata = realloc(rawdata, brl->y * brl->x);
		      prevdata = realloc(prevdata, brl->x * brl->y);
		      lcd_data = realloc(lcd_data, brl->x * brl->y);
		      brl->resizeRequired = 1;
		    }
		}
	      ReWrite = 1;
	    }
	  break;
	}
      p += lg;
    }
  return (res);
}
