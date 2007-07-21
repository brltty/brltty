/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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
 *    january 2004:
 *              - Added USB support.
 *              - Improved key bindings for Satellite models.
 *              - Moved autorepeat (typematic) support to the core.
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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "misc.h"
#include "brltty.h"

#define BRLSTAT ST_AlvaStyle
#define BRL_HAVE_FIRMNESS
#define BRLCONST
#include "brl_driver.h"
#include "braille.h"

static const int logInputBuffer = 0;
static const int logInputPackets = 0;
static const int logOutputPackets = 0;

/* Braille display parameters */
typedef struct {
  const char *name;
  unsigned char identifier;
  unsigned char columns;
  unsigned char statusCells;
  unsigned char flags;
  unsigned char helpPage;
} ModelEntry;
#define MOD_FLG__CONFIGURABLE 0X01

static const ModelEntry modelTable[] = {
  { .identifier = 0X00,
    .name = "ABT 320",
    .columns = 20,
    .statusCells = 3,
    .flags = 0,
    .helpPage = 0
  }
  ,
  { .identifier = 0X01,
    .name = "ABT 340",
    .columns = 40,
    .statusCells = 3,
    .flags = 0,
    .helpPage = 0
  }
  ,
  { .identifier = 0X02,
    .name = "ABT 340 Desktop",
    .columns = 40,
    .statusCells = 5,
    .flags = 0,
    .helpPage = 0
  }
  ,
  { .identifier = 0X03,
    .name = "ABT 380",
    .columns = 80,
    .statusCells = 5,
    .flags = 0,
    .helpPage = 0
  }
  ,
  { .identifier = 0X04,
    .name = "ABT 382 Twin Space",
    .columns = 80,
    .statusCells = 5,
    .flags = 0,
    .helpPage = 0
  }
  ,
  { .identifier = 0X0A,
    .name = "Delphi 420",
    .columns = 20,
    .statusCells = 3,
    .flags = 0,
    .helpPage = 0
  }
  ,
  { .identifier = 0X0B,
    .name = "Delphi 440",
    .columns = 40,
    .statusCells = 3,
    .flags = 0,
    .helpPage = 0
  }
  ,
  { .identifier = 0X0C,
    .name = "Delphi 440 Desktop",
    .columns = 40,
    .statusCells = 5,
    .flags = 0,
    .helpPage = 0
  }
  ,
  { .identifier = 0X0D,
    .name = "Delphi 480",
    .columns = 80,
    .statusCells = 5,
    .flags = 0,
    .helpPage = 0
  }
  ,
  { .identifier = 0X0E,
    .name = "Satellite 544",
    .columns = 40,
    .statusCells = 3,
    .flags = MOD_FLG__CONFIGURABLE,
    .helpPage = 1
  }
  ,
  { .identifier = 0X0F,
    .name = "Satellite 570 Pro",
    .columns = 66,
    .statusCells = 3,
    .flags = MOD_FLG__CONFIGURABLE,
    .helpPage = 1
  }
  ,
  { .identifier = 0X10,
    .name = "Satellite 584 Pro",
    .columns = 80,
    .statusCells = 3,
    .flags = MOD_FLG__CONFIGURABLE,
    .helpPage = 1
  }
  ,
  { .identifier = 0X11,
    .name = "Satellite 544 Traveller",
    .columns = 40,
    .statusCells = 3,
    .flags = MOD_FLG__CONFIGURABLE,
    .helpPage = 1
  }
  ,
  { .identifier = 0X13,
    .name = "Braille System 40",
    .columns = 40,
    .statusCells = 0,
    .flags = MOD_FLG__CONFIGURABLE,
    .helpPage = 1
  }
  ,
  { .name = NULL }
};


#define MAX_STCELLS	5	/* hiest number of status cells */



/* This is the brltty braille mapping standard to Alva's mapping table.
 */
static TranslationTable outputTable;



/* Global variables */

static unsigned char *rawdata = NULL;	/* translated data to send to Braille */
static unsigned char *prevdata = NULL;	/* previously sent raw data */
static unsigned char StatusCells[MAX_STCELLS];		/* to hold status info */
static unsigned char PrevStatus[MAX_STCELLS];	/* to hold previous status */
static unsigned char NbStCells;	/* number of status cells */
static const ModelEntry *model;		/* points to terminal model config struct */
static int rewriteRequired = 0;		/* 1 if display need to be rewritten */
static int rewriteInterval;
static struct timeval rewriteTime;



/* Communication codes */

static const unsigned char BRL_START[] = {'\r', 0X1B, 'B'};	/* escape code to display braille */
#define BRL_START_LENGTH (sizeof(BRL_START)) 

static const unsigned char BRL_END[] = {'\r'};		/* to send after the braille sequence */
#define BRL_END_LENGTH (sizeof(BRL_END))

static const unsigned char BRL_ID[] = {0X1B, 'I', 'D', '='};
#define BRL_ID_LENGTH (sizeof(BRL_ID))
#define BRL_ID_SIZE (BRL_ID_LENGTH + 1)


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

