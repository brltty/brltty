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

/* EuroBraille/braille.c - Braille display library for the IRIS family.
 * Copyright (C) 2004 by Yannick Plassiard <yan@mistigri.org>
 *                        and Nicolas Pitre <nico@cam.org>
 * See the GNU General Public License for details in the LICENSE-GPL file
 * See the README file for details about copyrights and version informations
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include	<fcntl.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<sys/ioctl.h>

#include	"message.h"
#include	"misc.h"


#define		BRL_HAVE_PACKET_IO	1
#define		BRL_HAVE_KEY_CODES	1
#include	"brldefs.h"
#include	"brl_driver.h"
#include	"braille.h"
#include	"io_serial.h"

/*
** For debugging only
*/
/* #define		LOG_IO */



/*
** next tables define assigmenss bitween keys and HLLTTY commands depending on
** the terminal type (Scriba, Clio)NoteBraille, or AzerBraille).
**
** If you make changes to those tables I'd like to be informed in order to
** update the driver for next releases.
*/

static t_key	iris_keys[25] = {
  {BRL_BLK_PASSKEY | BRL_KEY_CURSOR_UP, VK_FH, 0},
  {BRL_BLK_PASSKEY | BRL_KEY_CURSOR_LEFT, VK_FG, 0},
  {BRL_BLK_PASSKEY | BRL_KEY_CURSOR_RIGHT, VK_FD, 0},
  {BRL_BLK_PASSKEY | BRL_KEY_CURSOR_DOWN, VK_FB, 0},
  {BRL_CMD_NOOP, VK_FGB, Program},
  {BRL_CMD_NOOP, VK_FDH, SynthControl},
  {BRL_CMD_NOOP, VK_FDB, ViewOn},
  {BRL_CMD_FWINLT, VK_L1, 0},
  {BRL_CMD_LNUP, VK_L2, 0},
  {BRL_CMD_PRPROMPT, VK_L3, 0},
  {BRL_CMD_PREFMENU, VK_L4, 0},
  {BRL_CMD_INFO, VK_L5, 0},
  {BRL_CMD_NOOP, VK_L5OLD, 0},
  {BRL_CMD_NXPROMPT, VK_L6, 0},
  {BRL_CMD_LNDN, VK_L7, 0},
  {BRL_CMD_FWINRT, VK_L8, 0},
  {BRL_CMD_TOP, VK_L12, 0},
  {BRL_CMD_BOT, VK_L78, 0},
  {BRL_CMD_CSRVIS, VK_L23, 0},
  {BRL_CMD_TUNES, VK_L67, 0},
  {BRL_CMD_RESTARTSPEECH, VK_L68, 0},
  {BRL_CMD_NOOP, VK_L34, begblk},
  {BRL_CMD_NOOP, VK_L56, endblk},
  {BRL_CMD_PASTE, VK_L57, 0},
  {0, 0, 0}
};

static t_key	piris_keys[] =
{
  {BRL_CMD_NOOP, VK_L1, begblk},
  {BRL_CMD_NOOP, VK_L2, 0},
  {BRL_CMD_NOOP, VK_L3, 0},
  {BRL_CMD_NOOP, VK_L4, 0},
  {BRL_CMD_NOOP, VK_L5, 0},
  {BRL_CMD_NOOP, VK_L5OLD, 0},
  {BRL_CMD_NOOP, VK_L6, 0},
  {BRL_CMD_PASTE, VK_L7, 0},
  {BRL_CMD_NOOP, VK_L8, endblk},
  {0, 0, 0}
};


/*
** synth control keys (level 4 )
*/


