/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Version 1.9.0, 06 April 1998
 *
 * Copyright (C) 1995-1998 by The BRLTTY Team, All rights reserved.
 *
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 * Nicolas Pitre <nico@cam.org>
 * Stephane Doyon <s.doyon@videotron.ca>
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
 */

/* TSI/brl.c - Braille display driver for TSI displays
 *
 * Written by Stephane Doyon (s.doyon@videotron.ca)
 *
 * This is version 1.1 (September 25ft 1996) of the TSI driver.
 * It attempts full support for Navigator 20/40/80 and Powerbraille 40.
 * It is designed to be compiled into version 1.0.2 of BRLTTY.
 */

#define BRL_C 1

#define __EXTENSIONS__	/* for termios.h */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/termios.h>
#include <string.h>

#include "brlconf.h"
#include "../brl.h"
#include "../misc.h"
#include "../scr.h"


/* Braille display parameters that will never be changed by the user */
#define BRLROWS 1		/* only one row on braille display */
#define BAUDRATE B9600		/* Navigator's speed */

/* Definitions to avoid typematic repetitions of function keys other
   than movement keys */
#define NONREPEAT_TIMEOUT 500
static int lastcmd = 0;
static struct timeval lastcmd_time;
static struct timezone dum_tz;
/* Those functions it is OK to repeat */
static int repeat_list[] =
{CMD_FWINRT, CMD_FWINLT, CMD_LNUP, CMD_LNDN, CMD_WINUP,
 CMD_WINDN, CMD_CHRLT, CMD_CHRRT, CMD_KEY_LEFT,
 CMD_KEY_RIGHT, CMD_KEY_UP, CMD_KEY_DOWN,
 CMD_CSRTRK, 0};

/* Low battery warning */
#ifdef LOW_BATTERY_WARN
/* We must code the dots forming the message, not its text since handling
   of translation tables has been moved to the main module of brltty. */
static unsigned char battery_msg_20[] =		/* "-><- Low batteries" */
{0x30, 0x1a, 0x25, 0x30, 0x00, 0x55, 0x19, 0x2e, 0x00, 0x05,
 0x01, 0x1e, 0x1e, 0x09, 0x1d, 0x06, 0x09, 0x16, 0x00, 0x00}, battery_msg_40[] =	/* "-><-- Navigator:  Batteries are low" */
{0x30, 0x1a, 0x25, 0x30, 0x30, 0x00, 0x5b, 0x01, 0x35, 0x06,
 0x0f, 0x01, 0x1e, 0x19, 0x1d, 0x29, 0x00, 0x00, 0x45, 0x01,
 0x1e, 0x1e, 0x09, 0x1d, 0x06, 0x09, 0x16, 0x00, 0x01, 0x1d,
 0x09, 0x00, 0x15, 0x19, 0x2e, 0x00, 0x00, 0x00, 0x00, 0x00};
/* input codes signaling low power */
#define BATTERY_H1 (0x00)
#define BATTERY_H2 (0x01)
#endif

/* This defines the mapping between brltty and Navigator's dots coding. */
static char DotsTable[256] =
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
  0xF4, 0xF5, 0xFC, 0xFD, 0xF6, 0xF7, 0xFE, 0xFF};

#define RESET_DELAY (1000)	/* msec */

/* Communication codes */
static char BRL_QUERY[] =
{0xFF, 0xFF, 0x0A};
#define DIM_BRL_QUERY 3
static char BRL_TYPEMATIC[] =
{0xFF, 0xFF, 0x0D};
#define DIM_BRL_TYPEMATIC 3
/* Normal header with cursor always off */
static char BRL_SEND_HEAD[] =
{0xFF, 0xFF, 0x04, 0x00, 0x00, 0x01};
#define DIM_BRL_SEND_FIXED 6
#define DIM_BRL_SEND 8
/* To extra bytes for lenght and offset */
#define BRL_SEND_LENGTH 6
#define BRL_SEND_OFFSET 7

/* Description of reply to query */
#define Q_REPLY_LENGTH 12
static char Q_HEADER[] =
{0x0, 0x05};
#define Q_HEADER_LENGTH 2
#define Q_OFFSET_COLS 2
			    /*#define Q_OFFSET_DOTS 3 *//* not used */
