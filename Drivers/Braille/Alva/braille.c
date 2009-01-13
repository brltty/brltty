/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* Alva/brlmain.cc - Braille display library for Alva braille displays
 * Copyright (C) 1995-2002 by Nicolas Pitre <nico@cam.org>
 * See the GNU Public license for details in the LICENSE-GPL file
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

#define BRL_STATUS_FIELDS sfAlphabeticCursorCoordinates, sfAlphabeticWindowCoordinates, sfStateLetter
#define BRL_HAVE_STATUS_CELLS
#define BRL_HAVE_FIRMNESS
#define BRLCONST
#include "brl_driver.h"
#include "braille.h"

static const int logInputPackets = 0;
static const int logOutputPackets = 0;

typedef struct {
  const char *name;
  unsigned char identifier;
  unsigned char columns;
  unsigned char statusCells;
  unsigned char flags;
  unsigned char helpPage;
} ModelEntry;
static const ModelEntry *model;		/* points to terminal model config struct */

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

static const ModelEntry modelBC624 = {
  .identifier = 0X24,
  .name = "BC624",
  .columns = 24,
  .helpPage = 2
};

static const ModelEntry modelBC640 = {
  .identifier = 0X40,
  .name = "BC640",
  .columns = 40,
  .helpPage = 2
};

static const ModelEntry modelBC680 = {
  .identifier = 0X80,
  .name = "BC680",
  .columns = 80,
  .helpPage = 2
};

typedef struct {
  int (*openPort) (const char *device);
  void (*closePort) (void);
  int (*awaitInput) (int milliseconds);
  int (*readBytes) (unsigned char *buffer, int length, int wait);
  int (*writeBytes) (const unsigned char *buffer, int length, unsigned int *delay);
  int (*getHidFeature) (unsigned char report, unsigned char *buffer, int length);
} InputOutputOperations;
static const InputOutputOperations *io;

typedef struct {
  void (*initializeVariables) (void);
  int (*readPacket) (unsigned char *packet, int size);
  int (*detectModel) (BrailleDisplay *brl);
  int (*readCommand) (BrailleDisplay *brl, BRL_DriverCommandContext context);
  int (*writeBraille) (BrailleDisplay *brl, const unsigned char *cells, int start, int count);
} ProtocolOperations;
static const ProtocolOperations *protocol;

typedef enum {
  STATUS_FIRST,
  STATUS_LEFT,
  STATUS_RIGHT
} StatusType;

static TranslationTable outputTable;
static unsigned char *previousText = NULL;
static unsigned char *previousStatus = NULL;

static unsigned char cellCount;
static unsigned char textOffset;
static unsigned char statusOffset;

static int textRewriteRequired = 0;
static int textRewriteInterval;
static struct timeval textRewriteTime;
static int statusRewriteRequired;

static int
readByte (unsigned char *byte, int wait) {
  int count = io->readBytes(byte, 1, wait);
  if (count > 0) return 1;

  if (count == 0) errno = EAGAIN;
  return 0;
}

static int
writeBytes (const unsigned char *buffer, int length, unsigned int *delay) {
  return io->writeBytes(buffer, length, delay) != -1;
}

static int
reallocateBuffer (unsigned char **buffer, int size) {
  void *address = realloc(*buffer, size);
  if (size && !address) return 0;
  *buffer = address;
  return 1;
}

static int
reallocateBuffers (BrailleDisplay *brl) {
  if (reallocateBuffer(&previousText, brl->textColumns*brl->textRows))
    if (reallocateBuffer(&previousStatus, brl->statusColumns*brl->statusRows))
      return 1;

  LogPrint(LOG_ERR, "cannot allocate braille buffers");
  return 0;
}

static int
setDefaultConfiguration (BrailleDisplay *brl) {
  LogPrint(LOG_INFO, "detected Alva %s: %d columns, %d status cells",
           model->name, model->columns, model->statusCells);

  brl->textColumns = model->columns;
  brl->textRows = 1;
  brl->statusColumns = model->statusCells;
  brl->statusRows = 1;
  brl->helpPage = model->helpPage;			/* initialise size of display */

  cellCount = model->columns;
  statusOffset = 0;
  textOffset = statusOffset + model->statusCells;
  textRewriteRequired = 1;			/* To write whole display at first time */
  statusRewriteRequired = 1;
  return reallocateBuffers(brl);
}

static int
updateConfiguration (BrailleDisplay *brl, int autodetecting, int textColumns, int statusColumns, StatusType statusType) {
  int changed = 0;
  int separator = 0;

  if (statusType == STATUS_FIRST) {
    statusOffset = 0;
    textOffset = statusOffset + statusColumns;
  } else if (statusColumns) {
    separator = 1;
    textColumns -= statusColumns + separator;

    switch (statusType) {
      case STATUS_LEFT:
        statusOffset = 0;
        textOffset = statusOffset + statusColumns + separator;
        break;

      case STATUS_RIGHT:
        textOffset = 0;
        statusOffset = textOffset + textColumns + separator;
        break;

      default:
        break;
    }
  } else {
    statusOffset = 0;
    textOffset = 0;
  }

  if (statusColumns != brl->statusColumns) {
    LogPrint(LOG_INFO, "status cell count changed to %d", statusColumns);
    brl->statusColumns = statusColumns;
    changed = 1;
  }

  if (textColumns != brl->textColumns) {
    LogPrint(LOG_INFO, "text column count changed to %d", textColumns);
    brl->textColumns = textColumns;
    if (!autodetecting) brl->resizeRequired = 1;
    changed = 1;
  }

  if (changed)
    if (!reallocateBuffers(brl))
      return 0;

  if (separator) {
    unsigned char cell = 0;
    if (!protocol->writeBraille(brl, &cell, MAX(textOffset, statusOffset)-1, 1)) return 0;
  }

  textRewriteRequired = 1;
  statusRewriteRequired = 1;
  return 1;
}