static t_key	psynth_keys[6] = {
  {BRL_CMD_SAY_BELOW, VK_FB, 0},
  {BRL_CMD_SAY_ABOVE, VK_FH, 0},
  {BRL_CMD_SAY_SOFTER, VK_FG, 0},
  {BRL_CMD_SAY_LOUDER, VK_FD, 0},
  {BRL_CMD_SAY_LINE, VK_FBH, 0},
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

static t_alias	brl_key[] = {
  {0x100,	BRL_BLK_PASSKEY + BRL_KEY_BACKSPACE},
  {0x300,	BRL_BLK_PASSKEY + BRL_KEY_ENTER},
  {0x232,	BRL_BLK_PASSKEY + BRL_KEY_TAB},
  {0x208,	BRL_BLK_PASSKEY + BRL_KEY_CURSOR_UP},
  {0x220,	BRL_BLK_PASSKEY + BRL_KEY_CURSOR_DOWN},
  {0x210,	BRL_BLK_PASSKEY + BRL_KEY_CURSOR_RIGHT},
  {0x202,	BRL_BLK_PASSKEY + BRL_KEY_CURSOR_LEFT},
  {0x205,	BRL_BLK_PASSKEY + BRL_KEY_PAGE_UP},
  {0x228,	BRL_BLK_PASSKEY + BRL_KEY_PAGE_DOWN},
  {0x207,	BRL_BLK_PASSKEY + BRL_KEY_HOME},
  {0x238,	BRL_BLK_PASSKEY + BRL_KEY_END},
  {0x224,	BRL_BLK_PASSKEY + BRL_KEY_DELETE},
  {0x21b,	BRL_BLK_PASSKEY + BRL_KEY_ESCAPE},
  {0x215,	BRL_BLK_PASSKEY + BRL_KEY_INSERT},
  {0x101,	BRL_BLK_PASSKEY + BRL_KEY_FUNCTION},
  {0x103,	BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 1},
  {0x109,	BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 2},
  {0x119,	BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 3},
  {0x111,	BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 4},
  {0x10b,	BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 5},
  {0x11b,	BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 6},
  {0x113,	BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 7},
  {0x10a,	BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 8},
  {0x11a,	BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 9},
  {0x105,	BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 10},
  {0x107,	BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 11},
  {0,		0}
};


#define BRLROWS		1
#define MAX_STCELLS	5	/* hiest number of status cells */

/* This is the brltty braille mapping standard to Eurobraille's mapping table.
 */
static TranslationTable outputTable;

/* Global variables */

static int		chars_per_sec;
static SerialDevice *	serialDevice;			/* file descriptor for Braille display */
static unsigned char	*prevdata = NULL;	/* previously sent raw data */
static wchar_t	*lcd_data = NULL;	/* previously sent to LCD */
static int		NbCols = 0;			/* number of cells available */
static char		IdentString[41] = "Device not recognized!";
static int		gio_fd = -1;

static short	ReWrite = 0;		/* 1 if display need to be rewritten */
static short	ReWrite_LCD = 0;		/* same as rewrite, for LCD */
static int	OffsetType = BRL_BLK_ROUTE;
static int	context = 0;
static int	control = 0;
static int	alt = 0;


/* Communication codes */

#define SOH     0x01
#define EOT     0x04
#define ACK     0x06
#define DLE     0x10
#define NACK    0x15

#define DIM_INBUFSZ 256

static int readbrlkey(BrailleDisplay *brl, char key_context);

static int begblk(BrailleDisplay *brl)
{
  OffsetType = BRL_BLK_CUTBEGIN;
  return (EOF);
}

static int endblk(BrailleDisplay *brl)
{
  OffsetType = BRL_BLK_CUTLINE;
  return (EOF);
}


static int WriteToBrlDisplay (BrailleDisplay *brl, int len, const char *data)
{
  unsigned char	buf[1024];
  unsigned char		*p = buf;
#ifdef	LOG_IO
  int		logfd;
#endif

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
	     *p++ = *data++;
	  }
     }
   *p++ = EOT;
   // brl->writeDelay += (p - buf) * 1000 / chars_per_sec;
#ifdef		LOG_IO
   logfd = open("/eb-log.out", O_CREAT | O_APPEND | O_WRONLY, 0600);
   write(logfd, "WritePacket: ", 13);
   write(logfd, buf, p - buf);
   write(logfd, "\n", 1);
   close(logfd);
#endif
   return (serialWriteData(serialDevice, buf, p - buf));
}

static ssize_t brl_writePacket(BrailleDisplay *brl, const void *p, size_t sz)
{
  char			c;

  if (serialWriteData(serialDevice, p, sz) != sz)
    return (0);
  if (!serialAwaitInput(serialDevice, 20))
    return (0);
  if (serialReadData(serialDevice, &c, 1, 0, 0) == 1 && c == ACK)
    return (1);
  else
    serialReadData(serialDevice, &c, 1, 0, 0); /* This is done to trap the error code */
  return (0);
}

