/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

/* MultiBraille/braille.c - Braille display library
 * the following Tieman B.V. braille terminals are supported
 * (infos out of a techn. product description sent to me from tieman by fax):
 *
 * - Brailleline 125 (no explicit description)
 * - Brailleline PICO II or MB145CR (45 braille modules + 1 dummy)
 * - Brailleline MB185CR (85 braille modules + 1 dummy)
 *
 * Wolfgang Astleitner, March/April 2000
 * Email: wolfgang.astleitner@liwest.at
 * braille.c,v 1.0
 *
 * Mostly based on CombiBraille/braille.c by Nikhil Nair
 */


/* Description of the escape-sequences used by these lines:
 - [ESC][0]
   signal sent to the braille line so that we get the init message
 - [ESC][V][braille length][firmware version][CR]
   init message sent back by the braille line
   * braille length:   20 / 25 / 40 / 80 (decimal)
   * firmware version: needs to be divided by 10.0: so if we receive
     21 (decimal) --> version 2.1
 - [ESC][F][braillekey data][CR]
   don't know what this is good for. description: init the PC for reading
   the top keys as braille keys (0: mode off, 1: mode on)
 - [ESC][Z][braille data][CR]
   braille data from PC to braille line
   (braille-encoded characters ([20|25|40|80] * 8 bit)
 - [ESC][B][beep data][CR]
   send a beep to the piezo-beeper:
   1: long beep
   0: short beep
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/termios.h>
#include <string.h>

#include "Programs/brl.h"
#include "Programs/misc.h"
#include "brlconf.h"
#include "Programs/brl_driver.h"
#include "tables.h"		/* for keybindings */

#define ESC '\033'
#define CR '\015'

static unsigned char combitrans[256];	/* dot mapping table (output) */
int brl_fd;			/* file descriptor for Braille display */
static int brlcols;		/* length of braille line (auto-detected) */
static unsigned char *prevdata;	/* previously received data */
static unsigned char status[5], oldstatus[5];	/* status cells - always five */
unsigned char *rawdata;		/* writebrl() buffer for raw Braille data */
short rawlen;			/* length of rawdata buffer */
static struct termios oldtio;		/* old terminal settings */

/* message event coming from the braille line to the PC */
typedef struct KeyStroke {
	int block;				/* EOF or blocknumber: */
										/* front keys: 84 (~ [ESC][T][keynumber][CR] )
										 *   (MB185CR also block with '0'-'9', '*', '#')
										 * top keys: 83 (~ [ESC][S][keynumber][CR] )
										 * cursorrouting keys: 82 (~ [ESC][R][keynumber][CR] )
                                                                                 */
	int key;					/* => keynumber */
} KeyStroke;


/* Function prototypes: */
static struct KeyStroke getbrlkey (void);		/* get a keystroke from the MultiBraille */


static void brl_identify (void) {
	LogPrint(LOG_NOTICE, "Tieman B.V. MultiBraille driver");
	LogPrint(LOG_INFO, "   Copyright (C) 2000 by Wolfgang Astleitner."); 
}


