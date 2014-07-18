/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

/* TSI/braille.c - Braille display driver for TSI displays
 *
 * Written by Stéphane Doyon (s.doyon@videotron.ca)
 *
 * It attempts full support for Navigator 20/40/80 and Powerbraille 40/65/80.
 * It is designed to be compiled into BRLTTY version 3.5.
 *
 * History:
 * Version 2.74 apr2004: use message() to report low battery condition.
 * Version 2.73 jan2004: Fix key bindings for speech commands for PB80.
 *   Add CMD_SPKHOME to help.
 * Version 2.72 jan2003: brl->buffer now allocated by core.
 * Version 2.71: Added CMD_LEARN, BRL_CMD_NXPROMPT/CMD_PRPROMPT and CMD_SIXDOTS.
 * Version 2.70: Added CR_CUTAPPEND, BRL_BLK_CUTLINE, BRL_BLK_SETMARK, BRL_BLK_GOTOMARK
 *   and CR_SETLEFT. Changed binding for NXSEARCH.. Adjusted PB80 cut&paste
 *   bindings. Replaced CMD_CUT_BEG/CMD_CUT_END by CR_CUTBEGIN/CR_CUTRECT,
 *   and CMD_CSRJMP by CR_ROUTE+0. Adjusted cut_cursor for new cut&paste
 *   bindings (untested).
 * Version 2.61: Adjusted key bindings for preferences menu.
 * Version 2.60: Use TCSADRAIN when closing serial port. Slight API and
 *   name changes for BRLTTY 3.0. Argument to readbrl now ignore, instead
 *   of being validated. 
 * Version 2.59: Added bindings for CMD_LNBEG/LNEND.
 * Version 2.58: Added bindings for CMD_BACK and CR_MSGATTRIB.
 * Version 2.57: Fixed help screen/file for Nav80. We finally have a
 *   user who confirms it works!
 * Version 2.56: Added key binding for NXSEARCH.
 * Version 2.55: Added key binding for NXINDENT and NXBLNKLNS.
 * Version 2.54: Added key binding for switchvt.
 * Version 2.53: The IXOFF bit in the termios setting was inverted?
 * Version 2.52: Changed LOG_NOTICE to LOG_INFO. Was too noisy.
 * Version 2.51: Added CMD_RESTARTSPEECH.
 * Version 2.5: Added CMD_SPKHOME, sacrificed LNBEG and LNEND.
 * Version 2.4: Refresh display even if unchanged after every now and then so
 *   that it will clear up if it was garbled. Added speech key bindings (had
 *   to change a few bindings to make room). Added SKPEOLBLNK key binding.
 * Version 2.3: Reset serial port attributes at each detection attempt in
 *   initbrl. This should help BRLTTY recover if another application (such
 *   as kudzu) scrambles the serial port while BRLTTY is running.
 * Unnumbered version: Fixes for dynmically loading drivers (declare all
 *   non-exported functions and variables static).
 * Version 2.2beta3: Option to disable CTS checking. Apparently, Vario
 *   does not raise CTS when connected.
 * Version 2.2beta1: Exploring problems with emulators of TSI (PB40): BAUM
 *   and mdv mb408s. See if we can provide timing options for more flexibility.
 * Version 2.1: Help screen fix for new keys in preferences menu.
 * Version 2.1beta1: Less delays in writing braille to display for
 *   nav20/40 and pb40, delays still necessary for pb80 on probably for nav80.
 *   Additional routing keys for navigator. Cut&paste binding that combines
 *   routing key and normal key.
 * Version 2.0: Tested with Nav40 PB40 PB80. Support for functions added
 *   in BRLTTY 2.0: added key bindings for new fonctions (attributes and
 *   routing). Support for PB at 19200baud. Live detection of display, checks
 *   both at 9600 and 19200baud. RS232 wire monitoring. Ping when idle to 
 *   detect when display turned off and issue a CMD_RESTARTBRL.
 * Version 1.2 (not released) introduces support for PB65/80. Rework of key
 *   binding mechanism and readbrl(). Slight modifications to routing keys
 *   support, + corrections. May have broken routing key support for PB40.
 * Version 1.1 worked on nav40 and was reported to work on pb40.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>

#include "log.h"
#include "parse.h"
#include "io_generic.h"
#include "async_wait.h"
#include "timing.h"
#include "message.h"

typedef enum {
  PARM_HIGHBAUD
} DriverParameter;
#define BRLPARMS "highbaud"

#include "brl_driver.h"
#include "braille.h"
#include "brldefs-ts.h"

/* Braille display parameters that do not change */
#define BRLROWS 1		/* only one row on braille display */

/* Type of delay the display requires after sending it a command.
   0 -> no delay, 1 -> drain only, 2 -> drain + wait for SEND_DELAY. */
static unsigned char slowUpdate;

/* Whether multiple packets can be sent for a single update. */
static unsigned char noMultipleUpdates;

/* We periodicaly refresh the display even if nothing has changed, will clear
   out any garble... */
#define FULL_FRESHEN_EVERY 12 /* do a full update every nth writeWindow(). This
				 should be a little over every 0.5secs. */
static int fullFreshenEvery;

/* for routing keys */
static int must_init_oldstat = 1;

/* Definitions to avoid typematic repetitions of function keys other
   than movement keys */
#define NONREPEAT_TIMEOUT 300
#define READBRL_SKIP_TIME 300
static int lastcmd = EOF;
static TimeValue lastcmd_time, last_readbrl_time;
/* Those functions it is OK to repeat */
static int repeat_list[] =
{BRL_CMD_FWINRT, BRL_CMD_FWINLT, BRL_CMD_LNUP, BRL_CMD_LNDN, BRL_CMD_WINUP, BRL_CMD_WINDN,
 BRL_CMD_CHRLT, BRL_CMD_CHRRT, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT,
 BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP,
 BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN,
 BRL_CMD_CSRTRK, 0};

/* Stabilization delay after changing baud rate */
#define BAUD_DELAY (100)

/* Normal header for sending dots, with cursor always off */
static unsigned char BRL_SEND_HEAD[] = {0XFF, 0XFF, 0X04, 0X00, 0X99, 0X00};
#define DIM_BRL_SEND_FIXED 6
#define DIM_BRL_SEND 8
/* Two extra bytes for lenght and offset */
#define BRL_SEND_LENGTH 6
#define BRL_SEND_OFFSET 7

/* Description of reply to query */
#define IDENTITY_H1 0X00
#define IDENTITY_H2 0X05

/* Bit definition of key codes returned by the display */
/* Navigator and pb40 return 2bytes, pb65/80 returns 6. Each byte has a
   different specific mask/signature in the 3 most significant bits.
   Other bits indicate whether a specific key is pressed.
   See readbrl(). */

/* We combine all key bits into one 32bit int. Each byte is masked by the
 * corresponding "mask" to extract valid bits then those are shifted by
 * "shift" and or'ed into the 32bits "code".
 */

