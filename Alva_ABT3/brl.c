/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 * Nicolas Pitre <nico@cam.org>
 * Stephane Doyon <doyons@jsp.umontreal.ca>
 *
 * Version 1.0.2, 17 September 1996
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

/* Alva_ABT3/brl.c - Braille display library for Alva ABT3xx series
 * Copyright (C) 1995-1996 by Nicolas Pitre <nico@cam.org>
 * See the GNU Public license for details in the ../COPYING file
 *
 * $Id: brl.c,v 1.4 1996/10/03 08:08:13 nn201 Exp $
 */

/* Changes:
 *    oct 02, 1996:
 *		- bound CMD_SAY and CMD_MUTE
 *    sep 22, 1996:
 *		- bound CMD_PRDIFLN and CMD_NXDIFLN.
 *    aug 15, 1996:
 *              - adeded automatic model detection for new firmware.
 *              - support for selectable help screen.
 *    feb 19, 1996: 
 *              - added small hack for automatic rewrite of display when
 *                the terminal is turned off and back on, replugged, etc.
 *      feb 15, 1996:
 *              - Modified writebrl() for lower bandwith
 *              - Joined the forced ReWrite function to the CURSOR key
 *      jan 31, 1996:
 *              - moved user configurable parameters into brlconf.h
 *              - added identbrl()
 *              - added overide parameter for serial device
 *              - added keybindings for BRLTTY configuration menu
 *      jan 23, 1996:
 *              - modifications to be compatible with the BRLTTY braille
 *                mapping standard.
 *      dec 27, 1995:
 *              - Added conditions to support all ABT3xx series
 *              - changed directory Alva_ABT40 to Alva_ABT3
 *      dec 02, 1995:
 *              - made changes to support latest Alva ABT3 firmware (new
 *                serial protocol).
 *      nov 05, 1995:
 *              - added typematic facility
 *              - added key bindings for Stephane Doyon's cut'n paste.
 *                <doyons@jsp.umontreal.ca>
 *              - added cursor routing key block marking
 *              - fixed a bug in readbrl() about released keys
 *      sep 30' 1995:
 *              - initial Alva driver code, inspired from the
 *                BrailleLite code.
 */


#define BRL_C 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#include "brlconf.h"
#include "../brl.h"
#include "../scr.h"
#include "../misc.h"


static char StartupString[] =
"  Alva ABT3xx driver, version 1.33 \n"
"  Copyright (C) 1995-1996 by Nicolas Pitre <nico@cam.org> \n";



/* Braille display parameters */

typedef struct
  {
    char *Name;
    int Cols;
    int NbStCells;
  }
BRLPARAMS;

BRLPARAMS Models[NB_MODEL] =
{
  {
    /* ID == 0 */
    "ABT320",
    20,
    3
  }
  ,
  {
    /* ID == 1 */
    "ABT340",
    40,
    3
  }
  ,
  {
    /* ID == 2 */
    "ABT380",
    80,
    5
  }
  ,
  {
    /* ID == 3 */
    "ABT340 Desktop",
    40,
    5
  }
  ,
  {
    /* ID == 4 */
    "ABT380 Twin Space",
    80,
    5
  }
};


#define BRLROWS		1
#define MAX_STCELLS	5	/* hiest number of status cells */



/* This is the brltty braille mapping standard to Alva's mapping table.
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
  0xF4, 0xF5, 0xFC, 0xFD, 0xF6, 0xF7, 0xFE, 0xFF};



/* Global variables */

char DefDev[] = BRLDEV;		/* default braille device */
int brl_fd;			/* file descriptor for Braille display */
struct termios oldtio;		/* old terminal settings */
unsigned char *rawdata;		/* translated data to send to Braille */
unsigned char *prevdata;	/* previously sent raw data */
unsigned char StatusCells[MAX_STCELLS];		/* to hold status info */
unsigned char PrevStatus[MAX_STCELLS];	/* to hold previous status */
BRLPARAMS *model;		/* points to terminal model config struct */
short ReWrite = 0;		/* 1 if display need to be rewritten */



