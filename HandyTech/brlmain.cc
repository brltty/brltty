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

#define BRL_C 1

#define __EXTENSIONS__	/* for termios.h */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/termios.h>
#include <sys/time.h>

extern "C" 
{
#include "brlconf.h"
#include "../common.h"
#include "../brl.h"
#include "../scr.h"
#include "../misc.h"
#include "../config.h"
#include "../brl_driver.h"
}

/* Communication codes */
static unsigned char HandyDescribe[] = {0XFF};
static unsigned char HandyDescription[] = {0XFE};
static unsigned char HandyBrailleStart[] = {0X01};	/* general header to display braille */
static unsigned char BookwormBrailleEnd[] = {0X16};	/* bookworm trailer to display braille */
static unsigned char BookwormStop[] = {0X05, 0X07};	/* bookworm trailer to display braille */


/* Braille display parameters */
typedef int (GetCommandHandler) (DriverCommandContext cmds);
static GetCommandHandler getHandyCommand;
static GetCommandHandler getBookwormCommand;
typedef struct {
  const char *Name;
  int ID;
  int Columns;
  int StatusCells;
  GetCommandHandler *GetCommand;
  int HelpPage;
  unsigned char *BrailleStartAddress;
  unsigned char BrailleStartLength;
  unsigned char *BrailleEndAddress;
  unsigned char BrailleEndLength;
  unsigned char *StopAddress;
  unsigned char StopLength;
} ModelDescription;
static const ModelDescription Models[] = {
  {
    /* ID == 0x80 */
    "Modular 20+4", 0X80,
    20, 4,
    getHandyCommand, 0,
    HandyBrailleStart, sizeof(HandyBrailleStart),
    NULL, 0,
    NULL, 0
  }
  ,
  {
    /* ID == 89 */
    "Modular 40+4", 0X89,
    40, 4,
    getHandyCommand, 0,
    HandyBrailleStart, sizeof(HandyBrailleStart),
    NULL, 0,
    NULL, 0
  }
  ,
  {
    /* ID == 0x88 */
    "Modular 80+4", 0X88,
    80, 4,
    getHandyCommand, 0,
    HandyBrailleStart, sizeof(HandyBrailleStart),
    NULL, 0,
    NULL, 0
  }
  ,
  {
    /* ID == 0x05 */
    "Braille Wave", 0X05,
    40, 0,
    getHandyCommand, 0,
    HandyBrailleStart, sizeof(HandyBrailleStart),
    NULL, 0,
    NULL, 0
  }
  ,
  {
    /* ID == 0x90 */
    "Bookworm", 0X90,
    8, 0,
    getBookwormCommand, 1,
    HandyBrailleStart, sizeof(HandyBrailleStart),
    BookwormBrailleEnd, sizeof(BookwormBrailleEnd),
    BookwormStop, sizeof(BookwormStop)
  }
  ,
  {
    /* end of table */
    NULL, 0,
    0, 0,
    NULL, 0,
    NULL, 0,
    NULL, 0,
    NULL, 0
  }
};
static unsigned int CurrentKeys[0x81], LastKeys[0x81], ReleasedKeys[0x81], nullKeys[0x81];


#define BRLROWS		1
#define MAX_STCELLS	4	/* highest number of status cells */



/* This is the brltty braille mapping standard to Handy's mapping table.
 */
