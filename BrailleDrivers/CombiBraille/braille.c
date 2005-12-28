/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

/* CombiBraille/braille.c - Braille display library
 * For Tieman B.V.'s CombiBraille (serial interface only)
 * Was maintained by Nikhil Nair <nn201@cus.cam.ac.uk>
 * $Id: braille.c,v 1.3 1996/09/24 01:04:29 nn201 Exp $
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>

#include "Programs/misc.h"

#define BRLSTAT ST_TiemanStyle
#include "Programs/brl_driver.h"
#include "braille.h"
#include "Programs/io_serial.h"

/* Command translation table: */
static int cmdtrans[0X100] = {
   #include "cmdtrans.h"		/* for keybindings */
};

static TranslationTable outputTable;	/* dot mapping table (output) */
SerialDevice *CB_serialDevice;			/* file descriptor for Braille display */
static int brl_cols;			/* file descriptor for Braille display */
int CB_charactersPerSecond;			/* file descriptor for Braille display */
static unsigned char *prevdata;	/* previously received data */
static unsigned char status[5], oldstatus[5];	/* status cells - always five */
static unsigned char *rawdata;		/* writebrl() buffer for raw Braille data */
static short rawlen;			/* length of rawdata buffer */

/* Function prototypes: */
static int getbrlkey (void);		/* get a keystroke from the CombiBraille */


static void
brl_identify (void)
{
  LogPrint(LOG_NOTICE, "Tieman B.V. CombiBraille driver");
  LogPrint(LOG_INFO, "   Copyright (C) 1995, 1996 by Nikhil Nair.");
}


static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device)
{
  short n, success;		/* loop counters, flags, etc. */
  unsigned char *init_seq = (unsigned char *)INIT_SEQ;	/* bytewise accessible copies */
  unsigned char *init_ack = (unsigned char *)INIT_ACK;
  unsigned char c;
  char id = -1;

  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X80, 0X40, 0X20, 0X08, 0X10};
    makeOutputTable(dots, outputTable);
  }

  if (!isSerialDevice(&device)) {
    unsupportedDevice(device);
    return 0;
  }

  prevdata = rawdata = NULL;		/* clear pointers */

  /* No need to load translation tables, as these are now
   * defined in tables.h
   */

  /* Now open the Braille display device for random access */
  if (!(CB_serialDevice = serialOpenDevice(device))) goto failure;
  if (!serialRestartDevice(CB_serialDevice, BAUDRATE)) goto failure;
  CB_charactersPerSecond = BAUDRATE / 10;

  /* CombiBraille initialisation procedure: */
  success = 0;
  if (init_seq[0])
    if (serialWriteData (CB_serialDevice, init_seq + 1, init_seq[0]) != init_seq[0])
      goto failure;
  hasTimedOut (0);		/* initialise timeout testing */
  n = 0;
  do
    {
      approximateDelay (20);
      if (serialReadData (CB_serialDevice, &c, 1, 0, 0) != 1)
        continue;
      if (n < init_ack[0] && c != init_ack[1 + n])
        continue;
      if (n == init_ack[0]) {
        id = c;
        success = 1;
        break;
      }
      n++;
    }
  while (!hasTimedOut (ACK_TIMEOUT) && n <= init_ack[0]);

  if (!success)
    {
      goto failure;
    }

  if (!serialSetFlowControl(CB_serialDevice, SERIAL_FLOW_HARDWARE)) goto failure;

  brl->y = BRLROWS;
  if ((brl->x = brl_cols = BRLCOLS(id)) == -1)
    goto failure;

  /* Allocate space for buffers */
  prevdata = mallocWrapper (brl->x * brl->y);
  /* rawdata has to have room for the pre- and post-data sequences,
   * the status cells, and escaped 0x1b's: */
  rawdata = mallocWrapper (20 + brl->x * brl->y * 2);
  return 1;

failure:
  if (prevdata)
    free (prevdata);
  if (rawdata)
    free (rawdata);
  if (CB_serialDevice)
    serialCloseDevice (CB_serialDevice);
  return 0;
}


static void
brl_close (BrailleDisplay *brl)
{
  unsigned char *pre_data = (unsigned char *)PRE_DATA;	/* bytewise accessible copies */
  unsigned char *post_data = (unsigned char *)POST_DATA;
  unsigned char *close_seq = (unsigned char *)CLOSE_SEQ;

  rawlen = 0;
  if (pre_data[0])
    {
      memcpy (rawdata + rawlen, pre_data + 1, pre_data[0]);
      rawlen += pre_data[0];
    }
  /* Clear the five status cells and the main display: */
  memset (rawdata + rawlen, 0, 5 + brl->x * brl->y);
  rawlen += 5 + brl->x * brl->y;
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
  serialWriteData (CB_serialDevice, rawdata, rawlen);

  free (prevdata);
  free (rawdata);

  serialCloseDevice (CB_serialDevice);
}


static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *s)
{
  short i;

  /* Dot mapping: */
  for (i = 0; i < 5; status[i] = outputTable[s[i]], i++);
}


static void
brl_writeWindow (BrailleDisplay *brl)
{
  short i;			/* loop counter */
  unsigned char *pre_data = (unsigned char *)PRE_DATA;	/* bytewise accessible copies */
  unsigned char *post_data = (unsigned char *)POST_DATA;

  /* Only refresh display if the data has changed: */
  if (memcmp (brl->buffer, prevdata, brl->x * brl->y) || \
      memcmp (status, oldstatus, 5))
    {
      /* Save new Braille data: */
      memcpy (prevdata, brl->buffer, brl->x * brl->y);

      /* Dot mapping from standard to CombiBraille: */
      for (i = 0; i < brl->x * brl->y; brl->buffer[i] = outputTable[brl->buffer[i]], \
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
      for (i = 0; i < brl->x * brl->y; i++)
	{
	  rawdata[rawlen++] = brl->buffer[i];
	  if (brl->buffer[i] == 0x1b)	/* CombiBraille hack */
	    rawdata[rawlen++] = brl->buffer[i];
	}
      if (post_data[0])
	{
	  memcpy (rawdata + rawlen, post_data + 1, post_data[0]);
	  rawlen += post_data[0];
	}
      serialWriteData (CB_serialDevice, rawdata, rawlen);
      brl->writeDelay += (rawlen * 1000 / CB_charactersPerSecond) + 1;
    }
}


static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context)
{
  static int status = 0;	/* cursor routing keys mode */
  int cmd = getbrlkey();
  if (cmd != EOF) {
    int blk = (cmd = cmdtrans[cmd]) & BRL_MSK_BLK;
    if (blk) {
      int arg = cmd & BRL_MSK_ARG;
      if (arg == BRL_MSK_ARG) {
        status = blk;
	return BRL_CMD_NOOP;
      }
      if (arg == (BRL_MSK_ARG - 1)) {
        cmd += brl_cols - 1;
      }
      if (status && (blk == BRL_BLK_ROUTE)) {
        cmd = status + arg;
      }
    }
    status = 0;
  }
  return cmd;
}


static int
getbrlkey (void)
{
  static short ptr = 0;		/* input queue pointer */
  static unsigned char q[4];	/* input queue */
  unsigned char c;		/* character buffer */

  while (serialReadData (CB_serialDevice, &c, 1, 0, 0) == 1)
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
