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

typedef struct {
  unsigned long int keys;
  char column;
  char status;
} Keys;
static Keys currentKeys, pressedKeys, nullKeys;
static int inputMode;

/* Braille display parameters */
typedef int (GetCommandHandler) (DriverCommandContext context);
static GetCommandHandler getHandyCommand;
static GetCommandHandler getBookwormCommand;
typedef int (InterpretCommandHandler) (DriverCommandContext context, const Keys &keys, int &command);
static InterpretCommandHandler interpretModularCommand;
static InterpretCommandHandler interpretBrailleWaveCommand;
static InterpretCommandHandler interpretBrailleStarCommand;
typedef struct {
  const char *name;
  int identifier;
  int columns;
  int statusCells;
  GetCommandHandler *getCommand;
  InterpretCommandHandler *interpretCommand;
  int helpPage;
  unsigned char *brailleStartAddress;
  unsigned char brailleStartLength;
  unsigned char *brailleEndAddress;
  unsigned char brailleEndLength;
  unsigned char *stopAddress;
  unsigned char stopLength;
} ModelDescription;
static const ModelDescription Models[] = {
  {
    "Modular 20+4", 0X80,
    20, 4,
    getHandyCommand, interpretModularCommand, 0,
    HandyBrailleStart, sizeof(HandyBrailleStart),
    NULL, 0,
    NULL, 0
  }
  ,
  {
    "Modular 40+4", 0X89,
    40, 4,
    getHandyCommand, interpretModularCommand, 0,
    HandyBrailleStart, sizeof(HandyBrailleStart),
    NULL, 0,
    NULL, 0
  }
  ,
  {
    "Modular 80+4", 0X88,
    80, 4,
    getHandyCommand, interpretModularCommand, 0,
    HandyBrailleStart, sizeof(HandyBrailleStart),
    NULL, 0,
    NULL, 0
  }
  ,
  {
    "Braille Wave", 0X05,
    40, 0,
    getHandyCommand, interpretBrailleWaveCommand, 0,
    HandyBrailleStart, sizeof(HandyBrailleStart),
    NULL, 0,
    NULL, 0
  }
  ,
  {
    "Bookworm", 0X90,
    8, 0,
    getBookwormCommand, NULL, 1,
    HandyBrailleStart, sizeof(HandyBrailleStart),
    BookwormBrailleEnd, sizeof(BookwormBrailleEnd),
    BookwormStop, sizeof(BookwormStop)
  }
  ,
  {
    "Braille Star 40", 0X74,
    40, 0,
    getHandyCommand, interpretBrailleStarCommand, 2,
    HandyBrailleStart, sizeof(HandyBrailleStart),
    NULL, 0,
    NULL, 0
  }
  ,
  {
    "Braille Star 80", 0X78,
    80, 0,
    getHandyCommand, interpretBrailleStarCommand, 2,
    HandyBrailleStart, sizeof(HandyBrailleStart),
    NULL, 0,
    NULL, 0
  }
  ,
  { /* end of table */
    NULL, 0,
    0, 0,
    NULL, NULL, 0,
    NULL, 0,
    NULL, 0,
    NULL, 0
  }
};

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
static unsigned int updateRequired = 0;

/* common key constants */
#define KEY_RELEASE 0X80
#define KEY_ROUTING 0X20
#define KEY_STATUS  0X70
#define KEY(code)   (1 << (code))

/* modular front keys */
#define KEY_B1              KEY(0X03)
#define KEY_B2              KEY(0X07)
#define KEY_B3              KEY(0X0B)
#define KEY_B4              KEY(0X0F)
#define KEY_B5              KEY(0X13)
#define KEY_B6              KEY(0X17)
#define KEY_B7              KEY(0X1B)
#define KEY_B8              KEY(0X1F)
#define KEY_UP              KEY(0X04)
#define KEY_DOWN            KEY(0X08)

