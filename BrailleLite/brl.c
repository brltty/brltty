/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

/* BrailleLite/brl.c - Braille display library
 * For Blazie Engineering's Braille Lite series
 * Author: Nikhil Nair <nn201@cus.cam.ac.uk>
 * Copyright (C) 1998 by Nikhil Nair.
 * Some additions by: Nicolas Pitre <nico@cam.org>
 * Some modifications copyright 2001 by Stéphane Doyon <s.doyon@videotron.ca>.
 */

#define VERSION \
"Braille Lite driver, version 0.5.3 (September 2001)"

#define BRL_C

#define __EXTENSIONS__		/* for termios.h */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/termios.h>
#include <string.h>

#include "../config.h"
#include "../brl.h"
#include "../misc.h"
#include "../inskey.h"
#include "../message.h"
#include "../scr.h"

#include "brlconf.h"

typedef enum {
  PARM_BAUDRATE=0,
} DriverParameter;
#define BRLPARMS "baudrate"

#include "../brl_driver.h"

#include "bindings.h"		/* for keybindings */


#define QSZ 256			/* size of internal input queue in bytes */
#define INT_CSR_SPEED 2		/* on/off time in cycles */
#define ACK_TIMEOUT 1000	/* timeout in ms for an ACK to come back */

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

int blite_fd = -1;		/* file descriptor for Braille display */

static unsigned char blitetrans[256];	/* dot mapping table (output) */
static unsigned char revtrans[256];	/* mapping for reversed display */
static unsigned char *prevdata;	/* previously received data */
static unsigned char *rawdata;	/* writebrl() buffer for raw Braille data */
static struct termios oldtio;	/* old terminal settings */
static int blitesz;	/* set to 18 or 40 */
static int waiting_ack = 0;	/* waiting acknowledgement flag */
static int reverse_kbd = 0;	/* reverse keyboard flag */
static int intoverride = 0;	/* internal override flag -
				 * highly dubious behaviour ...
				 */
static int int_cursor = 0;	/* position of internal cursor: 0 = none */

/* The input queue is only manipulated by the qput() and qget()
 * functions.
 */
static unsigned char *qbase;	/* start of queue in memory */
static int qoff = 0;		/* offset of first byte */
static int qlen = 0;		/* number of items in the queue */


/* Data type for a Braille Lite key, including translation into command
 * codes:
 */
typedef struct
  {
    unsigned char raw;		/* raw value, after any keyboard reversal */
    int cmd;			/* command code */
    unsigned char asc;		/* ASCII translation of Braille keys */
    unsigned char spcbar;	/* 1 = on, 0 = off */
    unsigned char routing;	/* routing key number */
  }
blkey;


/* Local function prototypes: */
static void getbrlkeys (void);	/* process keystrokes from the Braille Lite */
static int qput (unsigned char);	/* add a byte to the input queue */
static int qget (blkey *);	/* get a byte from the input queue */


static void
identbrl (void)
{
  LogPrint(LOG_NOTICE, VERSION);
  /* I don't mean to take away anyone's copyright, but now that significant
     modifs have been made in more recent years by 2-3 others... it'd become
     bulky IMHO so let's just live without the copyright notice. */
  /*LogPrint(LOG_INFO, "   Copyright (C) 1998 by Nikhil Nair.");*/
}