/* bits to take into account when checking each byte's signature */
#define KEYS_BYTE_SIGNATURE_MASK 0XE0

/* how we describe each byte */
typedef struct {
  unsigned char signature; /* it's signature */
  unsigned char mask; /* bits that do represent keys */
  unsigned char shift; /* where to shift them into "code" */
} KeysByteDescriptor;

/* Description of bytes for navigator and pb40. */
static const KeysByteDescriptor keysDescriptor_Navigator[] = {
  {.signature=0X60, .mask=0X1F, .shift=0},
  {.signature=0XE0, .mask=0X1F, .shift=5}
};

/* Description of bytes for pb65/80 */
static const KeysByteDescriptor keysDescriptor_PowerBraille[] = {
  {.signature=0X40, .mask=0X0F, .shift=10},
  {.signature=0XC0, .mask=0X0F, .shift=14},
  {.signature=0X20, .mask=0X05, .shift=18},
  {.signature=0XA0, .mask=0X05, .shift=21},
  {.signature=0X60, .mask=0X1F, .shift=24},
  {.signature=0XE0, .mask=0X1F, .shift=5}
};

/* Symbolic labels for keys
   Each key has it's own bit in "code". Key combinations are ORs. */

/* For navigator and pb40 */
/* bits from byte 1: navigator right pannel keys, pb right rocker +round button
   + display forward/backward controls on the top of the display */
#define KEY_BLEFT  (1<<0)
#define KEY_BUP	   (1<<1)
#define KEY_BRIGHT (1<<2)
#define KEY_BDOWN  (1<<3)
#define KEY_BROUND (1<<4)
/* bits from byte 2: navigator's left pannel; pb's left rocker and round
   button; pb cannot produce CLEFT and CRIGHT. */
#define KEY_CLEFT  (1<<5)
#define KEY_CUP	   (1<<6)
#define KEY_CRIGHT (1<<7)
#define KEY_CDOWN  (1<<8)
#define KEY_CROUND (1<<9)

/* For pb65/80 */
/* Bits from byte 5, could be just renames of byte 1 from navigator, but
   we want to distinguish BAR1-2 from BUP/BDOWN. */
#define KEY_BAR1   (1<<24)
#define KEY_R2UP   (1<<25)
#define KEY_BAR2   (1<<26)
#define KEY_R2DN   (1<<27)
#define KEY_CNCV   (1<<28)
/* Bits from byte 6, are just renames of byte 2 from navigator */
#define KEY_BUT1   (1<<5)
#define KEY_R1UP   (1<<6)
#define KEY_BUT2   (1<<7)
#define KEY_R1DN   (1<<8)
#define KEY_CNVX   (1<<9)
/* bits from byte 1: left rocker switches */
#define KEY_S1UP   (1<<10)
#define KEY_S1DN   (1<<11)
#define KEY_S2UP   (1<<12)
#define KEY_S2DN   (1<<13)
/* bits from byte 2: right rocker switches */
#define KEY_S3UP   (1<<14)
#define KEY_S3DN   (1<<15)
#define KEY_S4UP   (1<<16)
#define KEY_S4DN   (1<<17)
/* Special mask: switches are special keys to distinguish... */
#define KEY_SWITCHMASK (KEY_S1UP|KEY_S1DN | KEY_S2UP|KEY_S2DN \
			| KEY_S3UP|KEY_S3DN | KEY_S4UP|KEY_S4DN)
/* bits from byte 3: rightmost forward bars from display top */
#define KEY_BAR3   (1<<18)
  /* one unused bit */
#define KEY_BAR4   (1<<20)
/* bits from byte 4: two buttons on the top, right side (left side buttons
   are mapped in byte 6) */
#define KEY_BUT3   (1<<21)
  /* one unused bit */
#define KEY_BUT4   (1<<23)

/* Some special case input codes */
/* input codes signaling low battery power (2bytes) */
#define BATTERY_H1 0x00
#define BATTERY_H2 0x01
/* Sensor switches/cursor routing keys information (2bytes header) */
#define SWITCHES_H1 0x00
#define SWITCHES_H2 0x08

/* Definitions for sensor switches/cursor routing keys */
#define SW_NVERT 4 /* vertical switches. unused info. 4bytes to skip */
#define SW_MAXHORIZ 11	/* bytes of horizontal info (81cells
			   / 8bits per byte = 11bytes) */
/* actual total number of switch information bytes depending on size
 * of display (40/65/81) including 4 bytes of unused vertical switches
 */
#define SWITCH_BYTES_40 9
#define SWITCH_BYTES_80 14
#define SWITCH_BYTES_81 15

/* Global variables */

typedef struct {
  const char *modelName;
  const char *keyBindings;

  unsigned char switchBytes;
  signed char lastRoutingKey;

  unsigned hasRoutingKeys:1;
  unsigned slowUpdate:2;
  unsigned highBaudSupported:1;
  unsigned isPB40:1;
} ModelEntry;

static const ModelEntry modelNavigator20 = {
  .modelName = "Navigator 20",

  .switchBytes = SWITCH_BYTES_40,
  .lastRoutingKey = 19,

  .keyBindings = "nav20_nav40"
};

static const ModelEntry modelNavigator40 = {
  .modelName = "Navigator 40",

  .switchBytes = SWITCH_BYTES_40,
  .lastRoutingKey = 39,

  .slowUpdate = 1,

  .keyBindings = "nav20_nav40"
};

static const ModelEntry modelNavigator80 = {
  .modelName = "Navigator 80",

  .switchBytes = SWITCH_BYTES_80,
  .lastRoutingKey = 79,

  .hasRoutingKeys = 1,
  .slowUpdate = 2,

  .keyBindings = "nav80"
};

static const ModelEntry modelPowerBraille40 = {
  .modelName = "Power Braille 40",

  .switchBytes = SWITCH_BYTES_40,
  .lastRoutingKey = 39,

  .hasRoutingKeys = 1,
  .highBaudSupported = 1,
  .isPB40 = 1,

  .keyBindings = "pb40"
};

static const ModelEntry modelPowerBraille65 = {
  .modelName = "Power Braille 65",

  .switchBytes = SWITCH_BYTES_81,
  .lastRoutingKey = 64,

  .hasRoutingKeys = 1,
  .slowUpdate = 2,
  .highBaudSupported = 1,

  .keyBindings = "pb65_pb81"
};

static const ModelEntry modelPowerBraille80 = {
  .modelName = "Power Braille 80",

  .switchBytes = SWITCH_BYTES_81,
  .lastRoutingKey = 80,

  .hasRoutingKeys = 1,
  .slowUpdate = 2,
  .highBaudSupported = 1,

  .keyBindings = "pb65_pb81"
};

typedef enum {
  IPT_IDENTITY,
  IPT_SWITCHES,
  IPT_BATTERY,
  IPT_KEYS
} InputPacketType;

