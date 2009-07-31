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
#include "brldefs-al.h"
#include "braille.h"

BEGIN_KEY_NAME_TABLE(routing1)
  KEY_SET_ENTRY(AL_SET_RoutingKeys1, "RoutingKey1"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(routing2)
  KEY_SET_ENTRY(AL_SET_RoutingKeys2, "RoutingKey2"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(status1)
  KEY_NAME_ENTRY(AL_KEY_STATUS1+0, "Status1A"),
  KEY_NAME_ENTRY(AL_KEY_STATUS1+1, "Status1B"),
  KEY_NAME_ENTRY(AL_KEY_STATUS1+2, "Status1C"),
  KEY_NAME_ENTRY(AL_KEY_STATUS1+3, "Status1D"),
  KEY_NAME_ENTRY(AL_KEY_STATUS1+4, "Status1E"),
  KEY_NAME_ENTRY(AL_KEY_STATUS1+5, "Status1F"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(status2)
  KEY_NAME_ENTRY(AL_KEY_STATUS2+0, "Status2A"),
  KEY_NAME_ENTRY(AL_KEY_STATUS2+1, "Status2B"),
  KEY_NAME_ENTRY(AL_KEY_STATUS2+2, "Status2C"),
  KEY_NAME_ENTRY(AL_KEY_STATUS2+3, "Status2D"),
  KEY_NAME_ENTRY(AL_KEY_STATUS2+4, "Status2E"),
  KEY_NAME_ENTRY(AL_KEY_STATUS2+5, "Status2F"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(abt_delphi)
  KEY_NAME_ENTRY(AL_KEY_Prog, "Prog"),
  KEY_NAME_ENTRY(AL_KEY_Home, "Home"),
  KEY_NAME_ENTRY(AL_KEY_Cursor, "Cursor"),

  KEY_NAME_ENTRY(AL_KEY_Up, "Up"),
  KEY_NAME_ENTRY(AL_KEY_Left, "Left"),
  KEY_NAME_ENTRY(AL_KEY_Right, "Right"),
  KEY_NAME_ENTRY(AL_KEY_Down, "Down"),

  KEY_NAME_ENTRY(AL_KEY_Cursor2, "Cursor2"),
  KEY_NAME_ENTRY(AL_KEY_Home2, "Home2"),
  KEY_NAME_ENTRY(AL_KEY_Prog2, "Prog2"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(satellite)
  KEY_NAME_ENTRY(AL_KEY_Home, "Home"),
  KEY_NAME_ENTRY(AL_KEY_Cursor, "Cursor"),

  KEY_NAME_ENTRY(AL_KEY_Up, "Up"),
  KEY_NAME_ENTRY(AL_KEY_Left, "Left"),
  KEY_NAME_ENTRY(AL_KEY_Right, "Right"),
  KEY_NAME_ENTRY(AL_KEY_Down, "Down"),

  KEY_NAME_ENTRY(AL_KEY_LeftPadF1, "LeftPadF1"),
  KEY_NAME_ENTRY(AL_KEY_LeftPadUp, "LeftPadUp"),
  KEY_NAME_ENTRY(AL_KEY_LeftPadLeft, "LeftPadLeft"),
  KEY_NAME_ENTRY(AL_KEY_LeftPadDown, "LeftPadDown"),
  KEY_NAME_ENTRY(AL_KEY_LeftPadRight, "LeftPadRight"),
  KEY_NAME_ENTRY(AL_KEY_LeftPadF2, "LeftPadF2"),

  KEY_NAME_ENTRY(AL_KEY_RightPadF1, "RightPadF1"),
  KEY_NAME_ENTRY(AL_KEY_RightPadUp, "RightPadUp"),
  KEY_NAME_ENTRY(AL_KEY_RightPadLeft, "RightPadLeft"),
  KEY_NAME_ENTRY(AL_KEY_RightPadDown, "RightPadDown"),
  KEY_NAME_ENTRY(AL_KEY_RightPadRight, "RightPadRight"),
  KEY_NAME_ENTRY(AL_KEY_RightPadF2, "RightPadF2"),

  KEY_NAME_ENTRY(AL_KEY_LeftTumblerLeft, "LeftTumblerLeft"),
  KEY_NAME_ENTRY(AL_KEY_LeftTumblerRight, "LeftTumblerRight"),
  KEY_NAME_ENTRY(AL_KEY_RightTumblerLeft, "RightTumblerLeft"),
  KEY_NAME_ENTRY(AL_KEY_RightTumblerRight, "RightTumblerRight"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(etouch)
  KEY_NAME_ENTRY(AL_KEY_ETouchLeftRear, "ETouchLeftRear"),
  KEY_NAME_ENTRY(AL_KEY_ETouchLeftFront, "ETouchLeftFront"),
  KEY_NAME_ENTRY(AL_KEY_ETouchRightRear, "ETouchRightRear"),
  KEY_NAME_ENTRY(AL_KEY_ETouchRightFront, "ETouchRightFront"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(smartpad)
  KEY_NAME_ENTRY(AL_KEY_SmartpadF1, "SmartpadF1"),
  KEY_NAME_ENTRY(AL_KEY_SmartpadF2, "SmartpadF2"),
  KEY_NAME_ENTRY(AL_KEY_SmartpadLeft, "SmartpadLeft"),
  KEY_NAME_ENTRY(AL_KEY_SmartpadEnter, "SmartpadEnter"),
  KEY_NAME_ENTRY(AL_KEY_SmartpadUp, "SmartpadUp"),
  KEY_NAME_ENTRY(AL_KEY_SmartpadDown, "SmartpadDown"),
  KEY_NAME_ENTRY(AL_KEY_SmartpadRight, "SmartpadRight"),
  KEY_NAME_ENTRY(AL_KEY_SmartpadF3, "SmartpadF3"),
  KEY_NAME_ENTRY(AL_KEY_SmartpadF4, "SmartpadF4"),

END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(thumb)
  KEY_NAME_ENTRY(AL_KEY_THUMB+0, "Thumb1"),
  KEY_NAME_ENTRY(AL_KEY_THUMB+1, "Thumb2"),
  KEY_NAME_ENTRY(AL_KEY_THUMB+2, "Thumb3"),
  KEY_NAME_ENTRY(AL_KEY_THUMB+3, "Thumb4"),
  KEY_NAME_ENTRY(AL_KEY_THUMB+4, "Thumb5"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(abt_delphi)
  KEY_NAME_TABLE(abt_delphi),
  KEY_NAME_TABLE(status1),
  KEY_NAME_TABLE(routing1),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(satellite)
  KEY_NAME_TABLE(satellite),
  KEY_NAME_TABLE(status1),
  KEY_NAME_TABLE(status2),
  KEY_NAME_TABLE(routing1),
  KEY_NAME_TABLE(routing2),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(bc)
  KEY_NAME_TABLE(etouch),
  KEY_NAME_TABLE(smartpad),
  KEY_NAME_TABLE(thumb),
  KEY_NAME_TABLE(routing1),
  KEY_NAME_TABLE(routing2),
END_KEY_NAME_TABLES

typedef struct {
  const char *keyBindings;
  const KeyNameEntry *const *keyNameTables;
} ModelTypeEntry;

typedef enum {
  MOD_TYPE_ABT,
  MOD_TYPE_Delphi,
  MOD_TYPE_Satellite,
  MOD_TYPE_BrailleController
} ModelType;

static const ModelTypeEntry modelTypeTable[] = {
  [MOD_TYPE_ABT] = {
    .keyBindings = "abt+delphi",
    .keyNameTables = keyNameTables_abt_delphi
  }
  ,
  [MOD_TYPE_Delphi] = {
    .keyBindings = "abt+delphi",
    .keyNameTables = keyNameTables_abt_delphi
  }
  ,
  [MOD_TYPE_Satellite] = {
    .keyBindings = "satellite",
    .keyNameTables = keyNameTables_satellite
  }
  ,
  [MOD_TYPE_BrailleController] = {
    .keyBindings = "bc",
    .keyNameTables = keyNameTables_bc
  }
};

typedef struct {
  const char *name;
  unsigned char identifier;
  unsigned char columns;
  unsigned char statusCells;
  unsigned char flags;
  unsigned char type;
} ModelEntry;
static const ModelEntry *model;		/* points to terminal model config struct */

#define MOD_FLAG_CONFIGURABLE 0X01

static const ModelEntry modelTable[] = {
  { .identifier = 0X00,
    .name = "ABT 320",
    .columns = 20,
    .statusCells = 3,
    .flags = 0,
    .type = MOD_TYPE_ABT
  }
  ,
  { .identifier = 0X01,
    .name = "ABT 340",
    .columns = 40,
    .statusCells = 3,
    .flags = 0,
    .type = MOD_TYPE_ABT
  }
  ,
  { .identifier = 0X02,
    .name = "ABT 340 Desktop",
    .columns = 40,
    .statusCells = 5,
    .flags = 0,
    .type = MOD_TYPE_ABT
  }
  ,
  { .identifier = 0X03,
    .name = "ABT 380",
    .columns = 80,
    .statusCells = 5,
    .flags = 0,
    .type = MOD_TYPE_ABT
  }
  ,
  { .identifier = 0X04,
    .name = "ABT 382 Twin Space",
    .columns = 80,
    .statusCells = 5,
    .flags = 0,
    .type = MOD_TYPE_ABT
  }
  ,
  { .identifier = 0X0A,
    .name = "Delphi 420",
    .columns = 20,
    .statusCells = 3,
    .flags = 0,
    .type = MOD_TYPE_Delphi
  }
  ,
  { .identifier = 0X0B,
    .name = "Delphi 440",
    .columns = 40,
    .statusCells = 3,
    .flags = 0,
    .type = MOD_TYPE_Delphi
  }
  ,
  { .identifier = 0X0C,
    .name = "Delphi 440 Desktop",
    .columns = 40,
    .statusCells = 5,
    .flags = 0,
    .type = MOD_TYPE_Delphi
  }
  ,
  { .identifier = 0X0D,
    .name = "Delphi 480",
    .columns = 80,
    .statusCells = 5,
    .flags = 0,
    .type = MOD_TYPE_Delphi
  }
  ,
  { .identifier = 0X0E,
    .name = "Satellite 544",
    .columns = 40,
    .statusCells = 3,
    .flags = MOD_FLAG_CONFIGURABLE,
    .type = MOD_TYPE_Satellite
  }
  ,
  { .identifier = 0X0F,
    .name = "Satellite 570 Pro",
    .columns = 66,
    .statusCells = 3,
    .flags = MOD_FLAG_CONFIGURABLE,
    .type = MOD_TYPE_Satellite
  }
  ,
  { .identifier = 0X10,
    .name = "Satellite 584 Pro",
    .columns = 80,
    .statusCells = 3,
    .flags = MOD_FLAG_CONFIGURABLE,
    .type = MOD_TYPE_Satellite
  }
  ,
  { .identifier = 0X11,
    .name = "Satellite 544 Traveller",
    .columns = 40,
    .statusCells = 3,
    .flags = MOD_FLAG_CONFIGURABLE,
    .type = MOD_TYPE_Satellite
  }
  ,
  { .identifier = 0X13,
    .name = "Braille System 40",
    .columns = 40,
    .statusCells = 0,
    .flags = MOD_FLAG_CONFIGURABLE,
    .type = MOD_TYPE_Satellite
  }
  ,
  { .name = NULL }
};

static const ModelEntry modelBC624 = {
  .identifier = 0X24,
  .name = "BC624",
  .columns = 24,
  .type = MOD_TYPE_BrailleController
};

static const ModelEntry modelBC640 = {
  .identifier = 0X40,
  .name = "BC640",
  .columns = 40,
  .type = MOD_TYPE_BrailleController
};

static const ModelEntry modelBC680 = {
  .identifier = 0X80,
  .name = "BC680",
  .columns = 80,
  .type = MOD_TYPE_BrailleController
};

typedef struct {
  int (*openPort) (const char *device);
  void (*closePort) (void);
  int (*awaitInput) (int milliseconds);
  int (*readBytes) (unsigned char *buffer, int length, int wait);
  int (*writeBytes) (const unsigned char *buffer, int length, unsigned int *delay);
  int (*getFeatureReport) (unsigned char report, unsigned char *buffer, int length);
} InputOutputOperations;
static const InputOutputOperations *io;

typedef struct {
  void (*initializeVariables) (void);
  int (*readPacket) (unsigned char *packet, int size);
  int (*updateConfiguration) (BrailleDisplay *brl, int autodetecting, const unsigned char *packet);
  int (*detectModel) (BrailleDisplay *brl);
  int (*readCommand) (BrailleDisplay *brl);
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

static unsigned char actualColumns;
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

  {
    const ModelTypeEntry *type = &modelTypeTable[model->type];
    brl->keyBindings = type->keyBindings;
    brl->keyNameTables = type->keyNameTables;
  }

  actualColumns = model->columns;
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

  actualColumns = textColumns;
  if (statusType == STATUS_FIRST) {
    statusOffset = 0;
    textOffset = statusOffset + statusColumns;
  } else if ((statusColumns = MIN(statusColumns, (actualColumns-1)/2))) {
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
updateConfiguration1 (BrailleDisplay *brl, int autodetecting, const unsigned char *packet) {
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
      if (model->flags & MOD_FLAG_CONFIGURABLE) {
        BRLSYMBOL.firmness = brl_firmness;

        if (!writeFunction1(brl, 0X07)) return 0;
        while (io->awaitInput(200)) {
          unsigned char packet[MAXIMUM_PACKET_SIZE];
          int count = protocol->readPacket(packet, sizeof(packet));

          if (count == -1) break;
          if (count == 0) continue;

          if ((packet[0] == 0X7F) && (packet[1] == 0X07)) {
            updateConfiguration1(brl, 1, packet);
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
        if (started) logPartialPacket(packet, offset);
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
        logIgnoredByte(byte);
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
        logShortPacket(packet, offset);
        offset = 0;
        length = 0;
        goto gotByte;
      }
    }

    if (offset < size) {
      packet[offset] = byte;
    } else {
      if (offset == size) logTruncatedPacket(packet, offset);
      logDiscardedByte(byte);
    }

    if (++offset == length) {
      if ((offset > size) || !packet[0]) {
        offset = 0;
        length = 0;
        continue;
      }

      logInputPacket(packet, offset);
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
readCommand1 (BrailleDisplay *brl) {
  unsigned char packet[MAXIMUM_PACKET_SIZE];
  int length;

  while ((length = protocol->readPacket(packet, sizeof(packet))) > 0) {
#if !ABT3_OLD_FIRMWARE
    unsigned char group = packet[0];
    unsigned char key = packet[1];
    int press = !(key & AL_KEY_RELEASE);
    key &= ~AL_KEY_RELEASE;

    switch (group) {
      case 0X71: /* operating keys and status keys */
        if (key <= 0X0D) {
          enqueueKeyEvent(AL_SET_NavigationKeys, key+AL_KEY_OPERATION, press);
          continue;
        }

        if ((key >= 0X20) && (key <= 0X25)) {
          enqueueKeyEvent(AL_SET_NavigationKeys, key-0X20+AL_KEY_STATUS1, press);
          continue;
        }

        if ((key >= 0X30) && (key <= 0X35)) {
          enqueueKeyEvent(AL_SET_NavigationKeys, key-0X30+AL_KEY_STATUS2, press);
          continue;
        }

        break;;

      case 0X72: /* primary (lower) routing keys */
        if (key <= 0X5F) {			/* make */
          enqueueKeyEvent(AL_SET_RoutingKeys1, key, press);
          continue;
        }

        break;

      case 0X75: /* secondary (upper) routing keys */
        if (key <= 0X5F) {			/* make */
          enqueueKeyEvent(AL_SET_RoutingKeys2, key, press);
          continue;
        }

        break;

      case 0X77: /* satellite keypads */
        if (key <= 0X05) {
          enqueueKeyEvent(AL_SET_NavigationKeys, key+AL_KEY_LEFT_PAD, press);
          continue;
        }

        if ((key >= 0X20) && (key <= 0X25)) {
          enqueueKeyEvent(AL_SET_NavigationKeys, key-0X20+AL_KEY_RIGHT_PAD, press);
          continue;
        }

        continue;

      case 0X7F:
        switch (packet[1]) {
          case 0X07: /* text/status cells reconfigured */
            if (!updateConfiguration1(brl, 0, packet)) return BRL_CMD_RESTARTBRL;
            continue;

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

            continue;
          }
        }

        break;

      default:
        if (length >= BRL_ID_SIZE) {
          if (memcmp(packet, BRL_ID, BRL_ID_LENGTH) == 0) {
            /* The terminal has been turned off and back on. */
            if (!identifyModel1(brl, packet[BRL_ID_LENGTH])) return BRL_CMD_RESTARTBRL;
            brl->resizeRequired = 1;
            continue;
          }
        }

        break;
    }
#else /* ABT3_OLD_FIRMWARE */
    const unsigned char routingBase = 0XA8;
    unsigned char byte = packet[0];

    if (!(byte & 0X80)) {
      static const unsigned char keys[7] = {
        AL_KEY_Up, AL_KEY_Cursor, AL_KEY_Home, AL_KEY_Prog, AL_KEY_Left, AL_KEY_Right, AL_KEY_Down
      };

      unsigned char pressedKeys[ARRAY_COUNT(keys)];
      unsigned char pressedCount = 0;

      const unsigned char set = AL_SET_NavigationKeys;
      const unsigned char *key = keys;
      unsigned char bit = 0X01;

      while (byte) {
        if (byte & bit) {
          byte &= ~bit;
          enqueueKeyEvent(set, *key, 1);
          pressedKeys[pressedCount++] = *key;
        }

        key += 1;
        bit <<= 1;
      }

      while (pressedCount) enqueueKeyEvent(set, pressedKeys[--pressedCount], 0);
      continue;
    }

    if (byte >= routingBase) {
      if ((byte -= routingBase) < brl->textColumns) {
        enqueueKeyEvent(AL_SET_RoutingKeys1, byte, 1);
        enqueueKeyEvent(AL_SET_RoutingKeys1, byte, 0);
        continue;
      }

      if ((byte -= brl->textColumns) < brl->statusColumns) {
        byte += AL_KEY_STATUS1;
        enqueueKeyEvent(AL_SET_NavigationKeys, byte, 1);
        enqueueKeyEvent(AL_SET_NavigationKeys, byte, 0);
        continue;
      }
    }
#endif /* ! ABT3_OLD_FIRMWARE */

    logUnexpectedPacket(packet, length);
  }

  return (length < 0)? BRL_CMD_RESTARTBRL: EOF;
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
  readPacket1, updateConfiguration1, detectModel1,
  readCommand1, writeBraille1
};

static uint32_t firmwareVersion2;
static unsigned char splitOffset2;

static void
initializeVariables2 (void) {
}

static int
interpretKeyEvent2 (BrailleDisplay *brl, unsigned char group, unsigned char key) {
  unsigned char release = group & 0X80;
  int press = !release;
  group &= ~release;

  switch (group) {
    case 0X01:
      switch (key) {
        case 0X01:
          if (!protocol->updateConfiguration(brl, 0, NULL)) return BRL_CMD_RESTARTBRL;
          return EOF;

        default:
          break;
      }
      break;

    {
      unsigned int base;
      unsigned int count;
      int secondary;

    case 0X71: /* thumb key */
      base = AL_KEY_THUMB;
      count = AL_KEYS_THUMB;
      secondary = 1;
      goto doKey;

    case 0X72: /* etouch key */
      base = AL_KEY_ETOUCH;
      count = AL_KEYS_ETOUCH;
      secondary = 0;
      goto doKey;

    case 0X73: /* smartpad key */
      base = AL_KEY_SMARTPAD;
      count = AL_KEYS_SMARTPAD;
      secondary = 1;
      goto doKey;

    doKey:
      if (secondary) {
        if ((key / count) == 1) {
          key -= count;
        }
      }

      if (key < count) {
        enqueueKeyEvent(AL_SET_NavigationKeys, base+key, press);
        return EOF;
      }
      break;
    }

    case 0X74: { /* routing key */
      unsigned char secondary = key & 0X80;
      key &= ~secondary;

      if (firmwareVersion2 < 0X011102)
        if (key >= splitOffset2)
          key -= splitOffset2;

      if (key >= textOffset) {
        if ((key -= textOffset) < brl->textColumns) {
          unsigned char set = secondary? AL_SET_RoutingKeys2: AL_SET_RoutingKeys1;
          enqueueKeyEvent(set, key, press);
          return EOF;
        }
      }
      break;
    }

    default:
      break;
  }

  LogPrint(LOG_WARNING, "unknown key: group=%02X key=%02X", group, key);
  return EOF;
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
        if (started) logPartialPacket(packet, offset);
        return result;
      }
    }

    if (offset == 0) {
      switch (byte) {
        case 0X1B:
          length = 2;
          break;

        default:
          logIgnoredByte(byte);
          continue;
      }
    }

    if (offset < size) {
      packet[offset] = byte;
    } else {
      if (offset == size) logTruncatedPacket(packet, offset);
      logDiscardedByte(byte);
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

      logInputPacket(packet, offset);
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
updateConfiguration2s (BrailleDisplay *brl, int autodetecting, const unsigned char *packet) {
  unsigned char buffer[0X20];

  if (getAttributes2s(0X45, buffer, sizeof(buffer))) {
    unsigned char textColumns = buffer[2];

    if (getAttributes2s(0X54, buffer, sizeof(buffer))) {
      unsigned char statusColumns = buffer[2];
      unsigned char statusSide = buffer[3];

      if (updateConfiguration(brl, autodetecting, textColumns, statusColumns,
                              (statusSide == 'R')? STATUS_RIGHT: STATUS_LEFT)) {
        splitOffset2 = (model->columns == actualColumns)? 0: actualColumns+1;
        return 1;
      }
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

  while ((model = *modelEntry++)) {
    if (model->identifier == identifier) {
      BRLSYMBOL.firmness = NULL;

      firmwareVersion2 = 0;
      if (getAttributes2s(0X56, packet, sizeof(packet))) {
        firmwareVersion2 |= (packet[4] << 16);
        firmwareVersion2 |= (packet[5] <<  8);
        firmwareVersion2 |= (packet[6] <<  0);

        if (setDefaultConfiguration(brl)) {
          if (updateConfiguration2s(brl, 1, NULL)) {
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
readCommand2s (BrailleDisplay *brl) {
  while (1) {
    unsigned char packet[MAXIMUM_PACKET_SIZE];
    int length = protocol->readPacket(packet, sizeof(packet));

    if (!length) return EOF;
    if (length < 0) return BRL_CMD_RESTARTBRL;

    switch (packet[0]) {
      case 0X1B:
        switch (packet[1]) {
          case 0X4B: /* K */ {
            int command = interpretKeyEvent2(brl, packet[2], packet[3]);
            if (command != EOF) return command;
            continue;
          }

          default:
            break;
        }
        break;

      default:
        break;
    }

    logUnexpectedPacket(packet, length);
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
  readPacket2s, updateConfiguration2s, detectModel2s,
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
        if (started) logPartialPacket(packet, offset);
        return result;
      }
    }

    if (offset == 0) {
      switch (byte) {
        case 0X04:
          length = 3;
          break;

        default:
          logIgnoredByte(byte);
          continue;
      }
    }

    if (offset < size) {
      packet[offset] = byte;
    } else {
      if (offset == size) logTruncatedPacket(packet, offset);
      logDiscardedByte(byte);
    }

    if (++offset == length) {
      if (offset > size) {
        offset = 0;
        length = 0;
        continue;
      }

      logInputPacket(packet, offset);
      return length;
    }
  }
}

static int
updateConfiguration2u (BrailleDisplay *brl, int autodetecting, const unsigned char *packet) {
  unsigned char buffer[0X20];
  int length = io->getFeatureReport(0X05, buffer, sizeof(buffer));

  if (length != -1) {
    int textColumns = brl->textColumns;
    int statusColumns = brl->statusColumns;
    int statusSide = 0;

    if (length >= 2) statusColumns = buffer[1];
    if (length >= 3) statusSide = buffer[2];
    if (length >= 7) textColumns = buffer[6];

    if (updateConfiguration(brl, autodetecting, textColumns, statusColumns,
                            statusSide? STATUS_RIGHT: STATUS_LEFT)) {
      splitOffset2 = model->columns - actualColumns;
      return 1;
    }
  }

  return 0;
}

static int
detectModel2u (BrailleDisplay *brl) {
  BRLSYMBOL.firmness = NULL;

  {
    unsigned char buffer[0X20];
    int length = io->getFeatureReport(0X09, buffer, sizeof(buffer));

    firmwareVersion2 = 0;
    if (length >= 6) firmwareVersion2 |= (buffer[5] << 16);
    if (length >= 7) firmwareVersion2 |= (buffer[6] <<  8);
    if (length >= 8) firmwareVersion2 |= (buffer[7] <<  0);
  }

  if (setDefaultConfiguration(brl))
    if (updateConfiguration2u(brl, 1, NULL))
      return 1;

  return 0;
}

static int
readCommand2u (BrailleDisplay *brl) {
  while (1) {
    unsigned char packet[MAXIMUM_PACKET_SIZE];
    int length = protocol->readPacket(packet, sizeof(packet));

    if (!length) return EOF;
    if (length < 0) return BRL_CMD_RESTARTBRL;

    switch (packet[0]) {
      case 0X04: {
        int command = interpretKeyEvent2(brl, packet[2], packet[1]);
        if (command != EOF) return command;
        continue;
      }

      default:
        break;
    }

    logUnexpectedPacket(packet, length);
  }
}

static int
writeBraille2u (BrailleDisplay *brl, const unsigned char *cells, int start, int count) {
  while (count > 0) {
    int length = MIN(count, 40);
    unsigned char packet[3 + length];
    unsigned char *byte = packet;

    *byte++ = 0X02;
    *byte++ = start;
    *byte++ = length;

    memcpy(byte, cells, length);
    byte += length;

    if (!writeBytes(packet, byte-packet, &brl->writeDelay)) return 0;
    cells += length;
    start += length;
    count -= length;
  }

  return 1;
}

static const ProtocolOperations protocol2uOperations = {
  initializeVariables2,
  readPacket2u, updateConfiguration2u, detectModel2u,
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
  logOutputPacket(buffer, length);
  if (delay) *delay += (length * 1000 / serialCharactersPerSecond) + 1;
  return serialWriteData(serialDevice, buffer, length);
}

static int
getSerialFeatureReport (unsigned char report, unsigned char *buffer, int length) {
  errno = ENOSYS;
  return -1;
}

static const InputOutputOperations serialOperations = {
  openSerialPort, closeSerialPort,
  awaitSerialInput, readSerialBytes, writeSerialBytes,
  getSerialFeatureReport
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
  logOutputPacket(buffer, length);

  if (usbChannel->definition.outputEndpoint) {
    return usbWriteEndpoint(usbChannel->device, usbChannel->definition.outputEndpoint, buffer, length, 1000);
  } else {
    return usbHidSetReport(usbChannel->device, usbChannel->definition.interface, buffer[0], buffer, length, 1000);
  }
}

static int
getUsbFeatureReport (unsigned char report, unsigned char *buffer, int length) {
  return usbHidGetFeature(usbChannel->device, usbChannel->definition.interface, report, buffer, length, 1000);
}

static const InputOutputOperations usbOperations = {
  openUsbPort, closeUsbPort,
  awaitUsbInput, readUsbBytes, writeUsbBytes,
  getUsbFeatureReport
};
#endif /* ENABLE_USB_SUPPORT */

#ifdef ENABLE_BLUETOOTH_SUPPORT
/* Bluetooth IO */
#include "io_bluetooth.h"
#include "io_misc.h"

static int bluetoothConnection = -1;

static int
openBluetoothPort (const char *device) {
  if ((bluetoothConnection = btOpenConnection(device, 1, 0)) != -1) {
    textRewriteInterval = REWRITE_INTERVAL;
    protocol = &protocol2sOperations;
    return 1;
  }

  return 0;
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
getBluetoothFeatureReport (unsigned char report, unsigned char *buffer, int length) {
  errno = ENOSYS;
  return -1;
}

static const InputOutputOperations bluetoothOperations = {
  openBluetoothPort, closeBluetoothPort,
  awaitBluetoothInput, readBluetoothBytes, writeBluetoothBytes,
  getBluetoothFeatureReport
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
  return protocol->readCommand(brl);
}

static void
brl_firmness (BrailleDisplay *brl, BrailleFirmness setting) {
  writeParameter1(brl, 3,
                 setting * 4 / BRL_FIRMNESS_MAXIMUM);
}