static void
initbrl (char **parameters, brldim * brl, const char *brldev)
{
  static unsigned good_baudrates[]
    = {300,600,1200,2400,4800,9600,19200,38400, 0};
  speed_t baudrate;
  brldim res;			/* return result */
  struct termios newtio;	/* new terminal settings */
  short i, n;			/* loop counters */
  static unsigned char standard[8] =
  {0, 1, 2, 3, 4, 5, 6, 7};	/* BRLTTY standard mapping */
  static unsigned char Blazie[8] =
  {0, 3, 1, 4, 2, 5, 6, 7};	/* Blazie standard */
  static unsigned char prebrl[2] =
  {"\005D"};			/* code to send before Braille */
  /* Init string for Model detection */
  static unsigned char InitData[18]="";
#ifndef DETECT_FOREVER
  int timeout;			/* while waiting for an ACK */
#endif
  unsigned char BltLen;

  res.disp = prevdata = rawdata = NULL;		/* clear pointers */
  if(!(qbase = (unsigned char *) malloc (QSZ))) {
    LogPrint(LOG_ERR, "Cannot allocate qbase");
    return;
  }

  if(!parameters[PARM_BAUDRATE]
     || !validateBaud(&baudrate, "baud rate",
		     parameters[PARM_BAUDRATE], good_baudrates))
    baudrate = BAUDRATE;

  /* Open the Braille display device for random access */
  LogPrint(LOG_DEBUG, "Opening serial port %s", brldev);
  blite_fd = open (brldev, O_RDWR | O_NOCTTY);
  if (blite_fd < 0)
    {
      LogPrint (LOG_ERR, "open %s: %s", brldev, strerror (errno));
      return;
    }
  if(tcgetattr (blite_fd, &oldtio) <0){	/* save current settings */
    LogPrint(LOG_ERR, "tcgetattr: %s", strerror(errno));
    return;
  }

  /* Set bps, flow control and 8n1, enable reading */
  newtio.c_cflag = baudrate | CRTSCTS | CS8 | CLOCAL | CREAD;
  LogPrint(LOG_DEBUG, "Selecting baudrate %d",
	   baud2integer(baudrate));

  /* Ignore bytes with parity errors and make terminal raw and dumb */
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;		/* raw output */
  newtio.c_lflag = 0;		/* don't echo or generate signals */
  newtio.c_cc[VMIN] = 0;	/* set nonblocking read */
  newtio.c_cc[VTIME] = 0;
  tcflush (blite_fd, TCIFLUSH);	/* clean line */
  if(tcsetattr (blite_fd, TCSANOW, &newtio) <0){    /* activate new settings */
    LogPrint(LOG_ERR, "tcsetattr: %s", strerror(errno));
    goto failure;
  }

#ifndef DETECT_FOREVER
  /* Braille Lite identification */
  write (blite_fd, prebrl, 2);
  /* Now we must wait for an acknowledgement ... */
  waiting_ack = 1;
  timeout = ACK_TIMEOUT / 10;
  do {
    getbrlkeys ();
    delay (10);	/* sleep for 10 ms */
    if (--timeout < 0)
    {
      LogPrint(LOG_WARNING, "BLT doesn't seem to be connected, giving up!");
      goto RestorePort;
    }
  }
  while (waiting_ack);

#else
  /* wait forever method */
 again:
  write (blite_fd, prebrl, 2);
  waiting_ack = 1;
  delay (100);
  getbrlkeys ();
  if(waiting_ack) {
    delay(2000);
    goto again;
  }
#endif

  LogPrint(LOG_DEBUG, "Got response");

  /* Next, let's detect the BLT-Model (18 || 40). */
  BltLen=18;
  write (blite_fd, InitData, BltLen);
  waiting_ack = 1;
  delay (400);
  getbrlkeys();
  if (waiting_ack) // no response, so it must be BLT40
  { // assuming BLT40 now
    BltLen=40;
    sethlpscr(1);
  }else sethlpscr(0);
  
  blitesz = res.x = BltLen;	/* initialise size of display - */
  res.y = 1;			/* Braille Lites are single line displays */
  LogPrint(LOG_NOTICE, "Braille Lite %d detected",BltLen);

  /* Allocate space for buffers */
  res.disp = (unsigned char *) malloc (res.x);
  prevdata = (unsigned char *) malloc (res.x);
  rawdata = (unsigned char *) malloc (res.x);
  if (!res.disp || !prevdata || !rawdata)
    {
      LogPrint (LOG_ERR, "Cannot allocate braille buffers.");
      goto failure;
    }
  memset (prevdata, 0, res.x);

  /* Generate dot mapping tables: */
  memset (blitetrans, 0, 256);	/* ordinary dot mapping */
  memset (revtrans, 0, 256);	/* display reversal - extra mapping */
  for (n = 0; n < 256; n++)
    {
      for (i = 0; i < 8; i++)
	if (n & 1 << standard[i])
	  blitetrans[n] |= 1 << Blazie[i];
      for (i = 0; i < 8; i++)
	{
	  revtrans[n] <<= 1;
	  if (n & (1 << i))
	    revtrans[n] |= 1;
	}
    }
  *brl = res;
  return;

failure:
  free (res.disp);
  free (prevdata);
  free (rawdata);
  free (qbase);
  if (blite_fd >= 0) {
    tcsetattr (blite_fd, TCSANOW, &oldtio);	/* restore terminal settings */
    close (blite_fd);
  }
  brl->x = -1;
}