typedef struct {
  union {
    unsigned char bytes[1];

    struct {
      unsigned char header[2];
      unsigned char columns;
      unsigned char dots;
      char version[4];
      unsigned char checksum[4];
    } identity;

    struct {
      unsigned char header[2];
      unsigned char count;
      unsigned char vertical[4];
      unsigned char horizontal[0X100 - 4];
    } switches;

    unsigned char keys[6];
  } fields;

  InputPacketType type;

  union {
    struct {
      unsigned char count;
    } switches;

    struct {
      const KeysByteDescriptor *descriptor;
      unsigned char count;
    } keys;
  } data;
} InputPacket;

static SerialParameters serialParameters = {
  SERIAL_DEFAULT_PARAMETERS
};

#define LOW_BAUD     4800
#define NORMAL_BAUD  9600
#define HIGH_BAUD   19200

static const ModelEntry *model;

static unsigned char *rawdata,	/* translated data to send to display */
                     *prevdata, /* previous data sent */
                     *dispbuf;
static unsigned char brl_cols;		/* Number of cells available for text */
static int ncells;              /* Total number of cells on display: this is
				   brl_cols cells + 1 status cell on PB80. */
static char hardwareVersion[3]; /* version of the hardware */

static ssize_t
writeBytes (BrailleDisplay *brl, const void *data, size_t size) {
  brl->writeDelay += slowUpdate * 24;
  return writeBraillePacket(brl, NULL, data, size);
}

static BraillePacketVerifierResult
verifyPacket1 (
  BrailleDisplay *brl,
  const unsigned char *bytes, size_t size,
  size_t *length, void *data
) {
  InputPacket *packet = data;
  const off_t index = size - 1;
  const unsigned char byte = bytes[index];

  if (size == 1) {
    switch (byte) {
      case IDENTITY_H1:
        packet->type = IPT_IDENTITY;
        *length = 2;
        break;

      default:
        if ((byte & KEYS_BYTE_SIGNATURE_MASK) == keysDescriptor_Navigator[0].signature) {
          packet->data.keys.descriptor = keysDescriptor_Navigator;
          packet->data.keys.count = ARRAY_COUNT(keysDescriptor_Navigator);
          goto isKeys;
        }

        if ((byte & KEYS_BYTE_SIGNATURE_MASK) == keysDescriptor_PowerBraille[0].signature) {
          packet->data.keys.descriptor = keysDescriptor_PowerBraille;
          packet->data.keys.count = ARRAY_COUNT(keysDescriptor_PowerBraille);
          goto isKeys;
        }

        return BRL_PVR_INVALID;

      isKeys:
        packet->type = IPT_KEYS;
        *length = packet->data.keys.count;
        break;
    }
  } else {
    switch (packet->type) {
      case IPT_IDENTITY:
        if (size == 2) {
          switch (byte) {
            case IDENTITY_H2:
              *length = sizeof(packet->fields.identity);
              break;

            case SWITCHES_H2:
              packet->type = IPT_SWITCHES;
              *length = 3;
              break;

            case BATTERY_H2:
              packet->type = IPT_BATTERY;
              break;

            default:
              return BRL_PVR_INVALID;
          }
        }
        break;

      case IPT_SWITCHES:
        if (size == 3) {
          packet->data.switches.count = byte;
          *length += packet->data.switches.count;
        }
        break;

      case IPT_KEYS:
        if ((byte & KEYS_BYTE_SIGNATURE_MASK) != packet->data.keys.descriptor[index].signature) return BRL_PVR_INVALID;
        break;

      default:
        break;
    }
  }

  return BRL_PVR_INCLUDE;
}

static size_t
readPacket (BrailleDisplay *brl, InputPacket *packet) {
  return readBraillePacket(brl, NULL, &packet->fields, sizeof(packet->fields), verifyPacket1, packet);
}

static int
queryDisplay (BrailleDisplay *brl, InputPacket *reply) {
  static const unsigned char request[] = {0xFF, 0xFF, 0x0A};

  if (writeBytes(brl, request, sizeof(request))) {
    if (gioAwaitInput(brl->gioEndpoint, 100)) {
      size_t count = readPacket(brl, reply);

      if (count > 0) {
        if (reply->type == IPT_IDENTITY) return 1;
        logUnexpectedPacket(reply->fields.bytes, count);
      }
    } else {
      logMessage(LOG_DEBUG, "no response");
    }
  }

  return 0;
}


static int
resetTypematic (BrailleDisplay *brl) {
  static const unsigned char request[] = {
    0XFF, 0XFF, 0X0D, BRL_TYPEMATIC_DELAY, BRL_TYPEMATIC_REPEAT
  };

  return writeBytes(brl, request, sizeof(request));
}

static int
setBaud (BrailleDisplay *brl, int baud) {
  logMessage(LOG_DEBUG, "trying with %d baud", baud);
  serialParameters.baud = baud;
  return gioReconfigureResource(brl->gioEndpoint, &serialParameters);
}

static int
changeBaud (BrailleDisplay *brl, int baud) {
  unsigned char request[] = {0xFF, 0xFF, 0x05, 0};
  unsigned char *byte = &request[sizeof(request) - 1];

  switch (baud) {
    case LOW_BAUD:
      *byte = 2;
      break;

    case NORMAL_BAUD:
      *byte = 3;
      break;

    case HIGH_BAUD:
      *byte = 4;
      break;

    default:
      logMessage(LOG_WARNING, "display does not support %d baud", baud);
      return 0;
  }

  logMessage(LOG_WARNING, "changing display to %d baud", baud);
  return writeBraillePacket(brl, NULL, request, sizeof(request));
}

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.serial.parameters = &serialParameters;

  if (connectBrailleResource(brl, identifier, &descriptor, NULL)) {
    return 1;
  }

  return 0;
}


static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  int i=0;
  InputPacket reply;
  unsigned int allowHighBaud = 1;

  {
    const char *parameter = parameters[PARM_HIGHBAUD];

    if (parameter && *parameter) {
      if (!validateYesNo(&allowHighBaud, parameter)) {
        logMessage(LOG_WARNING, "unsupported high baud setting: %s", parameter);
      }
    }
  }

  dispbuf = rawdata = prevdata = NULL;

  if (!connectResource(brl, device)) goto failure;
  if (!setBaud(brl, NORMAL_BAUD)) goto failure;

  if (!queryDisplay(brl, &reply)) {
    /* Then send the query at 19200 baud, in case a PB was left ON
     * at that speed
     */

    if (!allowHighBaud) goto failure;
    if (!setBaud(brl, HIGH_BAUD)) goto failure;
    if (!queryDisplay(brl, &reply)) goto failure;
  }

  memcpy(hardwareVersion, &reply.fields.identity.version[1], sizeof(hardwareVersion));
  ncells = reply.fields.identity.columns;
  brl_cols = ncells;
  logMessage(LOG_INFO, "display replied: %d cells, version %.*s",
             ncells, sizeof(hardwareVersion), hardwareVersion);

  switch (brl_cols) {
    case 20:
      model = &modelNavigator20;
      break;

    case 40:
      model = (hardwareVersion[0] > '3')? &modelPowerBraille40: &modelNavigator40;
      break;

    case 80:
      model = &modelNavigator80;
      break;

    case 65:
      model = &modelPowerBraille65;
      break;

    case 81:
      model = &modelPowerBraille80;
      break;

    default:
      logMessage(LOG_ERR, "unrecognized braille display size: %u", brl_cols);
      goto failure;
  }

  logMessage(LOG_INFO, "detected %s", model->modelName);
  brl->keyBindings = model->keyBindings;

  slowUpdate = model->slowUpdate;
  noMultipleUpdates = 0;