/* Communication codes */

char BRL_START[] = "\r\033B";	/* escape code to display braille */
#define DIM_BRL_START 3
char BRL_END[] = "\r";		/* to send after the braille sequence */
#define DIM_BRL_END 1
char BRL_ID[] = "\033ID=";
#define DIM_BRL_ID 4


/* Key values */

/* NB: The first 7 key values are the same as those returned by the
 * old firmware, so they can be used directly from the input stream as
 * make and break sequence already combined... not to be changed.
 */
#define KEY_PROG 	0x008	/* the PROG key */
#define KEY_HOME 	0x004	/* the HOME key */
#define KEY_CURSOR 	0x002	/* the CURSOR key */
#define KEY_UP 		0x001	/* the UP key */
#define KEY_LEFT 	0x010	/* the LEFT key */
#define KEY_RIGHT 	0x020	/* the RIGHT key */
#define KEY_DOWN 	0x040	/* the DOWN key */
#define KEY_CURSOR2 	0x080	/* the CURSOR2 key */
#define KEY_HOME2 	0x100	/* the HOME2 key */
#define KEY_PROG2 	0x200	/* the PROG2 key */
#define KEY_ROUTING	0x400	/* cursor routing key set */

/* those keys are not supposed to be combined, so their corresponding 
 * values are not bit exclusive between them.
 */
#define KEY_ROUTING_A	0x1000	/* first cur routing over status display */
#define KEY_ROUTING_B	0x2000	/* second cur routing over status disp. */
#define KEY_ROUTING_C	0x3000	/* ... */
#define KEY_ROUTING_D	0x4000
#define KEY_ROUTING_E	0x5000
#define KEY_ROUTING_F	0x6000

/* first cursor routing offset on main display (old firmware only) */
#define KEY_ROUTING_OFFSET 168

/* Index for new firmware protocol */
int
  OperatingKeys[10] =
{KEY_PROG, KEY_HOME, KEY_CURSOR,
 KEY_UP, KEY_LEFT, KEY_RIGHT, KEY_DOWN,
 KEY_CURSOR2, KEY_HOME2, KEY_PROG2};
int StatusKeys[6] =
{KEY_ROUTING_A, KEY_ROUTING_B, KEY_ROUTING_C,
 KEY_ROUTING_D, KEY_ROUTING_E, KEY_ROUTING_F};





int
WriteToBrlDisplay (short Start, short Len, char *Data)
{
  char Coor[2] =
  {(char) Start, (char) Len};

  if (write (brl_fd, BRL_START, DIM_BRL_START) == DIM_BRL_START)
    if (write (brl_fd, Coor, 2) == 2)
      if (write (brl_fd, Data, Len) == Len)
	if (write (brl_fd, BRL_END, DIM_BRL_END) == DIM_BRL_END)
	  return (1);
  return (0);
}


void
identbrl (const char *dev)
{
  /* Hello display... */
  printf (StartupString);
  printf ("  - compiled for %s with %s version \n",
#if MODEL == ABT_AUTO
	  "terminal autodetection",
#else
	  Models[MODEL].Name,
#endif
#ifdef ABT3_OLD_FIRMWARE
	  "old firmware"
#else
	  "new firmware"
#endif
    );
  printf ("  - device = %s\n", (dev) ? dev : DefDev);
}


