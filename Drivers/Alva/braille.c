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

/* Alva/brlmain.cc - Braille display library for Alva braille displays
 * Copyright (C) 1995-2002 by Nicolas Pitre <nico@cam.org>
 * See the GNU Public license for details in the ../COPYING file
 *
 */

/* Changes:
 *    september 2002:
 *		- This pesky binary only parallel port library is just
 *		  causing trouble (not compatible with new compilers, etc).
 *		  It is also unclear if distribution of such closed source
 *		  library is allowed within a GPL'ed program archive.
 *		  Let's just nuke it until we can write an open source one.
 *		- Converted this file back to pure C source.
 *    may 21, 1999:
 *		- Added Alva Delphi 80 support.  Thanks to ???
*		  <cstrobel@crosslink.net>.
 *    mar 14, 1999:
 *		- Added LogPrint's (which is a good thing...)
 *		- Ugly ugly hack for parallel port support:  seems there
 *		  is a bug in the parallel port library so that the display
 *		  completely hang after an arbitrary period of time.
 *		  J. Lemmens didn't respond to my query yet... and since
 *		  the F***ing library isn't Open Source, I can't fix it.
 *    feb 05, 1999:
 *		- Added Alva Delphi support  (thanks to Terry Barnaby 
 *		  <terry@beam.demon.co.uk>).
 *		- Renamed Alva_ABT3 to Alva.
 *		- Some improvements to the autodetection stuff.
 *    dec 06, 1998:
 *		- added parallel port communication support using
 *		  J. lemmens <jlemmens@inter.nl.net> 's library.
 *		  This required brl.o to be sourced with C++ for the parallel 
 *		  stuff to link.  Now brl.o is a partial link of brlmain.o 
 *		  and the above library.
 *    jun 21, 1998:
 *		- replaced CMD_WINUP/DN with CMD_ATTRUP/DN wich seems
 *		  to be a more useful binding.  Modified help files 
 *		  acordingly.
 *    apr 23, 1998:
 *		- I finally had the chance to test with an ABT380... and
 *		  corrected the ABT380 model ID for autodetection.
 *		- Added a refresh delay to force redrawing the whole display
 *		  in order to minimize garbage due to noise on the 
 *		  serial line
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
 *              - added keybindings for BRLTTY preferences menu
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
 *              - added cursor routing key block marking
 *              - fixed a bug in readbrl() about released keys
 *      sep 30' 1995:
 *              - initial Alva driver code, inspired from the
 *                (old) BrailleLite code.
 */


#define BRL_C 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/termios.h>
#include <string.h>

#include "brlconf.h"
#include "Programs/scr.h"
#include "Programs/misc.h"
#include "Programs/brltty.h"
#include "Programs/brl_driver.h"

static int brl_fd;			/* file descriptor for Braille display */
static struct termios oldtio;		/* old terminal settings */

/* Braille display parameters */

typedef struct
  {
    char *Name;
    int ID;
    int Cols;
    int NbStCells;
  }
BRLPARAMS;

static BRLPARAMS Models[] =
{
  {
    /* ID == 0 */
    "ABT320",
    ABT320,
    20,
    3
  }
  ,
  {
    /* ID == 1 */
    "ABT340",
    ABT340,
    40,
    3
  }
  ,
  {
    /* ID == 2 */
    "ABT340 Desktop",
    ABT34D,
    40,
    5
  }
  ,
  {
    /* ID == 3 */
    "ABT380",
    ABT380,
    80,
    5
  }
  ,
  {
    /* ID == 4 */
    "ABT380 Twin Space",
    ABT38D,
    80,
    5
  }
  ,
  {
    /* ID == 11 */
    "Alva Delphi 40",
    DEL440,
    40,
    3
  }
  ,
  {
    /* ID == 13 */
    "Alva Delphi 80",
    DEL480,
    80,
    5
  }
  ,
  {
    /* ID == 14 */
    "Alva Satellite 540",
    SAT540,
    40,
    3
  }
  ,
  {
    /* ID == 15 */
    "Alva Satellite 570",
    SAT570,
    66,
    3
  }
  ,
  {
    0,
  }
};