#define KEY_STATUS1_A	0x01000	/* first lower status key */
#define KEY_STATUS1_B	0x02000	/* second lower status key */
#define KEY_STATUS1_C	0x03000	/* third lower status key */
#define KEY_STATUS1_D	0x04000	/* fourth lower status key */
#define KEY_STATUS1_E	0x05000	/* fifth lower status key */
#define KEY_STATUS1_F	0x06000	/* sixth lower status key */
#define KEY_ROUTING1	0x08000	/* lower cursor routing key set */

#define KEY_STATUS2_A	0x10000	/* first upper status key */
#define KEY_STATUS2_B	0x20000	/* second upper status key */
#define KEY_STATUS2_C	0x30000	/* third upper status key */
#define KEY_STATUS2_D	0x40000	/* fourth upper status key */
#define KEY_STATUS2_E	0x50000	/* fifth upper status key */
#define KEY_STATUS2_F	0x60000	/* sixth upper status key */
#define KEY_ROUTING2	0x80000	/* upper cursor routing key set */

#define KEY_SPK_F1	0x0100000
#define KEY_SPK_F2	0x0200000
#define KEY_SPK_UP	0x1000000
#define KEY_SPK_DOWN	0x2000000
#define KEY_SPK_LEFT	0x3000000
#define KEY_SPK_RIGHT	0x4000000

#define KEY_BRL_F1	0x0400000
#define KEY_BRL_F2	0x0800000
#define KEY_BRL_UP	0x5000000
#define KEY_BRL_DOWN	0x6000000
#define KEY_BRL_LEFT	0x7000000
#define KEY_BRL_RIGHT	0x8000000

#define KEY_TUMBLER1A	0x10000000	/* left end of left tumbler key */
#define KEY_TUMBLER1B	0x20000000	/* right end of left tumbler key */
#define KEY_TUMBLER2A	0x30000000	/* left end of right tumbler key */
#define KEY_TUMBLER2B	0x40000000	/* right end of right tumbler key */

/* first cursor routing offset on main display (old firmware only) */
#define KEY_ROUTING_OFFSET 168

#if ! ABT3_OLD_FIRMWARE
/* Index for new firmware protocol */
static int OperatingKeys[14] = {
  KEY_PROG, KEY_HOME, KEY_CURSOR,
  KEY_UP, KEY_LEFT, KEY_RIGHT, KEY_DOWN,
  KEY_CURSOR2, KEY_HOME2, KEY_PROG2,
  KEY_TUMBLER1A, KEY_TUMBLER1B, KEY_TUMBLER2A, KEY_TUMBLER2B
};
#endif /* ! ABT3_OLD_FIRMWARE */

static int StatusKeys1[6] = {
  KEY_STATUS1_A, KEY_STATUS1_B, KEY_STATUS1_C,
  KEY_STATUS1_D, KEY_STATUS1_E, KEY_STATUS1_F
};

static int StatusKeys2[6] = {
  KEY_STATUS2_A, KEY_STATUS2_B, KEY_STATUS2_C,
  KEY_STATUS2_D, KEY_STATUS2_E, KEY_STATUS2_F
};

static int SpeechPad[6] = {
  KEY_SPK_F1, KEY_SPK_UP, KEY_SPK_LEFT,
  KEY_SPK_DOWN, KEY_SPK_RIGHT, KEY_SPK_F2
};

static int BraillePad[6] = {
  KEY_BRL_F1, KEY_BRL_UP, KEY_BRL_LEFT,
  KEY_BRL_DOWN, KEY_BRL_RIGHT, KEY_BRL_F2
};

typedef struct {
  int (*openPort) (char **parameters, const char *device);
  int (*resetPort) (void);
  void (*closePort) (void);
  int (*awaitInput) (int milliseconds);
  int (*readPacket) (unsigned char *buffer, int length);
  int (*writePacket) (const unsigned char *buffer, int length, unsigned int *delay);
} InputOutputOperations;
static const InputOutputOperations *io;

#define PACKET_SIZE(count) (((count) * 2) + 4)
#define MAXIMUM_PACKET_SIZE PACKET_SIZE(0XFF)
static unsigned char inputBuffer[MAXIMUM_PACKET_SIZE];
static int inputUsed;

