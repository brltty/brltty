/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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
#include <string.h>
#include <errno.h>
#include <sys/termios.h>

#include "Programs/misc.h"
#include "Programs/brltty.h"

typedef enum {
  PARM_PORT,
  PARM_SERIALNUMBER
} DriverParameter;
#define BRLPARMS "port", "serialnumber"
#define BRLSTAT ST_AlvaStyle
#include "Programs/brl_driver.h"
#include "braille.h"

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
    "Delphi 420",
    DEL420,
    20,
    3
  }
  ,
  {
    /* ID == 11 */
    "Delphi 440",
    DEL440,
    40,
    3
  }
  ,
  {
    /* ID == 12 */
    "Delphi 440 Desktop",
    DEL44D,
    40,
    5
  }
  ,
  {
    /* ID == 13 */
    "Delphi 480",
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

static unsigned char *rawdata = NULL;	/* translated data to send to Braille */
static unsigned char *prevdata = NULL;	/* previously sent raw data */
static unsigned char StatusCells[MAX_STCELLS];		/* to hold status info */
static unsigned char PrevStatus[MAX_STCELLS];	/* to hold previous status */
static BRLPARAMS *model;		/* points to terminal model config struct */
static int ReWrite = 0;		/* 1 if display need to be rewritten */



/* Communication codes */

static char BRL_START[] = "\r\033B";	/* escape code to display braille */
#define DIM_BRL_START 3
static char BRL_END[] = "\r";		/* to send after the braille sequence */
#define DIM_BRL_END 1
static const char BRL_ID[] = {0X1B, 'I', 'D', '='};


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
#define KEY_ROUTING1	0x400	/* lower cursor routing key set */
#define KEY_ROUTING2	0x800	/* upper cursor routing key set */

/* those keys are not supposed to be combined, so their corresponding 
 * values are not bit exclusive between them.
 */
#define KEY_STATUS1_A	0x01000	/* first lower status key */
#define KEY_STATUS1_B	0x02000	/* second lower status key */
#define KEY_STATUS1_C	0x03000	/* third lower status key */
#define KEY_STATUS1_D	0x04000	/* fourth lower status key */
#define KEY_STATUS1_E	0x05000	/* fifth lower status key */
#define KEY_STATUS1_F	0x06000	/* sixth lower status key */
#define KEY_STATUS2_A	0x10000	/* first upper status key */
#define KEY_STATUS2_B	0x20000	/* second upper status key */
#define KEY_STATUS2_C	0x30000	/* third upper status key */
#define KEY_STATUS2_D	0x40000	/* fourth upper status key */
#define KEY_STATUS2_E	0x50000	/* fifth upper status key */
#define KEY_STATUS2_F	0x60000	/* sixth upper status key */

/* first cursor routing offset on main display (old firmware only) */
#define KEY_ROUTING_OFFSET 168

#if ! ABT3_OLD_FIRMWARE
/* Index for new firmware protocol */
static int OperatingKeys[10] =
{KEY_PROG, KEY_HOME, KEY_CURSOR,
 KEY_UP, KEY_LEFT, KEY_RIGHT, KEY_DOWN,
 KEY_CURSOR2, KEY_HOME2, KEY_PROG2};
#endif /* ! ABT3_OLD_FIRMWARE */

static int StatusKeys1[6] =
{KEY_STATUS1_A, KEY_STATUS1_B, KEY_STATUS1_C,
 KEY_STATUS1_D, KEY_STATUS1_E, KEY_STATUS1_F};

static int StatusKeys2[6] =
{KEY_STATUS2_A, KEY_STATUS2_B, KEY_STATUS2_C,
 KEY_STATUS2_D, KEY_STATUS2_E, KEY_STATUS2_F};

static int (*openPort) (char **parameters, const char *device);
static int (*resetPort) (void);
static void (*closePort) (void);
static int (*readPacket) (unsigned char *buffer, int length);
static int (*writePacket) (const unsigned char *buffer, int length);
static unsigned char inputBuffer[0X40];
static int inputUsed;

static int
verifyInputPacket (unsigned char *buffer, int *length) {
  int size = 0;
#if ! ABT3_OLD_FIRMWARE
  while (inputUsed > 0) {
    if (inputBuffer[0] == 0X7F) {
      if (inputUsed < 3) break;
      if (inputBuffer[2] != 0X7E) goto corrupt;
      if (inputUsed < 4) break;
      {
        int count = (inputBuffer[3] * 2) + 4;
        int complete = inputUsed >= count;
        int index;
        if (!complete) count = inputUsed;
        for (index=4; index<count; index+=2)
          if (inputBuffer[index] != 0X7E)
            goto corrupt;
        if (complete) size = count;
        break;
      }
    }

    if ((inputBuffer[0] & 0XF0) == 0X70) {
      if (inputUsed >= 2) size = 2;
      break;
    }

    {
      int count = sizeof(BRL_ID);
      if (inputUsed < count) count = inputUsed;
      if (memcmp(&inputBuffer[0], BRL_ID, count) != 0) goto corrupt;
      if (inputUsed >= 6) {
        if (inputBuffer[5] != '\r') goto corrupt;
        size = 6;
      }
      break;
    }

  corrupt:
    memcpy(&inputBuffer[0], &inputBuffer[1], --inputUsed);
  }
#else /* ABT3_OLD_FIRMWARE */
  if (inputUsed) size = 1;
#endif /* ! ABT3_OLD_FIRMWARE */
  if (!size) return 0;
  // LogBytes("Input packet", inputBuffer, size);

  if (*length < size) {
    LogPrint(LOG_WARNING, "Truncted input packet: %d < %d", *length, size);
    size = *length;
  } else {
    *length = size;
  }
  memcpy(buffer, &inputBuffer[0], *length);
  memcpy(&inputBuffer[0], &inputBuffer[*length], inputUsed-=*length);
  return 1;
}

static int
writeFunction (unsigned char code) {
  unsigned char bytes[] = {0X1B, 'F', 'U', 'N', code, '\r'};
  return writePacket(bytes, sizeof(bytes));
}

static int serialDevice = -1;
static struct termios oldSerialSettings;
static struct termios newSerialSettings;

static int
openSerialPort (char **parameters, const char *device) {
  if (!openSerialDevice(device, &serialDevice, &oldSerialSettings)) return 0;

  memset(&newSerialSettings, 0, sizeof(newSerialSettings));
  newSerialSettings.c_cflag = CS8 | CLOCAL | CREAD;
  newSerialSettings.c_iflag = IGNPAR;
  newSerialSettings.c_oflag = 0;		/* raw output */
  newSerialSettings.c_lflag = 0;		/* don't echo or generate signals */
  newSerialSettings.c_cc[VMIN] = 0;	/* set nonblocking read */
  newSerialSettings.c_cc[VTIME] = 0;

  return 1;
}

static int
resetSerialPort (void) {
  return resetSerialDevice(serialDevice, &newSerialSettings, BAUDRATE);	/* activate new settings */
}

static void
closeSerialPort (void) {
  if (serialDevice != -1) {
    tcsetattr(serialDevice, TCSADRAIN, &oldSerialSettings);		/* restore terminal settings */
    close(serialDevice);
    serialDevice = -1;
  }
}

static int
readSerialPacket (unsigned char *buffer, int length) {
  while (1) {
    unsigned char byte;
    int count = read(serialDevice, &byte, 1);
    if (count < 1) {
      if (count == -1) LogError("serial read");
      return count;
    }
    inputBuffer[inputUsed++] = byte;
    if (verifyInputPacket(buffer, &length)) return length;
  }
}

static int
writeSerialPacket (const unsigned char *buffer, int length) {
  return safe_write(serialDevice, buffer, length);
}
#ifdef ENABLE_USB
#include "Programs/usbio.h"

static UsbDevice *usbDevice = NULL;
static unsigned char usbOutputEndpoint;
static unsigned char usbInputEndpoint;

static int
chooseUsbDevice (UsbDevice *device, void *data) {
  char **parameters = data;
  const UsbDeviceDescriptor *descriptor = usbDeviceDescriptor(device);
  if ((descriptor->idVendor == 0X6b0) && (descriptor->idProduct == 1)) {
    const unsigned int interface = 0;

    if (parameters[PARM_SERIALNUMBER] && *parameters[PARM_SERIALNUMBER]) {
      int ok = 0;
      if (descriptor->iSerialNumber) {
        char *serialNumber;
        if ((serialNumber = usbGetString(device, descriptor->iSerialNumber, 1000))) {
          if (strcmp(serialNumber, parameters[PARM_SERIALNUMBER]) == 0) ok = 1;
          free(serialNumber);
        }
      }
      if (!ok) return 0;
    }

    if (usbClaimInterface(device, interface) != -1) {
      if (usbSetConfiguration(device, 1) != -1) {
        if (usbSetAlternative(device, interface, 0) != -1) {
          usbInputEndpoint = 1;
          usbOutputEndpoint = 2;
          return 1;
        } else {
          LogError("set USB alternative");
        }
      } else {
        LogError("set USB configuration");
      }
      usbReleaseInterface(device, interface);
    } else {
      LogError("claim USB interface");
    }
  }
  return 0;
}

static int
openUsbPort (char **parameters, const char *device) {
  if ((usbDevice = usbFindDevice(chooseUsbDevice, parameters))) {
    if (usbBeginInput(usbDevice, usbInputEndpoint, 8, 8) != -1) {
      return 1;
    } else {
      LogError("begin USB input");
    }

    usbCloseDevice(usbDevice);
    usbDevice = NULL;
  } else {
    LogPrint(LOG_DEBUG, "USB device not found.");
  }
  return 0;
}

static int
resetUsbPort (void) {
  return 1;
}

static void
closeUsbPort (void) {
  if (usbDevice) {
    usbCloseDevice(usbDevice);
    usbDevice = NULL;
  }
}

static int
readUsbPacket (unsigned char *buffer, int length) {
  while (1) {
    unsigned char bytes[2];
    int count = usbReapInput(usbDevice, bytes, sizeof(bytes), 0);
    if (count == -1) {
      if (errno == EAGAIN) return 0;
      LogError("USB read");
      return count;
    }

    if (bytes[0] || bytes[1]) {
      memcpy(&inputBuffer[inputUsed], bytes, count);
      inputUsed += count;
      if (verifyInputPacket(buffer, &length)) return length;
    } else {
      inputUsed = 0;
    }
  }
}

static int
writeUsbPacket (const unsigned char *buffer, int length) {
  return usbBulkWrite(usbDevice, usbOutputEndpoint, buffer, length, 1000);
}
#endif /* ENABLE_USB */

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
  if (writePacket(data, len) == len) return 1;
  return 0;
}