static int brl_reset(BrailleDisplay *brl)
{
  return (brl_writePacket(brl, "I", 1) == 1);
}

static int brl_construct (BrailleDisplay *brl, char **parameters, const char *device)
{
  static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
  makeOutputTable(dots, outputTable);

  if ((gio_fd = open("/dev/iris", O_RDWR)) == -1)
    {
      LogPrint(LOG_INFO, "Cannot open Iris-GIO device.\n");
      return 0;
    }
  
  /*
  ** Tells the hardware to turn on
  */
  if (ioctl(gio_fd, IOCTL_SETBIT_FIRSTBYTE, IO_D0) == -1)
    {
      LogPrint(LOG_INFO, "Cannot send ioctl to device.\n");
      return 0;
    }

  /*
  ** Give the hardware enough time to consider the request
  */
  usleep(8500);
  if (ioctl(gio_fd, IOCTL_SETBIT_FIRSTBYTE, 0x00) == -1)
    {
      LogPrint(LOG_INFO, "Cannot clear device bits.\n");
      return 0;
    }
  /*
  ** Now we can open the device
  */
  close(gio_fd);

   if (!isSerialDevice(&device)) {
     unsupportedDevice(device);
     return 0;
   }

   prevdata = NULL;		/* clear pointers */
   lcd_data = NULL;		/* clear pointers */

  /* Open the Braille display device for random access */
   if (!(serialDevice = serialOpenDevice(device))) return 0;
   serialSetParity(serialDevice, SERIAL_PARITY_EVEN);

  /* set speed */
   chars_per_sec = BAUDRATE / 11;
   serialRestartDevice(serialDevice, BAUDRATE);

  /* Set model params... */
   brl->helpPage = 0;
   brl->y = BRLROWS;
   while (!NbCols)
     {
	int i = 0;
	unsigned char AskIdent[1] = "V";
	WriteToBrlDisplay (brl, 1, (const char *)AskIdent);
	while (!NbCols)
	  {
	     drainBrailleOutput (brl, 100);
	     brl_readCommand (brl, BRL_CTX_SCREEN);       /* to get the answer */
	     if (++i >= 10)
	       break;
	  }
     }
   ReWrite = 1;  /* To write whole display at first time */
   ReWrite_LCD = 1;
   return 1;
}

static void brl_destruct (BrailleDisplay *brl)
{
  char	buf[43];
  
  memset(buf, 0, 43);
  buf[0] = SOH;
  buf[1] = 'B';
  buf[42] = EOT;
  brl_writePacket(brl, buf, 43);
  usleep(10000);
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
   if (serialDevice)
     {
       serialCloseDevice (serialDevice);
       serialDevice = NULL;
     }
   gio_fd = open("/dev/iris", O_RDWR);
   if (gio_fd == -1)
     return;
   if (ioctl(gio_fd, IOCTL_SETBIT_FIRSTBYTE, IO_D1) == -1)
     LogPrint(LOG_INFO, "Cannot turn off braille power.\n");
   usleep(8500);
   if (ioctl(gio_fd, IOCTL_SETBIT_FIRSTBYTE, 0x00) == -1)
     LogPrint(LOG_INFO, "Cannot send ioctl().\n");
   close(gio_fd);
   gio_fd = -1;
}

static int	writeVisual(BrailleDisplay *brl, const wchar_t *text)
{
  int		i = NbCols;
  char		OutBuf[2 * NbCols + 6];
  char	        *p = OutBuf;
  int		j;

  if (ReWrite_LCD == 0)
    if (wmemcmp(text, lcd_data, NbCols) != 0)
      {
	ReWrite_LCD = 1;
	wmemcpy(lcd_data, text, NbCols);
      }
  if (ReWrite_LCD)
    {
      memset(OutBuf, 0, NbCols + 2);
      *p++ = 'L';
      for (j = 0; j < i; j++)
        {
          wchar_t wc = text[j];
          *p++ = iswLatin1(wc)? wc: '?';
        }
      WriteToBrlDisplay(brl, p - OutBuf, OutBuf);
      ReWrite_LCD = 0;
    }
  return 1;
}