/* modular keypad keys */
#define KEY_B12             KEY(0X01)
#define KEY_ZERO            KEY(0X05)
#define KEY_B13             KEY(0X09)
#define KEY_B14             KEY(0X0D)
#define KEY_B11             KEY(0X11)
#define KEY_ONE             KEY(0X15)
#define KEY_TWO             KEY(0X19)
#define KEY_THREE           KEY(0X1D)
#define KEY_B10             KEY(0X02)
#define KEY_FOUR            KEY(0X06)
#define KEY_FIVE            KEY(0X0A)
#define KEY_SIX             KEY(0X0E)
#define KEY_B9              KEY(0X12)
#define KEY_SEVEN           KEY(0X16)
#define KEY_EIGHT           KEY(0X1A)
#define KEY_NINE            KEY(0X1E)

/* braille wave keys */
#define KEY_ESCAPE          KEY(0X0C)
#define KEY_SPACE           KEY(0X10)
#define KEY_RETURN          KEY(0X14)

/* braille star keys */
#define KEY_SPACE_LEFT      KEY_SPACE
#define KEY_SPACE_RIGHT     KEY(0X18)
#define ROCKER_LEFT_TOP     KEY_ESCAPE
#define ROCKER_LEFT_BOTTOM  KEY_RETURN
#define ROCKER_LEFT_MIDDLE  (ROCKER_LEFT_TOP | ROCKER_LEFT_BOTTOM)
#define ROCKER_RIGHT_TOP    KEY_UP
#define ROCKER_RIGHT_BOTTOM KEY_DOWN
#define ROCKER_RIGHT_MIDDLE (ROCKER_RIGHT_TOP | ROCKER_RIGHT_BOTTOM)