#ifdef FORCE_DRAIN_AFTER_SEND
  slowUpdate = 1;
#endif /* FORCE_DRAIN_AFTER_SEND */

#ifdef FORCE_FULL_SEND_DELAY
  slowUpdate = 2;
#endif /* FORCE_FULL_SEND_DELAY */

#ifdef NO_MULTIPLE_UPDATES
  noMultipleUpdates = 1;
#endif /* NO_MULTIPLE_UPDATES */

  if (slowUpdate == 2) noMultipleUpdates = 1;
  fullFreshenEvery = FULL_FRESHEN_EVERY;

  if ((serialParameters.baud < HIGH_BAUD) && allowHighBaud && model->highBaudSupported) {
    /* if supported (PB) go to 19200 baud */
    if (!changeBaud(brl, HIGH_BAUD)) goto failure;
  //serialAwaitOutput(brl->gioEndpoint);
    asyncWait(BAUD_DELAY);
    if (!setBaud(brl, HIGH_BAUD)) goto failure;
    logMessage(LOG_DEBUG, "switched to %d baud - checking if display followed", HIGH_BAUD);

    if (queryDisplay(brl, &reply)) {
      logMessage(LOG_DEBUG, "display responded at %d baud", HIGH_BAUD);
    } else {
      logMessage(LOG_INFO,
                 "display did not respond at %d baud"
	         " - falling back to %d baud", NORMAL_BAUD,
                 HIGH_BAUD);

      if (!setBaud(brl, NORMAL_BAUD)) goto failure;
    //serialAwaitOutput(brl->gioEndpoint);
      asyncWait(BAUD_DELAY); /* just to be safe */

      if (queryDisplay(brl, &reply)) {
	logMessage(LOG_INFO,
                   "found display again at %d baud"
                   " - must be a TSI emulator",
                   NORMAL_BAUD);

        fullFreshenEvery = 1;
      } else {
	logMessage(LOG_ERR, "display lost after baud switch");
	goto failure;
      }
    }
  }

  /* Mark time of last command to initialize typematic watch */
  getMonotonicTime(&last_readbrl_time);
  lastcmd_time = last_readbrl_time;
  lastcmd = EOF;
  must_init_oldstat = 1;

  resetTypematic(brl);

  brl->textColumns = brl_cols;		/* initialise size of display */
  brl->textRows = BRLROWS;		/* always 1 */

  makeOutputTable(dotsTable_ISO11548_1);

  /* Allocate space for buffers */
  dispbuf = malloc(ncells);
  prevdata = malloc(ncells);
  rawdata = malloc(2 * ncells + DIM_BRL_SEND);
  /* 2* to insert 0s for attribute code when sending to the display */
  if (!dispbuf || !prevdata || !rawdata)
    goto failure;

  /* Initialize rawdata. It will be filled in and used directly to
     write to the display in writebrl(). */
  for (i = 0; i < DIM_BRL_SEND_FIXED; i++)
    rawdata[i] = BRL_SEND_HEAD[i];
  memset (rawdata + DIM_BRL_SEND, 0, 2 * ncells * BRLROWS);

  /* Force rewrite of display on first writebrl */
  memset(prevdata, 0xFF, ncells);

  return 1;

failure:
  brl_destruct(brl);
  return 0;
}


static void 
brl_destruct (BrailleDisplay *brl) {
  disconnectBrailleResource(brl, NULL);

  if (dispbuf) {
    free(dispbuf);
    dispbuf = NULL;
  }

  if (rawdata) {
    free(rawdata);
    rawdata = NULL;
  }

  if (prevdata) {
    free(prevdata);
    prevdata = NULL;
  }
}

static void 
display (BrailleDisplay *brl, 
         const unsigned char *pattern, 
	 unsigned int from, unsigned int to)
/* display a given dot pattern. We process only part of the pattern, from
   byte (cell) start to byte stop. That pattern should be shown at position 
   start on the display. */
{
  int i, length;

  /* Assumes BRLROWS == 1 */
  length = to - from;

  rawdata[BRL_SEND_LENGTH] = 2 * length;
  rawdata[BRL_SEND_OFFSET] = from;

  for (i = 0; i < length; i++)
    rawdata[DIM_BRL_SEND + 2 * i + 1] = translateOutputCell(pattern[from + i]);

  /* Some displays apparently don't like rapid updating. Most or all apprently
     don't do flow control. If we update the display too often and too fast,
     then the packets queue up in the send queue, the info displayed is not up
     to date, and the info displayed continues to change after we stop
     updating while the queue empties (like when you release the arrow key and
     the display continues changing for a second or two). We also risk
     overflows which put garbage on the display, or often what happens is that
     some cells from previously displayed lines will remain and not be cleared
     or replaced; also the pinging fails and the display gets
     reinitialized... To expose the problem skim/scroll through a long file
     (with long lines) holding down the up/down arrow key on the PC keyboard.

     pb40 has no problems: it apparently can take whatever we throw at
     it. Nav40 is good but we drain just to be safe.

     pb80 (twice larger but twice as fast as nav40) cannot take a continuous
     full speed flow. There is no flow control: apparently not supported
     properly on at least pb80. My pb80 is recent yet the hardware version is
     v1.0a, so this may be a hardware problem that was fixed on pb40.  There's
     some UART handshake mode that might be relevant but only seems to break
     everything (on both pb40 and pb80)...

     Nav80 is untested but as it receives at 9600, we probably need to
     compensate there too.

     Finally, some TSI emulators (at least the mdv mb408s) may have timing
     limitations.

     I no longer have access to a Nav40 and PB80 for testing: I only have a
     PB40.  */

  {
    int count = DIM_BRL_SEND + 2 * length;
    writeBytes(brl, rawdata, count);
  }
}

static void 
display_all (BrailleDisplay *brl,
             unsigned char *pattern)
{
  display (brl, pattern, 0, ncells);
}