static int brl_writeWindow (BrailleDisplay *brl, const wchar_t *text)
{
   int i = 41, j = 0;

   if (text)
     writeVisual(brl, text);

   if (context) 
     {
       return 1;
     }
   if (!ReWrite)
     /* We update the display only if it has changed */
     if (memcmp(brl->buffer, prevdata, NbCols) != 0)
       {
	 ReWrite = 1;
	 memcpy(prevdata, brl->buffer, NbCols);
       }
   if (ReWrite)
     {
      /* right end cells don't have to be transmitted if all dots down */
       char OutBuf[i + 1];
       char *p = OutBuf;
       memset(OutBuf, 0, i + 1);
       *p++ = 'B';
       if (NbCols == 32)
	 for (j = 0; j < 8; j++)
	   *p++ = 0;
       for (j = NbCols - 1; j >= 0; j--)
	 *p++ = outputTable[brl->buffer[j]];
       WriteToBrlDisplay (brl, p - OutBuf, OutBuf);
       ReWrite = 0;
     }
   return 1;
}

static int Program(BrailleDisplay *brl)
{
  int		key = 0;
  t_key		*p = NULL; // piris_keys;
  short		i = 0;

  if (p)
    {
      message("Level 1 ...", MSG_NODELAY);
      while ((key = readbrlkey(brl, CTX_COMMANDS)) != VK_FGB)
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
  return (BRL_CMD_NOOP);
}



/*
** Synth control method 
*/

static int SynthControl(BrailleDisplay *brl)
{
  message("Level 4 ...", MSG_NODELAY);
  context = IN_LEVEL4;
  return (EOF);
}


int ViewOn(BrailleDisplay *brl)
{
  return BRL_CMD_NOOP;
}

static int brl_readKey(BrailleDisplay *brl)
{
  return (int)readbrlkey(brl, CTX_KEYCODES);
}

static int brl_keyToCommand(BrailleDisplay *brl, BRL_DriverCommandContext key_context, int code)
{
  if (code & 0x00010000)
    {
      code &= 0x00003fff;
      return (linear_handle(brl, code, CTX_COMMANDS));
    }
  else if (code & 0x00040000)
    {
      code &= 0x000000ff;
      return (routing(brl, code, CTX_COMMANDS));
    }
  else if (code & 0x00020000)
    {
      unsigned char buf[2];

      code &= 0x000003ff;
      buf[0] = 0; buf[1] = 0;
      buf[0] = (code >> 8) & 0x03;
      buf[1] = code & 0x000000ff;
      return (key_handle(brl, buf, CTX_COMMANDS));
    }
  return (EOF);
}

static int brl_readCommand(BrailleDisplay *brl, BRL_DriverCommandContext key_context)
{
  unsigned int	key;

  key = readbrlkey(brl, CTX_COMMANDS);
  return (key);
}

static int routing(BrailleDisplay *brl, int routekey, char ctx)
{
  int	res = 0;

  if (ctx == CTX_COMMANDS) 
    {
      res = handle_routekey(brl, routekey);
    }
  else
    {
      res = routekey | EB_ROUTING;
    }
  return (res);
}

static int handle_routekey(BrailleDisplay *brl, int routekey)
{
   int res = EOF;
   int	flag = 0;

   switch (context)
     {
       case 1:
	switch (routekey)
	  {
	   case 0x02: /* exit menu */
	     ReWrite = 1;
	     context = 0;
	     res = BRL_CMD_NOOP;
	     break;
	   case 0x06: /* Console Switching */
 	     context = 0;
	     if (NbCols == 20)
	       message("switch:1 2 3 4 5 6 t", MSG_NODELAY);
	     else
	       message("switch:1 2 3 4 5 6 7 t", MSG_NODELAY);
	     context = 2;
	     ReWrite = 0;
	     res = BRL_CMD_NOOP;
	     break;
	   case 0x0A: /* Help */
	     context = 0;
	     ReWrite = 1;
	     res = BRL_CMD_LEARN;
	     break;
	  case 0x0F: /* version information */
	    context = 0;
	    message(IdentString, MSG_WAITKEY);
	    res = BRL_CMD_NOOP;
	    break;
	  }
	break;
      case 2:
	switch(routekey)
	  {
	   case 0x07: /* exit */
	     context = 0;
	     ReWrite = 1;
	     res = BRL_CMD_NOOP;
	     break;
	   case 0x09: /* switch to console 1 */
	     res = BRL_BLK_SWITCHVT;   /* BRL_BLK_WITCHVT + 0 */
	     context = 0;
	     ReWrite = 1;
	     break;
	   case 0x0B: /* switch to console 2 */
	     res = BRL_BLK_SWITCHVT + 1;
	     ReWrite = 1;
	     context = 0;
	     break;
	   case 0x0D: /* switch to console 3 */
	     res = BRL_BLK_SWITCHVT + 2;
	     context = 0;
	     ReWrite = 1;
	     break;
	   case 0x0F: /* switch to console 4 */
	     res = BRL_BLK_SWITCHVT + 3;
	     ReWrite = 1;
	     context = 0;
	     break;
	   case 0x11: /* switch to console 5 */
	     res = BRL_BLK_SWITCHVT + 4;
	     context = 0;
	     ReWrite = 1;
	     break;
	   case 0x13: /* switch to console 6 */
	     res = BRL_BLK_SWITCHVT + 5;
	     context = 0;
	     ReWrite = 1;
	     break;
	   case 0x15: /* switch to console 6 */
	     res = BRL_BLK_SWITCHVT + 6;
	     context = 0;
	     ReWrite = 1;
	     break;
	  }
	break;
      case 0:
	switch (routekey)
	  {
	  case 0x57:
	  case 0x81: /* Entering in menu-mode */
	    flag = 1;
	    break;
	  default:
	    res = OffsetType + (routekey & 0x7f) - 1;
	    OffsetType = BRL_BLK_ROUTE;
	    break;
	  }
	if (flag == 1)
	  {
	    message("i:tty hlp info t", MSG_NODELAY);
	    context = 1;
	    res = BRL_CMD_NOOP;
	  }
	break;
     }
   return res;
}

static int	convert(int keys)
{
  int		res;

  res = 0;
  res = (keys & 128) ? BRL_DOT8 : 0;
  res += (keys & 64) ? BRL_DOT7 : 0;
  res += (keys & 32) ? BRL_DOT6 : 0;
  res += (keys & 16) ? BRL_DOT5 : 0;
  res += (keys & 8) ? BRL_DOT4 : 0;
  res += (keys & 4) ? BRL_DOT3 : 0;
  res += (keys & 2) ? BRL_DOT2 : 0;
  res += (keys & 1) ? BRL_DOT1 : 0;
  return (res);
}



static int	key_handle(BrailleDisplay *brl, unsigned char *buf, char key_context)
{
  int	res = EOF;
  /* here the braille keys are bitmapped into an int with
   * dots 1 through 8, left thumb and right thumb
   * respectively.  It makes up to 1023 possible
   * combinations! Here's some of them.
   */
  //  unsigned int keys = (buf[0] & 0x3F) |
  //    ((buf[1] & 0x03) << 6) |
  //    ((int) (buf[0] & 0xC0) << 2);
  unsigned int keys = buf[0] * 256 + buf[1];
  keys &= 0x000003ff;
  if (key_context == CTX_KEYCODES)
    return (keys | EB_BRAILLE);
  if (keys == 0)
    return (EOF);
  if (keys > 0xff && keys != 0x280 && keys != 0x2c0
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
      res = BRL_CMD_NOOP;
    }
  if (keys == 0x280 && !alt && !control) /* alt */
    {
      message("! alt", MSG_NODELAY);
      context = 4;
      ReWrite = 0;
      alt = 1;
      res = BRL_CMD_NOOP;
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
      res = BRL_CMD_NOOP;
      control = 0;
    }
  if (keys == 0x2c0 && !control) /* control */
    {
      control = 1;
      message("! control ", MSG_NODELAY);
      context = 4;
      ReWrite = 0;
      res = BRL_CMD_NOOP;
    }
  if (keys <= 0xff || keys == 0x200)
    {
      /*
      ** we pass a char
      */
      res = (BRL_BLK_PASSDOTS | convert(keys));
      if (control)
	{
	  res |= BRL_FLG_CHAR_CONTROL;
	  control = 0;
	  context = 0;
	}
      if (alt)
	{
	  res |= BRL_FLG_CHAR_META;
	  context = 0;
	  alt = 0;
	}
    }
  return (res);
}

#ifdef		BRL_HAVE_PACKET_IO

static ssize_t brl_readPacket(BrailleDisplay *brl, void *bp, size_t size)
{
  int			i;
  unsigned char		c;
  char			end;
  char			flag = 0;

  if (!serialAwaitInput(serialDevice, 20))
    return (0);
  memset(bp, 0, size);
  for (i = 0, end = 0; !end; i++)
    {
      if (serialReadData(serialDevice, &c, 1, 0, 0) != 1)
	return (0); /* Error while reading information */
      if (i >= size)
	return (0); /* Packet is too long to be read */
      ((unsigned char *)(bp))[i] = c;
      if (c == SOH && i == 0)
	flag = 1; /* start of packet */
      if (c == EOT && flag == 1 && ((unsigned char *)(bp))[i - 1] != DLE)
	end = 1; /* We've done reading a packet */
    }
  return (i);
}

#endif

static int readbrlkey(BrailleDisplay *brl, char key_context)
{
  int res = EOF;
#ifdef		LOG_IO
  int		logfd;
#endif
  unsigned char c;
  static int DLEflag = 0, ErrFlag = 0;
  static unsigned char buf[DIM_INBUFSZ];
  static int pos = 0, p = 0, pktready = 0;
  
  /* here we process incoming data */
  while (!pktready && (serialReadData (serialDevice, &c, 1, 0, 0) == 1))
    {
#ifdef		LOG_IO
      logfd = open("/tmp/eb-log.in", O_CREAT |  O_APPEND | O_WRONLY, 0600);
      write(logfd, &c, 1);
      close(logfd);
#endif
      if (DLEflag)
	{
	  DLEflag = 0;
	  if (pos < DIM_INBUFSZ) buf[pos++] = c;
	}
      else if (ErrFlag)
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
	    /* packet is finished */
	    p = 0;
	    pktready = 1;
	    break;
	  default:
	    if (pos < DIM_INBUFSZ)
	      buf[pos++] = c;
	    break;
	  }
    }
  /* Packet is OK, we go inside */
  if (pktready)
    {
      int key;

      switch (buf[p])
	{
	case 'B':
	  res = key_handle(brl, buf + p + 1, key_context);
	  break;
	case 'C': /* Input linear key(s) */
	  key = buf[p + 1] * 256 + buf[p + 2];
	  key &= 0x00003fff;
	  res = linear_handle(brl, key, key_context);
	  break;
	case 'I':
	  res = routing(brl, buf[p + 1], key_context);
	  break;
	case 'V':
	  /* it's an identification string */
	  memcpy(IdentString, buf + p + 1, 40);
	  LogPrint(LOG_INFO, "BIOS detected: %s", IdentString);
	  if (buf[p + 1] == 's' || buf[p + 1] == 'S')
	    NbCols = 32;
	  else
	    NbCols = 40;
	  brl->x = NbCols;
	  prevdata = realloc(prevdata, brl->x * brl->y);
	  lcd_data = realloc(lcd_data, brl->x * brl->y * sizeof(*lcd_data));
	  res = BRL_CMD_NOOP;
	  break;
	}
      pktready = 0;
    }
  return (res);
}