static char TransTable[256] = {
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
static char defaultDevice[] = BRLDEV;		/* default braille device */
static int fileDescriptor;			/* file descriptor for Braille display */
static struct termios originalAttributes;		/* old terminal settings */
static unsigned char *rawData;		/* translated data to send to Braille */
static unsigned char *prevData;	/* previously sent raw data */
static unsigned char rawStatus[MAX_STCELLS];		/* to hold status info */
static unsigned char prevStatus[MAX_STCELLS];	/* to hold previous status */
static const ModelDescription *model;		/* points to terminal model config struct */
static unsigned char refdata[1 + 4 + 80 + 1]; /* reference data */

typedef enum {
  BDS_OFF,
  BDS_RESETTING,
  BDS_IDENTIFYING,
  BDS_READY,
  BDS_WRITING
} BrailleDisplayState;
static BrailleDisplayState currentState = BDS_OFF;
static unsigned long int stateTimer = 0;
static unsigned int retryCount = 0;

/* Key values */
#define KEY_B1 0X03
#define KEY_B2 0X07
#define KEY_B3 0X0B
#define KEY_B4 0X0F
#define KEY_B5 0X13
#define KEY_B6 0X17
#define KEY_B7 0X1B
#define KEY_B8 0X1F
#define KEY_UP 0X04
#define KEY_DOWN 0X08
#define KEY_ESCAPE 0X0C
#define KEY_SPACE 0X10
#define KEY_RETURN 0X14
#define KEY_B12 0X01
#define KEY_ZERO 0X05
#define KEY_B13 0X09
#define KEY_B14 0X0D
#define KEY_B11 0X11
#define KEY_ONE 0X15
#define KEY_TWO 0X19
#define KEY_THREE 0X1D
#define KEY_B10 0X02
#define KEY_FOUR 0X06
#define KEY_FIVE 0X0A
#define KEY_SIX 0X0E
#define KEY_B9 0X12
#define KEY_SEVEN 0X16
#define KEY_EIGHT 0X1A
#define KEY_NINE 0X1E
#define KEY_ROUTING 0x20
#define KEY_STATUS 0x70


static void
identbrl (void) {
  LogPrint(LOG_NOTICE, "Handy Tech Driver, version 0.3");
  LogPrint(LOG_INFO, "  Copyright (C) 2000 by Andreas Gross <andi.gross@gmx.de>");
}

static void
setState (BrailleDisplayState state) {
  if (state == currentState) {
    ++retryCount;
  } else {
    retryCount = 0;
    currentState = state;
  }
  stateTimer = 0;
  // LogPrint(LOG_DEBUG, "State: %d+%d", currentState, retryCount);
}

static int
awaitData (int milliseconds) {
  struct timeval timeout;
  memset(&timeout, 0, sizeof(timeout));
  timeout.tv_sec = milliseconds / 1000;
  timeout.tv_usec = (milliseconds % 1000) * 1000;

  fd_set mask;
  FD_ZERO(&mask);
  FD_SET(fileDescriptor, &mask);
  if (select(fileDescriptor+1, &mask, NULL, NULL, &timeout) > 0) return 1;

  return 0;
}

static int
readData (unsigned char *data, int length) {
  int count = read(fileDescriptor, data, length);
  if (count == length) return 1;
  return 0;
}

static int
readByte (unsigned char &byte) {
  return readData(&byte, 1);
}

static int
writeData (unsigned char *data, int length) {
  // LogBytes("Write", data, length);
  if (write(fileDescriptor, data, length) == length) return 1;
  return 0;
}

static void
initbrl (char **parameters, brldim *brl, const char *dev) {
  brldim res;			/* return result */
  struct termios newtio;	/* new terminal settings */
  int ModelID = 0;
  unsigned char buffer[sizeof(HandyDescription) + 1];
  unsigned int i=0;

  /* renice because Handy is too slow!!! */
  nice(-10);
  res.disp = rawData = prevData = NULL;		/* clear pointers */

  /* Open the Braille display device for random access */
  if (!dev) dev = defaultDevice;
  fileDescriptor = open (dev, O_RDWR | O_NOCTTY);
  if (fileDescriptor < 0) {
    LogPrint( LOG_ERR, "%s: %s", dev, strerror(errno) );
    goto failure;
  }

  tcgetattr (fileDescriptor, &originalAttributes);	/* save current settings */
  /* Set flow control and 8n1, enable reading */
  newtio.c_cflag = (CLOCAL|PARODD|PARENB|CREAD|CS8);

  /* Ignore bytes with parity errors and make terminal raw and dumb */
  newtio.c_iflag = IGNPAR; 
  newtio.c_oflag = 0;		/* raw output */
  newtio.c_lflag = 0;		/* don't echo or generate signals */
  newtio.c_cc[VMIN] = 0;	/* set nonblocking read */
  newtio.c_cc[VTIME] = 0;
  do
    {
      if(
	 cfsetispeed(&newtio,B0) ||
	 cfsetospeed(&newtio,B0) ||
	 tcsetattr(fileDescriptor,TCSANOW,&newtio) ||
	 tcflush(fileDescriptor, TCIOFLUSH) ||
	 cfsetispeed(&newtio,B19200) ||
	 cfsetospeed(&newtio,B19200) ||
	 tcsetattr(fileDescriptor,TCSANOW,&newtio)
	 ) {
	LogPrint(LOG_ERR,"Port initialization failed: %s: %s",dev,strerror(errno));
	return;
      }
      usleep(500);
      /* autodetecting MODEL */
      writeData(HandyDescribe, sizeof(HandyDescribe));
      if (readData(buffer, sizeof(buffer)))
	{
	  if (memcmp(buffer, HandyDescription, sizeof(HandyDescription)) == 0)
	    ModelID = buffer[sizeof(HandyDescription)];
	}
    }
  while (ModelID == 0);
  /* Find out which model we are connected to... */
  for( model = Models;
       model->Name && model->ID != ModelID;
       model++ );
  if( !model->Name ) {
    /* Unknown model */
    LogPrint( LOG_ERR, "*** Detected unknown HandyTech model with ID %d.", ModelID );
    LogPrint( LOG_WARNING, "*** Please fix Models[] in HandyTech/brlmain.cc and mail the maintainer." );
    goto failure;
  }
  LogPrint(LOG_INFO, "Detected %s: %d data %s, %d status %s.",
           model->Name,
           model->Columns, (model->Columns == 1)? "cell": "cells",
           model->StatusCells, (model->StatusCells == 1)? "cell": "cells");

  /* Set model params... */
  setHelpPageNumber( model->HelpPage );		/* position in the model list */
  res.x = model->Columns;			/* initialise size of display */
  res.y = BRLROWS;

  /* Allocate space for buffers */
  res.disp = (unsigned char *) malloc (res.x * res.y);
  rawData = (unsigned char *) malloc (res.x * res.y);
  prevData = (unsigned char *) malloc (res.x * res.y);
  if (!res.disp || !rawData || !prevData) {
    LogPrint( LOG_ERR, "can't allocate braille buffers" );
    goto failure;
  }

  *brl = res;

  /* reset tables of pressed/released keys to 0 */
  for (i=0; i<=0x80; i++)
    CurrentKeys[i] = LastKeys[i] = ReleasedKeys[i] = nullKeys[i] = 0;

  setState(BDS_READY);
  return;

failure:
  if (res.disp) {
    free(res.disp);
    res.disp = NULL;
  }
  if (rawData) {
    free(rawData);
    rawData = NULL;
  }
  if (prevData) {
    free(prevData);
    prevData = NULL;
  }
  brl->x = brl->y = -1;
  return;
}

static void
closebrl (brldim *brl) {
  if (model->StopLength) {
    writeData(model->StopAddress, model->StopLength);
  }

  free(brl->disp);
  brl->disp = NULL;
  brl->x = brl->y = -1;

  free(rawData);
  rawData = NULL;

  free(prevData);
  prevData = NULL;

  tcsetattr(fileDescriptor, TCSADRAIN, &originalAttributes);		/* restore terminal settings */
  close(fileDescriptor);
  fileDescriptor = -1;
}

static int
updateBrailleCells (int force) {
  unsigned char buffer[model->BrailleStartLength + model->StatusCells + model->Columns + model->BrailleEndLength];
  int count = 0;

  if (model->BrailleStartLength) {
    memcpy(buffer+count, model->BrailleStartAddress, model->BrailleStartLength);
    count += model->BrailleStartLength;
  }

  memcpy(buffer+count, rawStatus, model->StatusCells);
  count += model->StatusCells;

  memcpy(buffer+count, rawData, model->Columns);
  count += model->Columns;

  if (model->BrailleEndLength) {
    memcpy(buffer+count, model->BrailleEndAddress, model->BrailleEndLength);
    count += model->BrailleEndLength;
  }

  if (!force) {
    if (currentState != BDS_READY) return 1;
    if (memcmp(buffer, &refdata, count) == 0) return 1;
  }
  if (!writeData(buffer, count)) return 0;
  memcpy(&refdata, buffer, count);
  setState(BDS_WRITING);
  return 1;
}

static void
writebrl (brldim *brl) {
  int i;
  for (i=0; i<model->Columns; ++i) {
    rawData[i] = TransTable[(prevData[i] = brl->disp[i])];
  }
  updateBrailleCells(0);
}

static void
setbrlstat (const unsigned char *st) {
  /* Update status cells on braille display */
  if (memcmp(st, prevStatus, model->StatusCells) != 0) {	/* only if it changed */
    int i;
    for (i=0; i<model->StatusCells; ++i) {
      rawStatus[i] = TransTable[(prevStatus[i] = st[i])];
    /* updateBrailleCells(0); writebrl will do this anyway */
    }
  }
}

static int
getHandyKey (unsigned int *Pos) {
  unsigned char c;

  if (readByte(c))
    {
      if (c == 0X7E) {
        setState(BDS_READY);
	return 0;
      }
      if (c >= 0x80) {
	c=c-0x80;
	CurrentKeys[0x80] =1;
      }
      if ((c >= KEY_ROUTING) &&
	  (c < (KEY_ROUTING + model->Columns)))
	{
	  /* make for display keys of Touch cursor */
	  *Pos = c - KEY_ROUTING;
	  CurrentKeys[KEY_ROUTING]=1;
	}
      else
	CurrentKeys[c]=CurrentKeys[c] ? 0 : 1;		/* check comments where KEY_xxxx are defined */
      return (1);
    }
  return (0);
}

static int
getHandyCommand (DriverCommandContext cmds) {
  int ProcessKey, res = EOF;
  static unsigned int RoutingPos = 0;
  static int Typematic = 0, KeyDelay = 0, KeyRepeat = 0;

  if (!(ProcessKey = getHandyKey(&RoutingPos)))
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
	      CurrentKeys = LastKeys;
              LastKeys=nullKeys;
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
      CurrentKeys = nullKeys;
      LastKeys = nullKeys;
      ReleasedKeys = nullKeys;
      return CMD_RESTARTBRL;
    }
  else if(ProcessKey > 0)
    {
      if (CurrentKeys[0x80]==0)
	{
	  LastKeys = CurrentKeys;	/* we keep it until it is released */
          return(0);
        }
      /* 3KEYs */
      if (LastKeys[KEY_B2] && LastKeys[KEY_B3] && LastKeys[KEY_UP]) {
	res = CMD_MUTE;
	goto end_switch;
      }
      if (LastKeys[KEY_B2] && LastKeys[KEY_B3] && LastKeys[KEY_DOWN]) {
	res = CMD_SAY;
	goto end_switch;
      }
      if (LastKeys[KEY_B8] && LastKeys[KEY_B3] && LastKeys[KEY_DOWN]) {
	res = CMD_CHRRT;
	Typematic = 1;
	goto end_switch;
      }
      if (LastKeys[KEY_B8] && LastKeys[KEY_B3] && LastKeys[KEY_UP]) {
	res = CMD_CHRLT;
	Typematic = 1;
	goto end_switch;
      }
      /* 2KEYS */
      if (LastKeys[KEY_B4] && LastKeys[KEY_B5]) {
	res = CMD_PASTE;
	goto end_switch;
      }
      if (LastKeys[KEY_B4] && LastKeys[KEY_UP]) {
	res = CMD_CUT_BEG;
	goto end_switch;
      }
      if (LastKeys[KEY_B4] && LastKeys[KEY_DOWN]) {
	res = CMD_CUT_END;
	goto end_switch;
      }
      if (LastKeys[KEY_B8] && LastKeys[KEY_B4]) {
	res = CMD_PRDIFLN;
	goto end_switch;
      }
      if (LastKeys[KEY_B8] && LastKeys[KEY_B5]) {
	res = CMD_NXDIFLN;
	goto end_switch;
      }
      if (LastKeys[KEY_B1] && LastKeys[KEY_B8]) {
	res = CMD_CSRTRK;
	goto end_switch;
      }
      if (LastKeys[KEY_B4] && LastKeys[KEY_ROUTING]) {
	/* marking beginning of block */
	res = CR_CUTBEGIN + RoutingPos;
	goto end_switch;
      }
      if (LastKeys[KEY_B5] && LastKeys[KEY_ROUTING]) {
	/* marking end of block */
	res = CR_CUTRECT + RoutingPos;
	goto end_switch;
      }
      if (LastKeys[KEY_B2] && LastKeys[KEY_UP]) {
	res = CMD_TOP;
	goto end_switch;
      }
      if (LastKeys[KEY_B2] && LastKeys[KEY_DOWN]) {
	res = CMD_BOT;
	goto end_switch;
      }
      if (LastKeys[KEY_B6] && LastKeys[KEY_B4]) {
	res = CMD_ATTRUP;
	goto end_switch;
      }
      if (LastKeys[KEY_B6] &&  LastKeys[KEY_B5]) {
	res = CMD_ATTRDN;
	goto end_switch;
      }
      if (LastKeys[KEY_B3] && LastKeys[KEY_B4]) {
	res = CMD_LNBEG;
	goto end_switch;
      }
      if (LastKeys[KEY_B3] && LastKeys[KEY_UP]) {
	res = CMD_HWINLT;
	goto end_switch;
      }
      if (LastKeys[KEY_B3] && LastKeys[KEY_B5]) {
	res = CMD_LNEND;
	goto end_switch;
      }
      if (LastKeys[KEY_B3] && LastKeys[KEY_DOWN]) {
	res = CMD_HWINRT;
	goto end_switch;
      }
      if (LastKeys[KEY_B8] && LastKeys[KEY_B6]) {
	res = CMD_SIXDOTS;
	goto end_switch;
      }
      if (LastKeys[KEY_B7] && LastKeys[KEY_B6]) {
	res = CMD_CSRSIZE;
	goto end_switch;
      }
      if (LastKeys[KEY_B3] && LastKeys[KEY_B8]) {
	res = CMD_SLIDEWIN;
	goto end_switch;
      }
      if (LastKeys[KEY_B8] && LastKeys[KEY_B5]) {
	res = CMD_FREEZE;
	goto end_switch;
      }
      if (LastKeys[KEY_B8] && LastKeys[KEY_B4]) {
	res = CMD_INFO;
	goto end_switch;
      }
      if (LastKeys[KEY_B7] && LastKeys[KEY_B1]) {
        res = CMD_CAPBLINK;
        goto end_switch;
      }
      if (LastKeys[KEY_B7] && LastKeys[KEY_B2]) {
        res = CMD_CSRBLINK;
        goto end_switch;
      }
      if (LastKeys[KEY_B7] && LastKeys[KEY_B3]) {
        res = CMD_PREFMENU;
        goto end_switch;
      }
      /* 1KYES */
      if (LastKeys[KEY_STATUS+2]) {
	res = CMD_PREFMENU;
	goto end_switch;
      }
      if (LastKeys[KEY_B6]) {
	res = CMD_DISPMD;
	goto end_switch;
      }
      if (LastKeys[KEY_B4]) {
	res = CMD_LNUP;
	Typematic = 1;
	goto end_switch;
      }
      if (LastKeys[KEY_B5]) {
	res = CMD_LNDN;
	Typematic = 1;
	goto end_switch;
      }
      if (LastKeys[KEY_UP]) {
	res = CMD_FWINLT;
	goto end_switch;
      }
      if (LastKeys[KEY_DOWN]) {
	res = CMD_FWINRT;
	goto end_switch;
      }
      if (LastKeys[KEY_STATUS+0]) {
	res = CMD_CAPBLINK;
	goto end_switch;
      }
      if (LastKeys[KEY_B7]) {
	res = CMD_CSRVIS;
	goto end_switch;
      }
      if (LastKeys[KEY_STATUS+1]) {
	res = CMD_CSRBLINK;
	goto end_switch;
      }
      if (LastKeys[KEY_ROUTING]) {
	/* normal Cursor routing keys */
	res = CR_ROUTE + RoutingPos;
	goto end_switch;
      }
      if (LastKeys[KEY_B2]) {
	res = CMD_TOP_LEFT;
        goto end_switch;
      }
      if (LastKeys[KEY_B1]) {
	res = CMD_HOME;
	goto end_switch;
      }
      if (LastKeys[KEY_B8]) {
	res = CMD_HELP;
	goto end_switch;
	/*res = CMD_SAY;*/
      }
    end_switch:
      CurrentKeys=nullKeys;
      LastKeys=nullKeys;
    }
  return (res);
}