#define Q_OFFSET_VER 4		/* not used */
#define Q_VER_LENGTH 4		/* not used */
			       /*#define Q_OFFSET_CHKSUM 8 *//* not used */
			       /*#define Q_CHKSUM_LENGTH 4 *//* not used */

/* Bit definition of key codes returned by the display */
#define KEY_LEFT 1
#define KEY_UP	2
#define KEY_RIGHT 4
#define KEY_DOWN 8
#define KEY_EXEC 16
#define KEY_BASELEFT 224
#define KEY_BASERIGHT 96
#define KEY_MASK 224		/* to keep the key base */

/* Definitions for sensor switches/cursor routing keys */
#define KEY_SW_H1 0
#define KEY_SW_H2 8
#define SW_NVERT 4
#define SW_MAXHORIZ 10		/* bytes of horizontal info */
#define SW_CNT40 9
#define SW_CNT80 14

/* Global variables */

static int brl_fd;		/* file descriptor for comm port */
static struct termios oldtio,	/* old terminal settings for com port */
                      curtio;   /* current settings */
static unsigned char *rawdata,	/* translated data to send to display */
 *prevdata;			/* previous data sent */
static int brl_cols;		/* Number of cells on display */
static char disp_ver[Q_VER_LENGTH],	/* version of the hardware */
#ifdef LOW_BATTERY_WARN
 *battery_msg,			/* Same thing for low battery warning msg */
  redisplay = 0,		/* a flag, causes complete refresh on next call
				   to display() */
#endif
  has_sw;			/* flag: has routing keys or not */

static void 
ResetTypematic (void)
/* Sends a command to the display to set the typematic parameters */
{
  char params[2] =
  {BRL_TYPEMATIC_DELAY, BRL_TYPEMATIC_REPEAT};
  write (brl_fd, BRL_TYPEMATIC, DIM_BRL_TYPEMATIC);
  write (brl_fd, &params, 2);
}


void 
identbrl (const char *tty)
{
  printf ("  BRLTTY driver for TSI displays, version 1.1\n");
  printf ("  Copyright (C) 1996 by Stephane Doyon <s.doyon@videotron.ca>\n");
  if (tty)
    printf ("  Using serial port %s\n", tty);
  else
    printf ("  Defaulting to serial port %s\n", BRLDEV);
}