static void
brl_identify (void) {
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
readBytes (unsigned char *bytes, int count) {
  int offset = 0;
  return readChunk(fileDescriptor, bytes, &offset, count, 100);
}

static int
readByte (unsigned char &byte) {
  return readBytes(&byte, 1);
}

static int
writeBytes (unsigned char *data, int length) {
  // LogBytes("Write", data, length);
  int count = safe_write(fileDescriptor, data, length);
  if (count == length) return 1;
  if (count == -1) {
    LogError("HandyTech write");
  } else {
    LogPrint(LOG_WARNING, "Trunccated write: %d < %d", count, length);
  }
  return 0;
}

static void
brl_initialize (char **parameters, brldim *brl, const char *dev) {
  int modelIdentifier = 0;

  brldim res;			/* return result */
  res.disp = rawData = prevData = NULL;		/* clear pointers */

  /* Open the Braille display device for random access */
  if (!dev) dev = defaultDevice;
  if (!openSerialDevice(dev, &fileDescriptor, &originalAttributes)) goto failure;

  struct termios newtio;	/* new terminal settings */
  newtio.c_cflag = CLOCAL | PARODD | PARENB | CREAD|CS8;
  newtio.c_iflag = IGNPAR; 
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  newtio.c_cc[VMIN] = 0;
  newtio.c_cc[VTIME] = 0;

  while (1) {
    if (!resetSerialDevice(fileDescriptor, &newtio, B19200)) return;
    /* autodetecting MODEL */
    if (writeBytes(HandyDescribe, sizeof(HandyDescribe))) {
      if (awaitInput(fileDescriptor, 1000)) {
        unsigned char buffer[sizeof(HandyDescription) + 1];
        if (readBytes(buffer, sizeof(buffer))) {
          if (memcmp(buffer, HandyDescription, sizeof(HandyDescription)) == 0) {
            modelIdentifier = buffer[sizeof(HandyDescription)];
            break;
          }
        }
      }
    }
    delay(1000);
  }

  /* Find out which model we are connected to... */
  for( model = Models;
       model->name && model->identifier != modelIdentifier;
       model++ );
  if( !model->name ) {
    /* Unknown model */
    LogPrint( LOG_ERR, "*** Detected unknown HandyTech model with ID %d.", modelIdentifier );
    LogPrint( LOG_WARNING, "*** Please fix Models[] in HandyTech/brlmain.cc and mail the maintainer." );
    goto failure;
  }
  LogPrint(LOG_INFO, "Detected %s: %d data %s, %d status %s.",
           model->name,
           model->columns, (model->columns == 1)? "cell": "cells",
           model->statusCells, (model->statusCells == 1)? "cell": "cells");

  /* Set model params... */
  setHelpPageNumber( model->helpPage );		/* position in the model list */
  res.x = model->columns;			/* initialise size of display */
  res.y = BRLROWS;

  /* Allocate space for buffers */
  res.disp = (unsigned char *) malloc (res.x * res.y);
  rawData = (unsigned char *) malloc (res.x * res.y);
  prevData = (unsigned char *) malloc (res.x * res.y);
  if (!res.disp || !rawData || !prevData) {
    LogPrint( LOG_ERR, "can't allocate braille buffers" );
    goto failure;
  }

  nullKeys.keys = 0;
  nullKeys.column = -1;
  nullKeys.status = -1;
  currentKeys = pressedKeys = nullKeys;
  inputMode = 0;

  memset(rawStatus, 0, model->statusCells);
  memset(rawData, 0, model->columns);

  currentState = BDS_OFF;
  stateTimer = 0;
  retryCount = 0;
  updateRequired = 0;
  setState(BDS_READY);

  *brl = res;
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
brl_close (brldim *brl) {
  if (model->stopLength) {
    writeBytes(model->stopAddress, model->stopLength);
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
updateBrailleCells (void) {
  if (updateRequired && (currentState == BDS_READY)) {
    unsigned char buffer[model->brailleStartLength + model->statusCells + model->columns + model->brailleEndLength];
    int count = 0;

    if (model->brailleStartLength) {
      memcpy(buffer+count, model->brailleStartAddress, model->brailleStartLength);
      count += model->brailleStartLength;
    }

    memcpy(buffer+count, rawStatus, model->statusCells);
    count += model->statusCells;

    memcpy(buffer+count, rawData, model->columns);
    count += model->columns;

    if (model->brailleEndLength) {
      memcpy(buffer+count, model->brailleEndAddress, model->brailleEndLength);
      count += model->brailleEndLength;
    }

    // LogBytes("Write", buffer, count);
    if (!writeBytes(buffer, count)) return 0;
    setState(BDS_WRITING);
    updateRequired = 0;
  }
  return 1;
}

static void
brl_writeWindow (brldim *brl) {
  if (memcmp(brl->disp, prevData, model->columns) != 0) {
    for (int i=0; i<model->columns; ++i) {
      rawData[i] = TransTable[(prevData[i] = brl->disp[i])];
    }
    updateRequired = 1;
  }
  updateBrailleCells();
}

static void
brl_writeStatus (const unsigned char *st) {
  if (memcmp(st, prevStatus, model->statusCells) != 0) {	/* only if it changed */
    for (int i=0; i<model->statusCells; ++i) {
      rawStatus[i] = TransTable[(prevStatus[i] = st[i])];
    }
    updateRequired = 1;
  }
}

static int
getHandyCommand (DriverCommandContext context) {
  int release = 1;
  unsigned char byte;
  while (readByte(byte)) {
    if (byte == 0X7E) {
      setState(BDS_READY);
      continue;
    }
    if ((release = ((byte & KEY_RELEASE) != 0))) byte &= ~KEY_RELEASE;
    currentKeys.column = -1;
    currentKeys.status = -1;

    if ((byte >= KEY_ROUTING) &&
        (byte < (KEY_ROUTING + model->columns))) {
      if (!release) {
        currentKeys.column = byte - KEY_ROUTING;
        int command;
        if (model->interpretCommand(context, currentKeys, command)) {
          pressedKeys = nullKeys;
          return command;
        }
      }
      break;
    }

    if ((byte >= KEY_STATUS) &&
        (byte < (KEY_STATUS + model->statusCells))) {
      if (!release) {
        currentKeys.status = byte - KEY_STATUS;
        int command;
        if (model->interpretCommand(context, currentKeys, command)) {
          pressedKeys = nullKeys;
          return command;
        }
      }
      break;
    }

    if (byte < 0X20) {
      unsigned long int key = KEY(byte);
      if (release) {
        currentKeys.keys &= ~key;
        if (pressedKeys.keys) {
          int command;
          int interpreted = model->interpretCommand(context, pressedKeys, command);
          pressedKeys = nullKeys;
          if (interpreted) return command;
        }
      } else {
        currentKeys.keys |= key;
        pressedKeys = currentKeys;
      }
    } else {
      LogPrint(LOG_WARNING, "Unexpected key %s: %02X",
               (release? "release": "press"), byte);
    }
    break;
  }
  return release? EOF: CMD_NOOP;
}

static int
interpretModularCommand (DriverCommandContext context, const Keys &keys, int &command) {
  if (keys.column >= 0) {
    switch (keys.keys) {
      default:
        break;
      case 0:
        command = CR_ROUTE + keys.column;
        return 1;
      case (KEY_B1):
        command = CR_SETLEFT + keys.column;
        return 1;
      case (KEY_B2):
        command = CR_DESCCHAR + keys.column;
        return 1;
      case (KEY_B3):
        command = CR_CUTAPPEND + keys.column;
        return 1;
      case (KEY_B4):
        command = CR_CUTBEGIN + keys.column;
        return 1;
      case (KEY_UP):
        command = CR_PRINDENT + keys.column;
        return 1;
      case (KEY_DOWN):
        command = CR_NXINDENT + keys.column;
        return 1;
      case (KEY_B5):
        command = CR_CUTRECT + keys.column;
        return 1;
      case (KEY_B6):
        command = CR_CUTLINE + keys.column;
        return 1;
      case (KEY_B7):
        command = CR_SETMARK + keys.column;
        return 1;
      case (KEY_B8):
        command = CR_GOTOMARK + keys.column;
        return 1;
    }
  } else if (keys.status >= 0) {
    switch (keys.keys) {
      default:
        break;
    }
  } else {
    if (inputMode) {
      const unsigned long int dots = KEY_B1 | KEY_B2 | KEY_B3 | KEY_B4 | KEY_B5 | KEY_B6 | KEY_B7 | KEY_B8;
      if (!(keys.keys & ~dots)) {
        command = VAL_PASSDOTS;
        if (keys.keys & KEY_B1) command |= B7;
        if (keys.keys & KEY_B2) command |= B3;
        if (keys.keys & KEY_B3) command |= B2;
        if (keys.keys & KEY_B4) command |= B1;
        if (keys.keys & KEY_B5) command |= B4;
        if (keys.keys & KEY_B6) command |= B5;
        if (keys.keys & KEY_B7) command |= B6;
        if (keys.keys & KEY_B8) command |= B8;
        return 1;
      }
      switch (keys.keys) {
        default:
          break;
        case (KEY_UP):
        case (KEY_DOWN):
          command = VAL_PASSDOTS;
          return 1;
      }
    }
    switch (keys.keys) {
      default:
        break;
      case (KEY_UP):
        command = CMD_FWINLT;
        return 1;
      case (KEY_DOWN):
        command = CMD_FWINRT;
        return 1;
      case (KEY_B1):
        command = CMD_HOME;
        return 1;
      case (KEY_B1 | KEY_UP):
        command = CMD_LNBEG;
        return 1;
      case (KEY_B1 | KEY_DOWN):
        command = CMD_LNEND;
        return 1;
      case (KEY_B2):
        command = CMD_TOP_LEFT;
        return 1;
      case (KEY_B2 | KEY_UP):
        command = CMD_TOP;
        return 1;
      case (KEY_B2 | KEY_DOWN):
        command = CMD_BOT;
        return 1;
      case (KEY_B3):
        command = CMD_BACK;
        return 1;
      case (KEY_B3 | KEY_UP):
        command = CMD_HWINLT;
        return 1;
      case (KEY_B3 | KEY_DOWN):
        command = CMD_HWINRT;
        return 1;
      case (KEY_B6 | KEY_UP):
        command = CMD_CHRLT;
        return 1;
      case (KEY_B6 | KEY_DOWN):
        command = CMD_CHRRT;
        return 1;
      case (KEY_B4):
        command = CMD_LNUP;
        return 1;
      case (KEY_B5):
        command = CMD_LNDN;
        return 1;
      case (KEY_B1 | KEY_B4):
        command = CMD_PRPGRPH;
        return 1;
      case (KEY_B1 |  KEY_B5):
        command = CMD_NXPGRPH;
        return 1;
      case (KEY_B2 | KEY_B4):
        command = CMD_PRPROMPT;
        return 1;
      case (KEY_B2 |  KEY_B5):
        command = CMD_NXPROMPT;
        return 1;
      case (KEY_B3 | KEY_B4):
        command = CMD_PRSEARCH;
        return 1;
      case (KEY_B3 |  KEY_B5):
        command = CMD_NXSEARCH;
        return 1;
      case (KEY_B6 | KEY_B4):
        command = CMD_ATTRUP;
        return 1;
      case (KEY_B6 |  KEY_B5):
        command = CMD_ATTRDN;
        return 1;
      case (KEY_B7 | KEY_B4):
        command = CMD_WINUP;
        return 1;
      case (KEY_B7 |  KEY_B5):
        command = CMD_WINDN;
        return 1;
      case (KEY_B8 | KEY_B4):
        command = CMD_PRDIFLN;
        return 1;
      case (KEY_B8 | KEY_B5):
        command = CMD_NXDIFLN;
        return 1;
      case (KEY_B8):
        command = CMD_HELP;
        return 1;
      case (KEY_B8 | KEY_B1):
        command = CMD_CSRTRK;
        return 1;
      case (KEY_B8 | KEY_B2):
        command = CMD_CSRVIS;
        return 1;
      case (KEY_B8 | KEY_B3):
        command = CMD_ATTRVIS;
        return 1;
      case (KEY_B8 | KEY_B6):
        command = CMD_SIXDOTS;
        return 1;
      case (KEY_B8 | KEY_B7):
        command = CMD_TUNES;
        return 1;
      case (KEY_B7):
        command = CMD_FREEZE;
        return 1;
      case (KEY_B7 | KEY_B1):
        command = CMD_PREFMENU;
        return 1;
      case (KEY_B7 | KEY_B2):
        command = CMD_PREFLOAD;
        return 1;
      case (KEY_B7 | KEY_B3):
        command = CMD_PREFSAVE;
        return 1;
      case (KEY_B7 | KEY_B6):
        command = CMD_INFO;
        return 1;
      case (KEY_B6):
        command = CMD_DISPMD;
        return 1;
      case (KEY_B6 | KEY_B1):
        command = CMD_SKPIDLNS;
        return 1;
      case (KEY_B6 | KEY_B2):
        command = CMD_SKPBLNKWINS;
        return 1;
      case (KEY_B6 | KEY_B3):
        command = CMD_SLIDEWIN;
        return 1;
      case (KEY_B2 | KEY_B3 | KEY_UP):
        command = CMD_MUTE;
        return 1;
      case (KEY_B2 | KEY_B3 | KEY_DOWN):
        command = CMD_SAY;
        return 1;
      case (KEY_B4 | KEY_B5):
        command = CMD_PASTE;
        return 1;
      case (KEY_B1 | KEY_B8 | KEY_UP):
        inputMode = 0;
        command = EOF;
        return 1;
      case (KEY_B1 | KEY_B8 | KEY_DOWN):
        inputMode = 1;
        command = EOF;
        return 1;
    }
  }
  return 0;
}

static int
interpretBrailleWaveCommand (DriverCommandContext context, const Keys &keys, int &command) {
  return interpretModularCommand(context, keys, command);
}

static int
interpretBrailleStarCommand (DriverCommandContext context, const Keys &keys, int &command) {
  if (keys.column >= 0) {
    switch (keys.keys) {
      default:
        break;
      case (ROCKER_LEFT_TOP):
        command = CR_CUTBEGIN + keys.column;
        return 1;
      case (ROCKER_LEFT_BOTTOM):
        command = CR_CUTAPPEND + keys.column;
        return 1;
      case (ROCKER_RIGHT_TOP):
        command = CR_CUTLINE + keys.column;
        return 1;
      case (ROCKER_RIGHT_BOTTOM):
        command = CR_CUTRECT + keys.column;
        return 1;
    }
  } else if (keys.status >= 0) {
    switch (keys.keys) {
      default:
        break;
    }
  } else {
    switch (keys.keys) {
      default:
        break;
      case (ROCKER_LEFT_TOP):
      case (ROCKER_RIGHT_TOP):
        command = CMD_LNUP;
        return 1;
      case (ROCKER_LEFT_BOTTOM):
      case (ROCKER_RIGHT_BOTTOM):
        command = CMD_LNDN;
        return 1;
      case (ROCKER_LEFT_MIDDLE):
        command = CMD_FWINLT;
        return 1;
      case (ROCKER_RIGHT_MIDDLE):
        command = CMD_FWINRT;
        return 1;
      case (ROCKER_LEFT_MIDDLE | ROCKER_RIGHT_MIDDLE):
        command = CMD_HOME;
        return 1;
      case (ROCKER_RIGHT_MIDDLE | ROCKER_LEFT_TOP):
        command = CMD_TOP_LEFT;
        return 1;
      case (ROCKER_RIGHT_MIDDLE | ROCKER_LEFT_BOTTOM):
        command = CMD_BOT_LEFT;
        return 1;
      case (ROCKER_LEFT_MIDDLE | ROCKER_RIGHT_TOP):
        command = CMD_TOP;
        return 1;
      case (ROCKER_LEFT_MIDDLE | ROCKER_RIGHT_BOTTOM):
        command = CMD_BOT;
        return 1;
      case (ROCKER_LEFT_TOP | ROCKER_RIGHT_TOP):
        command = CMD_PRDIFLN;
        return 1;
      case (ROCKER_LEFT_TOP | ROCKER_RIGHT_BOTTOM):
        command = CMD_NXDIFLN;
        return 1;
      case (ROCKER_LEFT_BOTTOM | ROCKER_RIGHT_TOP):
        command = CMD_ATTRUP;
        return 1;
      case (ROCKER_LEFT_BOTTOM | ROCKER_RIGHT_BOTTOM):
        command = CMD_ATTRDN;
        return 1;
    }
  }
  if (!(keys.keys & ~(KEY_B1 | KEY_B2 | KEY_B3 | KEY_B4 | KEY_B5 | KEY_B6 | KEY_B7 | KEY_B8 | KEY_SPACE_LEFT | KEY_SPACE_RIGHT))) {
    Keys modularKeys = keys;
    if (modularKeys.keys & KEY_SPACE_LEFT) {
      modularKeys.keys &= ~KEY_SPACE_LEFT;
      modularKeys.keys |= KEY_UP;
    }
    if (modularKeys.keys & KEY_SPACE_RIGHT) {
      modularKeys.keys &= ~KEY_SPACE_RIGHT;
      modularKeys.keys |= KEY_DOWN;
    }
    if (interpretModularCommand(context, modularKeys, command)) return 1;
  }
  return 0;
}

static int
interpretBookwormCommand (DriverCommandContext context, unsigned char key, int &command) {
  const int BWK_BACKWARD = 0X01;
  const int BWK_ESCAPE   = 0X02;
  const int BWK_ENTER    = 0X04;
  const int BWK_FORWARD  = 0X08;
  switch (context) {
    case CMDS_PREFS:
      switch (key) {
        case (BWK_BACKWARD):
          command = CMD_FWINLT;
          return 1;
        case (BWK_FORWARD):
          command = CMD_FWINRT;
          return 1;
        case (BWK_ESCAPE):
          command = CMD_PREFLOAD;
          return 1;
        case (BWK_ESCAPE | BWK_BACKWARD):
          command = CMD_MENU_PREV_SETTING;
          return 1;
        case (BWK_ESCAPE | BWK_FORWARD):
          command = CMD_MENU_NEXT_SETTING;
          return 1;
        case (BWK_ENTER):
          command = CMD_PREFMENU;
          return 1;
        case (BWK_ENTER | BWK_BACKWARD):
          command = CMD_MENU_PREV_ITEM;
          return 1;
        case (BWK_ENTER | BWK_FORWARD):
          command = CMD_MENU_NEXT_ITEM;
          return 1;
        case (BWK_ESCAPE | BWK_ENTER):
          command = CMD_PREFSAVE;
          return 1;
        case (BWK_ESCAPE | BWK_ENTER | BWK_BACKWARD):
          command = CMD_MENU_FIRST_ITEM;
          return 1;
        case (BWK_ESCAPE | BWK_ENTER | BWK_FORWARD):
          command = CMD_MENU_LAST_ITEM;
          return 1;
        case (BWK_BACKWARD | BWK_FORWARD):
          command = CMD_NOOP;
          return 1;
        case (BWK_BACKWARD | BWK_FORWARD | BWK_ESCAPE):
          command = CMD_NOOP;
          return 1;
        case (BWK_BACKWARD | BWK_FORWARD | BWK_ENTER):
          command = CMD_NOOP;
          return 1;
        default:
          break;
      }
      break;
    default:
      switch (key) {
        case (BWK_BACKWARD):
          command = CMD_FWINLT;
          return 1;
        case (BWK_FORWARD):
          command = CMD_FWINRT;
          return 1;
        case (BWK_ESCAPE):
          command = CMD_CSRTRK;
          return 1;
        case (BWK_ESCAPE | BWK_BACKWARD):
          command = CMD_BACK;
          return 1;
        case (BWK_ESCAPE | BWK_FORWARD):
          command = CMD_DISPMD;
          return 1;
        case (BWK_ENTER):
          command = CR_ROUTE;
          return 1;
        case (BWK_ENTER | BWK_BACKWARD):
          command = CMD_LNUP;
          return 1;
        case (BWK_ENTER | BWK_FORWARD):
          command = CMD_LNDN;
          return 1;
        case (BWK_ESCAPE | BWK_ENTER):
          command = CMD_PREFMENU;
          return 1;
        case (BWK_ESCAPE | BWK_ENTER | BWK_BACKWARD):
          command = CMD_LNBEG;
          return 1;
        case (BWK_ESCAPE | BWK_ENTER | BWK_FORWARD):
          command = CMD_LNEND;
          return 1;
        case (BWK_BACKWARD | BWK_FORWARD):
          command = CMD_HELP;
          return 1;
        case (BWK_BACKWARD | BWK_FORWARD | BWK_ESCAPE):
          command = CMD_CSRSIZE;
          return 1;
        case (BWK_BACKWARD | BWK_FORWARD | BWK_ENTER):
          command = CMD_FREEZE;
          return 1;
        default:
          break;
      }
      break;
  }
  return 0;
}

static int
getBookwormCommand (DriverCommandContext context) {
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
        continue;
      default:
        switch (currentState) {
          case BDS_OFF:
            continue;
          case BDS_RESETTING:
            switch (key) {
              case 0XFE:
                setState(BDS_IDENTIFYING);
                continue;
              default:
                break;
            }
            break;
          case BDS_IDENTIFYING:
            switch (key) {
              case 0X90:
                setState(BDS_READY);
                updateRequired = 1;
                updateBrailleCells();
                continue;
              default:
                break;
            }
            break;
          case BDS_WRITING:
            switch (key) {
              case 0X7E:
                setState(BDS_READY);
                continue;
              case 0X7D:
                updateRequired = 1;
                updateBrailleCells();
                continue;
              default:
                break;
            }
          case BDS_READY: {
            int command;
            if (interpretBookwormCommand(context, key, command)) return command;
            break;
          }
        }
        break;
    }
    LogPrint(LOG_WARNING, "Unexpected byte: %02X (state %d)", key, currentState);
  }
  if (timeout) {
    switch (currentState) {
      case BDS_OFF:
        break;
      case BDS_RESETTING:
        if (stateTimer > 3000) {
          if (retryCount > 3) {
            setState(BDS_OFF);
          } else if (writeBytes(HandyDescribe, sizeof(HandyDescribe))) {
            setState(BDS_RESETTING);
          } else {
            setState(BDS_OFF);
          }
        }
        break;
      case BDS_IDENTIFYING:
        if (stateTimer > 1000) {
          if (writeBytes(HandyDescribe, sizeof(HandyDescribe))) {
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
            if (writeBytes(HandyDescribe, sizeof(HandyDescribe))) {
              setState(BDS_RESETTING);
            } else {
              setState(BDS_OFF);
            }
          } else {
            updateRequired = 1;
            if (!updateBrailleCells()) setState(BDS_OFF);
          }
        }
        break;
    }
  }
  return EOF;
}

static int
brl_read (DriverCommandContext context) {
  stateTimer += refreshInterval;
  updateBrailleCells();
  return model->getCommand(context);
}