#define PACKET_SIZE(count) (((count) * 2) + 4)
#define MAXIMUM_PACKET_SIZE PACKET_SIZE(0XFF)
#define PACKET_BYTE(packet, index) ((packet)[PACKET_SIZE((index)) - 1])

static const unsigned char BRL_ID[] = {0X1B, 'I', 'D', '='};
#define BRL_ID_LENGTH (sizeof(BRL_ID))
#define BRL_ID_SIZE (BRL_ID_LENGTH + 1)

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
static const int OperatingKeys[14] = {
  KEY_PROG, KEY_HOME, KEY_CURSOR,
  KEY_UP, KEY_LEFT, KEY_RIGHT, KEY_DOWN,
  KEY_CURSOR2, KEY_HOME2, KEY_PROG2,
  KEY_TUMBLER1A, KEY_TUMBLER1B, KEY_TUMBLER2A, KEY_TUMBLER2B
};
#endif /* ! ABT3_OLD_FIRMWARE */

static const int StatusKeys1[6] = {
  KEY_STATUS1_A, KEY_STATUS1_B, KEY_STATUS1_C,
  KEY_STATUS1_D, KEY_STATUS1_E, KEY_STATUS1_F
};

static const int StatusKeys2[6] = {
  KEY_STATUS2_A, KEY_STATUS2_B, KEY_STATUS2_C,
  KEY_STATUS2_D, KEY_STATUS2_E, KEY_STATUS2_F
};

static const int SpeechPad[6] = {
  KEY_SPK_F1, KEY_SPK_UP, KEY_SPK_LEFT,
  KEY_SPK_DOWN, KEY_SPK_RIGHT, KEY_SPK_F2
};

static const int BraillePad[6] = {
  KEY_BRL_F1, KEY_BRL_UP, KEY_BRL_LEFT,
  KEY_BRL_DOWN, KEY_BRL_RIGHT, KEY_BRL_F2
};

static int
writeFunction1 (BrailleDisplay *brl, unsigned char code) {
  unsigned char bytes[] = {0X1B, 'F', 'U', 'N', code, '\r'};
  return writeBytes(bytes, sizeof(bytes), &brl->writeDelay);
}

static int
writeParameter1 (BrailleDisplay *brl, unsigned char parameter, unsigned char setting) {
  unsigned char bytes[] = {0X1B, 'P', 'A', 3, 0, parameter, setting, '\r'};
  return writeBytes(bytes, sizeof(bytes), &brl->writeDelay);
}

static int
updateConfiguration1 (BrailleDisplay *brl, const unsigned char *packet, int autodetecting) {
  int textColumns = brl->textColumns;
  int statusColumns = brl->statusColumns;
  int count = PACKET_BYTE(packet, 0);

  if (count >= 3) statusColumns = PACKET_BYTE(packet, 3);
  if (count >= 4) textColumns = PACKET_BYTE(packet, 4);
  return updateConfiguration(brl, autodetecting, textColumns, statusColumns, STATUS_FIRST);
}

static int
identifyModel1 (BrailleDisplay *brl, unsigned char identifier) {
  /* Find out which model we are connected to... */
  for (
    model = modelTable;
    model->name && (model->identifier != identifier);
    model += 1
  );

  if (model->name) {
    if (setDefaultConfiguration(brl)) {
      if (model->flags & MOD_FLG__CONFIGURABLE) {
        BRLSYMBOL.firmness = brl_firmness;

        if (!writeFunction1(brl, 0X07)) return 0;
        while (io->awaitInput(200)) {
          unsigned char packet[MAXIMUM_PACKET_SIZE];
          int count = protocol->readPacket(packet, sizeof(packet));

          if (count == -1) break;
          if (count == 0) continue;

          if ((packet[0] == 0X7F) && (packet[1] == 0X07)) {
            updateConfiguration1(brl, packet, 1);
            break;
          }
        }

        if (!writeFunction1(brl, 0X0B)) return 0;
      } else {
        BRLSYMBOL.firmness = NULL; 
      }

      return 1;
    }
  } else {
    LogPrint(LOG_ERR, "detected unknown Alva model with ID %02X (hex)", identifier);
  }

  return 0;
}

static void
initializeVariables1 (void) {
}