brldim 
initbrl (const char *tty)
{
  brldim res;			/* return result */
  int i;
  char reply[Q_REPLY_LENGTH];

  res.disp = rawdata = prevdata = NULL;

  /* Open the Braille display device for random access */
  if (tty != NULL)
    brl_fd = open (tty, O_RDWR | O_NOCTTY);
  else
    brl_fd = open (BRLDEV, O_RDWR | O_NOCTTY);
  if (brl_fd < 0)
    goto failure;
  tcgetattr (brl_fd, &oldtio);	/* save current settings */

  /* Elaborate new settings by working from current state */
  tcgetattr (brl_fd, &curtio);
  /* Set bps */
  if(cfsetispeed(&curtio,BAUDRATE) == -1) goto failure;
  if(cfsetospeed(&curtio,BAUDRATE) == -1) goto failure;
  cfmakeraw(&curtio);
  /* should we use cfmakeraw ? */
  /* local */
  curtio.c_lflag &= ~TOSTOP; /* no SIGTTOU to backgrounded processes */
  /* control */
  curtio.c_cflag |= CLOCAL /* ignore status lines (carrier detect...) */
                  | CREAD /* enable reading */
                  | CRTSCTS; /* hardware flow control */
  curtio.c_cflag &= ~( CSTOPB /* 1 stop bit */
                      | PARENB /* disable parity generation and detection */
                     );
  /* input */
  curtio.c_iflag &= ~( INPCK /* no input parity check */
                      | ~IXOFF
		     );

  /* noncanonical: for first operation */
  curtio.c_cc[VTIME] = 1;  /* 0.1sec timeout between chars */
  curtio.c_cc[VMIN] = Q_REPLY_LENGTH; /* fill buffer before read returns */
  tcflush (brl_fd, TCIOFLUSH);	/* clean line */
  tcsetattr (brl_fd, TCSANOW, &curtio);		/* activate new settings */

  /* Query display */
  do
    {
      if (write (brl_fd, BRL_QUERY, DIM_BRL_QUERY) != DIM_BRL_QUERY)
	{
	  delay (RESET_DELAY);
	  continue;
	}
      if (read (brl_fd, reply, Q_REPLY_LENGTH) != Q_REPLY_LENGTH)
	{
	  tcflush(brl_fd,TCIOFLUSH);
	  continue;
	}
    }
  while (memcmp (reply, Q_HEADER, Q_HEADER_LENGTH));

  memcpy (disp_ver, &reply[Q_OFFSET_VER], Q_VER_LENGTH);

  if (disp_ver[1] > '3')
    sethlpscr (2);		/* pb (?40) */
  else if (reply[Q_OFFSET_COLS] == 80)
    sethlpscr (1);		/* nav80 */
  else
    sethlpscr (0);		/* nav20/40 */

#ifdef BRLCOLS
  brl_cols = BRLCOLS;
#else
  brl_cols = reply[Q_OFFSET_COLS];
#endif

#ifdef HAS_SW
  has_sw = HAS_SW;
#else
  has_sw = (disp_ver[1] > '3' || brl_cols == 80);
#endif

  if (brl_cols == 20)
    {
#ifdef LOW_BATTERY_WARN
      battery_msg = battery_msg_20;
#endif
    }
  else if (brl_cols == 40)
    {
#ifdef LOW_BATTERY_WARN
      battery_msg = battery_msg_40;
#endif
    }
  else if (brl_cols == 80)
    {
#ifdef LOW_BATTERY_WARN
      battery_msg = battery_msg_40;
#endif
    }
  else
    goto failure;		/* unsupported display length (pb 65/81) */

  res.x = brl_cols;		/* initialise size of display */
  res.y = BRLROWS;		/* always 1 */

  curtio.c_cc[VTIME] = 0;
  curtio.c_cc[VMIN] = 0;
  tcsetattr (brl_fd, TCSANOW, &curtio);         /* activate new settings */

  /* Mark time of last command to initialize typematic watch */
  gettimeofday (&lastcmd_time, &dum_tz);

  ResetTypematic ();

  /* Allocate space for buffers */
  res.disp = (unsigned char *) malloc (brl_cols);
  prevdata = (unsigned char *) malloc (brl_cols);
  rawdata = (unsigned char *) malloc (2 * brl_cols + DIM_BRL_SEND);
  /* 2* to insert 0s for attribute code when sending to the display */
  if (!res.disp || !prevdata || !rawdata)
    goto failure;

  /* Initialize rawdata. It will be filled in and used directly to
     write to the display in writebrl(). */
  for (i = 0; i < DIM_BRL_SEND_FIXED; i++)
    rawdata[i] = BRL_SEND_HEAD[i];
  memset (rawdata + DIM_BRL_SEND, 0, 2 * brl_cols * BRLROWS);

  /* Force rewrite of display on first writebrl */
  for (i = 0; i < brl_cols; i++)
    prevdata[i] = 0xFF;

  return res;

failure:;
  if (brl_fd >= 0)
    {
      tcsetattr (brl_fd, TCSANOW, &oldtio);
      close (brl_fd);
    }
  if (res.disp)
    free (res.disp);
  if (rawdata)
    free (rawdata);
  if (prevdata)
    free (prevdata);
  res.x = -1;
  return res;
}


void 
closebrl (brldim brl)
{
  free (brl.disp);
  free (rawdata);
  free (prevdata);
  tcsetattr (brl_fd, TCSANOW, &oldtio);		/* restore terminal settings */
  close (brl_fd);
}

/* no status cells */
void setbrlstat (const unsigned char *s) {}


static void 
display (unsigned char *pattern, unsigned start, unsigned stop)
/* display a given dot pattern. The pattern contains brl_cols bytes, but we
   only want to update from byte (or cell) start, to byte (cell) stop. */
{
  int i, length;

#ifdef LOW_BATTERY_WARN
  /* A hack for battery warning message go away... */
  if (redisplay)
    {
      start = 0;
      stop = brl_cols - 1;
      redisplay = 0;
    }
#endif

  /* Assumes BRLROWS == 1 */
  length = stop - start + 1;

  rawdata[BRL_SEND_LENGTH] = 2 * length;
  rawdata[BRL_SEND_OFFSET] = start;

  for (i = 0; i < length; i++)
    rawdata[DIM_BRL_SEND + 2 * i + 1] = DotsTable[pattern[start + i]];

  write (brl_fd, rawdata, DIM_BRL_SEND + 2 * length);
}