#define BRLROWS		1
#define MAX_STCELLS	5	/* hiest number of status cells */



/* This is the brltty braille mapping standard to Alva's mapping table.
 */
static char TransTable[256] =
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

static unsigned char *rawdata;		/* translated data to send to Braille */
static unsigned char *prevdata;	/* previously sent raw data */
static unsigned char StatusCells[MAX_STCELLS];		/* to hold status info */
static unsigned char PrevStatus[MAX_STCELLS];	/* to hold previous status */
static BRLPARAMS *model;		/* points to terminal model config struct */
static int ReWrite = 0;		/* 1 if display need to be rewritten */



/* Communication codes */

static char BRL_START[] = "\r\033B";	/* escape code to display braille */
#define DIM_BRL_START 3
static char BRL_END[] = "\r";		/* to send after the braille sequence */
#define DIM_BRL_END 1
static char BRL_ID[] = "\033ID=";
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
static int OperatingKeys[10] =
{KEY_PROG, KEY_HOME, KEY_CURSOR,
 KEY_UP, KEY_LEFT, KEY_RIGHT, KEY_DOWN,
 KEY_CURSOR2, KEY_HOME2, KEY_PROG2};
static int StatusKeys[6] =
{KEY_ROUTING_A, KEY_ROUTING_B, KEY_ROUTING_C,
 KEY_ROUTING_D, KEY_ROUTING_E, KEY_ROUTING_F};





static void
brl_identify (void)
{
  LogPrint(LOG_NOTICE, "Alva driver, version 2.1");
  LogPrint(LOG_INFO, "   Copyright (C) 1995-2000 by Nicolas Pitre <nico@cam.org>.");
  LogPrint(LOG_INFO, "   Compiled for %s with %s version.",
#if MODEL == ABT_AUTO
	  "terminal autodetection",
#else
	  Models[MODEL].Name,
#endif /* MODEL == ABT_AUTO */
#if ABT3_OLD_FIRMWARE
	  "old firmware"
#else
	  "new firmware"
#endif /* ABT3_OLD_FIRMWARE */
    );
}


/* SendToAlva() is shared with speech.c */
extern int SendToAlva( unsigned char *data, int len );

int SendToAlva( unsigned char *data, int len )
{
  if( write( brl_fd, data, len ) == len ) return 1;
  return 0;
}


static void brl_initialize (char **parameters, brldim *brl, const char *dev)
{
  brldim res;			/* return result */
  int ModelID = MODEL;
  unsigned char buffer[DIM_BRL_ID + 1];
  struct termios newtio;	/* new terminal settings */
  unsigned char alva_init[]="\033FUN\006\r";

  res.disp = rawdata = prevdata = NULL;		/* clear pointers */

  /* Open the Braille display device for random access */
  brl_fd = open (dev, O_RDWR | O_NOCTTY);
  if (brl_fd < 0) {
    LogPrint( LOG_ERR, "%s: %s", dev, strerror(errno) );
    goto failure;
  }
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
      delay (500);
      tcflush (brl_fd, TCIOFLUSH);	/* clean line */
      delay (500);
      /* DTR back on */
      cfsetispeed (&newtio, BAUDRATE);
      cfsetospeed (&newtio, BAUDRATE);
      tcsetattr (brl_fd, TCSANOW, &newtio);	/* activate new settings */
      delay (1000);		/* delay before 2nd line drop */
      /* This "if" statement can be commented out to try autodetect once anyway */
      if (ModelID != ABT_AUTO)
	break;

      if (!(read (brl_fd, buffer, DIM_BRL_ID + 1) == DIM_BRL_ID +1))
	{ // try init method for AD4MM and AS...
          write (brl_fd,alva_init,DIM_BRL_ID + 2);
          delay(200);
          read (brl_fd, buffer, DIM_BRL_ID + 1);         
        }
      if (!strncmp ((char *)buffer, BRL_ID, DIM_BRL_ID))
	ModelID = buffer[DIM_BRL_ID];
    }
  while (ModelID == ABT_AUTO);

  /* Find out which model we are connected to... */
  for( model = Models;
       model->Name && model->ID != ModelID;
       model++ );
  if( !model->Name ) {
    /* Unknown model */
    LogPrint( LOG_ERR, "Detected unknown Alva model with ID %d.", ModelID );
    LogPrint( LOG_WARNING, "Please fix Models[] in Alva/brlmain.cc and mail the maintainer." );
    goto failure;
  }

  /* Set model params... */
  // too many help screens, too little difference between them, so for now...
  //setHelpPageNumber( model - Models );		/* position in the model list */
  res.x = model->Cols;			/* initialise size of display */
  res.y = BRLROWS;

  /* Allocate space for buffers */
  res.disp = (unsigned char *) malloc (res.x * res.y);
  rawdata = (unsigned char *) malloc (res.x * res.y);
  prevdata = (unsigned char *) malloc (res.x * res.y);
  if (!res.disp || !rawdata || !prevdata) {
    LogPrint( LOG_ERR, "Cannot allocate braille buffers." );
    goto failure;
  }

  ReWrite = 1;			/* To write whole display at first time */

  *brl = res;
  return;