static void
closebrl (brldim * brl)
{
#if 0 /* SDo: Let's leave the BRLTTY exit message there. */
  /* We just clear the display, using writebrl(): */
  memset (brl->disp, 0, brl->x);
  writebrl (brl);
#endif

  free (brl->disp);
  free (prevdata);
  free (rawdata);
  free (qbase);

  if(blite_fd >= 0) {
    tcsetattr (blite_fd, TCSADRAIN, &oldtio);	/* restore terminal settings */
    close (blite_fd);
  }
}


static void
setbrlstat (const unsigned char *s)
{
}


static void
writebrl (brldim * brl)
{
  short i;			/* loop counter */
  static unsigned char prebrl[2] =
  {"\005D"};			/* code to send before Braille */
  static int timer = 0;		/* for internal cursor */
  int timeout;			/* while waiting for an ACK */

  /* If the intoverride flag is set, then calls to writebrl() from the main
   * module are ignored, because the display is in internal use.
   * This is highly antisocial behaviour!
   */
  if (intoverride)
    return;

  /* First, the internal cursor: */
  if (int_cursor)
    {
      timer = (timer + 1) % (INT_CSR_SPEED * 2);
      if (timer < INT_CSR_SPEED)
	brl->disp[int_cursor - 1] = 0x55;
      else
	brl->disp[int_cursor - 1] = 0xaa;
    }

  /* Next we must handle display reversal: */
  if (reverse_kbd)
    for (i = 0; i < blitesz; i++)
      rawdata[i] = revtrans[brl->disp[blitesz - 1 - i]];
  else
    memcpy (rawdata, brl->disp, blitesz);

  /* Only refresh display if the data has changed: */
  if (memcmp (rawdata, prevdata, blitesz))
    {
      /* Save new Braille data: */
      memcpy (prevdata, rawdata, blitesz);

      /* Dot mapping from standard to BrailleLite: */
      for (i = 0; i < blitesz; i++)
	rawdata[i] = blitetrans[rawdata[i]];

      /* First we process any pending keystrokes, just in case any of them
       * are ^e ...
       */
      waiting_ack = 0;		/* Not really necessary, but ... */
      getbrlkeys ();

      /* Next we send the ^eD sequence, and wait for an ACK */
      waiting_ack = 1;
      /* send the ^ED... */
      write (blite_fd, prebrl, 2);

      /* Now we must wait for an acknowledgement ... */
      timeout = ACK_TIMEOUT / 10;
      do {
	getbrlkeys ();
        delay (10);	/* sleep for 10 ms */
        if (--timeout < 0)
	    return; // BLT doesn't seem to be connected any more!
	}
      while (waiting_ack);

      /* OK, now we'll suppose we're all clear to send Braille data. */
      write (blite_fd, rawdata, blitesz);

      /* And once again we wait for acknowledgement. */
      waiting_ack = 1;
      timeout = ACK_TIMEOUT / 10;
      do {
	  getbrlkeys ();
	  delay (10);		/* sleep for 10 ms */
	  if (--timeout < 0)
	    return; // BLT doesn't seem to be connected any more!
	}
      while (waiting_ack);
    }
}


