/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Version 1.0, 26 July 1996
 *
 * Copyright (C) 1995, 1996 by Nikhil Nair and others.  All rights reserved.
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nikhil Nair <nn201@cus.cam.ac.uk>.
 */

/* CombiBraille/brl.c - Braille display library
 * For Tieman B.V.'s CombiBraille (no speech support, serial only)
 * N. Nair, 25 January 1996
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#include "../brl.h"
#include "../misc.h"
#include "brlconf.h"

unsigned char combitrans[256];	/* dot mapping table (output) */
unsigned char cmdtrans[256];	/* command translation table (input) */
unsigned char argtrans[256];	/* argument translation table (input) */
int brl_fd;			/* file descriptor for Braille display */
unsigned char *prevdata;	/* previously received data */
unsigned char status[5], oldstatus[5];	/* status cells - always five */
unsigned char *rawdata;		/* writebrl() buffer for raw Braille data */
short rawlen;			/* length of rawdata buffer */
struct termios oldtio;		/* old terminal settings */

/* Function prototypes: */
int getbrlkey (void);		/* get a keystroke from the CombiBraille */


void
identbrl (const char *brldev)
{
  printf ("\nTieman B.V. CombiBraille driver   ");
  printf ("Copyright (C) 1995, 1996 by Nikhil Nair.\n");
  if (brldev)
    printf ("Using device %s\n\n", brldev);
  else
    printf ("Defaulting to device %s\n\n", BRLDEV);
}


brldim
initbrl (const char *brldev)
{
  brldim res;			/* return result */
  struct termios newtio;	/* new terminal settings */
  short i, n, success;		/* loop counters, flags, etc. */
  unsigned char *init_seq = INIT_SEQ;	/* bytewise accessible copies */
  unsigned char *init_ack = INIT_ACK;
  unsigned char c;
  char id = -1;
  unsigned char standard[8] =
  {0, 1, 2, 3, 4, 5, 6, 7};	/* BRLTTY standard mapping */
  unsigned char Tieman[8] =
  {0, 7, 1, 6, 2, 5, 3, 4};	/* Tieman standard */

  res.disp = prevdata = rawdata = NULL;		/* clear pointers */

  /* Load in translation tables */
  brl_fd = open (CMDTRN_NAME, O_RDONLY);
  if (brl_fd == -1)
    goto failure;
  if (read (brl_fd, cmdtrans, 256) != 256)
    goto failure;
  close (brl_fd);
  brl_fd = open (ARGTRN_NAME, O_RDONLY);
  if (brl_fd == -1)
    goto failure;
  if (read (brl_fd, argtrans, 256) != 256)
    goto failure;
  close (brl_fd);

  /* Now open the Braille display device for random access */
  if (brldev != NULL)
    brl_fd = open (brldev, O_RDWR | O_NOCTTY);
  else
    brl_fd = open (BRLDEV, O_RDWR | O_NOCTTY);
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

  /* CombiBraille initialisation procedure: */
  success = 0;
  /* Try MAX_ATTEMPTS times, or forever if MAX_ATTEMPTS is 0: */
#if MAX_ATTEMPTS == 0
  while (!success)
#else
  for (i = 0; i < MAX_ATTEMPTS && !success; i++)
#endif
    {
      if (init_seq[0])
	if (write (brl_fd, init_seq + 1, init_seq[0]) != init_seq[0])
	  continue;
      timeout_yet (0);		/* initialise timeout testing */
      n = 0;
      do
	{
	  delay (20);
	  if (read (brl_fd, &c, 1) == 0)
	    continue;
	  if (n < init_ack[0] && c != init_ack[1 + n])
	    continue;
	  if (n == init_ack[0])
	    id = c, success = 1;
	  n++;
	}
      while (!timeout_yet (ACK_TIMEOUT) && n <= init_ack[0]);
    }
  if (!success)
    {
      tcsetattr (brl_fd, TCSANOW, &oldtio);
      goto failure;
    }

  res.x = BRLCOLS (id);		/* initialise size of display */
  res.y = BRLROWS;
  if (res.x == -1)
    return res;

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
  return res;

failure:;
  if (res.disp)
    free (res.disp);
  if (prevdata)
    free (prevdata);
  if (rawdata)
    free (rawdata);
  if (brl_fd >= 0)
    close (brl_fd);
  res.x = -1;
  return res;
}