static int brl_open (BrailleDisplay *brl, char **parameters, const char *dev)
{
  int ModelID = MODEL;

  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(&dots, &outputTable);
  }

  {
    static const char *const serialPort = "serial";
#ifdef ENABLE_USB
    static const char *const usbPort = "usb";
#endif /* ENABLE_USB */
    const char *const ports[] = {
      serialPort,
#ifdef ENABLE_USB
      usbPort,
#endif /* ENABLE_USB */
      NULL
    };
    unsigned int port;
    validateChoice(&port, "port type", parameters[PARM_PORT], ports);

    if (ports[port] == serialPort) {
      openPort = openSerialPort;
      resetPort = resetSerialPort;
      closePort = closeSerialPort;
      readPacket = readSerialPacket;
      writePacket = writeSerialPacket;

#ifdef ENABLE_USB
    } else if (ports[port] == usbPort) {
      openPort = openUsbPort;
      resetPort = resetUsbPort;
      closePort = closeUsbPort;
      readPacket = readUsbPacket;
      writePacket = writeUsbPacket;
#endif /* ENABLE_USB */

    } else {
      LogPrint(LOG_WARNING, "Unsupported port type: %s", ports[port]);
      return 0;
    }
  }
  inputUsed = 0;

  /* Open the Braille display device */
  if (!openPort(parameters, dev)) goto failure;

  /* autodetecting ABT model */
  do {
    if (!resetPort()) goto failure;
    delay (1000);		/* delay before 2nd line drop */
    /* This "if" statement can be commented out to try autodetect once anyway */
    if (ModelID != ABT_AUTO) break;

    if (writeFunction(0X06) == -1) goto failure;
    delay(200);
    {
      unsigned char packet[6];
      if (readPacket(packet, sizeof(packet)) <= sizeof(BRL_ID)) continue;
      if (memcmp(packet, BRL_ID, sizeof(BRL_ID)) == 0) ModelID = packet[sizeof(BRL_ID)];
    }
  } while (ModelID == ABT_AUTO);

  /* Find out which model we are connected to... */
  for( model = Models;
       model->Name && model->ID != ModelID;
       model++ );
  if( !model->Name ) {
    /* Unknown model */
    LogPrint( LOG_ERR, "Detected unknown Alva model with ID %02X (hex).", ModelID );
    LogPrint( LOG_WARNING, "Please fix Models[] in Alva/braille.c and mail the maintainer." );
    goto failure;
  }
  LogPrint( LOG_INFO, "Detected Alva %s: %d columns, %d status cells.",
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
  brl_close(brl);
  return 0;
}