static void 
display_all (unsigned char *pattern)
{
  display (pattern, 0, brl_cols - 1);
}


void 
writebrl (brldim brl)
{
  int base = 0, i = 0, collecting = 0, simil = 0;

  if (brl.x != brl_cols || brl.y != BRLROWS)
    return;

  while (i < brl_cols)
    if (brl.disp[i] == prevdata[i])
      {
	simil++;
	if (collecting && 2 * simil > DIM_BRL_SEND)
	  {
	    display (brl.disp, base, i - simil);
	    base = i;
	    collecting = 0;
	    simil = 0;
	  }
	if (!collecting)
	  base++;
	i++;
      }
    else
      {
	prevdata[i] = brl.disp[i];
	collecting = 1;
	simil = 0;
	i++;
      }

  if (collecting)
    display (brl.disp, base, i - simil - 1);
}


static int 
is_repeat_cmd (int cmd)
{
  int *c = repeat_list;
  while (*c != 0)
    if (*(c++) == cmd)
      return (1);
  return (0);
}


static void 
flicker ()
{
  unsigned char *buf;

  /* Assumes BRLROWS == 1 */
  buf = (unsigned char *) malloc (brl_cols);
  if (buf)
    {
      memset (buf, FLICKER_CHAR, brl_cols);

      display_all (buf);
      shortdelay (FLICKER_DELAY);
      display_all (prevdata);
      shortdelay (FLICKER_DELAY);
      display_all (buf);
      shortdelay (FLICKER_DELAY);
      /* Note that we don't put prevdata back on the display, since flicker()
         normally preceeds the displaying of a special message. */

      free (buf);
    }
}


#ifdef LOW_BATTERY_WARN
static void 
do_battery_warn ()
{
  flicker ();
  display_all (battery_msg);
  delay (BATTERY_DELAY);
  redisplay = 1;
}
#endif


int readbrl (int);


/* OK this one is pretty strange and ugly. readbrl() reads keys from
   the display's key pads. It calls cut_cursor() if it gets a certain key
   press. cut_cursor() is an elaborate function that allows selection of
   portions of text for cut&paste (presenting a moving cursor to replace
   the cursor routing keys that the Navigator/20/40 does not have). The strange
   part is that cut_cursor() itself calls back to readbrl() to get keys
   from the user. It receives and processes keys by calling readbrl again
   and again and eventually returns a single code to the original readbrl()
   function that had called cut_cursor() in the first place. */

/* If cut_cursor() returns the following special code to readbrl(), then
   the cut_cursor() operation has been cancelled. */
#define CMD_CUT_CURSOR 0xF0F0

static int 
cut_cursor ()
{
  static int running = 0, pos = -1;
  int res = 0, key;
  unsigned char oldchar;

  if (running)
    return (CMD_CUT_CURSOR);
  /* Special return code. We are sending this to ourself through readbrl.
     That is: cut_cursor() was accepting input when it's activation keys
     were pressed a second time. The second readbrl() therefore activates
     a second cut_cursor(). It return CMD_CUT_CURSOR through readbrl() to
     the first cut_cursor() which then cancels the whole operation. */
  running = 1;

  if (pos == -1)
    {				/* initial cursor position */
      if (CUT_CSR_INIT_POS == 0)
	pos = 0;
      else if (CUT_CSR_INIT_POS == 1)
	pos = brl_cols / 2;
      else if (CUT_CSR_INIT_POS == 2)
	pos = brl_cols - 1;
    }

  flicker ();

  while (res == 0)
    {
      /* the if must not go after the switch */
      if (pos < 0)
	pos = 0;
      else if (pos >= brl_cols)
	pos = brl_cols - 1;
      oldchar = prevdata[pos];
      prevdata[pos] |= CUT_CSR_CHAR;
      display_all (prevdata);
      prevdata[pos] = oldchar;
      while ((key = readbrl (TBL_CMD)) == EOF);
      switch (key)
	{
	case CMD_FWINRT:
	  pos++;
	  break;
	case CMD_FWINLT:
	  pos--;
	  break;
	case CMD_LNUP:
	  pos += 5;
	  break;
	case CMD_LNDN:
	  pos -= 5;
	  break;
	case CMD_KEY_RIGHT:
	  pos = brl_cols - 1;
	  break;
	case CMD_KEY_LEFT:
	  pos = 0;
	  break;
	case CMD_KEY_UP:
	  pos += 10;
	  break;
	case CMD_KEY_DOWN:
	  pos -= 10;
	  break;
	case CMD_CUT_BEG:
	  res = CR_BEGBLKOFFSET + pos;
	  break;
	case CMD_CUT_END:
	  res = CR_ENDBLKOFFSET + pos;
	  pos = -1;
	  break;
	case CMD_CUT_CURSOR:
	  res = EOF;
	  break;
	  /* That's where we catch the special return code: user has typed
	     cut_cursor() activation key again, so we cancel it. */
	}
    }

  display_all (prevdata);
  running = 0;
  return (res);
}