static void brl_initialize (char **parameters, brldim *brl, const char *brldev) {
	brldim res;			/* return result */
	struct termios newtio;	/* new terminal settings */
	short i, n, success;		/* loop counters, flags, etc. */
	unsigned char *init_seq = "\002\0330";	/* string to send to Braille to initialise: [ESC][0] */
	unsigned char *init_ack = "\002\033V";	/* string to expect as acknowledgement: [ESC][V]... */
	unsigned char c;
	unsigned char standard[8] =
	{0, 1, 2, 3, 4, 5, 6, 7};	/* BRLTTY standard mapping */
	unsigned char Tieman[8] =
	{0, 7, 1, 6, 2, 5, 3, 4};	/* Tieman standard */

	brlcols = -1;		/* length of braille line (auto-detected) */
	res.disp = prevdata = rawdata = NULL;		/* clear pointers */

	/* No need to load translation tables, as these are now
	 * defined in tables.h
	 */

	/* Now open the Braille display device for random access */
	brl_fd = open (brldev, O_RDWR | O_NOCTTY);
	if (brl_fd < 0)
		goto failure;
	tcgetattr (brl_fd, &oldtio);	/* save current settings */

  /* Set bps, flow control and 8n1, enable reading */
  newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;

	/* Ignore bytes with parity errors and make terminal raw and dumb */
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;		/* raw output */
	newtio.c_lflag = 0;		/* don't echo or generate signals */
	newtio.c_cc[VMIN] = 0;	/* set nonblocking read */
	newtio.c_cc[VTIME] = 0;
	tcflush (brl_fd, TCIFLUSH);	/* clean line */
	tcsetattr (brl_fd, TCSANOW, &newtio);		/* activate new settings */

	/* MultiBraille initialisation procedure:
	 * [ESC][V][Braillelength][Software Version][CR]
	 * I guess, they mean firmware version with software version :*}
	 * firmware version == [Software Version] / 10.0
         */
	success = 0;
	/* Try MAX_ATTEMPTS times, or forever if MAX_ATTEMPTS is 0: */
#if MAX_ATTEMPTS == 0
	while (!success) {
#else
	for (i = 0; i < MAX_ATTEMPTS && !success; i++) {
#endif /* MAX_ATTEMPTS == 0 */
		if (init_seq[0])
			if (write (brl_fd, init_seq + 1, init_seq[0]) != init_seq[0])
				continue;
			timeout_yet (0);		/* initialise timeout testing */
			n = 0;
			do {
				delay (20);
				if (read (brl_fd, &c, 1) == 0)
					continue;
				if (n < init_ack[0] && c != init_ack[1 + n])
					continue;
				if (n == init_ack[0]) {
					brlcols = c, success = 1;
					/* reading version-info */
					/* firmware version == [Software Version] / 10.0 */
					read (brl_fd, &c, 1);
					LogPrint (LOG_INFO, "MultiBraille: Version: %2.1f", c/10.0);
					/* read trailing [CR] */
					read (brl_fd, &c, 1);
				}
				n++;
			}
			while (!timeout_yet (ACK_TIMEOUT) && n <= init_ack[0]);
	}
  if (!success) {
		tcsetattr (brl_fd, TCSANOW, &oldtio);
		goto failure;
	}

	if (brlcols == 25)
		return;						/* MultiBraille Vertical uses a different protocol --> not supported */

	res.x = brlcols;		/* initialise size of display */
	res.y = BRLROWS;
	
	if ((brl->x = res.x) == -1)		/* oops: braille length couldn't be read ... */
		return;

	/* Allocate space for buffers */
	res.disp = (unsigned char *) malloc (res.x * res.y);
	prevdata = (unsigned char *) malloc (res.x * res.y);
	/* rawdata has to have room for the pre- and post-data sequences,
	 * the status cells, and escaped 0x1b's: */
	rawdata = (unsigned char *) malloc (20 + res.x * res.y * 2);
	if (!res.disp || !prevdata || !rawdata)
		goto failure;

	/* Generate dot mapping table: */
	memset (combitrans, 0, 256);
	for (n = 0; n < 256; n++)
		for (i = 0; i < 8; i++)
			if (n & 1 << standard[i])
	combitrans[n] |= 1 << Tieman[i];
	*brl = res;
	return;

failure:;
	if (res.disp)
		free (res.disp);
	if (prevdata)
		free (prevdata);
	if (rawdata)
		free (rawdata);
	if (brl_fd >= 0)
		close (brl_fd);
	brl->x = -1;
	return;
}


static void brl_close (brldim *brl) {
	unsigned char *pre_data = "\002\033Z";	/* string to send to */
	unsigned char *post_data = "\001\015";
	unsigned char *close_seq = "";

	rawlen = 0;
	if (pre_data[0]) {
		memcpy (rawdata + rawlen, pre_data + 1, pre_data[0]);
		rawlen += pre_data[0];
	}
	/* Clear the five status cells and the main display: */
	memset (rawdata + rawlen, 0, 5 + 1+ brl->x * brl->y);
	rawlen += 5 + 1 + brl->x * brl->y;  /* +1 is for dummy module */
	if (post_data[0]) {
		memcpy (rawdata + rawlen, post_data + 1, post_data[0]);
		rawlen += post_data[0];
	}

	/* Send closing sequence: */
	if (close_seq[0]) {
		memcpy (rawdata + rawlen, close_seq + 1, close_seq[0]);
		rawlen += close_seq[0];
	}
	write (brl_fd, rawdata, rawlen);

	free (brl->disp);
	free (prevdata);
	free (rawdata);

#if 0
	tcsetattr (brl_fd, TCSADRAIN, &oldtio);		/* restore terminal settings */
#endif /* 0 */
	close (brl_fd);
}



static void brl_writeStatus (const unsigned char *s) {
	short i;

	/* Dot mapping: */
	for (i = 0; i < 5; status[i] = combitrans[s[i]], i++);
}


static void brl_writeWindow (brldim *brl) {
	short i;			/* loop counter */
	unsigned char *pre_data = "\002\033Z";	/* bytewise accessible copies */
	unsigned char *post_data = "\001\015";

	/* Only refresh display if the data has changed: */
	if (memcmp (brl->disp, prevdata, brl->x * brl->y) || memcmp (status, oldstatus, 5)) {
		/* Save new Braille data: */
		memcpy (prevdata, brl->disp, brl->x * brl->y);

		/* Dot mapping from standard to MultiBraille: */
		for (i = 0; i < brl->x * brl->y; brl->disp[i] = combitrans[brl->disp[i]], i++);

    rawlen = 0;
		if (pre_data[0]) {
			memcpy (rawdata + rawlen, pre_data + 1, pre_data[0]);
			rawlen += pre_data[0];
		}
		
		/* HACK - ALERT ;-)
		 * 6th module is a dummy-modul and not wired!
		 * but I need to but a dummy char at the beginning, else the stati are shifted ...
                 */
		rawdata[rawlen++] = 0;

		/* write stati */
		for (i = 0; i < 5; i++) {
			rawdata[rawlen++] = status[i];
		}
		
		
		/* write braille message itself */
		for (i = 0; i < brl->x * brl->y; i++) {
			rawdata[rawlen++] = brl->disp[i];
		}
      
		if (post_data[0]) {
			memcpy (rawdata + rawlen, post_data + 1, post_data[0]);
			rawlen += post_data[0];
		}
    
		write (brl_fd, rawdata, rawlen);
	}
}


static int brl_read (DriverCommandContext cmds) {
	static short status = 0;	/* cursor routing keys mode */
	
	KeyStroke keystroke;

	keystroke = getbrlkey ();
	if (keystroke.block == EOF)
		return EOF;
		
	if (keystroke.block != 'R') {
		/* translate only 'T' and 'S' events
		 * I kicked argtrans[] (never ever needed) --> simply return 0x00
                 */
		if (keystroke.block == 'T')
			keystroke.key = cmd_T_trans[keystroke.key];
		else
			keystroke.key = cmd_S_trans[keystroke.key];
		status = 0;
                if ((keystroke.key == CR_CUTLINE) || (keystroke.key == CR_CUTRECT))
                  keystroke.key += brlcols - 1;
		return keystroke.key;
	} else { /* directly process 'R' events */
	  /* cursor routing key 1 or 2 pressed: begin / end block to copy'n'paste */
		/* (counting starts with 0 !) */
		if (keystroke.key == 1 || keystroke.key == 2) {
			status = keystroke.key;
			return EOF;
		}
		/* cursor routing keys 3, 4 and 5: invidiually defined functions */
		/* (counting starts with 0 !) */
		if (keystroke.key >= 3 && keystroke.key <= 5) {
			return cmd_R_trans[keystroke.key];
		}
		switch (status) {
			case 0:			/* ordinary cursor routing */
				return keystroke.key + CR_ROUTE - MB_CR_EXTRAKEYS;
			case 1:			/* begin block */
				status = 0;
				return keystroke.key + CR_CUTBEGIN - MB_CR_EXTRAKEYS;
			case 2:			/* end block */
				status = 0;
				return keystroke.key + CR_CUTRECT - MB_CR_EXTRAKEYS;
		}
		status = 0;
	}
	/* should never reach this, just to keep compiler happy ;-) */
	return EOF;
}


/* getbrlkey ()
 * returns a keystroke event
 */

static struct KeyStroke getbrlkey (void) {
	unsigned char c, c_temp;		/* character buffer */
	KeyStroke keystroke;
		
	while (read (brl_fd, &c, 1)) {
		if (c != ESC) continue;	/* advance to next ESC-sequence */

		read (brl_fd, &c, 1);		/* read block number */
		switch (c) {
			case 'T':			/* front key message, (MB185CR only: also '0'-'9', '*', '#') */
			case 'S':			/* top key message [1-2-4--8-16-32] */
			case 'R':			/* cursor routing key [0 - maxkeys] */
				keystroke.block = c;
				read (brl_fd, &c, 1);		/* read keynumber */
				keystroke.key = c;
				read (brl_fd, &c, 1);		/* read trailing [CR] */
  				/* LogPrint(LOG_NOTICE, "MultiBraille.o: Receiving: Key=%d, Block=%c", keystroke.key, keystroke.block); */
				return keystroke;
			default:			/* not supported command --> ignore */
				c_temp = c;
				keystroke.block = EOF;	/* invalid / not supported keystroke */
				read (brl_fd, &c, 1);		/* read keynumber */
  				/* keystroke.key = c; */
  				/* LogPrint(LOG_NOTICE, "MultiBraille.o: Ignored: Key=%d, Block=%c", keystroke.key, c_temp); */
				return keystroke;
		}
	}
	keystroke.block = EOF;
	return keystroke;
}