static void brl_close (BrailleDisplay *brl)
{
  if (rawdata) {
    free(rawdata);
    rawdata = NULL;
  }

  if (prevdata) {
    free(prevdata);
    prevdata = NULL;
  }

  closePort();
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
  return writePacket(outbuf, outsz);
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



static int GetKey (unsigned int *Keys, unsigned int *Pos)
{
  unsigned char packet[0X40];
  int length = readPacket(packet, sizeof(packet));
  if (length < 1) return length;
#if ! ABT3_OLD_FIRMWARE

  switch (packet[0]) {
    case 0X71:		/* Operating keys and Status keys of Touch Cursor */ {
      unsigned char key = packet[1];
      if (key <= 0X09) {
        *Keys |= OperatingKeys[key];
      } else if ((key >= 0X80) && (key <= 0X89)) {
        *Keys &= ~OperatingKeys[key - 0X80];
      } else if ((key >= 0X20) && (key <= 0X25)) {
        *Keys |= StatusKeys1[key - 0X20];
      } else if ((key >= 0XA0) && (key <= 0XA5)) {
        *Keys &= ~StatusKeys1[key - 0XA0];
      } else if ((key >= 0X30) && (key <= 0X35)) {
        *Keys |= StatusKeys2[key - 0X30];
      } else if ((key >= 0XB0) && (key <= 0XB5)) {
        *Keys &= ~StatusKeys2[key - 0XB0];
      } else {
        *Keys = 0;
      }
      return 1;
    }

    case 0X72: {
      unsigned char key = packet[1];
      if (key <= 0X5F) {			/* make */
        *Pos = key;
        *Keys |= KEY_ROUTING1;
      } else {
        *Keys &= ~KEY_ROUTING1;
      }
      return 1;
    }

    case 0X75: {
      unsigned char key = packet[1];
      if (key <= 0X5F) {			/* make */
        *Pos = key;
        *Keys |= KEY_ROUTING2;
      } else {
        *Keys &= ~KEY_ROUTING2;
      }
      return 1;
    }

    default:
      if (length > sizeof(BRL_ID)) {
        if (memcmp(packet, BRL_ID, sizeof(BRL_ID)) == 0) {
          /* The terminal has been turned off and back on.  To be
           * sure we arrange for the driver to restart so
           * model probing, etc. will take place.
           */
          return -1;
        }
      }

      break;
  }

#else /* ABT3_OLD_FIRMWARE */

  int key = packet[0];
  if ((key >= (KEY_ROUTING_OFFSET + model->Cols)) &&
      (key < (KEY_ROUTING_OFFSET + model->Cols + 6))) {
    /* make for Status keys of Touch Cursor */
    *Keys |= StatusKeys1[key - (KEY_ROUTING_OFFSET + model->Cols)];
  } else if (key >= KEY_ROUTING_OFFSET) {
    /* make for display keys of Touch cursor */
    *Pos = key - KEY_ROUTING_OFFSET;
    *Keys |= KEY_ROUTING;
  } else {
    *Keys = key;		/* check comments where KEY_xxxx are defined */
  }
  return 1;

#endif /* ! ABT3_OLD_FIRMWARE */
  return 0;
}


static int brl_readCommand (BrailleDisplay *brl, DriverCommandContext cmds)
{
  int ProcessKey, res = EOF;
  static unsigned int RoutingPos = 0;
  static unsigned int CurrentKeys = 0, LastKeys = 0, ReleasedKeys = 0;
  static int Typematic = 0, KeyDelay = 0, KeyRepeat = 0;

  if (!(ProcessKey = GetKey (&CurrentKeys, &RoutingPos)))
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
	    case KEY_STATUS1_A:
	      res = CMD_CAPBLINK;
	      break;
	    case KEY_STATUS1_B:
	      res = CMD_CSRVIS;
	      break;
	    case KEY_STATUS1_C:
	      res = CMD_CSRBLINK;
	      break;
	    case KEY_CURSOR | KEY_STATUS1_A:
	      res = CMD_SIXDOTS;
	      break;
	    case KEY_CURSOR | KEY_STATUS1_B:
	      res = CMD_CSRSIZE;
	      break;
	    case KEY_CURSOR | KEY_STATUS1_C:
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
	    case KEY_ROUTING1:
	      /* normal Cursor routing keys */
	      res = CR_ROUTE + RoutingPos;
	      break;
	    case KEY_PROG | KEY_ROUTING1:
	      /* marking beginning of block */
	      res = CR_CUTBEGIN + RoutingPos;
	      break;
	    case KEY_HOME | KEY_ROUTING1:
	      /* marking end of block */
	      res = CR_CUTRECT + RoutingPos;
	      break;
	    case KEY_PROG | KEY_HOME | KEY_DOWN:
	      res = CMD_PASTE;
	      break;
	    case KEY_PROG | KEY_HOME | KEY_ROUTING1:
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