void
closebrl (brldim brl)
{
  unsigned char *pre_data = PRE_DATA;	/* bytewise accessible copies */
  unsigned char *post_data = POST_DATA;
  unsigned char *close_seq = CLOSE_SEQ;

  rawlen = 0;
  if (pre_data[0])
    {
      memcpy (rawdata + rawlen, pre_data + 1, pre_data[0]);
      rawlen += pre_data[0];
    }
  /* Clear the five status cells and the main display: */
  memset (rawdata + rawlen, 0, 5 + brl.x * brl.y);
  rawlen += 5 + brl.x * brl.y;
  if (post_data[0])
    {
      memcpy (rawdata + rawlen, post_data + 1, post_data[0]);
      rawlen += post_data[0];
    }

  /* Send closing sequence: */
  if (close_seq[0])
    {
      memcpy (rawdata + rawlen, close_seq + 1, close_seq[0]);
      rawlen += close_seq[0];
    }
  write (brl_fd, rawdata, rawlen);

  free (brl.disp);
  free (prevdata);
  free (rawdata);

#if 0
  tcsetattr (brl_fd, TCSANOW, &oldtio);		/* restore terminal settings */
#endif
  close (brl_fd);
}


void
setbrlstat (const unsigned char *s)
{
  short i;

  /* Dot mapping: */
  for (i = 0; i < 5; status[i] = combitrans[s[i]], i++);
}


void
writebrl (brldim brl)
{
  short i;			/* loop counter */
  unsigned char *pre_data = PRE_DATA;	/* bytewise accessible copies */
  unsigned char *post_data = POST_DATA;

  /* Only refresh display if the data has changed: */
  if (memcmp (brl.disp, prevdata, brl.x * brl.y) || \
      memcmp (status, oldstatus, 5))
    {
      /* Save new Braille data: */
      memcpy (prevdata, brl.disp, brl.x * brl.y);

      /* Dot mapping from standard to CombiBraille: */
      for (i = 0; i < brl.x * brl.y; brl.disp[i] = combitrans[brl.disp[i]], \
	   i++);

      rawlen = 0;
      if (pre_data[0])
	{
	  memcpy (rawdata + rawlen, pre_data + 1, pre_data[0]);
	  rawlen += pre_data[0];
	}
      for (i = 0; i < 5; i++)
	{
	  rawdata[rawlen++] = status[i];
	  if (status[i] == 0x1b)	/* CombiBraille hack */
	    rawdata[rawlen++] = 0x1b;
	}
      for (i = 0; i < brl.x * brl.y; i++)
	{
	  rawdata[rawlen++] = brl.disp[i];
	  if (brl.disp[i] == 0x1b)	/* CombiBraille hack */
	    rawdata[rawlen++] = brl.disp[i];
	}
      if (post_data[0])
	{
	  memcpy (rawdata + rawlen, post_data + 1, post_data[0]);
	  rawlen += post_data[0];
	}
      write (brl_fd, rawdata, rawlen);
    }
}


int
readbrl (int type)
{
  int c;
  static short status = 0;	/* cursor routing keys mode */

  c = getbrlkey ();
  if (c == EOF)
    return EOF;
  c = type == TBL_CMD ? cmdtrans[c] : argtrans[c];
  if (c == 1 || c == 2)
    {
      status = c;
      return EOF;
    }
  if (c & 0x80)			/* cursor routing keys */
    switch (status)
      {
      case 0:			/* ordinary cursor routing */
	return (c ^ 0x80) + CR_ROUTEOFFSET;
      case 1:			/* begin block */
	status = 0;
	return (c ^ 0x80) + CR_BEGBLKOFFSET;
      case 2:			/* end block */
	status = 0;
	return (c ^ 0x80) + CR_ENDBLKOFFSET;
      }
  status = 0;
  return c;
}


int
getbrlkey (void)
{
  static short ptr = 0;		/* input queue pointer */
  static unsigned char q[4];	/* input queue */
  unsigned char c;		/* character buffer */

  while (read (brl_fd, &c, 1))
    {
      if (ptr == 0 && c != 27)
	continue;
      if (ptr == 1 && c != 'K' && c != 'C')
	{
	  ptr = 0;
	  continue;
	}
      q[ptr++] = c;
      if (ptr < 3 || (ptr == 3 && q[1] == 'K' && !q[2]))
	continue;
      ptr = 0;
      if (q[1] == 'K')
	return (q[2] ? q[2] : q[3] | 0x0060);
      return ((int) q[2] | 0x80);
    }
  return EOF;
}