/* These macros, although unusual, make the key binding declarations look
   much better */
/* For front panel */
#define KEY(code,result) \
    case code: res = result; break;
#define KEYS(codeleft,coderight,result) \
    else if ( l == (codeleft) && r == (coderight) ) res = result;
#define KEYSPECIAL(codeleft,coderight,action) \
    else if ( l == (codeleft) && r == (coderight) ) { action }

/* For cursor routing */
#define SW_CHK(swnum) \
      ( sw_stat[swnum/8] & (1 << (swnum % 8)) )
#define SW_MAXKEY (brl_cols-1)


int 
readbrl (int type)
{
  static unsigned char sw_oldstat[SW_MAXHORIZ];
  unsigned char l, r, t;
  unsigned char sw_stat[SW_MAXHORIZ], sw_which[SW_MAXHORIZ * 8], sw_howmany = 0;
  int res = EOF;

  /* We have no need for command arguments... */
  if (type != TBL_CMD)
    return (EOF);

/* Key press codes for the front panel come in pairs of bytes. We must get
   both before we can proceed in case it's a combination. We get the right
   side first. Each byte has 1bit for each key + a special mask identifying
   the panel.

   The low battery warning from the display is a specific 2bytes code.

   Finally, the routing keys have a special 2bytes header followed by 9 or 14
   bytes of info (1bit for each routing key). The first 4bytes describe
   vertical routing keys and are ignored in this driver.
 */

  /* reset to nonblocking */
  curtio.c_cc[VTIME] = 0;
  curtio.c_cc[VMIN] = 0;
  tcsetattr (brl_fd, TCSANOW, &curtio);

  /* Get the first byte, or right panel key */
  if (!read (brl_fd, &r, 1))
    return (EOF);
  while ((r & KEY_MASK) != KEY_BASERIGHT
#ifdef LOW_BATTERY_WARN
	 && r != BATTERY_H1
#endif
	 && (!has_sw || r != KEY_SW_H1)
    )
    /* read through and discard garbage
       until we get to a valid first byte */
    if (!read (brl_fd, &r, 1))
      return (EOF);
  /* Now get left key or second byte of header. */
  curtio.c_cc[VTIME] = 1;
  curtio.c_cc[VMIN] = 1;
  tcsetattr (brl_fd, TCSANOW, &curtio);
  if (!read (brl_fd, &l, 1))
    return (EOF);
  /* If it didn't work, it's a timeout: that was not a pair */
#ifdef LOW_BATTERY_WARN
  if (r == BATTERY_H1 && l == BATTERY_H2)
    {
      do_battery_warn ();
      return (EOF);
    }
  else
#endif
  if (has_sw && r == KEY_SW_H1 && l == KEY_SW_H2)
    {				/* read the rest of the sequence */
      unsigned i, cnt;
      r = l = 0;

      /* read next byte: it indicates length of sequence */
      /* still VMIN = VTIME = 1 */
      if (!read (brl_fd, &t, 1))
	return (EOF);

      /* how long is the sequence supposed to be... */
      if (brl_cols == 40)
	cnt = SW_CNT40;
      else if (brl_cols == 80)
	cnt = SW_CNT80;
      else
	return (EOF);
      /* if cnt and brl_cols disagree, then must be garbage??? */
      if (t != cnt)
	return (EOF);

      /* skip the vertical keys info */
      curtio.c_cc[VTIME] = 1;
      curtio.c_cc[VMIN] = SW_NVERT;
      tcsetattr (brl_fd, TCSANOW, &curtio);
      if (read (brl_fd, &t, SW_NVERT) != SW_NVERT)
	return (EOF);
      cnt -= SW_NVERT;
      /* cnt now gives the number of bytes describing horizontal
         routing keys only */

      /* Finally, the horizontal keys */
      curtio.c_cc[VTIME] = 1;
      curtio.c_cc[VMIN] = cnt;
      tcsetattr (brl_fd, TCSANOW, &curtio);
      if (read (brl_fd, &sw_stat, cnt) != cnt)
	return (EOF);

      /* if key press is maintained, then packet is resent by display
         every 0.5secs. process only if it has changed form alst time.
         When the key is released, then display sends a packet with all
         info bits at 0, which will clear sw_oldstat for next press... */
      if (!memcmp (sw_stat, sw_oldstat, cnt))
	return (EOF);

      memcpy (sw_oldstat, sw_stat, cnt);

      for (sw_howmany = 0, i = 0; i < brl_cols; i++)
	if (SW_CHK (i))
	  sw_which[sw_howmany++] = i;
      /* SW_CHK(i) tells if routing key i is pressed.
         sw_which[0] to sw_which[howmany-1] give the numbers of the keys
         that are pressed. */
    }
  else if ((l & KEY_MASK) != KEY_BASELEFT)	/* It was not a real
						   keypress pair */
    return (EOF);

  /* if the key was a front panel key or combination, then remove the
     masks from l and r so we can extract the key codes properly */
  if (l)
    {
      l -= KEY_BASELEFT;
      r -= KEY_BASERIGHT;
    }

  /* Now associate a command to the key */

  if (has_sw && !l && !r)	/* routing key */
    {
      if (sw_howmany == 1)
	res = CR_ROUTEOFFSET + sw_which[0];
      else if (sw_howmany == 3 && sw_which[1] == SW_MAXKEY - 1
	       && sw_which[2] == SW_MAXKEY)
	res = CR_BEGBLKOFFSET + sw_which[0];
      else if (sw_howmany == 3 && sw_which[0] == 0 && sw_which[1] == 1)
	res = CR_ENDBLKOFFSET + sw_which[2];
      else if (sw_howmany == 2 && sw_which[0] == 0 && sw_which[1] == 1)
	res = CMD_CHRLT;
      else if (sw_howmany == 2 && sw_which[0] == SW_MAXKEY - 1
	       && sw_which[1] == SW_MAXKEY)
	res = CMD_CHRRT;
      else if (sw_howmany == 2 && sw_which[0] == 0 && sw_which[1] == 2)
	res = CMD_HWINLT;
      else if (sw_howmany == 2 && sw_which[0] == SW_MAXKEY - 2
	       && sw_which[1] == SW_MAXKEY)
	res = CMD_HWINRT;
      else if (sw_howmany == 2 && sw_which[0] == 0 && sw_which[1] == SW_MAXKEY)
	res = CMD_HELP;
      else if (sw_howmany == 2 && sw_which[0] == 1
	       && sw_which[1] == SW_MAXKEY - 1)
	{
	  ResetTypematic ();
	  display_all (prevdata);
	  /* Special: No res code... */
	}
    }
  else if (!l)			/* On the right panel only */
    switch (r)
      {
	KEY (KEY_UP, CMD_LNUP)
	  KEY (KEY_DOWN, CMD_LNDN)
	  KEY (KEY_LEFT, CMD_FWINLT)
	  KEY (KEY_RIGHT, CMD_FWINRT)
	  KEY (KEY_EXEC, CMD_HOME)
	  KEY (KEY_EXEC | KEY_UP, CMD_TOP)
	  KEY (KEY_LEFT | KEY_UP, CMD_TOP_LEFT)
	  KEY (KEY_EXEC | KEY_DOWN, CMD_BOT)
	  KEY (KEY_LEFT | KEY_DOWN, CMD_BOT_LEFT)
	  KEY (KEY_EXEC | KEY_LEFT, CMD_LNBEG)
	  KEY (KEY_EXEC | KEY_RIGHT, CMD_LNEND)
	  KEY (KEY_UP | KEY_DOWN, CMD_INFO)
	  KEY (KEY_LEFT | KEY_RIGHT, CMD_CONFMENU)
	  KEY (KEY_LEFT | KEY_RIGHT | KEY_DOWN, CMD_SKPIDLNS)
	  KEY (KEY_LEFT | KEY_RIGHT | KEY_EXEC, CMD_SAVECONF)
      }
  else if (!r)			/* On the left panel only */
    switch (l)
      {
	KEY (KEY_LEFT, CMD_KEY_LEFT)
	  KEY (KEY_RIGHT, CMD_KEY_RIGHT)
	  KEY (KEY_UP, CMD_KEY_UP)
	  KEY (KEY_DOWN, CMD_KEY_DOWN)
	  KEY (KEY_LEFT | KEY_EXEC, CMD_CHRLT)
	  KEY (KEY_RIGHT | KEY_EXEC, CMD_CHRRT)
	  KEY (KEY_LEFT | KEY_UP, CMD_HWINLT)
	  KEY (KEY_RIGHT | KEY_UP, CMD_HWINRT)
	  KEY (KEY_EXEC | KEY_UP, CMD_WINUP)
	  KEY (KEY_EXEC | KEY_DOWN, CMD_WINDN)
	  KEY (KEY_EXEC, CMD_CSRTRK)
	  KEY (KEY_UP | KEY_DOWN, CMD_DISPMD)
	  KEY (KEY_LEFT | KEY_RIGHT, CMD_HELP)
      }
  /* Combinations from both panels */
    KEYS (KEY_DOWN, KEY_UP, CMD_INFO)
    KEYS (KEY_UP, KEY_DOWN, CMD_DISPMD)
    KEYS (KEY_EXEC, KEY_EXEC, CMD_FREEZE)
    KEYS (KEY_DOWN, KEY_DOWN, CMD_CSRJMP)
    KEYS (KEY_UP, KEY_UP, CR_ROUTEOFFSET + 3 * brl_cols / 4 - 1)
    KEYS (KEY_LEFT, KEY_EXEC, CMD_CUT_BEG)
    KEYS (KEY_RIGHT, KEY_EXEC, CMD_CUT_END)
    KEYS (KEY_LEFT | KEY_RIGHT, KEY_EXEC, CMD_CUT_CURSOR)  /* special: see
						    at the end of this fn */
    KEYS (KEY_DOWN, KEY_EXEC, CMD_PASTE)
    KEYS (KEY_EXEC | KEY_LEFT, KEY_LEFT | KEY_RIGHT, CMD_CSRSIZE)
    KEYS (KEY_EXEC | KEY_UP, KEY_LEFT | KEY_RIGHT, CMD_CSRVIS)
    KEYS (KEY_EXEC | KEY_DOWN, KEY_LEFT | KEY_RIGHT, CMD_CSRBLINK)
    KEYS (KEY_UP, KEY_LEFT | KEY_RIGHT, CMD_CAPBLINK)
    KEYS (KEY_DOWN, KEY_LEFT | KEY_RIGHT, CMD_SLIDEWIN)
    KEYS (KEY_RIGHT, KEY_LEFT | KEY_RIGHT, CMD_SND)
    KEYS (KEY_EXEC, KEY_LEFT | KEY_RIGHT | KEY_EXEC, CMD_RESET)
    KEYSPECIAL (KEY_LEFT, KEY_RIGHT, ResetTypematic ();
		display_all (prevdata);
    )

  /* If this is a typematic repetition of some key other than movement keys */
    if (lastcmd == res && !is_repeat_cmd (res))
    {
      /* if to short a time has elapsed since last command, ignore this one */
      struct timeval now;
      gettimeofday (&now, &dum_tz);
      if (elapsed_msec (&lastcmd_time, &now) < NONREPEAT_TIMEOUT)
	res = EOF;
    }

  /* reset timer to avoid unwanted typematic */
  if (res != EOF)
    {
      lastcmd = res;
      gettimeofday (&lastcmd_time, &dum_tz);
    }

  /* Special: */
  if (res == CMD_CUT_CURSOR)
    res = cut_cursor ();
  /* This activates cut_cursor(). Done here rather than with KEYSPECIAL macro
     to allow clearing of l and r and adjustment of typematic variables.
     Since cut_cursor() calls readbrl again to get key inputs, it is
     possible that cut_cursor() is running (it called us) and we are calling
     it a second time. cut_cursor() will handle this as a signal to
     cancel it's operation. */

  return (res);
}
