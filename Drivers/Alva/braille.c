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
 *		- bound CMD_SAY_LINE and CMD_MUTE
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/termios.h>
#include <string.h>

#include "Programs/misc.h"
#include "Programs/brltty.h"

#define BRLNAME	"Alva"
#define PREFSTYLE ST_AlvaStyle

#include "Programs/brl_driver.h"
#include "braille.h"

static int brl_fd;			/* file descriptor for Braille display */
static struct termios oldtio;		/* old terminal settings */

/* Braille display parameters */

typedef struct
  {
    const char *Name;
    unsigned char ID;
    unsigned char Cols;
    unsigned char NbStCells;
  }
BRLPARAMS;

static BRLPARAMS Models[] =
{
  {
    /* ID == 0 */
    "ABT 320",
    ABT320,
    20,
    3
  }
  ,
  {
    /* ID == 1 */
    "ABT 340",
    ABT340,
    40,
    3
  }
  ,
  {
    /* ID == 2 */
    "ABT 340 Desktop",
    ABT34D,
    40,
    5
  }
  ,
  {
    /* ID == 3 */
    "ABT 380",
    ABT380,
    80,
    5
  }
  ,
  {
    /* ID == 4 */
    "ABT 382 Twin Space",
    ABT382,
    80,
    5
  }
  ,
  {
    /* ID == 10 */
    "Delphi 20",
    DEL420,
    20,
    3
  }
  ,
  {
    /* ID == 11 */
    "Delphi 40",
    DEL440,
    40,
    3
  }
  ,
  {
    /* ID == 12 */
    "Delphi 40 Desktop",
    DEL44D,
    40,
    5
  }
  ,
  {
    /* ID == 13 */
    "Delphi 80",
    DEL480,
    80,
    5
  }
  ,
  {
    /* ID == 14 */
    "Satellite 544",
    SAT544,
    40,
    3
  }
  ,
  {
    /* ID == 15 */
    "Satellite 570 Pro",
    SAT570P,
    66,
    3
  }
  ,
  {
    /* ID == 16 */
    "Satellite 584 Pro",
    SAT584P,
    80,
    3
  }
  ,
  {
    /* ID == 17 */
    "Satellite 544 Traveller",
    SAT544T,
    40,
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
static TranslationTable outputTable;



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
#else /* MODEL == ABT_AUTO */
	  Models[MODEL].Name,
#endif /* MODEL == ABT_AUTO */
#if ABT3_OLD_FIRMWARE
	  "old firmware"
#else /* ABT3_OLD_FIRMWARE */
	  "new firmware"
#endif /* ABT3_OLD_FIRMWARE */
    );
}


/* SendToAlva() is shared with speech.c */
extern int SendToAlva( unsigned char *data, int len );

int SendToAlva( unsigned char *data, int len )
{
  if( safe_write( brl_fd, data, len ) == len ) return 1;
  return 0;
}


static int brl_open (BrailleDisplay *brl, char **parameters, const char *dev)
{
  int ModelID = MODEL;
  unsigned char buffer[DIM_BRL_ID + 1];
  struct termios newtio;	/* new terminal settings */
  unsigned char alva_init[]="\033FUN\006\r";

  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(&dots, &outputTable);
  }

  rawdata = prevdata = NULL;		/* clear pointers */

  /* Open the Braille display device for random access */
  if (!openSerialDevice(dev, &brl_fd, &oldtio)) goto failure;

  /* Set flow control and 8n1, enable reading */
  newtio.c_cflag = CS8 | CLOCAL | CREAD;

  /* Ignore bytes with parity errors and make terminal raw and dumb */
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;		/* raw output */
  newtio.c_lflag = 0;		/* don't echo or generate signals */
  newtio.c_cc[VMIN] = 0;	/* set nonblocking read */
  newtio.c_cc[VTIME] = 0;

  /* autodetecting ABT model */
  do
    {
      resetSerialDevice(brl_fd, &newtio, BAUDRATE);	/* activate new settings */
      delay (1000);		/* delay before 2nd line drop */
      /* This "if" statement can be commented out to try autodetect once anyway */
      if (ModelID != ABT_AUTO)
	break;

      if (!(read (brl_fd, buffer, DIM_BRL_ID + 1) == DIM_BRL_ID +1))
	{ /* try init method for AD4MM and AS... */
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
    LogPrint( LOG_WARNING, "Please fix Models[] in Alva/braille.c and mail the maintainer." );
    goto failure;
  }
  LogPrint( LOG_INFO, "Detected Alva model %s: %d columns, %d status cells.",
            model->Name, model->Cols, model->NbStCells );

  /* Set model params... */
  /* too many help screens, too little difference between them, so for now... */
  /* brl->helpPage = model - Models; */
  brl->x = model->Cols;			/* initialise size of display */
  brl->y = BRLROWS;

  /* Allocate space for buffers */
  rawdata = (unsigned char *) malloc (brl->x * brl->y);
  prevdata = (unsigned char *) malloc (brl->x * brl->y);
  if (!rawdata || !prevdata) {
    LogPrint( LOG_ERR, "Cannot allocate braille buffers." );
    goto failure;
  }

  ReWrite = 1;			/* To write whole display at first time */

  return 1;

failure:
  if (rawdata)
    free (rawdata);
  if (prevdata)
    free (prevdata);
  return 0;
}


static void brl_close (BrailleDisplay *brl)
{
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

static void brl_writeWindow (BrailleDisplay *brl)
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
      while ((brl->buffer[i] == prevdata[i]) && (i < model->Cols))
	i++;
      j = model->Cols - 1;
      while ((brl->buffer[j] == prevdata[j]) && (j >= i))
	j--;
      j++;
    }
  if (i < j)			/* there is something different */
    {
      for (k = 0;
	   k < (j - i);
	   rawdata[k++] = outputTable[(prevdata[i + k] = brl->buffer[i + k])]);
      WriteToBrlDisplay (model->NbStCells + i, j - i, rawdata);
    }
}


static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *st)
{
  int i;

  /* Update status cells on braille display */
  if (memcmp (st, PrevStatus, model->NbStCells))	/* only if it changed */
    {
      for (i = 0;
	   i < model->NbStCells;
	   StatusCells[i++] = outputTable[(PrevStatus[i] = st[i])]);
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


static int brl_readCommand (BrailleDisplay *brl, DriverCommandContext cmds)
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
	      res = CMD_SAY_LINE;
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
	      res = CMD_SAY_BELOW;
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
		  /*res = CMD_SAY_LINE;*/
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