static int
verifyInputPacket (unsigned char *buffer, int *length) {
  int size = 0;
  if (logInputBuffer) LogBytes(LOG_DEBUG, "Input Buffer", inputBuffer, inputUsed);

#if ! ABT3_OLD_FIRMWARE
  while (inputUsed > 0) {
    if (inputBuffer[0] == 0X7F) {
      if (inputUsed < 3) break;
      if (inputBuffer[2] != 0X7E) goto unrecognized;
      if (inputUsed < 4) break;

      {
        int count = PACKET_SIZE(inputBuffer[3]);
        int complete = inputUsed >= count;
        int index;

        if (!complete) count = inputUsed;
        for (index=4; index<count; index+=2)
          if (inputBuffer[index] != 0X7E)
            goto unrecognized;

        if (complete) size = count;
        break;
      }
    }

    if ((inputBuffer[0] & 0XF0) == 0X70) {
      if (inputUsed >= 2) size = 2;
      break;
    }

    {
      int count = BRL_ID_LENGTH;
      if (inputUsed < count) count = inputUsed;
      if (memcmp(&inputBuffer[0], BRL_ID, count) != 0) goto unrecognized;
      if (inputUsed >= BRL_ID_SIZE) size = BRL_ID_SIZE;
      break;
    }

  unrecognized:
    LogBytes(LOG_WARNING, "Unrecognized Packet", inputBuffer, inputUsed);
    memcpy(&inputBuffer[0], &inputBuffer[1], --inputUsed);
  }
#else /* ABT3_OLD_FIRMWARE */
  if (inputUsed) size = 1;
#endif /* ! ABT3_OLD_FIRMWARE */

  if (!size) return 0;
  if (logInputPackets) LogBytes(LOG_DEBUG, "Input Packet", inputBuffer, size);

  if (*length < size) {
    LogPrint(LOG_DEBUG, "truncated input packet: %d < %d", *length, size);
    size = *length;
  } else {
    *length = size;
  }

  memcpy(buffer, &inputBuffer[0], size);
  memcpy(&inputBuffer[0], &inputBuffer[size], inputUsed-=size);
  return 1;
}

static int
writeFunction (BrailleDisplay *brl, unsigned char code) {
  unsigned char bytes[] = {0X1B, 'F', 'U', 'N', code, '\r'};
  return io->writePacket(bytes, sizeof(bytes), &brl->writeDelay);
}

static int
writeParameter (BrailleDisplay *brl, unsigned char parameter, unsigned char setting) {
  unsigned char bytes[] = {0X1B, 'P', 'A', 3, 0, parameter, setting, '\r'};
  return io->writePacket(bytes, sizeof(bytes), &brl->writeDelay);
}

#include "io_serial.h"
static SerialDevice *serialDevice = NULL;
static int serialCharactersPerSecond;

static int
openSerialPort (char **parameters, const char *device) {
  rewriteInterval = REWRITE_INTERVAL;

  if (!(serialDevice = serialOpenDevice(device))) return 0;

  return 1;
}

static int
resetSerialPort (void) {
  if (!serialRestartDevice(serialDevice, BAUDRATE)) return 0;
  serialCharactersPerSecond = BAUDRATE / 10;
  return 1;
}

static void
closeSerialPort (void) {
  if (serialDevice) {
    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }
}

static int
awaitSerialInput (int milliseconds) {
  return serialAwaitInput(serialDevice, milliseconds);
}

static int
readSerialPacket (unsigned char *buffer, int length) {
  while (1) {
    unsigned char byte;
    int count = serialReadData(serialDevice, &byte, 1, 0, 0);
    if (count < 1) {
      if (count == -1) LogError("serial read");
      return count;
    }
    inputBuffer[inputUsed++] = byte;
    if (verifyInputPacket(buffer, &length)) return length;
  }
}

static int
writeSerialPacket (const unsigned char *buffer, int length, unsigned int *delay) {
  if (logOutputPackets) LogBytes(LOG_DEBUG, "Output Packet", buffer, length);
  if (delay) *delay += (length * 1000 / serialCharactersPerSecond) + 1;
  return serialWriteData(serialDevice, buffer, length);
}

static const InputOutputOperations serialOperations = {
  openSerialPort, resetSerialPort, closeSerialPort,
  awaitSerialInput, readSerialPacket, writeSerialPacket
};

#ifdef ENABLE_USB_SUPPORT
#include "io_usb.h"

static UsbChannel *usb = NULL;

static int
openUsbPort (char **parameters, const char *device) {
  static const UsbChannelDefinition definitions[] = {
    { .vendor=0X06b0, .product=0X0001,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2
    }
    ,
    { .vendor=0 }
  };

  rewriteInterval = 0;
  if ((usb = usbFindChannel(definitions, (void *)device))) {
    usbBeginInput(usb->device, usb->definition.inputEndpoint, 8);
    return 1;
  }
  return 0;
}

static int
resetUsbPort (void) {
  return 1;
}

static void
closeUsbPort (void) {
  if (usb) {
    usbCloseChannel(usb);
    usb = NULL;
  }
}

static int
awaitUsbInput (int milliseconds) {
  return usbAwaitInput(usb->device, usb->definition.inputEndpoint, milliseconds);
}

static int
readUsbPacket (unsigned char *buffer, int length) {
  while (1) {
    unsigned char bytes[2];
    int count = usbReapInput(usb->device, usb->definition.inputEndpoint, bytes, sizeof(bytes), 0, 0);
    if (count == -1) {
      if (errno == EAGAIN) return 0;
      return count;
    }

    if (bytes[0] || bytes[1]) {
      memcpy(&inputBuffer[inputUsed], bytes, count);
      inputUsed += count;
      if (verifyInputPacket(buffer, &length)) return length;
    } else if (inputUsed) {
      LogBytes(LOG_WARNING, "Truncated Packet", inputBuffer, inputUsed);
      inputUsed = 0;
    }
  }
}

static int
writeUsbPacket (const unsigned char *buffer, int length, unsigned int *delay) {
  if (logOutputPackets) LogBytes(LOG_DEBUG, "Output Packet", buffer, length);
  return usbWriteEndpoint(usb->device, usb->definition.outputEndpoint, buffer, length, 1000);
}