static int
readPacket1 (unsigned char *packet, int size) {
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    {
      int started = offset > 0;

      if (!readByte(&byte, started)) {
        int result = (errno == EAGAIN)? 0: -1;
        if (started) LogBytes(LOG_WARNING, "Partial Packet", packet, offset);
        return result;
      }
    }

  gotByte:
    if (offset == 0) {
#if ! ABT3_OLD_FIRMWARE
      if (byte == 0X7F) {
        length = PACKET_SIZE(0);
      } else if ((byte & 0XF0) == 0X70) {
        length = 2;
      } else if (byte == BRL_ID[0]) {
        length = BRL_ID_SIZE;
      } else if (!byte) {
        length = 2;
      } else {
        LogBytes(LOG_WARNING, "Ignored Byte", &byte, 1);
        continue;
      }
#else /* ABT3_OLD_FIRMWARE */
      length = 1;
#endif /* ! ABT3_OLD_FIRMWARE */
    } else {
      int unexpected = 0;

#if ! ABT3_OLD_FIRMWARE
      unsigned char type = packet[0];

      if (type == 0X7F) {
        if (offset == 3) length = PACKET_SIZE(byte);
        if (((offset % 2) == 0) && (byte != 0X7E)) unexpected = 1;
      } else if (type == BRL_ID[0]) {
        if ((offset < BRL_ID_LENGTH) && (byte != BRL_ID[offset])) unexpected = 1;
      } else if (!type) {
        if (byte) unexpected = 1;
      }
#else /* ABT3_OLD_FIRMWARE */
#endif /* ! ABT3_OLD_FIRMWARE */

      if (unexpected) {
        LogBytes(LOG_WARNING, "Short Packet", packet, offset);
        offset = 0;
        length = 0;
        goto gotByte;
      }
    }

    if (offset < size) {
      packet[offset] = byte;
    } else {
      if (offset == size) LogBytes(LOG_WARNING, "Truncated Packet", packet, offset);
      LogBytes(LOG_WARNING, "Discarded Byte", &byte, 1);
    }

    if (++offset == length) {
      if ((offset > size) || !packet[0]) {
        offset = 0;
        length = 0;
        continue;
      }

      if (logInputPackets) LogBytes(LOG_DEBUG, "Input Packet", packet, offset);
      return length;
    }
  }
}

static int
detectModel1 (BrailleDisplay *brl) {
  int probes = 0;

  while (writeFunction1(brl, 0X06)) {
    while (io->awaitInput(200)) {
      unsigned char packet[MAXIMUM_PACKET_SIZE];

      if (protocol->readPacket(packet, sizeof(packet)) > 0) {
        if (memcmp(packet, BRL_ID, BRL_ID_LENGTH) == 0) {
          if (identifyModel1(brl, packet[BRL_ID_LENGTH])) {
            return 1;
          }
        }
      }
    }

    if (errno != EAGAIN) break;
    if (++probes == 3) break;
  }

  return 0;
}