static int
readbrl (DriverCommandContext cmds)
{
  static int kbemu = 0;		/* keyboard emulation flag */
  static int state = 0;		/* 0 = normal - transparent
				 * 1 = positioning internal cursor
				 * 2 = setting repeat count
				 * 3 = configuration options
				 */
  static int repeat = 0;		/* repeat count for command */
  static int repeatNext = 0; /* flag to indicate  whether 0 we repeat the
				same command or 1 we get the next command
				and repeat that. */
  static int hold, shift, shiftlck, ctrl;
#ifdef USE_TEXTTRANS
  static int dot8shift;
#endif
  static blkey key;
  static unsigned char outmsg[41];
  int temp = CMD_NOOP;

 again:
  if(repeatNext || repeat == 0) {
    /* Process any new keystrokes: */
    getbrlkeys ();
    if (qget (&key) == EOF)	/* no keys to process */
      return EOF;
    repeatNext = 0;
  }
  if(repeat>0)
    repeat--;

  /* Our overall behaviour depends on the state variable (see above). */
  switch (state)
    {
    case 0:			/* transparent */
      /* First we deal with external commands: */
      do
	{
	  /* if it's not an external command, go on */
	  if (!key.cmd)
	    break;

	  /* if advance bar, return with corresponding command */
	  if (key.asc == 0)
	    return key.cmd;

	  /* always OK if chorded */
	  if (key.spcbar)
	    return key.cmd;

	  /* kbemu could be on, then go on */
	  if (kbemu)
	    break;

	  /* if it's a dangerous command it should have been chorded */
	  if (dangcmd[(key.raw & 0x38) >> 3] & (1 << (key.raw & 0x07)))
	    break;

	  /* finally we are OK */
	  return key.cmd;
	}
      while (0);

      /* Next, internal commands: */
      if (key.spcbar)
	switch (key.asc)
	  {
	  case BLT_KBEMU:	/* set keyboard emulation */
	    kbemu ^= 1;
	    shift = shiftlck = ctrl = 0;
#ifdef USE_TEXTTRANS
	    dot8shift = 0;
#endif
	    outmsg[0] = 0;
	    if(kbemu)
	      message ("keyboard emu on", MSG_SILENT);
	    else message ("keyboard emu off", MSG_SILENT);
	    return CMD_NOOP;
	  case BLT_ROTATE:	/* rotate Braille Lite by 180 degrees */
	    reverse_kbd ^= 1;
	    return CMD_NOOP;
	  case BLT_POSITN:	/* position internal cursor */
	    int_cursor = blitesz / 2;
	    state = 1;
	    return CMD_NOOP;
	  case BLT_REPEAT:	/* set repeat count */
	    hold = 0;
	    sprintf (outmsg, "Repeat count:");
	    message (outmsg, MSG_SILENT | MSG_NODELAY);
	    intoverride = 1;
	    state = 2;
	    return CMD_NOOP;
	  case BLT_CONFIG:	/* configuration menu */
	    sprintf (outmsg, "Config? [m/s/r/z]");
	    message (outmsg, MSG_SILENT | MSG_NODELAY);
	    intoverride = 1;
	    state = 3;
	    return CMD_NOOP;
	  case ' ':		/* practical exception for */
	    /* If keyboard mode off, space bar == CMD_HOME */
	    if (!kbemu)
	      return CMD_HOME;
	  }

      /* check for routing keys */
      if (key.routing)
	return (CR_ROUTEOFFSET + key.routing - 1);

      if (!kbemu)
	return CMD_NOOP;

      /* Now kbemu is definitely on. */
      switch (key.raw & 0xC0)
	{
	case 0x40:		/* dot 7 */
	  shift = 1;
	  break;
	case 0xC0:		/* dot 78 */
	  ctrl = 1;
	  break;
	case 0x80:		/* dot 8 */
#ifndef USE_TEXTTRANS
	  outmsg[0] = 27;
	  outmsg[1] = 0;
#else
	  dot8shift = 1;
#endif
	  break;
	}

      if (key.spcbar && key.asc != ' ')
	switch (key.asc)
	  {
	  case BLT_UPCASE:	/* upper case next */
	    if (shift)
	      shiftlck = 1;
	    else
	      shift = 1;
	    return CMD_NOOP;
	  case BLT_UPCOFF:	/* cancel upper case */
	    shift = shiftlck = 0;
	    return CMD_NOOP;
	  case BLT_CTRL:	/* control next */
	    ctrl = 1;
	    return CMD_NOOP;
#ifdef USE_TEXTTRANS
	  case BLT_DOT8SHIFT:	/* control next */
	    dot8shift = 1;
	    return CMD_NOOP;
#endif
	  case BLT_META:	/* meta next */
	    outmsg[0] = 27;
	    outmsg[1] = 0;
	    return CMD_NOOP;
	  case BLT_ESCAPE:
	    outmsg[0] = 27;
	    outmsg[1] = 0;
	    inskey (outmsg);
	    if (!shiftlck)
	      shift = 0;
	    ctrl = 0;
#ifdef USE_TEXTTRANS
	    dot8shift = 0;
#endif
	    outmsg[0] = 0;
	    return CMD_NOOP;
	  case BLT_TAB:
	    outmsg[0] = '\t';
	    outmsg[1] = 0;
	    inskey (outmsg);
	    if (!shiftlck)
	      shift = 0;
	    ctrl = 0;
#ifdef USE_TEXTTRANS
	    dot8shift = 0;
#endif
	    outmsg[0] = 0;
	    return CMD_NOOP;
	  case BLT_BACKSP:	/* remove backwards */
	    outmsg[0] = 127;
	    outmsg[1] = 0;
	    inskey (outmsg);
	    if (!shiftlck)
	      shift = 0;
	    ctrl = 0;
#ifdef USE_TEXTTRANS
	    dot8shift = 0;
#endif
	    outmsg[0] = 0;
	    return CMD_NOOP;
	  case BLT_DELETE:	/* remove forwards */
	    outmsg[0] = 27;
	    outmsg[1] = '[';
	    outmsg[2] = '3';
	    outmsg[3] = '~';
	    outmsg[4] = 0;
	    inskey (outmsg);
	    if (!shiftlck)
	      shift = 0;
	    ctrl = 0;
#ifdef USE_TEXTTRANS
	    dot8shift = 0;
#endif
	    outmsg[0] = 0;
	    return CMD_NOOP;
	  case BLT_ENTER:	/* enter - do ^m, or ^j if control-enter */
	    temp = outmsg[0] == 0 ? 0 : 1;
	    outmsg[temp] = ctrl ? '\n' : '\r';
	    outmsg[temp + 1] = 0;
	    inskey (outmsg);
	    if (!shiftlck)
	      shift = 0;
	    ctrl = 0;
#ifdef USE_TEXTTRANS
	    dot8shift = 0;
#endif
	    outmsg[0] = 0;
	    return CMD_NOOP;
	  case BLT_ABORT:	/* abort - quit keyboard emulation */
	    kbemu = 0;
	    message ("keyboard emu off", MSG_SILENT);
	    return CMD_NOOP;
	  default:		/* unrecognised command */
	    shift = shiftlck = ctrl = 0;
#ifdef USE_TEXTTRANS
	    dot8shift = 0;
#endif
	    outmsg[0] = 0;
	    return CMD_NOOP;
	  }

      /* OK, it's an ordinary (non-chorded) keystroke, and kbemu is on. */
      temp = (outmsg[0] == 0) ? 0 : 1;	/* just checking for a meta character */
#ifndef USE_TEXTTRANS
      if (ctrl && key.asc >= 96)
	outmsg[temp] = key.asc & 0x1f;
      else if (shift && (key.asc & 0x40))
	outmsg[temp] = key.asc & 0xdf;
      else
	outmsg[temp] = key.asc;
#else
      outmsg[temp]
	= untexttrans[
		      (keys_to_dots[key.raw]
		       | ((ctrl) ? 0xC0 : 
			  (shift) ? 0x40 : 
			  (dot8shift) ? 0x80 : 0))];
#endif
      outmsg[temp + 1] = 0;
      inskey (outmsg);
      if (!shiftlck)
	shift = 0;
      ctrl = 0;
#ifdef USE_TEXTTRANS
      dot8shift = 0;
#endif
      outmsg[0] = 0;
      return CMD_NOOP;

    case 1:			/* position internal cursor */
      switch (key.cmd)
	{
	case CMD_HOME:		/* go to middle */
	  int_cursor = blitesz / 2;
	  break;
	case CMD_LNBEG:	/* beginning of display */
	  int_cursor = 1;
	  break;
	case CMD_LNEND:	/* end of display */
	  int_cursor = blitesz;
	  break;
	case CMD_FWINLT:	/* quarter left */
	  int_cursor = MAX (int_cursor - blitesz / 4, 1);
	  break;
	case CMD_FWINRT:	/* quarter right */
	  int_cursor = MIN (int_cursor + blitesz / 4, blitesz);
	  break;
	case CMD_CHRLT:	/* one character left */
	  if (int_cursor > 1)
	    int_cursor--;
	  break;
	case CMD_CHRRT:	/* one character right */
	  if (int_cursor < blitesz)
	    int_cursor++;
	  break;
	case CMD_CSRJMP:	/* route cursor */
	  if (key.spcbar)
	    {
	      temp = CR_ROUTEOFFSET + int_cursor - 1;
	      int_cursor = state = 0;
	    }
	  return temp;
	case CMD_CUT_BEG:	/* begin cut */
	  if (key.spcbar)
	    {
	      temp = CR_BEGBLKOFFSET + int_cursor - 1;
	      int_cursor = state = 0;
	    }
	  return temp;
	case CMD_CUT_END:	/* end cut */
	  if (key.spcbar)
	    {
	      temp = CR_ENDBLKOFFSET + int_cursor - 1;
	      int_cursor = state = 0;
	    }
	  return temp;
	case CMD_DISPMD: /* attribute info */
	  temp = CR_MSGATTRIB + int_cursor - 1;
	  int_cursor = state = 0;
	  return temp;
	default:
	  if (key.asc == BLT_ABORT)	/* cancel cursor positioning */
	    int_cursor = state = 0;
	  break;
	}
      if (key.routing)
	int_cursor = key.routing;
      return CMD_NOOP;
    case 2:			/* set repeat count */
      if (key.asc >= '0' && key.asc <= '9')
	{
	  hold = (hold * 10 + key.asc - '0') % 100;
	  if(hold)
	    sprintf (outmsg, "Repeat count: %d", hold);
	  else sprintf (outmsg, "Repeat count: ");
	  intoverride = 0;
	  message (outmsg, MSG_SILENT | MSG_NODELAY);
	  intoverride = 1;
	}
      else if (key.routing)
	{
	  hold = key.routing +1;
	  sprintf (outmsg, "Repeat count: %d", hold);
	  intoverride = 0;
	  message (outmsg, MSG_SILENT | MSG_NODELAY);
	  intoverride = 1;
	}
      else{
	intoverride = 0;
	outmsg[0] = 0;
	state = 0;
	if(hold > 0) {
	  if(key.asc == SWITCHVT_NEXT || key.asc == SWITCHVT_PREV)
	    /* That's chorded or not... */
	    return CR_SWITCHVT + (hold-1);
	  else if (key.spcbar)		/* chorded */
	    switch(key.asc)
	      {
	      case BLT_ENDCMD:	/* set repeat count */
		if (hold > 1) {
		  /* repeat next command */
		  repeat = hold;
		  repeatNext = 1;
		}
		/* fall through */
	      case BLT_ABORT:	/* abort or endcmd */
		return CMD_NOOP;
	      }
	  /* if the key is any other, start repeating it. */
	  repeat = hold;
	  goto again;
	}
      }
      return CMD_NOOP;
    case 3:			/* preferences options */
      if (!key.spcbar)		/* not chorded */
	return CMD_NOOP;
      switch (key.asc)
	{
	case 'm':		/* preferences menu */
	  intoverride = state = 0;
	  return CMD_PREFMENU;
	case 's':		/* save preferences */
	  intoverride = state = 0;
	  return CMD_PREFSAVE;
	case 'r':		/* restore saved preferences */
	  intoverride = state = 0;
	  return CMD_PREFLOAD;
	case BLT_ABORT:	/* abort */
	  intoverride = state = 0;
	default:		/* in any case */
	  return CMD_NOOP;
	}
    }

  /* We should never reach this point ... */
  return EOF;
}