static const InputOutputOperations usbOperations = {
  openUsbPort, resetUsbPort, closeUsbPort,
  awaitUsbInput, readUsbPacket, writeUsbPacket
};
#endif /* ENABLE_USB_SUPPORT */

int AL_writeData( unsigned char *data, int len )
{
  if (io->writePacket(data, len, NULL) == len) return 1;
  return 0;
}

static int
reallocateBuffer (unsigned char **buffer, int size) {
  void *address = realloc(*buffer, size);
  if (!address) return 0;
  *buffer = address;
  return 1;
}

static int
reallocateBuffers (BrailleDisplay *brl) {
  if (reallocateBuffer(&rawdata, brl->x*brl->y))
    if (reallocateBuffer(&prevdata, brl->x*brl->y))
      return 1;

  LogPrint(LOG_ERR, "Cannot allocate braille buffers.");
  return 0;
}

static int
updateDisplayConfiguration (BrailleDisplay *brl, const unsigned char *packet, int autodetecting) {
  int count = packet[3];

  if (count >= 3) {
    unsigned char cells = packet[9];
    if (cells != NbStCells) {
      NbStCells = cells;
      LogPrint(LOG_INFO, "Status cell count changed to %d.", NbStCells);
    }
  }

  if (count >= 4) {
    unsigned char columns = packet[11];
    if (columns != brl->x) {
      brl->x = columns;
      if (!autodetecting) {
        if (!reallocateBuffers(brl)) return 0;
        brl->resizeRequired = 1;
      }
    }
  }

  return 1;
}

static int
identifyModel (BrailleDisplay *brl, unsigned char identifier) {
  /* Find out which model we are connected to... */
  for (
    model = modelTable;
    model->name && (model->identifier != identifier);
    model++
  );

  if (!model->name) {
    /* Unknown model */
    LogPrint(LOG_ERR, "Detected unknown Alva model with ID %02X (hex).", identifier);
    return 0;
  }
  LogPrint(LOG_INFO, "Detected Alva %s: %d columns, %d status cells",
           model->name, model->columns, model->statusCells);

  /* Set model parameters... */
  brl->x = model->columns;
  brl->y = 1;
  brl->helpPage = model->helpPage;			/* initialise size of display */
  NbStCells = model->statusCells;

  if (model->flags & MOD_FLG__CONFIGURABLE) {
    BRLSYMBOL.firmness = brl_firmness;

    writeFunction(brl, 0X07);
    while (io->awaitInput(200)) {
      unsigned char packet[MAXIMUM_PACKET_SIZE];
      int count = io->readPacket(packet, sizeof(packet));
      if (count == -1) break;
      if (count == 0) continue;

      if ((packet[0] == 0X7F) && (packet[1] == 0X07)) {
        updateDisplayConfiguration(brl, packet, 1);
        break;
      }
    }

    writeFunction(brl, 0X0B);
  } else {
    BRLSYMBOL.firmness = NULL; 
  }

  /* Allocate space for buffers */
  if (!reallocateBuffers(brl)) return 0;
  rewriteRequired = 1;			/* To write whole display at first time */
  gettimeofday(&rewriteTime, NULL);

  return 1;
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  unsigned char ModelID;

  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(dots, outputTable);
  }

  if (isSerialDevice(&device)) {
    io = &serialOperations;
  } else

#ifdef ENABLE_USB_SUPPORT
  if (isUsbDevice(&device)) {
    io = &usbOperations;
  } else
#endif /* ENABLE_USB_SUPPORT */

  {
    unsupportedDevice(device);
    return 0;
  }
  inputUsed = 0;

  /* Open the Braille display device */
  if (io->openPort(parameters, device)) {
    if (io->resetPort()) {
      int probes = 0;
      while (writeFunction(brl, 0X06) != -1) {
        while (io->awaitInput(200)) {
          unsigned char packet[MAXIMUM_PACKET_SIZE];
          if (io->readPacket(packet, sizeof(packet)) > 0) {
            if (memcmp(packet, BRL_ID, BRL_ID_LENGTH) == 0) {
              ModelID = packet[BRL_ID_LENGTH];
              if (identifyModel(brl, ModelID)) {
                return 1;
              }
            }
          }
        }
        if (errno != EAGAIN) break;
        if (++probes == 3) break;
      }
    }

    io->closePort();
  }

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

  io->closePort();
}


static int WriteToBrlDisplay (BrailleDisplay *brl, int Start, int Len, unsigned char *Data)
{
  unsigned char outbuf[ BRL_START_LENGTH + 2 + Len + BRL_END_LENGTH ];
  int outsz = 0;

  memcpy( outbuf, BRL_START, BRL_START_LENGTH );
  outsz += BRL_START_LENGTH;
  outbuf[outsz++] = (char)Start;
  outbuf[outsz++] = (char)Len;
  memcpy( outbuf+outsz, Data, Len );
  outsz += Len;
  memcpy( outbuf+outsz, BRL_END, BRL_END_LENGTH );
  outsz += BRL_END_LENGTH;
  return io->writePacket(outbuf, outsz, &brl->writeDelay);
}