failure:
  if (res.disp)
    free (res.disp);
  if (rawdata)
    free (rawdata);
  if (prevdata)
    free (prevdata);
  brl->x = -1;
  return;
}


static void brl_close (brldim *brl)
{
  free (brl->disp);
  free (rawdata);
  free (prevdata);
  tcsetattr (brl_fd, TCSADRAIN, &oldtio);		/* restore terminal settings */
  close (brl_fd);
}


static int WriteToBrlDisplay (int Start, int Len, unsigned char *Data)
{
  unsigned char outbuf[ DIM_BRL_START + 2 + Len + DIM_BRL_END ];
  int outsz = 0;

  memcpy( outbuf, BRL_START, DIM_BRL_START );
  outsz += DIM_BRL_START;
  outbuf[outsz++] = (char)Start;
  outbuf[outsz++] = (char)Len;
  memcpy( outbuf+outsz, Data, Len );
  outsz += Len;
  memcpy( outbuf+outsz, BRL_END, DIM_BRL_END );
  outsz += DIM_BRL_END;
  return SendToAlva( outbuf, outsz );
}

static void brl_writeWindow (brldim *brl)
{
  int i, j, k;
  static int Timeout = 0;

  if (ReWrite ||  ++Timeout > (REFRESH_RATE/refreshInterval))
    {
      ReWrite = Timeout = 0;
      /* We rewrite the whole display */
      i = 0;
      j = model->Cols;
    }
  else
    {
      /* We update only the display part that has been changed */
      i = 0;
      while ((brl->disp[i] == prevdata[i]) && (i < model->Cols))
	i++;
      j = model->Cols - 1;
      while ((brl->disp[j] == prevdata[j]) && (j >= i))
	j--;
      j++;
    }
  if (i < j)			/* there is something different */
    {
      for (k = 0;
	   k < (j - i);
	   rawdata[k++] = TransTable[(prevdata[i + k] = brl->disp[i + k])]);
      WriteToBrlDisplay (model->NbStCells + i, j - i, rawdata);
    }
}


static void
brl_writeStatus (const unsigned char *st)
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



static int GetABTKey (unsigned int *Keys, unsigned int *Pos)
{
  unsigned char c;
  static int KeyGroup = 0;
  static int id_l = 0;

  while (read (brl_fd, &c, 1) > 0)
    {

#if ! ABT3_OLD_FIRMWARE

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
	  if ((c == 0x71) || (c == 0x72)) {
	     /* Key group selection */
	     KeyGroup = c;
	     id_l = 0;
	  }else if( c == BRL_ID[id_l] ) {
	     id_l++;
	     if( id_l >= DIM_BRL_ID ) {
		/* The terminal has been turned off and back on.  To be
		 * sure we arrange for the driver to restart so
		 * model probing, etc. will take place.
		 */
		id_l = 0;
		return -1;
	     }
	  }else{
	     /* Probably garbage came on the line.
	      * Let's reset the keys and state.
	      */
	     *Keys = 0;
	     ReWrite = 1;
	     return 0;
	  }
	  break;
	}

#else /* ABT3_OLD_FIRMWARE */

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

#endif /* ! ABT3_OLD_FIRMWARE */

    }
  return (0);
}