static void
getbrlkeys (void)
{
  unsigned char c;		/* character buffer */

  while (read (blite_fd, &c, 1))
    {
      if (waiting_ack && c == 5)	/* ^e is the acknowledgement character ... */
	waiting_ack = 0;
      else
	qput (c);
    }
}


static int
qput (unsigned char c)
{
  if (qlen == QSZ)
    return EOF;
  qbase[(qoff + qlen++) % QSZ] = c;
  return 0;
}


static int
qget (blkey * kp)
{
  unsigned char c, c2, c3;
  int ext;

  if (qlen == 0)
    return EOF;
  c = qbase[qoff];

  /* extended sequences start with a zero */
  ext = (c == 0);

  /* extended sequences requires 3 bytes */
  if (ext && qlen < 3)
    return EOF;

  memset (kp, 0, sizeof (*kp));

  if (!ext)
    {
      /* non-extended sequences (BL18) */

      /* We must deal with keyboard reversal here: */
      if (reverse_kbd)
	{
	  if (c >= 0x80)	/* advance bar */
	    c ^= 0x03;
	  else
	    c = (c & 0x40) | ((c & 0x38) >> 3) | ((c & 0x07) << 3);
	}

      /* Now we fill in all the info about the keypress: */
      if (c >= 0x80)		/* advance bar */
	{
	  kp->raw = c;
	  switch (c)
	    {
	    case 0x83:		/* left */
	      kp->cmd = BLT_BARLT;
	      break;
	    case 0x80:		/* right */
	      kp->cmd = BLT_BARRT;
	      break;
	    default:		/* unrecognised keypress */
	      kp->cmd = 0;
	    }
	}
      else
	{
	  kp->spcbar = ((c & 0x40) ? 1 : 0);
	  c &= 0x3f;		/* leave only dot key info */
	  kp->raw = c;
	  kp->cmd = cmdtrans[c];
	  kp->asc = brltrans[c];
	}
    }
  else
    {
      /* extended sequences (BL40) */

      c2 = qbase[((qoff + 1) % QSZ)];
      c3 = qbase[((qoff + 2) % QSZ)];

      /* We must deal with keyboard reversal here: */
      if (reverse_kbd)
	{
	  if (c2 == 0)
	    {			/* advance bars or routing keys */
	      if (c3 & 0x80)	/* advance bars */
		c3 = ((c3 & 0xF0) |
		      ((c3 & 0x1) << 3) | ((c3 & 0x2) << 1) |
		      ((c3 & 0x4) >> 1) | ((c3 & 0x8) >> 3));
	      else if (c3 > 0 && c3 <= blitesz)
		c3 = blitesz - c3 + 1;
	    }
	  else
	    c2 = (((c2 & 0x38) >> 3) | ((c2 & 0x07) << 3) |
		  ((c2 & 0x40) << 1) | ((c2 & 0x80) >> 1));
	  c3 = ((c3 & 0x40) | ((c3 & 0x38) >> 3) | ((c3 & 0x07) << 3));
	}

      /* Now we fill in all the info about the keypress: */
      if (c2 == 0)		/* advance bars or routing keys */
	{
	  kp->raw = c3;
	  if (c3 & 0x80)
	    {			/* advance bars */
	      switch (c3 & 0xF)
		{
		case 0x8:	/* left 1 */
		  kp->cmd = BLT_BARLT1;
		  break;
		case 0x4:	/* right 1 */
		  kp->cmd = BLT_BARRT1;
		  break;
		case 0x2:	/* left 2 */
		  kp->cmd = BLT_BARLT2;
		  break;
		case 0x1:	/* right 2 */
		  kp->cmd = BLT_BARRT2;
		  break;
		default:	/* unrecognised keypress */
		  kp->cmd = 0;
		}
	    }
	  else if (c3 > 0 && c3 <= blitesz)
	    kp->routing = c3;
	}
      else
	{
	  kp->raw = c2;
	  kp->spcbar = ((c3 & 0x40) ? 1 : 0);
	  c3 &= 0x3f;		/* leave only dot key info */
	  kp->cmd = cmdtrans[c3];
	  kp->asc = brltrans[c3];
	}
    }

  /* adjust queue variables for next member */
  qoff = (qoff + (ext ? 3 : 1)) % QSZ;
  qlen -= (ext ? 3 : 1);

  return 0;
}