static int
getBookwormCommand (DriverCommandContext cmds) {
  int timeout = 1;
  unsigned char key;
  while (readByte(key)) {
    // LogPrint(LOG_DEBUG, "Read: %02X", key);
    timeout = 0;
    if (key == 0X06) {
      if (currentState != BDS_OFF) {
        if (awaitData(10)) {
          setState(BDS_OFF);
          continue;
        }
      }
    }
    switch (key) {
      case 0XFE:
        setState(BDS_IDENTIFYING);
        break;
      default:
        switch (currentState) {
          case BDS_OFF:
            break;
          case BDS_RESETTING:
            switch (key) {
              case 0XFE:
                setState(BDS_IDENTIFYING);
                break;
              default:
                break;
            }
            break;
          case BDS_IDENTIFYING:
            switch (key) {
              case 0X90:
                setState(BDS_READY);
                updateBrailleCells(1);
                break;
              default:
                break;
            }
            break;
          case BDS_READY: {
            const int BWK_BACKWARD = 0X01;
            const int BWK_ESCAPE   = 0X02;
            const int BWK_ENTER    = 0X04;
            const int BWK_FORWARD  = 0X08;
            switch (cmds) {
              case CMDS_PREFS:
                switch (key) {
                  case (BWK_BACKWARD):
                    return CMD_FWINLT;
                  case (BWK_FORWARD):
                    return CMD_FWINRT;
                  case (BWK_ESCAPE):
                    return CMD_PREFLOAD;
                  case (BWK_ESCAPE | BWK_BACKWARD):
                    return CMD_MENU_PREV_SETTING;
                  case (BWK_ESCAPE | BWK_FORWARD):
                    return CMD_MENU_NEXT_SETTING;
                  case (BWK_ENTER):
                    return CMD_PREFMENU;
                  case (BWK_ENTER | BWK_BACKWARD):
                    return CMD_MENU_PREV_ITEM;
                  case (BWK_ENTER | BWK_FORWARD):
                    return CMD_MENU_NEXT_ITEM;
                  case (BWK_ESCAPE | BWK_ENTER):
                    return CMD_PREFSAVE;
                  case (BWK_ESCAPE | BWK_ENTER | BWK_BACKWARD):
                    return CMD_MENU_FIRST_ITEM;
                  case (BWK_ESCAPE | BWK_ENTER | BWK_FORWARD):
                    return CMD_MENU_LAST_ITEM;
                  case (BWK_BACKWARD | BWK_FORWARD):
                    return CMD_NOOP;
                  case (BWK_BACKWARD | BWK_FORWARD | BWK_ESCAPE):
                    return CMD_NOOP;
                  case (BWK_BACKWARD | BWK_FORWARD | BWK_ENTER):
                    return CMD_NOOP;
                  default:
                    break;
                }
                break;
              default:
                switch (key) {
                  case (BWK_BACKWARD):
                    return CMD_FWINLT;
                  case (BWK_FORWARD):
                    return CMD_FWINRT;
                  case (BWK_ESCAPE):
                    return CMD_CSRTRK;
                  case (BWK_ESCAPE | BWK_BACKWARD):
                    return CMD_BACK;
                  case (BWK_ESCAPE | BWK_FORWARD):
                    return CMD_DISPMD;
                  case (BWK_ENTER):
                    return CMD_CSRJMP;
                  case (BWK_ENTER | BWK_BACKWARD):
                    return CMD_LNUP;
                  case (BWK_ENTER | BWK_FORWARD):
                    return CMD_LNDN;
                  case (BWK_ESCAPE | BWK_ENTER):
                    return CMD_PREFMENU;
                  case (BWK_ESCAPE | BWK_ENTER | BWK_BACKWARD):
                    return CMD_LNBEG;
                  case (BWK_ESCAPE | BWK_ENTER | BWK_FORWARD):
                    return CMD_LNEND;
                  case (BWK_BACKWARD | BWK_FORWARD):
                    return CMD_HELP;
                  case (BWK_BACKWARD | BWK_FORWARD | BWK_ESCAPE):
                    return CMD_CSRSIZE;
                  case (BWK_BACKWARD | BWK_FORWARD | BWK_ENTER):
                    return CMD_FREEZE;
                  default:
                    break;
                }
                break;
            }
            break;
          }
          case BDS_WRITING:
            switch (key) {
              case 0X7E:
                setState(BDS_READY);
                break;
              case 0X7D:
                updateBrailleCells(1);
                break;
              default:
                break;
            }
            break;
        }
        break;
    }
  }
  if (timeout) {
    switch (currentState) {
      case BDS_OFF:
        break;
      case BDS_RESETTING:
        if (stateTimer > 3000) {
          if (retryCount > 3) {
            setState(BDS_OFF);
          } else if (writeData(HandyDescribe, sizeof(HandyDescribe))) {
            setState(BDS_RESETTING);
          } else {
            setState(BDS_OFF);
          }
        }
        break;
      case BDS_IDENTIFYING:
        if (stateTimer > 1000) {
          if (writeData(HandyDescribe, sizeof(HandyDescribe))) {
            setState(BDS_RESETTING);
          } else {
            setState(BDS_OFF);
          }
        }
        break;
      case BDS_READY:
        break;
      case BDS_WRITING:
        if (stateTimer > 1000) {
          if (retryCount > 3) {
            if (writeData(HandyDescribe, sizeof(HandyDescribe))) {
              setState(BDS_RESETTING);
            } else {
              setState(BDS_OFF);
            }
          } else if (!updateBrailleCells(1)) {
            setState(BDS_OFF);
          }
        }
        break;
    }
  }
  return EOF;
}

static int
readbrl (DriverCommandContext cmds) {
  stateTimer += refreshInterval;
  return model->GetCommand (cmds);
}