static int brl_read (DriverCommandContext cmds)
{
  int ProcessKey, res = EOF;
  static unsigned int RoutingPos = 0;
  static unsigned int CurrentKeys = 0, LastKeys = 0, ReleasedKeys = 0;
  static int Typematic = 0, KeyDelay = 0, KeyRepeat = 0;

  if (!(ProcessKey = GetABTKey (&CurrentKeys, &RoutingPos)))
    {
      if (Typematic)
	{
	  /* a key is being held */
	  if (KeyDelay < TYPEMATIC_DELAY)
	    KeyDelay++;
	  else if (KeyRepeat < TYPEMATIC_REPEAT)
	    KeyRepeat++;
	  else
	    {
	      /* It's time to issue the command again */
	      CurrentKeys = LastKeys;
	      LastKeys = 0;
	      KeyRepeat = 0;
	      ProcessKey = 1;
	    }
	}
    }
  else
    {
      /* A new key is being pressed/released, we clear the typematic counters */
      Typematic = KeyDelay = KeyRepeat = 0;
    }

  if (ProcessKey < 0)
    {
      /* Oops... seems we should restart from scratch... */
      RoutingPos = 0;
      CurrentKeys = LastKeys = ReleasedKeys = 0;
      return CMD_RESTARTBRL;
    }
  else if(ProcessKey > 0)
    {
      if (CurrentKeys > LastKeys)
	{
	  /* These are the keys that should be processed when pressed */
	  LastKeys = CurrentKeys;	/* we keep it until it is released */
	  ReleasedKeys = 0;
	  switch (CurrentKeys)
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
	      res = CMD_ATTRUP;
	      break;
	    case KEY_DOWN:
	      res = CMD_LNDN;
	      Typematic = 1;
	      break;
	    case KEY_CURSOR | KEY_DOWN:
	      res = CMD_ATTRDN;
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
	    case KEY_PROG | KEY_DOWN:
	      res = CMD_FREEZE;
	      break;
	    case KEY_PROG | KEY_UP:
	      res = CMD_INFO;
	      break;
	    case KEY_PROG | KEY_CURSOR | KEY_LEFT:
	      res = CMD_BACK;
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
	    case KEY_PROG | KEY_HOME | KEY_UP:
	      res = CMD_SPKHOME;
	      break;
	    case KEY_PROG | KEY_HOME | KEY_LEFT:
	      res = CMD_RESTARTSPEECH;
	      break;
	    case KEY_PROG | KEY_HOME | KEY_RIGHT:
	      res = CMD_SAYALL;
	      break;
	    case KEY_ROUTING:
	      /* normal Cursor routing keys */
	      res = CR_ROUTE + RoutingPos;
	      break;
	    case KEY_PROG | KEY_ROUTING:
	      /* marking beginning of block */
	      res = CR_CUTBEGIN + RoutingPos;
	      break;
	    case KEY_HOME | KEY_ROUTING:
	      /* marking end of block */
	      res = CR_CUTRECT + RoutingPos;
	      break;
	    case KEY_PROG | KEY_HOME | KEY_DOWN:
	      res = CMD_PASTE;
	      break;
	    case KEY_PROG | KEY_HOME | KEY_ROUTING:
	      /* attribute for pointed character */
	      res = CR_DESCCHAR + RoutingPos;
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
		case KEY_PROG | KEY_CURSOR:
		  res = CMD_PREFMENU;
		  break;
		}
	    }
	  LastKeys = CurrentKeys;
	  if (!CurrentKeys)
	    ReleasedKeys = 0;
	}
    }
  return (res);
}