static int 
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text)
{
  static int count = 0;

  /* assert: brl->textColumns == brl_cols */

  memcpy(dispbuf, brl->buffer, brl_cols);

  if (--count<=0) {
    /* Force an update of the whole display every now and then to clear any
       garble. */
    count = fullFreshenEvery;
    memcpy(prevdata, dispbuf, ncells);
    display_all (brl, dispbuf);
  } else if (noMultipleUpdates) {
    unsigned int from, to;
    
    if (cellsHaveChanged(prevdata, dispbuf, ncells, &from, &to, NULL)) {
      display (brl, dispbuf, from, to);
    }
  }else{
    int base = 0, i = 0, collecting = 0, simil = 0;
    
    while (i < ncells)
      if (dispbuf[i] == prevdata[i])
	{
	  simil++;
	  if (collecting && 2 * simil > DIM_BRL_SEND)
	    {
	      display (brl, dispbuf, base, i - simil + 1);
	      base = i;
	      collecting = 0;
	      simil = 0;
	    }
	  if (!collecting)
	    base++;
	  i++;
	}
      else
	{
	  prevdata[i] = dispbuf[i];
	  collecting = 1;
	  simil = 0;
	  i++;
	}
    
    if (collecting)
      display (brl, dispbuf, base, i - simil );
  }
return 1;
}


static int 
is_repeat_cmd (int cmd)
{
  int *c = repeat_list;
  while (*c != 0)
    if (*(c++) == cmd)
      return (1);
  return (0);
}


static void 
flicker (BrailleDisplay *brl)
{
  unsigned char *buf;

  /* Assumes BRLROWS == 1 */
  buf = malloc(brl_cols);
  if (buf)
    {
      memset (buf, FLICKER_CHAR, ncells);

      display_all (brl, buf);
      accurateDelay (FLICKER_DELAY);
      display_all (brl, prevdata);
      accurateDelay (FLICKER_DELAY);
      display_all (brl, buf);
      accurateDelay (FLICKER_DELAY);
      /* Note that we don't put prevdata back on the display, since flicker()
         normally preceeds the displaying of a special message. */

      free (buf);
    }
}


/* OK this one is pretty strange and ugly. readbrl() reads keys from
   the display's key pads. It calls cut_cursor() if it gets a certain key
   press. cut_cursor() is an elaborate function that allows selection of
   portions of text for cut&paste (presenting a moving cursor to replace
   the cursor routing keys that the Navigator 20/40 does not have). (This
   function is not bound to PB keys). The strange part is that cut_cursor()
   itself calls back to readbrl() to get keys from the user. It receives
   and processes keys by calling readbrl again and again and eventually
   returns a single code to the original readbrl() function that had 
   called cut_cursor() in the first place. */

/* If cut_cursor() returns the following special code to readbrl(), then
   the cut_cursor() operation has been cancelled. */
#define CMD_CUT_CURSOR 0xF0F0F0F0

static int 
cut_cursor (BrailleDisplay *brl)
{
  static int running = 0, pos = -1;
  int res = 0, key;
  unsigned char oldchar;

  if (running)
    return (CMD_CUT_CURSOR);
  /* Special return code. We are sending this to ourself through readbrl.
     That is: cut_cursor() was accepting input when it's activation keys
     were pressed a second time. The second readbrl() therefore activates
     a second cut_cursor(). It return CMD_CUT_CURSOR through readbrl() to
     the first cut_cursor() which then cancels the whole operation. */
  running = 1;

  if (pos == -1)
    {				/* initial cursor position */
      if (CUT_CSR_INIT_POS == 0)
	pos = 0;
      else if (CUT_CSR_INIT_POS == 1)
	pos = brl_cols / 2;
      else if (CUT_CSR_INIT_POS == 2)
	pos = brl_cols - 1;
    }

  flicker (brl);

  while (res == 0)
    {
      /* the if must not go after the switch */
      if (pos < 0)
	pos = 0;
      else if (pos >= brl_cols)
	pos = brl_cols - 1;
      oldchar = prevdata[pos];
      prevdata[pos] |= CUT_CSR_CHAR;
      display_all (brl, prevdata);
      prevdata[pos] = oldchar;

      while ((key = brl_readCommand (brl, KTB_CTX_DEFAULT)) == EOF) asyncWait(1); /* just yield */
      if((key &BRL_MSK_BLK) == BRL_BLK_CLIP_NEW)
	  res = BRL_BLK_CLIP_NEW + pos;
      else if((key &BRL_MSK_BLK) == BRL_BLK_CLIP_ADD)
	  res = BRL_BLK_CLIP_ADD + pos;
      else if((key &BRL_MSK_BLK) == BRL_BLK_COPY_RECT) {
	  res = BRL_BLK_COPY_RECT + pos;
	  pos = -1;
      }else if((key &BRL_MSK_BLK) == BRL_BLK_COPY_LINE) {
	  res = BRL_BLK_COPY_LINE + pos;
	  pos = -1;
      }else switch (key)
	{
	case BRL_CMD_FWINRT:
	  pos++;
	  break;
	case BRL_CMD_FWINLT:
	  pos--;
	  break;
	case BRL_CMD_LNUP:
	  pos += 5;
	  break;
	case BRL_CMD_LNDN:
	  pos -= 5;
	  break;
	case BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT:
	  pos = brl_cols - 1;
	  break;
	case BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT:
	  pos = 0;
	  break;
	case BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP:
	  pos += 10;
	  break;
	case BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN:
	  pos -= 10;
	  break;
	case CMD_CUT_CURSOR:
	  res = EOF;
	  break;
	  /* That's where we catch the special return code: user has typed
	     cut_cursor() activation key again, so we cancel it. */
	}
    }

  display_all (brl, prevdata);
  running = 0;
  return (res);
}


/* Now for readbrl().
   Description of received bytes and key names are near the top of the file.

   These macros, although unusual, make the key binding declarations look
   much better */

#define KEY(code,result) \
    case code: res = result; break;
#define KEYAND(code) \
    case code:
#define KEYSPECIAL(code,action) \
    case code: { action }; break;
#define KEYSW(codeon, codeoff, result) \
    case codeon: res = result | BRL_FLG_TOGGLE_ON; break; \
    case codeoff: res = result | BRL_FLG_TOGGLE_OFF; break;

/* For cursor routing */
/* lookup so find out if a certain key is active */
#define SW_CHK(swnum) \
      ( routingKeyBits[swnum/8] & (1 << (swnum % 8)) )

/* These are (sort of) states of the state machine parsing the bytes
   received form the display. These can be navigator|pb40 keys, pb80 keys,
   sensor switch info, low battery warning or query reply. The last three
   cannot be distinguished until the second byte, so at first they both fall
   under K_SPECIAL. */
#define K_SPECIAL 1
#define K_NAVKEY 2
#define K_PBKEY 3
#define K_BATTERY 4
#define K_SW 5
#define K_QUERYREP 6

/* read buffer size: maximum is query reply minus header */
#define MAXREAD 10