brldim
initbrl (const char *dev)
{
  brldim res;			/* return result */
  struct termios newtio;	/* new terminal settings */
  short ModelID = MODEL;
  unsigned char buffer[DIM_BRL_ID + 1];

  res.disp = rawdata = prevdata = NULL;		/* clear pointers */

  /* Open the Braille display device for random access */
  if (!dev)
    dev = DefDev;
  brl_fd = open (dev, O_RDWR | O_NOCTTY);
  if (brl_fd < 0)
    goto failure;
  tcgetattr (brl_fd, &oldtio);	/* save current settings */

  /* Set flow control and 8n1, enable reading */
  newtio.c_cflag = CRTSCTS | CS8 | CLOCAL | CREAD;

  /* Ignore bytes with parity errors and make terminal raw and dumb */
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;		/* raw output */
  newtio.c_lflag = 0;		/* don't echo or generate signals */
  newtio.c_cc[VMIN] = 0;	/* set nonblocking read */
  newtio.c_cc[VTIME] = 0;

  /* autodetecting ABT model */
  do
    {
      /* to force DTR off */
      cfsetispeed (&newtio, B0);
      cfsetospeed (&newtio, B0);
      tcsetattr (brl_fd, TCSANOW, &newtio);	/* activate new settings */
      delay (100);
      tcflush (brl_fd, TCIOFLUSH);	/* clean line */
      /* DTR back on */
      cfsetispeed (&newtio, BAUDRATE);
      cfsetospeed (&newtio, BAUDRATE);
      tcsetattr (brl_fd, TCSANOW, &newtio);	/* activate new settings */
      delay (400);		/* give time to send ID string */
      /* The 2 next lines can be commented out to try autodetect once anyway */
      if (ModelID != ABT_AUTO)
	break;
      if (read (brl_fd, &buffer, DIM_BRL_ID + 1) == DIM_BRL_ID + 1)
	{
	  if (!strncmp (buffer, BRL_ID, DIM_BRL_ID))
	    ModelID = buffer[DIM_BRL_ID];
	}
    }
  while (ModelID == ABT_AUTO);
  if (ModelID >= NB_MODEL || ModelID < 0)
    goto failure;		/* unknown model */

  /* Set model params... */
  model = &Models[ModelID];
  sethlpscr (ModelID);
  res.x = model->Cols;		/* initialise size of display */
  res.y = BRLROWS;

  /* Allocate space for buffers */
  res.disp = (char *) malloc (res.x * res.y);
  rawdata = (unsigned char *) malloc (res.x * res.y);
  prevdata = (unsigned char *) malloc (res.x * res.y);
  if (!res.disp || !rawdata || !prevdata)
    goto failure;

  ReWrite = 1;			/* To write whole display at first time */

  return res;

failure:;
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


void
writebrl (brldim brl)
{
  int i, j, k;

  if (ReWrite)
    {
      /* We rewrite the whole display */
      i = 0;
      j = model->Cols;
      ReWrite = 0;
    }
  else
    {
      /* We update only the display part that has been changed */
      i = 0;
      while ((brl.disp[i] == prevdata[i]) && (i < model->Cols))
	i++;
      j = model->Cols - 1;
      while ((brl.disp[j] == prevdata[j]) && (j >= i))
	j--;
      j++;
    }
  if (i < j)			/* there is something different */
    {
      for (k = 0;
	   k < (j - i);
	   rawdata[k++] = TransTable[(prevdata[i + k] = brl.disp[i + k])]);
      WriteToBrlDisplay (model->NbStCells + i, j - i, rawdata);
    }
}


void
setbrlstat (const unsigned char *st)
{
  int i;

  /* Update status cells on braille display */
  if (memcmp (st, PrevStatus, model->NbStCells))	/* only if it changed */
    {
      for (i = 0;
	   i < model->NbStCells;
	   StatusCells[i++] = TransTable[(PrevStatus[i] = st[i])]);
      WriteToBrlDisplay (0, model->NbStCells, StatusCells);
    }
}



int
GetABTKey (unsigned int *Keys, unsigned int *Pos)
{
  unsigned char c;
  static int KeyGroup = 0;

  while (read (brl_fd, &c, 1))
    {

#ifndef ABT3_OLD_FIRMWARE

      switch (KeyGroup)
	{
	case 0x71:		/* Operating keys and Status keys of Touch Cursor */
	  /* make keys */
	  if (c <= 0x09)
	    *Keys |= OperatingKeys[c];
	  else if ((c >= 0x20) && (c <= 0x25))
	    *Keys |= StatusKeys[c - 0x20];
	  /* break keys */
	  else if ((c >= 0x80) && (c <= 0x89))
	    *Keys &= ~OperatingKeys[c - 0x80];
	  else if ((c >= 0xA0) && (c <= 0xA5))
	    *Keys &= ~StatusKeys[c - 0xA0];
	  else
	    *Keys = 0;
	  KeyGroup = 0;
	  return (1);

	case 0x72:		/* Display keys of Touch Cursor */
	  /* make keys */
	  if (c <= 0x5F)
	    {			/* make */
	      *Pos = c;
	      *Keys |= KEY_ROUTING;
	    }
	  /* break keys */
	  else
	    *Keys &= ~KEY_ROUTING;
	  KeyGroup = 0;
	  return (1);

	default:
	  /* Key group selection */
	  if ((c == 0x71) || (c == 0x72))
	    KeyGroup = c;
	  else
	    {
	      /* Let's reset the keys and rewrite the display */
	      *Keys = 0;
	      ReWrite = 1;
	    }
	  break;
	}

#else /* defined ABT3_OLD_FIRMWARE */

      if ((c >= (KEY_ROUTING_OFFSET + model->Cols)) &&
	  (c < (KEY_ROUTING_OFFSET + model->Cols + 6)))
	{
	  /* make for Status keys of Touch Cursor */
	  *Keys |= StatusKeys[c - (KEY_ROUTING_OFFSET + model->Cols)];
	}
      else if (c >= KEY_ROUTING_OFFSET)
	{
	  /* make for display keys of Touch cursor */
	  *Pos = c - KEY_ROUTING_OFFSET;
	  *Keys |= KEY_ROUTING;
	}
      else
	*Keys = c;		/* check comments where KEY_xxxx are defined */
      return (1);

#endif /* defined ABT3_OLD_FIRMWARE */

    }
  return (0);
}


int
readbrl (int type)
{
  int ProcessKey, res = EOF;
  static unsigned int RoutingPos = 0;
  static unsigned int ActualKeys = 0, LastKeys = 0, ReleasedKeys = 0;
  static int Typematic = 0, KeyDelay = 0, KeyRepeat = 0;

  if (!(ProcessKey = GetABTKey (&ActualKeys, &RoutingPos)))
    {
      if (Typematic)
	{
	  /* a key is being hold */
	  if (KeyDelay < TYPEMATIC_DELAY)
	    KeyDelay++;
	  else if (KeyRepeat < TYPEMATIC_REPEAT)
	    KeyRepeat++;
	  else
	    {
	      /* It's time to issue the command again */
	      ActualKeys = LastKeys;
	      LastKeys = 0;
	      KeyRepeat = 0;
	      ProcessKey = 1;
	    }
	}
    }
  else
    {
      /* A new key is being pressed/released, we clear the typematic dounters */
      Typematic = KeyDelay = KeyRepeat = 0;
    }

  if (ProcessKey)
    {
      if (ActualKeys > LastKeys)
	{
	  /* These are the keys that should be processed when pressed */
	  LastKeys = ActualKeys;	/* we keep it until it is released */
	  ReleasedKeys = 0;
	  switch (ActualKeys)
	    {
	    case KEY_HOME | KEY_UP:
	      res = CMD_TOP;
	      break;
	    case KEY_HOME | KEY_DOWN:
	      res = CMD_BOT;
	      break;
	    case KEY_UP:
	      res = CMD_LNUP;
	      Typematic = 1;
	      break;
	    case KEY_CURSOR | KEY_UP:
	      res = CMD_WINUP;
	      Typematic = 1;
	      break;
	    case KEY_DOWN:
	      res = CMD_LNDN;
	      Typematic = 1;
	      break;
	    case KEY_CURSOR | KEY_DOWN:
	      res = CMD_WINDN;
	      Typematic = 1;
	      break;
	    case KEY_LEFT:
	      res = CMD_FWINLT;
	      break;
	    case KEY_HOME | KEY_LEFT:
	      res = CMD_LNBEG;
	      break;
	    case KEY_CURSOR | KEY_LEFT:
	      res = CMD_HWINLT;
	      break;
	    case KEY_PROG | KEY_LEFT:
	      res = CMD_CHRLT;
	      Typematic = 1;
	      break;
	    case KEY_RIGHT:
	      res = CMD_FWINRT;
	      break;
	    case KEY_HOME | KEY_RIGHT:
	      res = CMD_LNEND;
	      break;
	    case KEY_PROG | KEY_RIGHT:
	      res = CMD_CHRRT;
	      Typematic = 1;
	      break;
	    case KEY_CURSOR | KEY_RIGHT:
	      res = CMD_HWINRT;
	      break;
	    case KEY_HOME | KEY_CURSOR | KEY_UP:
	      res = CMD_PRDIFLN;
	      break;
	    case KEY_HOME | KEY_CURSOR | KEY_DOWN:
	      res = CMD_NXDIFLN;
	      break;
	    case KEY_HOME | KEY_CURSOR | KEY_LEFT:
	      res = CMD_MUTE;
	      break;
	    case KEY_HOME | KEY_CURSOR | KEY_RIGHT:
	      res = CMD_SAY;
	      break;
	    case KEY_PROG | KEY_CURSOR:
	      res = CMD_CONFMENU;
	      break;
	    case KEY_PROG | KEY_DOWN:
	      res = CMD_FREEZE;
	      break;
	    case KEY_PROG | KEY_UP:
	      res = CMD_INFO;
	      break;
	    case KEY_ROUTING_A:
	      res = CMD_CAPBLINK;
	      break;
	    case KEY_ROUTING_B:
	      res = CMD_CSRVIS;
	      break;
	    case KEY_ROUTING_C:
	      res = CMD_CSRBLINK;
	      break;
	    case KEY_CURSOR | KEY_ROUTING_A:
	      res = CMD_SIXDOTS;
	      break;
	    case KEY_CURSOR | KEY_ROUTING_B:
	      res = CMD_CSRSIZE;
	      break;
	    case KEY_CURSOR | KEY_ROUTING_C:
	      res = CMD_SLIDEWIN;
	      break;
	    case KEY_PROG | KEY_HOME | KEY_LEFT:
	      res = CMD_CUT_BEG;
	      break;
	    case KEY_PROG | KEY_HOME | KEY_RIGHT:
	      res = CMD_CUT_END;
	      break;
	    case KEY_ROUTING:
	      /* normal Cursor routing keys */
	      res = CR_ROUTEOFFSET + RoutingPos;
	      break;
	    case KEY_PROG | KEY_ROUTING:
	      /* marking beginning of block */
	      res = CR_BEGBLKOFFSET + RoutingPos;
	      break;
	    case KEY_HOME | KEY_ROUTING:
	      /* marking end of block */
	      res = CR_ENDBLKOFFSET + RoutingPos;
	      break;
	    case KEY_PROG | KEY_HOME | KEY_DOWN:
	      res = CMD_PASTE;
	      break;
	    }
	}
      else
	{
	  /* These are the keys that should be processed when released */
	  if (!ReleasedKeys)
	    {
	      ReleasedKeys = LastKeys;
	      switch (ReleasedKeys)
		{
		case KEY_HOME:
		  res = CMD_TOP_LEFT;
		  break;
		case KEY_CURSOR:
		  res = CMD_HOME;
		  ReWrite = 1;	/* force rewrite of whole display */
		  break;
		case KEY_PROG:
		  res = CMD_HELP;
		  /*res = CMD_SAY;*/
		  break;
		case KEY_PROG | KEY_HOME:
		  res = CMD_DISPMD;
		  break;
		case KEY_HOME | KEY_CURSOR:
		  res = CMD_CSRTRK;
		  break;
		}
	    }
	  LastKeys = ActualKeys;
	  if (!ActualKeys)
	    ReleasedKeys = 0;
	}
    }
  return (res);
}