static int	linear_handle(BrailleDisplay *brl, unsigned int key, char key_context)
{
  int		i;
  int		res = EOF;
  static int	pressed = 0;
  t_key		*p = NULL;

  if (key_context == CTX_KEYCODES)
    {
      key |= EB_LINEAR;
      return (key);
    }
  switch (context)
    {
    case IN_LEVEL1: p = piris_keys; break;
    case IN_LEVEL4: p = psynth_keys; break;
    default: p = iris_keys; break;
    }
  if (key == VK_NONE)
    {
      if (pressed == 0)
	return (EOF);
      else
	{
	  pressed = 0;
	  return (BRL_CMD_NOOP);
	}
    }  
  for (i = 0; p[i].brl_key; i++)
    {
      if (iris_keys[i].brl_key == key)
	{
	  if (iris_keys[i].f)
	    res = iris_keys[i].f(brl);
	  else
	    res = iris_keys[i].res;
	}
    }
  if (res == EOF)
    return res;
  if (context == IN_LEVEL1 || context == IN_LEVEL2 
      || context == IN_LEVEL3 || context == IN_LEVEL4)
    context = 0;
  else
    {
      res |= (BRL_FLG_REPEAT_INITIAL | BRL_FLG_REPEAT_DELAY);
      pressed = 1;
    }
  return (res);
}