static void brl_writeWindow (BrailleDisplay *brl)
{
  int from, to;

  if (rewriteInterval) {
    struct timeval now;
    gettimeofday(&now, NULL);
    if (millisecondsBetween(&rewriteTime, &now) > rewriteInterval) rewriteRequired = 1;
    if (rewriteRequired) rewriteTime = now;
  }

  if (rewriteRequired) {
    /* We rewrite the whole display */
    from = 0;
    to = brl->x;
    rewriteRequired = 0;
  } else {
    /* We update only the display part that has been changed */
    from = 0;
    while ((from < brl->x) && (brl->buffer[from] == prevdata[from])) from++;

    to = brl->x - 1;
    while ((to > from) && (brl->buffer[to] == prevdata[to])) to--;
    to++;
  }

  if (from < to)			/* there is something different */ {
    int index;
    for (index=from; index<to; index++)
      rawdata[index - from] = outputTable[(prevdata[index] = brl->buffer[index])];
    WriteToBrlDisplay (brl, NbStCells+from, to-from, rawdata);
  }
}


static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *st)
{
  int i;

  /* Update status cells on braille display */
  if (memcmp (st, PrevStatus, NbStCells))	/* only if it changed */
    {
      for (i=0; i<NbStCells; ++i)
        StatusCells[i] = outputTable[(PrevStatus[i] = st[i])];
      WriteToBrlDisplay (brl, 0, NbStCells, StatusCells);
    }
}