static int 
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  /* static bit vector recording currently pressed sensor switches (for
     repetition detection) */
  static unsigned char routingKeyBits[SW_MAXHORIZ];
  static unsigned char routingKeyNumbers[SW_MAXHORIZ * 8], /* list of pressed keys */
                       routingKeyCount = 0; /* length of that list */
  static unsigned char ignore_routing = 0;
     /* flag: after combo between routing and non-routing keys, don't act
	on any key until routing resets (releases). */
  uint32_t code = 0; /* 32-bit code representing pressed keys once the
			    input bytes are interpreted */
  int res = EOF; /* command code to return. code is mapped to res. */
  InputPacket packet;
  size_t size;
  int skip_this_cmd = 0;

  {
    TimeValue now;
    getMonotonicTime(&now);

    if (millisecondsBetween(&last_readbrl_time, &now) > READBRL_SKIP_TIME) {
      /* if the key we get this time is the same as the one we returned at last
         call, and if it has been abnormally long since we were called
         (presumably sound was being played or a message displayed) then
         the bytes we will read can be old... so we forget this key, if it is
         a repeat. */
      skip_this_cmd = 1;
    }

    last_readbrl_time = now;
  }
       
/* Key press codes come in pairs of bytes for nav and pb40, in 6bytes
   for pb65/80. Each byte has bits representing individual keys + a special
   mask/signature in the most significant 3bits.

   The low battery warning from the display is a specific 2bytes code.

   Finally, the routing keys have a special 2bytes header followed by 9, 14
   or 15 bytes of info (1bit for each routing key). The first 4bytes describe
   vertical routing keys and are ignored in this driver.

   We might get a query reply, since we send queries when we don't get
   any keys in a certain time. That a 2byte header + 10 more bytes ignored.
 */

  if (!(size = readPacket(brl, &packet))) return EOF;

#ifdef RECV_DELAY
  accurateDelay(SEND_DELAY);