static int
getKey1 (BrailleDisplay *brl, unsigned int *Keys, unsigned int *Pos) {
  unsigned char packet[MAXIMUM_PACKET_SIZE];
  int length = protocol->readPacket(packet, sizeof(packet));
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
          if (!updateConfiguration1(brl, packet, 0)) return -1;
          return 0;

        case 0X0B: { /* display parameters reconfigured */
          int count = PACKET_BYTE(packet, 0);

          if (count >= 8) {
            unsigned char frontKeys = PACKET_BYTE(packet, 8);
            const unsigned char progKey = 0X02;
            if (frontKeys & progKey) {
              unsigned char newSetting = frontKeys & ~progKey;
              LogPrint(LOG_DEBUG, "Reconfiguring front keys: %02X -> %02X",
                       frontKeys, newSetting);
              writeParameter1(brl, 6, newSetting);
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
          if (!identifyModel1(brl, packet[BRL_ID_LENGTH])) return -1;
          brl->resizeRequired = 1;
          return 0;
        }
      }

      break;
  }

#else /* ABT3_OLD_FIRMWARE */

  int key = packet[0];
  if (key < (KEY_ROUTING_OFFSET + brl->textColumns + brl->statusColumns)) {
    if (key >= (KEY_ROUTING_OFFSET + brl->textColumns)) {
      /* status key */
      *Keys |= StatusKeys1[key - (KEY_ROUTING_OFFSET + brl->textColumns)];
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
readCommand1 (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  static unsigned int CurrentKeys = 0, LastKeys = 0, ReleasedKeys = 0;
  static unsigned int RoutingPos = 0;
  int res = EOF;
  int ProcessKey = getKey1(brl, &CurrentKeys, &RoutingPos);

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
               * getKey1() overwrite RoutingPos until some key is released.
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

static int
writeBraille1 (BrailleDisplay *brl, const unsigned char *cells, int start, int count) {
  static const unsigned char header[] = {'\r', 0X1B, 'B'};	/* escape code to display braille */
  static const unsigned char trailer[] = {'\r'};		/* to send after the braille sequence */

  unsigned char packet[sizeof(header) + 2 + count + sizeof(trailer)];
  unsigned char *byte = packet;

  memcpy(byte, header, sizeof(header));
  byte += sizeof(header);

  *byte++ = start;
  *byte++ = count;

  memcpy(byte, cells, count);
  byte += count;

  memcpy(byte, trailer, sizeof(trailer));
  byte += sizeof(trailer);

  return writeBytes(packet, byte-packet, &brl->writeDelay);
}

static const ProtocolOperations protocol1Operations = {
  initializeVariables1,
  readPacket1, detectModel1,
  readCommand1, writeBraille1
};

#define KEY2_THUMB_COUNT 5
#define KEY2_ETOUCH_COUNT 4
#define KEY2_SMARTPAD_COUNT 9

#define KEY2_THUMB_SHIFT 0
#define KEY2_ETOUCH_SHIFT (KEY2_THUMB_SHIFT + KEY2_THUMB_COUNT)
#define KEY2_SMARTPAD_SHIFT (KEY2_ETOUCH_SHIFT + KEY2_ETOUCH_COUNT)

#define KEY2(type,index) (1 << (KEY2_##type##_SHIFT + (index)))

#define KEY2_TH_1 KEY2(THUMB, 0)
#define KEY2_TH_2 KEY2(THUMB, 1)
#define KEY2_TH_3 KEY2(THUMB, 2)
#define KEY2_TH_4 KEY2(THUMB, 3)
#define KEY2_TH_5 KEY2(THUMB, 4)

#define KEY2_ET_1 KEY2(ETOUCH, 0)
#define KEY2_ET_2 KEY2(ETOUCH, 1)
#define KEY2_ET_3 KEY2(ETOUCH, 2)
#define KEY2_ET_4 KEY2(ETOUCH, 3)

#define KEY2_SP_1 KEY2(SMARTPAD, 0)
#define KEY2_SP_2 KEY2(SMARTPAD, 1)
#define KEY2_SP_L KEY2(SMARTPAD, 2)
#define KEY2_SP_E KEY2(SMARTPAD, 3)
#define KEY2_SP_U KEY2(SMARTPAD, 4)
#define KEY2_SP_D KEY2(SMARTPAD, 5)
#define KEY2_SP_R KEY2(SMARTPAD, 6)
#define KEY2_SP_3 KEY2(SMARTPAD, 7)
#define KEY2_SP_4 KEY2(SMARTPAD, 8)

static uint32_t firmwareVersion2;
static unsigned long pressedKeys2;
static unsigned long activeKeys2;

static void
initializeVariables2 (void) {
  pressedKeys2 = 0;
  activeKeys2 = 0;
}

static int
updateConfiguration2u (BrailleDisplay *brl, int autodetecting) {
  unsigned char buffer[0X20];
  int length = io->getHidFeature(0X05, buffer, sizeof(buffer));

  if (length != -1) {
    int textColumns = brl->textColumns;
    int statusColumns = brl->statusColumns;
    int statusSide = 0;

    if (length >= 2) statusColumns = buffer[1];
    if (length >= 3) statusSide = buffer[2];
    if (length >= 7) textColumns = buffer[6];

    if (updateConfiguration(brl, autodetecting, textColumns, statusColumns,
                            statusSide? STATUS_RIGHT: STATUS_LEFT))
      return 1;
  }

  return 0;
}

static int
interpretKeyCombination2 (void) {
  switch (activeKeys2) {
    case KEY2_SP_1: return BRL_CMD_HELP;
    case KEY2_SP_2: return BRL_CMD_LEARN;
    case KEY2_SP_3: return BRL_CMD_INFO;
    case KEY2_SP_4: return BRL_CMD_PREFMENU;

    case KEY2_SP_L: return BRL_CMD_SIXDOTS;
    case KEY2_SP_R: return BRL_CMD_CSRTRK;
    case KEY2_SP_U: return BRL_CMD_FREEZE;
    case KEY2_SP_D: return BRL_CMD_DISPMD;
    case KEY2_SP_E: return BRL_CMD_PASTE;

    case KEY2_TH_3: return BRL_CMD_HOME;

    case KEY2_TH_2: return BRL_CMD_LNUP;
    case KEY2_TH_4: return BRL_CMD_LNDN;
    case KEY2_TH_1: return BRL_CMD_FWINLT;
    case KEY2_TH_5: return BRL_CMD_FWINRT;

    case KEY2_TH_3 | KEY2_TH_2: return BRL_CMD_PRDIFLN;
    case KEY2_TH_3 | KEY2_TH_4: return BRL_CMD_NXDIFLN;
    case KEY2_TH_3 | KEY2_TH_1: return BRL_CMD_FWINLTSKIP;
    case KEY2_TH_3 | KEY2_TH_5: return BRL_CMD_FWINRTSKIP;

    case KEY2_SP_1 | KEY2_TH_3: return BRL_CMD_BACK;
    case KEY2_SP_1 | KEY2_TH_2: return BRL_CMD_ATTRUP;
    case KEY2_SP_1 | KEY2_TH_4: return BRL_CMD_ATTRDN;
    case KEY2_SP_1 | KEY2_TH_1: return BRL_CMD_TOP_LEFT;
    case KEY2_SP_1 | KEY2_TH_5: return BRL_CMD_BOT_LEFT;

    case KEY2_SP_4 | KEY2_TH_3: return BRL_CMD_CSRJMP_VERT;
    case KEY2_SP_4 | KEY2_TH_2: return BRL_CMD_PRPGRPH;
    case KEY2_SP_4 | KEY2_TH_4: return BRL_CMD_NXPGRPH;
    case KEY2_SP_4 | KEY2_TH_1: return BRL_CMD_PRPROMPT;
    case KEY2_SP_4 | KEY2_TH_5: return BRL_CMD_NXPROMPT;

    case KEY2_ET_1: return BRL_CMD_LNBEG;
    case KEY2_ET_2: return BRL_CMD_CHRLT;
    case KEY2_ET_3: return BRL_CMD_LNEND;
    case KEY2_ET_4: return BRL_CMD_CHRRT;

    case KEY2_SP_1 | KEY2_SP_L: return BRL_CMD_SAY_SLOWER;
    case KEY2_SP_1 | KEY2_SP_R: return BRL_CMD_SAY_FASTER;
    case KEY2_SP_1 | KEY2_SP_D: return BRL_CMD_SAY_SOFTER;
    case KEY2_SP_1 | KEY2_SP_U: return BRL_CMD_SAY_LOUDER;
    case KEY2_SP_1 | KEY2_SP_E: return BRL_CMD_AUTOSPEAK;
    case KEY2_SP_4 | KEY2_SP_L: return BRL_CMD_MUTE;
    case KEY2_SP_4 | KEY2_SP_R: return BRL_CMD_SAY_LINE;
    case KEY2_SP_4 | KEY2_SP_U: return BRL_CMD_SAY_ABOVE;
    case KEY2_SP_4 | KEY2_SP_D: return BRL_CMD_SAY_BELOW;
    case KEY2_SP_4 | KEY2_SP_E: return BRL_CMD_SPKHOME;
  }

  return EOF;
}

static int
interpretPrimaryRoutingKey2 (void) {
  switch (activeKeys2) {
    case 0: return BRL_BLK_ROUTE;

    case KEY2_SP_1: return BRL_BLK_CUTBEGIN;
    case KEY2_SP_2: return BRL_BLK_CUTAPPEND;
    case KEY2_SP_3: return BRL_BLK_CUTLINE;
    case KEY2_SP_4: return BRL_BLK_CUTRECT;

    case KEY2_SP_L: return BRL_BLK_PRINDENT;
    case KEY2_SP_R: return BRL_BLK_NXINDENT;
    case KEY2_SP_U: return BRL_BLK_PRDIFCHAR;
    case KEY2_SP_D: return BRL_BLK_NXDIFCHAR;
    case KEY2_SP_E: return BRL_BLK_SETLEFT;
  }

  return EOF;
}

static int
interpretSecondaryRoutingKey2 (void) {
  switch (activeKeys2) {
    case 0: return BRL_BLK_DESCCHAR;
  }

  return EOF;
}

static int
interpretKeyEvent2 (BrailleDisplay *brl, int *command, unsigned char group, unsigned char key) {
  unsigned char release = group & 0X80;
  group &= ~release;

  switch (group) {
    case 0X01:
      switch (key) {
        case 0X01:
          if (updateConfiguration2u(brl, 0)) return 0;
          *command = BRL_CMD_RESTARTBRL;
          return 1;

        default:
          break;
      }
      break;

    {
      unsigned int shift;
      unsigned int count;

    case 0X71: /* thumb key */
      shift = KEY2_THUMB_SHIFT;
      count = KEY2_THUMB_COUNT;
      goto doKey;

    case 0X72: /* etouch key */
      shift = KEY2_ETOUCH_SHIFT;
      count = KEY2_ETOUCH_COUNT;
      goto doKey;

    case 0X73: /* smartpad key */
      shift = KEY2_SMARTPAD_SHIFT;
      count = KEY2_SMARTPAD_COUNT;
      goto doKey;

    doKey:
      if (key < count) {
        unsigned long bit = 1 << (shift + key);

        if (release) {
          *command = interpretKeyCombination2();
          pressedKeys2 &= ~bit;
          activeKeys2 = 0;
          return 1;
        }

        pressedKeys2 |= bit;
        activeKeys2 = pressedKeys2;
        *command = interpretKeyCombination2();

        if (*command == EOF) {
          *command = BRL_CMD_NOOP;
        } else {
          *command |= BRL_FLG_REPEAT_DELAY;
        }

        return 1;
      }
      break;
    }

    case 0X74: { /* routing key */
      unsigned char secondary = key & 0X80;
      key &= ~secondary;

      if (firmwareVersion2 < 0X011102) {
        int splitpoint = model->columns - cellCount;

        if (key < splitpoint) {
          key += cellCount;
        } else {
          key -= splitpoint;
        }
      }

      if (key >= textOffset) {
        if ((key -= textOffset) < brl->textColumns) {
          if (release) {
            *command = EOF;
          } else {
            *command = secondary? interpretSecondaryRoutingKey2(): interpretPrimaryRoutingKey2();

            if (*command == EOF) {
              *command = BRL_CMD_NOOP;
            } else {
              *command |= key;
            }
          }

          activeKeys2 = 0;
          return 1;
        }
      }
      break;
    }

    default:
      break;
  }

  LogPrint(LOG_WARNING, "unknown key: group=%02X key=%02X", group, key);
  return 0;
}

static int
readPacket2s (unsigned char *packet, int size) {
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    {
      int started = offset > 0;

      if (!readByte(&byte, started)) {
        int result = (errno == EAGAIN)? 0: -1;
        if (started) LogBytes(LOG_WARNING, "Partial Packet", packet, offset);
        return result;
      }
    }

    if (offset == 0) {
      switch (byte) {
        case 0X1B:
          length = 2;
          break;

        default:
          LogBytes(LOG_WARNING, "Ignored Byte", &byte, 1);
          continue;
      }
    }

    if (offset < size) {
      packet[offset] = byte;
    } else {
      if (offset == size) LogBytes(LOG_WARNING, "Truncated Packet", packet, offset);
      LogBytes(LOG_WARNING, "Discarded Byte", &byte, 1);
    }

    if (offset == 1) {
      switch (byte) {
        case 0X3F: /* ? */ length =  3; break;
        case 0X45: /* E */ length =  3; break;
        case 0X4B: /* K */ length =  4; break;
        case 0X54: /* T */ length =  4; break;
        case 0X56: /* V */ length = 13; break;
      }
    }

    if (++offset == length) {
      if (offset > size) {
        offset = 0;
        length = 0;
        continue;
      }

      if (logInputPackets) LogBytes(LOG_DEBUG, "Input Packet", packet, offset);
      return length;
    }
  }
}

static int
getAttributes2s (unsigned char item, unsigned char *packet, int size) {
  unsigned char request[] = {0X1B, item, 0X3F};

  if (writeBytes(request, sizeof(request), NULL)) {
    while (io->awaitInput(200)) {
      int length = protocol->readPacket(packet, size);
      if (length <= 0) break;
      if ((packet[0] == 0X1B) && (packet[1] == item)) return 1;
    }
  }

  return 0;
}

static int
updateConfiguration2s (BrailleDisplay *brl, int autodetecting) {
  unsigned char packet[0X20];

  if (getAttributes2s(0X45, packet, sizeof(packet))) {
    unsigned char textColumns = packet[2];

    if (getAttributes2s(0X54, packet, sizeof(packet))) {
      unsigned char statusColumns = packet[2];
      unsigned char statusSide = packet[3];

      if (updateConfiguration(brl, autodetecting, textColumns, statusColumns,
                              (statusSide == 'R')? STATUS_RIGHT: STATUS_LEFT))
        return 1;
    }
  }

  return 0;
}

static int
identifyModel2s (BrailleDisplay *brl, unsigned char identifier) {
  static const ModelEntry *const models[] = {
    &modelBC624, &modelBC640, &modelBC680,
    NULL
  };

  unsigned char packet[0X20];
  const ModelEntry *const *modelEntry = models;

  while (*modelEntry) {
    if ((model = *modelEntry++)->identifier == identifier) {
      BRLSYMBOL.firmness = NULL;

      firmwareVersion2 = 0;
      if (getAttributes2s(0X56, packet, sizeof(packet))) {
        firmwareVersion2 |= (packet[4] << 16);
        firmwareVersion2 |= (packet[5] <<  8);
        firmwareVersion2 |= (packet[6] <<  0);

        if (setDefaultConfiguration(brl)) {
          if (updateConfiguration2s(brl, 1)) {
            return 1;
          }
        }
      }

      return 0;
    }
  }

  LogPrint(LOG_ERR, "detected unknown Alva model with ID %02X (hex)", identifier);
  return 0;
}

static int
detectModel2s (BrailleDisplay *brl) {
  int probes = 0;

  do {
    unsigned char packet[0X20];

    if (getAttributes2s(0X3F, packet, sizeof(packet))) {
      if (identifyModel2s(brl, packet[2])) {
        return 1;
      }
    } else if (errno != EAGAIN) {
      break;
    }
  } while (++probes < 3);

  return 0;
}

static int
readCommand2s (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  while (1) {
    unsigned char packet[MAXIMUM_PACKET_SIZE];
    int length = protocol->readPacket(packet, sizeof(packet));

    if (!length) return EOF;
    if (length < 0) return BRL_CMD_RESTARTBRL;

    switch (packet[0]) {
      case 0X1B:
        switch (packet[1]) {
          case 0X4B: /* K */ {
            int command;
            if (interpretKeyEvent2(brl, &command, packet[2], packet[3])) return command;
            continue;
          }

          default:
            break;
        }
        break;

      default:
        break;
    }

    LogBytes(LOG_WARNING, "Unexpected Packet", packet, length);
  }
}

static int
writeBraille2s (BrailleDisplay *brl, const unsigned char *cells, int start, int count) {
  unsigned char packet[4 + count];
  unsigned char *byte = packet;

  *byte++ = 0X1B;
  *byte++ = 0X42;
  *byte++ = start;
  *byte++ = count;

  memcpy(byte, cells, count);
  byte += count;

  return writeBytes(packet, byte-packet, &brl->writeDelay);
}

static const ProtocolOperations protocol2sOperations = {
  initializeVariables2,
  readPacket2s, detectModel2s,
  readCommand2s, writeBraille2s
};

static int
readPacket2u (unsigned char *packet, int size) {
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    {
      int started = offset > 0;

      if (!readByte(&byte, started)) {
        int result = (errno == EAGAIN)? 0: -1;
        if (started) LogBytes(LOG_WARNING, "Partial Packet", packet, offset);
        return result;
      }
    }

    if (offset == 0) {
      switch (byte) {
        case 0X04:
          length = 3;
          break;

        default:
          LogBytes(LOG_WARNING, "Ignored Byte", &byte, 1);
          continue;
      }
    }

    if (offset < size) {
      packet[offset] = byte;
    } else {
      if (offset == size) LogBytes(LOG_WARNING, "Truncated Packet", packet, offset);
      LogBytes(LOG_WARNING, "Discarded Byte", &byte, 1);
    }

    if (++offset == length) {
      if (offset > size) {
        offset = 0;
        length = 0;
        continue;
      }

      if (logInputPackets) LogBytes(LOG_DEBUG, "Input Packet", packet, offset);
      return length;
    }
  }
}

static int
detectModel2u (BrailleDisplay *brl) {
  BRLSYMBOL.firmness = NULL;

  {
    unsigned char buffer[0X20];
    int length = io->getHidFeature(0X09, buffer, sizeof(buffer));

    firmwareVersion2 = 0;
    if (length >= 6) firmwareVersion2 |= (buffer[5] << 16);
    if (length >= 7) firmwareVersion2 |= (buffer[6] <<  8);
    if (length >= 8) firmwareVersion2 |= (buffer[7] <<  0);
  }

  if (setDefaultConfiguration(brl))
    if (updateConfiguration2u(brl, 1))
      return 1;

  return 0;
}

static int
readCommand2u (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  while (1) {
    unsigned char packet[MAXIMUM_PACKET_SIZE];
    int length = protocol->readPacket(packet, sizeof(packet));

    if (!length) return EOF;
    if (length < 0) return BRL_CMD_RESTARTBRL;

    switch (packet[0]) {
      case 0X04: {
        int command;
        if (interpretKeyEvent2(brl, &command, packet[2], packet[1])) return command;
        continue;
      }

      default:
        break;
    }

    LogBytes(LOG_WARNING, "Unexpected Packet", packet, length);
  }
}

static int
writeBraille2u (BrailleDisplay *brl, const unsigned char *cells, int start, int count) {
  unsigned char packet[3 + count];
  unsigned char *byte = packet;

  *byte++ = 0X02;
  *byte++ = start;
  *byte++ = count;

  memcpy(byte, cells, count);
  byte += count;

  return writeBytes(packet, byte-packet, &brl->writeDelay);
}

static const ProtocolOperations protocol2uOperations = {
  initializeVariables2,
  readPacket2u,detectModel2u,
  readCommand2u, writeBraille2u
};

#include "io_serial.h"
static SerialDevice *serialDevice = NULL;
static int serialCharactersPerSecond;

static int
openSerialPort (const char *device) {
  if ((serialDevice = serialOpenDevice(device))) {
    if (serialRestartDevice(serialDevice, BAUDRATE)) {
      serialCharactersPerSecond = BAUDRATE / serialGetCharacterBits(serialDevice);
      textRewriteInterval = REWRITE_INTERVAL;
      protocol = &protocol1Operations;
      return 1;
    }
  }

  return 0;
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
readSerialBytes (unsigned char *buffer, int count, int wait) {
  const int timeout = 100;
  return serialReadData(serialDevice, buffer, count,
                        (wait? timeout: 0), timeout);
}

static int
writeSerialBytes (const unsigned char *buffer, int length, unsigned int *delay) {
  if (logOutputPackets) LogBytes(LOG_DEBUG, "Output Packet", buffer, length);
  if (delay) *delay += (length * 1000 / serialCharactersPerSecond) + 1;
  return serialWriteData(serialDevice, buffer, length);
}

static int
getSerialHidFeature (unsigned char report, unsigned char *buffer, int length) {
  errno = ENOSYS;
  return -1;
}

static const InputOutputOperations serialOperations = {
  openSerialPort, closeSerialPort,
  awaitSerialInput, readSerialBytes, writeSerialBytes,
  getSerialHidFeature
};

#ifdef ENABLE_USB_SUPPORT
#include "io_usb.h"

static UsbChannel *usbChannel = NULL;

static int
openUsbPort (const char *device) {
  static const UsbChannelDefinition definitions[] = {
    { /* Alva 5nn */
      .vendor=0X06b0, .product=0X0001,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2
    }
    ,
    { /* Alva BC624 */
      .vendor=0X0798, .product=0X0624,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0,
      .data=&modelBC624
    }
    ,
    { /* Alva BC640 */
      .vendor=0X0798, .product=0X0640,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0,
      .data=&modelBC640
    }
    ,
    { /* Alva BC680 */
      .vendor=0X0798, .product=0X0680,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0,
      .data=&modelBC680
    }
    ,
    { .vendor=0 }
  };

  if ((usbChannel = usbFindChannel(definitions, (void *)device))) {
    if (usbChannel->definition.outputEndpoint) {
      protocol = &protocol1Operations;
    } else {
      protocol = &protocol2uOperations;
    }

    model = usbChannel->definition.data;
    textRewriteInterval = 0;
    usbBeginInput(usbChannel->device, usbChannel->definition.inputEndpoint, 8);
    return 1;
  }
  return 0;
}

static void
closeUsbPort (void) {
  if (usbChannel) {
    usbCloseChannel(usbChannel);
    usbChannel = NULL;
  }
}

static int
awaitUsbInput (int milliseconds) {
  return usbAwaitInput(usbChannel->device, usbChannel->definition.inputEndpoint, milliseconds);
}

static int
readUsbBytes (unsigned char *buffer, int length, int wait) {
  const int timeout = 100;
  int count = usbReapInput(usbChannel->device,
                           usbChannel->definition.inputEndpoint,
                           buffer, length,
                           (wait? timeout: 0), timeout);
  if (count != -1) return count;
  if (errno == EAGAIN) return 0;
  return -1;
}

static int
writeUsbBytes (const unsigned char *buffer, int length, unsigned int *delay) {
  if (logOutputPackets) LogBytes(LOG_DEBUG, "Output Packet", buffer, length);

  if (usbChannel->definition.outputEndpoint) {
    return usbWriteEndpoint(usbChannel->device, usbChannel->definition.outputEndpoint, buffer, length, 1000);
  } else {
    return usbHidSetReport(usbChannel->device, usbChannel->definition.interface, buffer[0], buffer, length, 1000);
  }
}

static int
getUsbHidFeature (unsigned char report, unsigned char *buffer, int length) {
  return usbHidGetFeature(usbChannel->device, usbChannel->definition.interface, report, buffer, length, 1000);
}

static const InputOutputOperations usbOperations = {
  openUsbPort, closeUsbPort,
  awaitUsbInput, readUsbBytes, writeUsbBytes,
  getUsbHidFeature
};
#endif /* ENABLE_USB_SUPPORT */

#ifdef ENABLE_BLUETOOTH_SUPPORT
/* Bluetooth IO */
#include "io_bluetooth.h"
#include "io_misc.h"

static int bluetoothConnection = -1;

static int
openBluetoothPort (const char *device) {
  return (bluetoothConnection = btOpenConnection(device, 1, 0)) != -1;
}

static void
closeBluetoothPort (void) {
  close(bluetoothConnection);
  bluetoothConnection = -1;
}

static int
awaitBluetoothInput (int milliseconds) {
  return awaitInput(bluetoothConnection, milliseconds);
}

static int
readBluetoothBytes (unsigned char *buffer, int length, int wait) {
  const int timeout = 100;
  size_t offset = 0;
  return readChunk(bluetoothConnection,
                   buffer, &offset, length,
                   (wait? timeout: 0), timeout);
}

static int
writeBluetoothBytes (const unsigned char *buffer, int length, unsigned int *delay) {
  int count = writeData(bluetoothConnection, buffer, length);
  if (count != length) {
    if (count == -1) {
      LogError("Alva Bluetooth write");
    } else {
      LogPrint(LOG_WARNING, "trunccated bluetooth write: %d < %d", count, length);
    }
  }
  return count;
}

static int
getBluetoothHidFeature (unsigned char report, unsigned char *buffer, int length) {
  errno = ENOSYS;
  return -1;
}

static const InputOutputOperations bluetoothOperations = {
  openBluetoothPort, closeBluetoothPort,
  awaitBluetoothInput, readBluetoothBytes, writeBluetoothBytes,
  getBluetoothHidFeature
};
#endif /* ENABLE_BLUETOOTH_SUPPORT */

int
AL_writeData( unsigned char *data, int len ) {
  return writeBytes(data, len, NULL);
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
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

#ifdef ENABLE_BLUETOOTH_SUPPORT
  if (isBluetoothDevice(&device)) {
    io = &bluetoothOperations;
    protocol = &protocol2sOperations;
  } else
#endif /* ENABLE_BLUETOOTH_SUPPORT */

  {
    unsupportedDevice(device);
    return 0;
  }

  /* Open the Braille display device */
  if (io->openPort(device)) {
    protocol->initializeVariables();

    if (protocol->detectModel(brl)) {
      memset(&textRewriteTime, 0, sizeof(textRewriteTime));
      return 1;
    }

    io->closePort();
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  if (previousText) {
    free(previousText);
    previousText = NULL;
  }

  if (previousStatus) {
    free(previousStatus);
    previousStatus = NULL;
  }

  io->closePort();
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  int from = 0;
  int to = brl->textColumns * brl->textRows;

  if (textRewriteInterval) {
    struct timeval now;
    gettimeofday(&now, NULL);
    if (millisecondsBetween(&textRewriteTime, &now) > textRewriteInterval) textRewriteRequired = 1;
    if (textRewriteRequired) textRewriteTime = now;
  }

  if (textRewriteRequired) {
    textRewriteRequired = 0;
  } else {
    while ((from < to) && (brl->buffer[from] == previousText[from])) from++;
    while ((--to > from) && (brl->buffer[to] == previousText[to]));
    to++;
  }

  if (from < to) {
    unsigned char cells[to - from - 1];

    {
      int index;
      for (index=from; index<to; index++)
        cells[index - from] = outputTable[(previousText[index] = brl->buffer[index])];
    }

    if (!protocol->writeBraille(brl, cells, textOffset+from, to-from)) return 0;
  }
  return 1;
}

static int
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
  if (statusRewriteRequired || (memcmp(status, previousStatus, brl->statusColumns) != 0)) {
    unsigned char cells[brl->statusColumns];

    {
      int i;
      for (i=0; i<brl->statusColumns; ++i)
        cells[i] = outputTable[(previousStatus[i] = status[i])];
    }

    if (!protocol->writeBraille(brl, cells, statusOffset, brl->statusColumns)) return 0;
    statusRewriteRequired = 0;
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  return protocol->readCommand(brl, context);
}

static void
brl_firmness (BrailleDisplay *brl, BrailleFirmness setting) {
  writeParameter1(brl, 3,
                 setting * 4 / BRL_FIRMNESS_MAXIMUM);
}