static int
GetKey (BrailleDisplay *brl, unsigned int *Keys, unsigned int *Pos) {
  unsigned char packet[MAXIMUM_PACKET_SIZE];
  int length = io->readPacket(packet, sizeof(packet));
  if (length < 1) return length;
#if ! ABT3_OLD_FIRMWARE

  switch (packet[0]) {
    case 0X71: { /* operating keys and status keys */
      unsigned char key = packet[1];
      if (key <= 0X0D) {
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

    case 0X72: { /* primary (lower) routing keys */
      unsigned char key = packet[1];
      if (key <= 0X5F) {			/* make */
        *Pos = key;
        *Keys |= KEY_ROUTING1;
      } else {
        *Keys &= ~KEY_ROUTING1;
      }
      return 1;
    }

    case 0X75: { /* secondary (upper) routing keys */
      unsigned char key = packet[1];
      if (key <= 0X5F) {			/* make */
        *Pos = key;
        *Keys |= KEY_ROUTING2;
      } else {
        *Keys &= ~KEY_ROUTING2;
      }
      return 1;
    }

    case 0X77: { /* windows keys and speech keys */
      unsigned char key = packet[1];
      if (key <= 0X05) {
        *Keys |= SpeechPad[key];
      } else if ((key >= 0X80) && (key <= 0X85)) {
        *Keys &= ~SpeechPad[key - 0X80];
      } else if ((key >= 0X20) && (key <= 0X25)) {
        *Keys |= BraillePad[key - 0X20];
      } else if ((key >= 0XA0) && (key <= 0XA5)) {
        *Keys &= ~BraillePad[key - 0XA0];
      } else {
        *Keys = 0;
      }
      return 1;
    }

    case 0X7F:
      switch (packet[1]) {
        case 0X07: /* text/status cells reconfigured */
          if (!updateDisplayConfiguration(brl, packet, 0)) return -1;
          rewriteRequired = 1;
          return 0;

        case 0X0B: { /* display parameters reconfigured */
          int count = packet[3];

          if (count >= 8) {
            unsigned char frontKeys = packet[19];
            const unsigned char progKey = 0X02;
            if (frontKeys & progKey) {
              unsigned char newSetting = frontKeys & ~progKey;
              LogPrint(LOG_DEBUG, "Reconfiguring front keys: %02X -> %02X",
                       frontKeys, newSetting);
              writeParameter(brl, 6, newSetting);
            }
          }

          return 0;
        }
      }
      break;

    default:
      if (length >= BRL_ID_SIZE) {
        if (memcmp(packet, BRL_ID, BRL_ID_LENGTH) == 0) {
          /* The terminal has been turned off and back on. */
          if (!identifyModel(brl, packet[BRL_ID_LENGTH])) return -1;
          brl->resizeRequired = 1;
          return 0;
        }
      }

      break;
  }

#else /* ABT3_OLD_FIRMWARE */

  int key = packet[0];
  if (key < (KEY_ROUTING_OFFSET + brl->x + NbStCells)) {
    if (key >= (KEY_ROUTING_OFFSET + brl->x)) {
      /* status key */
      *Keys |= StatusKeys1[key - (KEY_ROUTING_OFFSET + brl->x)];
      return 1;
    }

    if (key >= KEY_ROUTING_OFFSET) {
      /* routing key */
      *Pos = key - KEY_ROUTING_OFFSET;
      *Keys |= KEY_ROUTING1;
      return 1;
    }

    if (!(key & 0X80)) {
      /* operating key */
      *Keys = key;		/* check comments where KEY_xxxx are defined */
      return 1;
    }
  }

#endif /* ! ABT3_OLD_FIRMWARE */
  LogBytes(LOG_WARNING, "Unexpected Packet", packet, length);
  return 0;
}


static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  static unsigned int CurrentKeys = 0, LastKeys = 0, ReleasedKeys = 0;
  static unsigned int RoutingPos = 0;
  int res = EOF;
  int ProcessKey = GetKey(brl, &CurrentKeys, &RoutingPos);

  if (ProcessKey < 0) {
    /* An input error occurred (perhaps disconnect of USB device) */
    res = BRL_CMD_RESTARTBRL;
  } else if (ProcessKey > 0) {
    res = BRL_CMD_NOOP;

    if (CurrentKeys > LastKeys) {
      /* These are the keys that should be processed when pressed */
      LastKeys = CurrentKeys;	/* we keep it until it is released */
      ReleasedKeys = 0;
      switch (brl->helpPage) {
        case 0: /* ABT and Delphi models */
          switch (CurrentKeys) {
            case KEY_HOME | KEY_UP:
              res = BRL_CMD_TOP;
              break;
            case KEY_HOME | KEY_DOWN:
              res = BRL_CMD_BOT;
              break;
            case KEY_UP:
              res = BRL_CMD_LNUP;
              break;
            case KEY_CURSOR | KEY_UP:
              res = BRL_CMD_ATTRUP;
              break;
            case KEY_DOWN:
              res = BRL_CMD_LNDN;
              break;
            case KEY_CURSOR | KEY_DOWN:
              res = BRL_CMD_ATTRDN;
              break;
            case KEY_LEFT:
              res = BRL_CMD_FWINLT;
              break;
            case KEY_HOME | KEY_LEFT:
              res = BRL_CMD_LNBEG;
              break;
            case KEY_CURSOR | KEY_LEFT:
              res = BRL_CMD_HWINLT;
              break;
            case KEY_PROG | KEY_LEFT:
              res = BRL_CMD_CHRLT;
              break;
            case KEY_RIGHT:
              res = BRL_CMD_FWINRT;
              break;
            case KEY_HOME | KEY_RIGHT:
              res = BRL_CMD_LNEND;
              break;
            case KEY_PROG | KEY_RIGHT:
              res = BRL_CMD_CHRRT;
              break;
            case KEY_CURSOR | KEY_RIGHT:
              res = BRL_CMD_HWINRT;
              break;
            case KEY_HOME | KEY_CURSOR | KEY_UP:
              res = BRL_CMD_PRDIFLN;
              break;
            case KEY_HOME | KEY_CURSOR | KEY_DOWN:
              res = BRL_CMD_NXDIFLN;
              break;
            case KEY_HOME | KEY_CURSOR | KEY_LEFT:
              res = BRL_CMD_MUTE;
              break;
            case KEY_HOME | KEY_CURSOR | KEY_RIGHT:
              res = BRL_CMD_SAY_LINE;
              break;
            case KEY_PROG | KEY_DOWN:
              res = BRL_CMD_FREEZE;
              break;
            case KEY_PROG | KEY_UP:
              res = BRL_CMD_INFO;
              break;
            case KEY_PROG | KEY_CURSOR | KEY_LEFT:
              res = BRL_CMD_BACK;
              break;
            case KEY_STATUS1_A:
              res = BRL_CMD_CAPBLINK;
              break;
            case KEY_STATUS1_B:
              res = BRL_CMD_CSRVIS;
              break;
            case KEY_STATUS1_C:
              res = BRL_CMD_CSRBLINK;
              break;
            case KEY_CURSOR | KEY_STATUS1_A:
              res = BRL_CMD_SIXDOTS;
              break;
            case KEY_CURSOR | KEY_STATUS1_B:
              res = BRL_CMD_CSRSIZE;
              break;
            case KEY_CURSOR | KEY_STATUS1_C:
              res = BRL_CMD_SLIDEWIN;
              break;
            case KEY_PROG | KEY_HOME | KEY_UP:
              res = BRL_CMD_PRPROMPT;
              break;
            case KEY_PROG | KEY_HOME | KEY_LEFT:
              res = BRL_CMD_RESTARTSPEECH;
              break;
            case KEY_PROG | KEY_HOME | KEY_RIGHT:
              res = BRL_CMD_SAY_BELOW;
              break;
            case KEY_ROUTING1:
              /* normal Cursor routing keys */
              res = BRL_BLK_ROUTE + RoutingPos;
              break;
            case KEY_PROG | KEY_ROUTING1:
              /* marking beginning of block */
              res = BRL_BLK_CUTBEGIN + RoutingPos;
              break;
            case KEY_HOME | KEY_ROUTING1:
              /* marking end of block */
              res = BRL_BLK_CUTRECT + RoutingPos;
              break;
            case KEY_PROG | KEY_HOME | KEY_DOWN:
              res = BRL_CMD_PASTE;
              break;

            /* See released keys handling below for these. There appears
             * to be a bug with at least some models when a routing key
             * is pressed in conjunction with more than one front key.
             */
            case KEY_PROG | KEY_HOME | KEY_ROUTING1:
            case KEY_HOME | KEY_CURSOR | KEY_ROUTING1:
              break;
          }
          break;

        case 1: /* Satellite models */
          switch (CurrentKeys) {
            case KEY_UP:
              res = BRL_CMD_LNUP;
              break;
            case KEY_DOWN:
              res = BRL_CMD_LNDN;
              break;
            case KEY_HOME | KEY_UP:
              res = BRL_CMD_TOP_LEFT;
              break;
            case KEY_HOME | KEY_DOWN:
              res = BRL_CMD_BOT_LEFT;
              break;
            case KEY_CURSOR | KEY_UP:
              res = BRL_CMD_TOP;
              break;
            case KEY_CURSOR | KEY_DOWN:
              res = BRL_CMD_BOT;
              break;
            case KEY_BRL_F1 | KEY_UP:
              res = BRL_CMD_PRDIFLN;
              break;
            case KEY_BRL_F1 | KEY_DOWN:
              res = BRL_CMD_NXDIFLN;
              break;
            case KEY_BRL_F2 | KEY_UP:
              res = BRL_CMD_ATTRUP;
              break;
            case KEY_BRL_F2 | KEY_DOWN:
              res = BRL_CMD_ATTRDN;
              break;

            case KEY_LEFT:
              res = BRL_CMD_FWINLT;
              break;
            case KEY_RIGHT:
              res = BRL_CMD_FWINRT;
              break;
            case KEY_TUMBLER2A:
            case KEY_HOME | KEY_LEFT:
              res = BRL_CMD_LNBEG;
              break;
            case KEY_TUMBLER2B:
            case KEY_HOME | KEY_RIGHT:
              res = BRL_CMD_LNEND;
              break;
            case KEY_CURSOR | KEY_LEFT:
              res = BRL_CMD_FWINLTSKIP;
              break;
            case KEY_CURSOR | KEY_RIGHT:
              res = BRL_CMD_FWINRTSKIP;
              break;
            case KEY_TUMBLER1A:
            case KEY_BRL_F1 | KEY_LEFT:
              res = BRL_CMD_CHRLT;
              break;
            case KEY_TUMBLER1B:
            case KEY_BRL_F1 | KEY_RIGHT:
              res = BRL_CMD_CHRRT;
              break;
            case KEY_BRL_F2 | KEY_LEFT:
              res = BRL_CMD_HWINLT;
              break;
            case KEY_BRL_F2 | KEY_RIGHT:
              res = BRL_CMD_HWINRT;
              break;

            case KEY_ROUTING2:
              res = BRL_BLK_DESCCHAR + RoutingPos;
              break;
            case KEY_ROUTING1:
              res = BRL_BLK_ROUTE + RoutingPos;
              break;
            case KEY_BRL_F1 | KEY_ROUTING2:
              res = BRL_BLK_CUTAPPEND + RoutingPos;
              break;
            case KEY_BRL_F1 | KEY_ROUTING1:
              res = BRL_BLK_CUTBEGIN + RoutingPos;
              break;
            case KEY_BRL_F2 | KEY_ROUTING2:
              res = BRL_BLK_CUTLINE + RoutingPos;
              break;
            case KEY_BRL_F2 | KEY_ROUTING1:
              res = BRL_BLK_CUTRECT + RoutingPos;
              break;
            case KEY_HOME | KEY_ROUTING2:
              res = BRL_BLK_SETMARK + RoutingPos;
              break;
            case KEY_HOME | KEY_ROUTING1:
              res = BRL_BLK_GOTOMARK + RoutingPos;
              break;
            case KEY_CURSOR | KEY_ROUTING2:
              res = BRL_BLK_PRINDENT + RoutingPos;
              break;
            case KEY_CURSOR | KEY_ROUTING1:
              res = BRL_BLK_NXINDENT + RoutingPos;
              break;

            case KEY_STATUS1_A:
              res = BRL_CMD_CSRVIS;
              break;
            case KEY_STATUS2_A:
              res = BRL_CMD_SKPIDLNS;
              break;
            case KEY_STATUS1_B:
              res = BRL_CMD_ATTRVIS;
              break;
            case KEY_STATUS2_B:
              res = BRL_CMD_DISPMD;
              break;
            case KEY_STATUS1_C:
              res = BRL_CMD_CAPBLINK;
              break;
            case KEY_STATUS2_C:
              res = BRL_CMD_SKPBLNKWINS;
              break;

            case KEY_BRL_LEFT:
              res = BRL_CMD_PREFMENU;
              break;
            case KEY_BRL_RIGHT:
              res = BRL_CMD_INFO;
              break;

            case KEY_BRL_F1 | KEY_BRL_LEFT:
              res = BRL_CMD_FREEZE;
              break;
            case KEY_BRL_F1 | KEY_BRL_RIGHT:
              res = BRL_CMD_SIXDOTS;
              break;

            case KEY_BRL_F2 | KEY_BRL_LEFT:
              res = BRL_CMD_PASTE;
              break;
            case KEY_BRL_F2 | KEY_BRL_RIGHT:
              res = BRL_CMD_CSRJMP_VERT;
              break;

            case KEY_BRL_UP:
              res = BRL_CMD_PRPROMPT;
              break;
            case KEY_BRL_DOWN:
              res = BRL_CMD_NXPROMPT;
              break;
            case KEY_BRL_F1 | KEY_BRL_UP:
              res = BRL_CMD_PRPGRPH;
              break;
            case KEY_BRL_F1 | KEY_BRL_DOWN:
              res = BRL_CMD_NXPGRPH;
              break;
            case KEY_BRL_F2 | KEY_BRL_UP:
              res = BRL_CMD_PRSEARCH;
              break;
            case KEY_BRL_F2 | KEY_BRL_DOWN:
              res = BRL_CMD_NXSEARCH;
              break;

            case KEY_SPK_LEFT:
              res = BRL_CMD_MUTE;
              break;
            case KEY_SPK_RIGHT:
              res = BRL_CMD_SAY_LINE;
              break;
            case KEY_SPK_UP:
              res = BRL_CMD_SAY_ABOVE;
              break;
            case KEY_SPK_DOWN:
              res = BRL_CMD_SAY_BELOW;
              break;
            case KEY_SPK_F2 | KEY_SPK_LEFT:
              res = BRL_CMD_SAY_SLOWER;
              break;
            case KEY_SPK_F2 | KEY_SPK_RIGHT:
              res = BRL_CMD_SAY_FASTER;
              break;
            case KEY_SPK_F2 | KEY_SPK_DOWN:
              res = BRL_CMD_SAY_SOFTER;
              break;
            case KEY_SPK_F2 | KEY_SPK_UP:
              res = BRL_CMD_SAY_LOUDER;
              break;
          }
          break;

      }
      res |= BRL_FLG_REPEAT_INITIAL | BRL_FLG_REPEAT_DELAY;
    } else {
      /* These are the keys that should be processed when released */
      if (!ReleasedKeys) {
        ReleasedKeys = LastKeys;
        switch (brl->helpPage) {
          case 0: /* ABT and Delphi models */
            switch (ReleasedKeys) {
              case KEY_HOME:
                res = BRL_CMD_TOP_LEFT;
                break;
              case KEY_CURSOR:
                res = BRL_CMD_RETURN;
                rewriteRequired = 1;	/* force rewrite of whole display */
                break;
              case KEY_PROG:
                res = BRL_CMD_HELP;
                break;
              case KEY_PROG | KEY_HOME:
                res = BRL_CMD_DISPMD;
                break;
              case KEY_HOME | KEY_CURSOR:
                res = BRL_CMD_CSRTRK;
                break;
              case KEY_PROG | KEY_CURSOR:
                res = BRL_CMD_PREFMENU;
                break;

              /* The following bindings really belong in the "pressed keys"
               * section, but it appears (at least on my ABT340) that a bogus
               * routing key press event is sometimes generated when at least
               * two front keys are already pressed. The correct routing key
               * event follows eventually, so the best workaround is to let
               * GetKey() overwrite RoutingPos until some key is released.
               * The exact bug as I observe it is that for routing keys 25-29
               * an extra press packet for routing key 35-31 is sent before
               * the correct one. In other words, if 25 <= x <= 29 then a key
               * press event is prepended with x = 30 + (30-x).
               */
              case KEY_PROG | KEY_HOME | KEY_ROUTING1:
                /* attribute for pointed character */
                res = BRL_BLK_DESCCHAR + RoutingPos;
                break;
              case KEY_HOME | KEY_CURSOR | KEY_ROUTING1:
                /* align window to pointed character */
                res = BRL_BLK_SETLEFT + RoutingPos;
                break;
            }
            break;

          case 1: /* Satellite models */
            switch (ReleasedKeys) {
              case KEY_HOME:
                res = BRL_CMD_BACK;
                break;
              case KEY_CURSOR:
                res = BRL_CMD_HOME;
                break;
              case KEY_HOME | KEY_CURSOR:
                res = BRL_CMD_CSRTRK;
                break;

              case KEY_BRL_F1:
                res = BRL_CMD_HELP;
                break;
              case KEY_BRL_F2:
                res = BRL_CMD_LEARN;
                break;
              case KEY_BRL_F1 | KEY_BRL_F2:
                res = BRL_CMD_RESTARTBRL;
                break;

              case KEY_SPK_F1:
                res = BRL_CMD_SPKHOME;
                break;
              case KEY_SPK_F2:
                res = BRL_CMD_AUTOSPEAK;
                break;
              case KEY_SPK_F1 | KEY_SPK_F2:
                res = BRL_CMD_RESTARTSPEECH;
                break;
            }
            break;

        }
      }
      LastKeys = CurrentKeys;
      if (!CurrentKeys)
        ReleasedKeys = 0;
    }
  }

  if (res == BRL_CMD_RESTARTBRL) {
    CurrentKeys = LastKeys = ReleasedKeys = 0;
    RoutingPos = 0;
  }

  return res;
}

static void
brl_firmness (BrailleDisplay *brl, BrailleFirmness setting) {
  writeParameter(brl, 3,
                 setting * 4 / BF_MAXIMUM);
}