#endif /* RECV_DELAY */

  if (model->hasRoutingKeys && must_init_oldstat) {
    unsigned int i;

    must_init_oldstat = 0;
    ignore_routing = 0;
    routingKeyCount = 0;
    for (i=0; i<SW_MAXHORIZ; i+=1) routingKeyBits[i] = 0;
  }

  switch (packet.type) {
    case IPT_KEYS: {
      unsigned int i;

      for (i=0; i<packet.data.keys.count; i+=1) {
        const KeysByteDescriptor *kbd = &packet.data.keys.descriptor[i];

        code |= (packet.fields.keys[i] & kbd->mask) << kbd->shift;
      }

      break;
    }

    case IPT_SWITCHES: {
      unsigned char count = packet.data.switches.count;
      unsigned int i;

      /* if model->switchBytes and packet.data.switches.count disagree, then must be garbage??? */
      if (count != model->switchBytes) return EOF;
      count -= sizeof(packet.fields.switches.vertical);

      /* if key press is maintained, then packet is resent by display
         every 0.5secs. When the key is released, then display sends a packet
         with all info bits at 0. */
      for (i=0; i<count; i+=1) {
        routingKeyBits[i] |= packet.fields.switches.horizontal[i];
      }

      routingKeyCount = 0;
      for (i=0; i<ncells; i+=1) {
        if (SW_CHK(i)) {
          routingKeyNumbers[routingKeyCount++] = i;
        }
      }

      /* SW_CHK(i) tells if routing key i is pressed.
         routingKeyNumbers[0] to routingKeyNumbers[howmany-1] give the numbers of the keys
         that are pressed. */

      for (i=0; i<count; i+=1) {
        if (packet.fields.switches.horizontal[i] != 0) {
          return EOF;
        }
      }

      must_init_oldstat = 1;
      if (ignore_routing) return EOF;
      break;
    }

    case IPT_BATTERY:
      message(NULL, gettext("battery low"), MSG_WAITKEY);
      return EOF;

    default:
      logUnexpectedPacket(packet.fields.bytes, size);
      return EOF;
  }
  /* Now associate a command (in res) to the key(s) (in code and sw_...) */

  if (model->hasRoutingKeys && code && routingKeyCount) {
    if (ignore_routing) return EOF;
    ignore_routing = 1;

    if (routingKeyCount == 1) {
      switch (code) {
	KEYAND(KEY_BUT3) KEY(KEY_BRIGHT, BRL_BLK_CLIP_NEW + routingKeyNumbers[0]);
	KEYAND(KEY_BUT2) KEY(KEY_BLEFT, BRL_BLK_COPY_RECT + routingKeyNumbers[0]);
	KEYAND(KEY_R2DN) KEY (KEY_BDOWN, BRL_BLK_NXINDENT + routingKeyNumbers[0]);
	KEYAND(KEY_R2UP) KEY (KEY_BUP, BRL_BLK_PRINDENT + routingKeyNumbers[0]);
	KEY (KEY_CROUND, BRL_BLK_SETMARK + routingKeyNumbers[0]);
	KEYAND(KEY_CNCV) KEY (KEY_BROUND, BRL_BLK_GOTOMARK + routingKeyNumbers[0]);
	KEY (KEY_CUP, BRL_BLK_SETLEFT + routingKeyNumbers[0]);
	KEY (KEY_CDOWN, BRL_BLK_SWITCHVT + routingKeyNumbers[0]);
	KEYAND(KEY_CDOWN | KEY_BUP) KEY(KEY_CUP | KEY_CDOWN,
					BRL_BLK_DESCCHAR +routingKeyNumbers[0]);
      }
    } else if (routingKeyCount == 2) {
      if (((routingKeyNumbers[0] + 1) == routingKeyNumbers[1]) &&
          ((code == KEY_BRIGHT) || (code == KEY_BUT3))) {
	res = BRL_BLK_CLIP_ADD + routingKeyNumbers[0];
      } else if (((routingKeyNumbers[0] + 1) == routingKeyNumbers[1]) &&
                 ((code == KEY_BLEFT) || (code == KEY_BUT2))) {
	res = BRL_BLK_COPY_LINE + routingKeyNumbers[1];
      } else if ((routingKeyNumbers[0] == 0) && (routingKeyNumbers[1] == 1)) {
	switch (code) {
	  KEYAND(KEY_R2DN) KEY (KEY_BDOWN, BRL_CMD_NXPGRPH);
	  KEYAND(KEY_R2UP) KEY (KEY_BUP, BRL_CMD_PRPGRPH);
	}
      } else if ((routingKeyNumbers[0] == 1) && (routingKeyNumbers[1] == 2)) {
	switch (code) {
	  KEY (KEY_BDOWN, BRL_CMD_NXPROMPT);
	  KEY (KEY_BUP, BRL_CMD_PRPROMPT);
	}
      } else if ((routingKeyNumbers[0] == 0) && (routingKeyNumbers[1] == 2)) {
	switch (code) {
	  KEYAND(KEY_R2DN) KEY (KEY_BDOWN, BRL_CMD_NXSEARCH);
	  KEYAND(KEY_R2UP) KEY (KEY_BUP, BRL_CMD_PRSEARCH);
	}
      }
    }
  } else if (model->hasRoutingKeys && routingKeyCount)	/* routing key */
    {
      if (routingKeyCount == 1)
	res = BRL_BLK_ROUTE + routingKeyNumbers[0];
      else if(routingKeyCount == 2 && routingKeyNumbers[0] == 1 && routingKeyNumbers[1] == 2)
 	res = BRL_CMD_PASTE;
      else if (routingKeyCount == 2 && routingKeyNumbers[0] == 0 && routingKeyNumbers[1] == 1)
	res = BRL_CMD_CHRLT;
      else if (routingKeyCount == 2 && routingKeyNumbers[0] == model->lastRoutingKey - 1
	       && routingKeyNumbers[1] == model->lastRoutingKey)
	res = BRL_CMD_CHRRT;
      else if (routingKeyCount == 2 && routingKeyNumbers[0] == 0 && routingKeyNumbers[1] == 2)
	res = BRL_CMD_HWINLT;
      else if (routingKeyCount == 2 && routingKeyNumbers[0] == model->lastRoutingKey - 2
	       && routingKeyNumbers[1] == model->lastRoutingKey)
	res = BRL_CMD_HWINRT;
      else if (routingKeyCount == 2 && routingKeyNumbers[0] == 0
	       && routingKeyNumbers[1] == model->lastRoutingKey)
	res = BRL_CMD_HELP;
      else if (routingKeyCount == 4 && routingKeyNumbers[0] == 0 && routingKeyNumbers[1] == 1
	       && routingKeyNumbers[2] == model->lastRoutingKey-1 && routingKeyNumbers[3] == model->lastRoutingKey)
	res = BRL_CMD_LEARN;
      else if(routingKeyCount == 3 && routingKeyNumbers[0]+2 == routingKeyNumbers[1]){
	  res = BRL_BLK_CLIP_COPY | BRL_ARG(routingKeyNumbers[0]) | BRL_EXT(routingKeyNumbers[2]);
	}
    }
  else switch (code){
  /* renames: CLEFT=BUT1 CRIGHT=BUT2 CUP=R1UP CDOWN=R2DN CROUNT=CNVX */

  /* movement */
    KEYAND(KEY_BAR1) KEYAND(KEY_R2UP) KEY (KEY_BUP, BRL_CMD_LNUP);
    KEYAND(KEY_BAR2) KEYAND(KEY_BAR3) KEYAND(KEY_BAR4)
      KEYAND(KEY_R2DN) KEY (KEY_BDOWN, BRL_CMD_LNDN);
    KEYAND(KEY_BUT3) KEY (KEY_BLEFT, BRL_CMD_FWINLT);
    KEYAND(KEY_BUT4) KEY (KEY_BRIGHT, BRL_CMD_FWINRT);

    KEYAND(KEY_CNCV) KEY (KEY_BROUND, BRL_CMD_HOME);
    KEYAND(KEY_CNCV | KEY_CUP) KEY(KEY_BROUND | KEY_CUP, BRL_CMD_BACK);
    KEY (KEY_CROUND, (context == KTB_CTX_MENU) ? BRL_CMD_MENU_PREV_SETTING
	 : BRL_CMD_CSRTRK);

    KEYAND(KEY_BUT1 | KEY_BAR1) KEY (KEY_BLEFT | KEY_BUP, BRL_CMD_TOP_LEFT);
    KEYAND(KEY_BUT1 | KEY_BAR2) KEY (KEY_BLEFT | KEY_BDOWN, BRL_CMD_BOT_LEFT);

    KEYAND(KEY_BUT2 | KEY_BAR1) KEY (KEY_BROUND | KEY_BUP, BRL_CMD_PRDIFLN);
    KEYAND(KEY_BUT2 | KEY_BAR2) KEYAND(KEY_BUT2 | KEY_BAR3)
      KEYAND(KEY_BUT2 | KEY_BAR4) KEY (KEY_BROUND | KEY_BDOWN, BRL_CMD_NXDIFLN);
    KEYAND(KEY_BUT2 | KEY_R2UP) KEY(KEY_CROUND | KEY_BUP, BRL_CMD_ATTRUP);
    KEYAND(KEY_BUT2 | KEY_R2DN) KEY(KEY_CROUND | KEY_BDOWN, BRL_CMD_ATTRDN);

    KEY (KEY_CLEFT | KEY_CROUND, BRL_CMD_CHRLT);
    KEY (KEY_CRIGHT | KEY_CROUND, BRL_CMD_CHRRT);

    KEY (KEY_CLEFT | KEY_CUP, BRL_CMD_HWINLT);
    KEY (KEY_CRIGHT | KEY_CUP, BRL_CMD_HWINRT);

    KEYAND(KEY_BUT1|KEY_BUT2|KEY_BAR1)  KEY (KEY_CROUND | KEY_CUP, BRL_CMD_WINUP);
    KEYAND(KEY_BUT1|KEY_BUT2|KEY_BAR2) KEY (KEY_CROUND | KEY_CDOWN, BRL_CMD_WINDN);

    KEYAND(KEY_R1UP | KEY_BUT3) KEY(KEY_CUP | KEY_BLEFT, BRL_CMD_LNBEG);
    KEYAND(KEY_R1UP | KEY_BUT4) KEY(KEY_CUP | KEY_BRIGHT, BRL_CMD_LNEND);

  /* keyboard cursor keys simulation */
    KEY (KEY_CLEFT, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT);
    KEY (KEY_CRIGHT, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT);
    KEY (KEY_CUP, (context == KTB_CTX_MENU && model->isPB40)
	 ? BRL_CMD_MENU_PREV_SETTING : BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP);
    KEY (KEY_CDOWN, (context == KTB_CTX_MENU && model->isPB40)
	 ? BRL_CMD_MENU_NEXT_SETTING : BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN);

  /* special modes */
    KEY (KEY_CLEFT | KEY_CRIGHT, BRL_CMD_HELP);
    KEYAND (KEY_BUT1 | KEY_BUT2 | KEY_BUT3 | KEY_BUT4)
      KEY (KEY_CLEFT | KEY_CRIGHT | KEY_CUP | KEY_CDOWN, BRL_CMD_LEARN);
    KEY (KEY_CROUND | KEY_BROUND, BRL_CMD_FREEZE);
    KEYAND (KEY_CUP | KEY_BDOWN) KEYAND(KEY_BUT3 | KEY_BUT4)
      KEY (KEY_BUP | KEY_BDOWN, BRL_CMD_INFO);
    KEYAND (KEY_CDOWN | KEY_BUP)
      KEY (KEY_CUP | KEY_CDOWN, BRL_CMD_ATTRVIS);
    KEYAND (KEY_CDOWN | KEY_BUP | KEY_CROUND)
      KEY (KEY_CUP | KEY_CDOWN | KEY_CROUND, BRL_CMD_DISPMD)

  /* Emulation of cursor routing */
    KEYAND(KEY_R1DN | KEY_R2DN) KEY (KEY_CDOWN | KEY_BDOWN, BRL_CMD_CSRJMP_VERT);
    KEY (KEY_CDOWN | KEY_BDOWN | KEY_BLEFT, BRL_BLK_ROUTE +0);
    KEY (KEY_CDOWN | KEY_BDOWN | KEY_BRIGHT, BRL_BLK_ROUTE + 3 * brl_cols / 4 - 1);

  /* Emulation of routing keys for cut&paste */
    KEY (KEY_CLEFT | KEY_BROUND, BRL_BLK_CLIP_NEW +0);
    KEY (KEY_CLEFT | KEY_BROUND | KEY_BUP, BRL_BLK_CLIP_ADD +0);
    KEY (KEY_CRIGHT | KEY_BROUND, BRL_BLK_COPY_RECT +brl_cols-1);
    KEY (KEY_CRIGHT | KEY_BROUND | KEY_BUP, BRL_BLK_COPY_LINE +brl_cols-1);
    KEY (KEY_CLEFT | KEY_CRIGHT | KEY_BROUND, CMD_CUT_CURSOR);  /* special: see
	 				    at the end of this fn */
  /* paste */
    KEY (KEY_CDOWN | KEY_BROUND, BRL_CMD_PASTE);

  /* speech */
  /* experimental speech support */
    KEYAND(KEY_CRIGHT | KEY_BLEFT) KEYAND(KEY_BAR2 | KEY_R2DN)
      KEY (KEY_BRIGHT | KEY_BDOWN, BRL_CMD_SAY_LINE);
    KEYAND(KEY_BAR1 | KEY_BAR2 | KEY_R2DN) 
      KEY (KEY_BLEFT | KEY_BRIGHT | KEY_BDOWN, BRL_CMD_SAY_BELOW);
    KEYAND(KEY_BROUND | KEY_BAR2) KEY (KEY_BROUND | KEY_BRIGHT, BRL_CMD_SPKHOME);
    KEYAND(KEY_CRIGHT | KEY_CUP | KEY_BLEFT | KEY_BUP)
      KEYAND(KEY_BAR2 | KEY_R2UP) KEY (KEY_BRIGHT | KEY_BUP, BRL_CMD_MUTE);
    KEYAND(KEY_BAR1 | KEY_BAR2 | KEY_R1UP | KEY_R2UP)
      KEY (KEY_BRIGHT | KEY_BUP | KEY_CUP | KEY_BLEFT, BRL_CMD_RESTARTSPEECH);
    
  /* preferences menu */
    KEYAND(KEY_BAR1 | KEY_BAR2) KEY (KEY_BLEFT | KEY_BRIGHT, BRL_CMD_PREFMENU);
    KEYAND(KEY_BAR1 | KEY_BAR2 | KEY_CNCV) 
      KEY (KEY_BLEFT | KEY_BRIGHT | KEY_BROUND, BRL_CMD_PREFSAVE);
    KEYAND(KEY_BAR1 | KEY_BAR2 | KEY_CNVX | KEY_CNCV) 
      KEY (KEY_CROUND | KEY_BLEFT | KEY_BRIGHT | KEY_BROUND, BRL_CMD_PREFLOAD);
    KEY (KEY_BLEFT | KEY_BRIGHT | KEY_BROUND | KEY_BDOWN, BRL_CMD_SKPIDLNS);
    KEY (KEY_BLEFT | KEY_BRIGHT | KEY_CROUND | KEY_BDOWN, BRL_CMD_SKPIDLNS);
    KEY (KEY_CLEFT | KEY_BLEFT | KEY_BRIGHT, BRL_CMD_SLIDEWIN);
    KEYAND(KEY_BUT2 | KEY_BAR1 | KEY_BAR2)
      KEY (KEY_CLEFT | KEY_CROUND | KEY_BLEFT | KEY_BRIGHT, BRL_CMD_TUNES);
    KEYAND(KEY_BUT1 | KEY_BAR1 | KEY_BAR2)    
      KEY (KEY_CUP | KEY_BLEFT | KEY_BRIGHT, BRL_CMD_CSRVIS);
    KEYAND(KEY_R1DN | KEY_BAR1 | KEY_BAR2)    
      KEY (KEY_CDOWN | KEY_BLEFT | KEY_BRIGHT, BRL_CMD_SIXDOTS);
    KEYAND(KEY_BUT1 | KEY_BAR1 | KEY_BAR2 | KEY_CNVX)
      KEY (KEY_CROUND | KEY_CUP | KEY_BLEFT | KEY_BRIGHT, BRL_CMD_CSRBLINK);
    KEYAND(KEY_BUT2 | KEY_BAR1 | KEY_BAR2 | KEY_CNVX)
      KEY (KEY_CROUND | KEY_CDOWN | KEY_BLEFT | KEY_BRIGHT, BRL_CMD_CAPBLINK);
    KEYAND (KEY_CROUND | KEY_BUP | KEY_BLEFT | KEY_BRIGHT)
      KEY (KEY_CROUND | KEY_CRIGHT | KEY_BLEFT | KEY_BRIGHT, BRL_CMD_ATTRBLINK);

  /* PB80 switches */
    KEYSW(KEY_S1UP, KEY_S1DN, BRL_CMD_ATTRVIS);
    KEYSW(KEY_S1UP |KEY_BAR1|KEY_BAR2|KEY_CNVX,
	  KEY_S1DN |KEY_BAR1|KEY_BAR2|KEY_CNVX, BRL_CMD_ATTRBLINK);
    KEYSW(KEY_S1UP | KEY_CNVX, KEY_S1DN | KEY_CNVX, BRL_CMD_ATTRBLINK);
    KEYSW(KEY_S2UP, KEY_S2DN, BRL_CMD_FREEZE);
    KEYSW(KEY_S3UP, KEY_S3DN, BRL_CMD_SKPIDLNS);
    KEYSW(KEY_S3UP |KEY_BAR1, KEY_S3DN |KEY_BAR1, BRL_CMD_SKPIDLNS);
    KEYSW(KEY_S4UP, KEY_S4DN, BRL_CMD_DISPMD);
  };

  /* If this is a typematic repetition of some key other than movement keys */
  if (lastcmd == res && !is_repeat_cmd (res)){
    if(skip_this_cmd){
      getMonotonicTime(&lastcmd_time);
      res = EOF;
    } else {
      TimeValue now;
      getMonotonicTime(&now);

      /* if to short a time has elapsed since last command, ignore this one */
      if (millisecondsBetween(&lastcmd_time, &now) < NONREPEAT_TIMEOUT) {
	res = EOF;
      }
    }
  }
  /* reset timer to avoid unwanted typematic */
  if (res != EOF){
    lastcmd = res;
    getMonotonicTime(&lastcmd_time);
  }

  /* Special: */
  if (res == CMD_CUT_CURSOR)
    res = cut_cursor (brl);
  /* This activates cut_cursor(). Done here rather than with KEYSPECIAL macro
     to allow clearing of l and r and adjustment of typematic variables.
     Since cut_cursor() calls readbrl again to get key inputs, it is
     possible that cut_cursor() is running (it called us) and we are calling
     it a second time. cut_cursor() will handle this as a signal to
     cancel it's operation. */

  return (res);
}
