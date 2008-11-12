/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#include "misc.h"
#include "message.h"

#define BRLSTAT ST_PB80Style
#define BRL_HAVE_STATUS_CELLS
#include "brl_driver.h"
#include "braille.h"
#include "io_serial.h"

/* Braille display parameters that do not change */
#define BRLROWS 1		/* only one row on braille display */

/* Type of delay the display requires after sending it commands.
   0 -> no delay, 1 -> drain only, 2 -> drain + wait for SEND_DELAY. */
static int slow_update;

/* Whether multiple packets can be sent for a single update. */
static int no_multiple_updates;

/* We periodicaly refresh the display even if nothing has changed, will clear
   out any garble... */
#define FULL_FRESHEN_EVERY 12 /* do a full update every nth writeWindow(). This
				 should be a little over every 0.5secs. */
static int fullFreshenEvery;

/* A query is sent if we don't get any keys in a certain time, to detect
   if the display was turned off. */
/* We record the time at which the last ping reply was received,
   and the time at which the last ping (query) was sent. */
static struct timeval last_ping, last_ping_sent;
/* that many pings are sent, that many chances to reply */
#define PING_MAXNQUERY 2
static int pings; /* counts number of pings sent since last reply */
/* how long we wait for a reply */
#define PING_REPLY_DELAY 300

/* for routing keys */
static int must_init_oldstat = 1;

/* Definitions to avoid typematic repetitions of function keys other
   than movement keys */
#define NONREPEAT_TIMEOUT 300
#define READBRL_SKIP_TIME 300
static int lastcmd = EOF;
static struct timeval lastcmd_time, last_readbrl_time;
/* Those functions it is OK to repeat */
static int repeat_list[] =
{BRL_CMD_FWINRT, BRL_CMD_FWINLT, BRL_CMD_LNUP, BRL_CMD_LNDN, BRL_CMD_WINUP, BRL_CMD_WINDN,
 BRL_CMD_CHRLT, BRL_CMD_CHRRT, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT,
 BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP,
 BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN,
 BRL_CMD_CSRTRK, 0};

/* This defines the mapping between brltty and Navigator's dots coding. */
static TranslationTable outputTable;

/* Stabilization delay after changing baud rate */
#define BAUD_DELAY (100)

/* Communication codes */
static unsigned char BRL_QUERY[] = {0xFF, 0xFF, 0x0A};
#define DIM_BRL_QUERY 3
static unsigned char BRL_TYPEMATIC[] = {0xFF, 0xFF, 0x0D};
#define DIM_BRL_TYPEMATIC 3
#ifdef HIGHBAUD
/* Command to put the PB at 19200baud */
static unsigned char BRL_UART192[] = {0xFF, 0xFF, 0x05, 0x04};
#define DIM_BRL_UART192 4
#endif /* HIGHBAUD */
#if 0
/* Activate handshake ? */
static unsigned char BRL_UART_HANDSHAK[] = {0xFF, 0xFF, 0x05, 0x01};
#define DIM_BRL_UART_HANDSHAK 4
#endif /* 0 */
/* Normal header for sending dots, with cursor always off */
static unsigned char BRL_SEND_HEAD[] = {0XFF, 0XFF, 0X04, 0X00, 0X99, 0X00};
#define DIM_BRL_SEND_FIXED 6
#define DIM_BRL_SEND 8
/* Two extra bytes for lenght and offset */
#define BRL_SEND_LENGTH 6
#define BRL_SEND_OFFSET 7

/* Description of reply to query */
#define Q_REPLY_LENGTH 12
static char Q_HEADER[] = {0x0, 0x05};
#define Q_HEADER_LENGTH 2
#define Q_OFFSET_COLS 2
/*#define Q_OFFSET_DOTS 3 *//* not used */
#define Q_OFFSET_VER 4 /* hardware version */
#define Q_VER_LENGTH 4
/*#define Q_OFFSET_CHKSUM 8 *//* not used */
/*#define Q_CHKSUM_LENGTH 4 *//* not used */

/* Bit definition of key codes returned by the display */
/* Navigator and pb40 return 2bytes, pb65/80 returns 6. Each byte has a
   different specific mask/signature in the 3 most significant bits.
   Other bits indicate whether a specific key is pressed.
   See readbrl(). */

/* Bits to take into account when checking each byte's signature */
#define KEY_SIGMASK 0xE0

/* How we describe each byte */
struct inbytedesc {
  unsigned char sig, /* it's signature */
                mask, /* bits that do represent keys */
                shift; /* where to shift them into "code" */
};
/* We combine all key bits into one 32bit int. Each byte is masked by the
   corresponding "mask" to extract valid bits then those are shifted by
   "shift" and or'ed into the 32bits "code". */

/* Description of bytes for navigator and pb40. */
#define NAV_KEY_LEN 2
static struct inbytedesc nav_key_desc[NAV_KEY_LEN] =
  {{0x60, 0x1F, 0},
   {0xE0, 0x1F, 5}};
/* Description of bytes for pb65/80 */
#define PB_KEY_LEN 6
static struct inbytedesc pb_key_desc[PB_KEY_LEN] =
  {{0x40, 0x0F, 10},
   {0xC0, 0x0F, 14},
   {0x20, 0x05, 18},
   {0xA0, 0x05, 21},
   {0x60, 0x1F, 24},
   {0xE0, 0x1F, 5}};

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
#define KEY_SW_H1 0x00
#define KEY_SW_H2 0x08

/* Definitions for sensor switches/cursor routing keys */
#define SW_NVERT 4 /* vertical switches. unused info. 4bytes to skip */
#define SW_MAXHORIZ 11	/* bytes of horizontal info (81cells
			   / 8bits per byte = 11bytes) */
/* actual total number of switch informatio bytes depending on size
   of display (40/65/81) including 4bytes of unused vertical switches */
#define SW_CNT40 9
#define SW_CNT80 14
#define SW_CNT81 15

/* Global variables */

static SerialDevice *serialDevice;              /* file descriptor for comm port */
static int serialBaud;
static int charactersPerSecond;
static unsigned char *rawdata,	/* translated data to send to display */
                     *prevdata, /* previous data sent */
                     *dispbuf;
static int brl_cols;		/* Number of cells available for text */
static int ncells;              /* Total number of cells on display: this is
				   brl_cols cells + 1 status cell on PB80. */
static unsigned char has_sw,    /* flag: has routing keys or not */
                     sw_lastkey,/* index of the last routing key (first
				   being 0). On PB80 this excludes the 81st. */
                     sw_bcnt;   /* bytes of sensor switch information */
static char disp_ver[Q_VER_LENGTH]; /* version of the hardware */

static enum { NAV20_40=0, NAV80, PB40, PB65_80} displayType;

static int
myread(void *buf, unsigned len)
{
  return serialReadData(serialDevice,buf,len,100,100);
}


static int
QueryDisplay(unsigned char *reply)
/* For auto-detect: send query, read response and validate response header. */
{
  int count;
  if ((count = serialWriteData(serialDevice, BRL_QUERY, DIM_BRL_QUERY)) == DIM_BRL_QUERY) {
    if (serialAwaitInput(serialDevice, 100)) {
      if ((count = myread(reply, Q_REPLY_LENGTH)) != -1) {
        if ((count == Q_REPLY_LENGTH) && (memcmp(reply, Q_HEADER, Q_HEADER_LENGTH) == 0)) {
          LogBytes(LOG_DEBUG, "TSI Reply", reply, count);
          return 1;
        } else {
          LogBytes(LOG_WARNING, "Unexpected Response", reply, count);
        }
      } else {
        LogError("Read");
      }
    }
  } else if (count != -1) {
    LogPrint(LOG_ERR, "Short write: %d < %d", count, DIM_BRL_QUERY);
  } else {
    LogError("Write");
  }
  return 0;
}


static void 
ResetTypematic (void)
/* Sends a command to the display to set the typematic parameters */
{
  static unsigned char params[2] =
    {BRL_TYPEMATIC_DELAY, BRL_TYPEMATIC_REPEAT};
  serialWriteData (serialDevice, BRL_TYPEMATIC, DIM_BRL_TYPEMATIC);
  serialWriteData (serialDevice, &params, 2);
}


static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device)
{
  int i=0;
  unsigned char reply[Q_REPLY_LENGTH];
  int speed;

  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(dots, outputTable);
  }

  if (!isSerialDevice(&device)) {
    unsupportedDevice(device);
    return 0;
  }

  dispbuf = rawdata = prevdata = NULL;

  /* Open the Braille display device for random access */
  if (!(serialDevice = serialOpenDevice(device))) goto failure;
  /* Try to detect display by sending query */
  LogPrint(LOG_DEBUG,"Sending query at 9600bps");
  if(!serialRestartDevice(serialDevice, serialBaud=9600)) goto failure;
  if(!QueryDisplay(reply)){
#ifdef HIGHBAUD
    /* Then send the query at 19200bps, in case a PB was left ON
       at that speed */
    LogPrint(LOG_DEBUG,"Sending query at 19200bps");
    if(!serialSetBaud(serialDevice, serialBaud=19200)) goto failure;
    if(!QueryDisplay(reply)) goto failure;
#endif /* HIGHBAUD */
  }

  memcpy (disp_ver, &reply[Q_OFFSET_VER], Q_VER_LENGTH);
  ncells = reply[Q_OFFSET_COLS];
  LogPrint(LOG_INFO,"Display replied: %d cells, version %c%c%c%c", ncells,
	   disp_ver[0], disp_ver[1], disp_ver[2], disp_ver[3]);

  brl_cols = ncells;
  sw_lastkey = brl_cols-1;
  speed = 1;
  slow_update = 0;

  switch(brl_cols){
  case 20:
    /* nav 20 */
    displayType = NAV20_40;
    has_sw = 0;
    LogPrint(LOG_INFO, "Detected Navigator 20");
    break;
  case 40:
    if(disp_ver[1] > '3'){
      /* pb 40 */
      displayType = PB40;
      has_sw = 1;
      sw_bcnt = SW_CNT40;
      sw_lastkey = 39;
      speed = 2;
      LogPrint(LOG_INFO, "Detected PowerBraille 40");
    }else{
      /* nav 40 */
      has_sw = 0;
      displayType = NAV20_40;
      slow_update = 1;
      LogPrint(LOG_INFO, "Detected Navigator 40");
    }
    break;
  case 80:
    /* nav 80 */
    displayType = NAV80;
    has_sw = 1;
    sw_bcnt = SW_CNT80;
    sw_lastkey = 79;
    slow_update = 2;
    LogPrint(LOG_INFO, "Detected Navigator 80");
    break;
  case 65:
    /* pb65 */
    displayType = PB65_80;
    has_sw = 1;
    sw_bcnt = SW_CNT81;
    sw_lastkey = 64;
    speed = 2;
    slow_update = 2;
    LogPrint(LOG_INFO, "Detected PowerBraille 65");
    break;
  case 81:
    /* pb80 */
    displayType = PB65_80;
    has_sw = 1;
    sw_bcnt = SW_CNT81;
    sw_lastkey = 79;
    brl_cols = 80;
    speed = 2;
    slow_update = 2;
    LogPrint(LOG_INFO, "Detected PowerBraille 80");
    break;
  default:
    LogPrint(LOG_ERR,"Unrecognized braille display");
    goto failure;
  };
  brl->helpPage = displayType;

  no_multiple_updates = 0;
#ifdef FORCE_DRAIN_AFTER_SEND
  slow_update = 1;
#endif /* FORCE_DRAIN_AFTER_SEND */
#ifdef FORCE_FULL_SEND_DELAY
  slow_update = 2;
#endif /* FORCE_FULL_SEND_DELAY */
#ifdef NO_MULTIPLE_UPDATES
  no_multiple_updates = 1;
#endif /* NO_MULTIPLE_UPDATES */
  if(slow_update == 2) no_multiple_updates = 1;

  fullFreshenEvery = FULL_FRESHEN_EVERY;
#ifdef HIGHBAUD
  if(speed == 2){ /* if supported (PB) go to 19.2Kbps */
    serialWriteData (serialDevice, BRL_UART192, DIM_BRL_UART192);
    serialDrainOutput(serialDevice);
    approximateDelay(BAUD_DELAY);
    if(!serialSetBaud(serialDevice, serialBaud=19200)) goto failure;
    LogPrint(LOG_DEBUG,"Switched to 19200bps. Checking if display followed.");
    if(QueryDisplay(reply))
      LogPrint(LOG_DEBUG,"Display responded at 19200bps.");
    else{
      LogPrint(LOG_INFO,"Display did not respond at 19200bps, "
	       "falling back to 9600bps.");
      if(!serialSetBaud(serialDevice, serialBaud=9600)) goto failure;
      serialDrainOutput(serialDevice);
      approximateDelay(BAUD_DELAY); /* just to be safe */
      if(QueryDisplay(reply)) {
	LogPrint(LOG_INFO,"Found display again at 9600bps.");
	LogPrint(LOG_INFO, "Must be a TSI emulator.");
        fullFreshenEvery = 1;
      }else{
	LogPrint(LOG_ERR,"Display lost after baud switching");
	goto failure;
      }
    }
  }
#endif /* HIGHBAUD */

  charactersPerSecond = serialBaud / serialGetCharacterBits(serialDevice);
  slow_update = 0;

  /* Mark time of last command to initialize typematic watch */
  gettimeofday (&last_ping, NULL);
  memcpy(&last_readbrl_time, &last_ping, sizeof(struct timeval));
  memcpy(&lastcmd_time, &last_ping, sizeof(struct timeval));
  lastcmd = EOF;
  pings=0;
  must_init_oldstat = 1;

  ResetTypematic ();

  brl->x = brl_cols;		/* initialise size of display */
  brl->y = BRLROWS;		/* always 1 */

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

failure:;
  brl_destruct(brl);
  return 0;
}


static void 
brl_destruct (BrailleDisplay *brl)
{
  if (serialDevice) {
    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }

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
	 unsigned start, unsigned stop)
/* display a given dot pattern. We process only part of the pattern, from
   byte (cell) start to byte stop. That pattern should be shown at position 
   start on the display. */
{
  int i, length;

  /* Assumes BRLROWS == 1 */
  length = stop - start + 1;

  rawdata[BRL_SEND_LENGTH] = 2 * length;
  rawdata[BRL_SEND_OFFSET] = start;

  for (i = 0; i < length; i++)
    rawdata[DIM_BRL_SEND + 2 * i + 1] = outputTable[pattern[start + i]];

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
    serialWriteData (serialDevice, rawdata, count);
    brl->writeDelay += (count * 1000 / charactersPerSecond) + 1;
  }

  /* First a drain after the write helps make sure we don't fill up the
     buffer with info that will be overwritten immediately. This is not needed
     on pb40, but it might be good on nav40 (which runs only at 9600bps).

     Then for pb80 (and probably also needed for nav80 though untested) as
     well as for some TSI emulators we add a supplementary delay: drain
     probably waits until the bytes are in the UART, but we still need a few
     ms to send them out.  This delay will combine with the sleep for this
     BRLTTY cycle (delay in the main program loop of 40ms).  Keeping the delay
     in the main brltty loop low keeps the response time good, but SEND_DELAY
     gives the display time to show what we're sending.

     I found experimentally that a delay of 30ms seems best (at least for
     pb80). A longer delay, or using approximateDelay() instead of
     accurateDelay() is too much. This makes the pb a little bit sluggish,
     but prevents any communication problems. */

  switch(slow_update){
  /* 0 does nothing */
  case 1: /* nav 40 */
    serialDrainOutput(serialDevice); break;
  case 2: /* nav80, pb80, some emulators */
    serialDrainOutput(serialDevice);
    accurateDelay(SEND_DELAY);
    break;
  };
}

static int
brl_writeStatus (BrailleDisplay *brl, const unsigned char *s)
/* Only the PB80, which actually has 81cells, can be considered to have status
   cells, and it has only one. We could also decide to devote some of
   the cells of the PB65? */
{
  if(ncells == 81)
    dispbuf[80] = s[0];
  return 1;
}


static void 
display_all (BrailleDisplay *brl,
             unsigned char *pattern)
{
  display (brl, pattern, 0, ncells - 1);
}


static int 
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text)
{
  static int count = 0;

  /* assert: brl->x == brl_cols */

  memcpy(dispbuf, brl->buffer, brl_cols);

  if (--count<=0) {
    /* Force an update of the whole display every now and then to clear any
       garble. */
    count = fullFreshenEvery;
    memcpy(prevdata, dispbuf, ncells);
    display_all (brl, dispbuf);
  }else if(no_multiple_updates){
    int start, stop;
    
    for(start=0; start<ncells; start++)
      if(dispbuf[start] != prevdata[start]) break;
    if(start == ncells) return 1;
    for(stop = ncells-1; stop > start; stop--)
      if(dispbuf[stop] != prevdata[stop]) break;
    
    memcpy(prevdata+start, dispbuf+start, stop-start+1);
    display (brl, dispbuf, start, stop);
  }else{
    int base = 0, i = 0, collecting = 0, simil = 0;
    
    while (i < ncells)
      if (dispbuf[i] == prevdata[i])
	{
	  simil++;
	  if (collecting && 2 * simil > DIM_BRL_SEND)
	    {
	      display (brl, dispbuf, base, i - simil);
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
      display (brl, dispbuf, base, i - simil - 1);
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

      while ((key = brl_readCommand (brl, BRL_CTX_SCREEN)) == EOF) approximateDelay(1); /* just yield */
      if((key &BRL_MSK_BLK) == BRL_BLK_CUTBEGIN)
	  res = BRL_BLK_CUTBEGIN + pos;
      else if((key &BRL_MSK_BLK) == BRL_BLK_CUTAPPEND)
	  res = BRL_BLK_CUTAPPEND + pos;
      else if((key &BRL_MSK_BLK) == BRL_BLK_CUTRECT) {
	  res = BRL_BLK_CUTRECT + pos;
	  pos = -1;
      }else if((key &BRL_MSK_BLK) == BRL_BLK_CUTLINE) {
	  res = BRL_BLK_CUTLINE + pos;
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
      ( sw_oldstat[swnum/8] & (1 << (swnum % 8)) )

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
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context)
{
  /* static bit vector recording currently pressed sensor switches (for
     repetition detection) */
  static unsigned char sw_oldstat[SW_MAXHORIZ];
  unsigned char sw_stat[SW_MAXHORIZ]; /* received switch data */
  static unsigned char sw_which[SW_MAXHORIZ * 8], /* list of pressed keys */
                       sw_howmany = 0; /* length of that list */
  static unsigned char ignore_routing = 0;
     /* flag: after combo between routing and non-routing keys, don't act
	on any key until routing resets (releases). */
  unsigned int code = 0; /* 32bits code representing pressed keys once the
			    input bytes are interpreted */
  int res = EOF; /* command code to return. code is mapped to res. */
  static int pending_cmd = EOF;
  char buf[MAXREAD], /* read buffer */
       packtype = 0; /* type of packet being received (state) */
  unsigned i;
  struct timeval now;
  int skip_this_cmd = 0;

  gettimeofday (&now, NULL);
  if (millisecondsBetween(&last_readbrl_time, &now) > READBRL_SKIP_TIME)
    /* if the key we get this time is the same as the one we returned at last
       call, and if it has been abnormally long since we were called
       (presumably sound was being played or a message displayed) then
       the bytes we will read can be old... so we forget this key, if it is
       a repeat. */
    skip_this_cmd = 1;
  memcpy(&last_readbrl_time, &now, sizeof(struct timeval));
       
  if(pending_cmd != EOF){
    res = pending_cmd;
    lastcmd = EOF;
    pending_cmd = EOF;
    return(res);
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

  /* Check for first byte */
  if (serialReadData (serialDevice, buf, 1, 0, 0) != 1){
    if((i = millisecondsBetween(&last_ping, &now) > PING_INTRVL)){
      int ping_due = (pings==0 || (millisecondsBetween(&last_ping_sent, &now)
				   > PING_REPLY_DELAY));
      if((pings>=PING_MAXNQUERY && ping_due))
	return BRL_CMD_RESTARTBRL;
      else if(ping_due){
	LogPrint(LOG_DEBUG,"Display idle: sending query");
	serialDrainOutput(serialDevice);
	approximateDelay(2*SEND_DELAY);
	serialWriteData (serialDevice, BRL_QUERY, DIM_BRL_QUERY);
	if(slow_update == 1)
	  serialDrainOutput(serialDevice);
	else if(slow_update == 2){
	  serialDrainOutput(serialDevice);
	  approximateDelay(SEND_DELAY);
	}
	pings++;
	gettimeofday(&last_ping_sent, NULL);
      }
    }
    return (EOF);
  }
  /* there was some input, we heard something. */
  gettimeofday(&last_ping, NULL);
  pings=0;

#ifdef RECV_DELAY
  accurateDelay(SEND_DELAY);
#endif /* RECV_DELAY */

  /* read bytes */
  i=0;
  while(1){
    if(i==0){
      if(buf[0] == BATTERY_H1)
	 /* || buf[0] == KEY_SW_H1 || buf[0] == Q_HEADER[0] */
	packtype = K_SPECIAL;
      else if((buf[0]&KEY_SIGMASK) == nav_key_desc[0].sig) packtype = K_NAVKEY;
      else if((buf[0]&KEY_SIGMASK) == pb_key_desc[0].sig) packtype = K_PBKEY;
      else return(EOF);
      /* unrecognized byte (garbage...?) */
    }else{ /* i>0 */
      if(packtype == K_SPECIAL){
	if(buf[1] == BATTERY_H2) packtype = K_BATTERY;
	else if(buf[1] == KEY_SW_H2) packtype = K_SW;
	else if(buf[1] == Q_HEADER[1]) packtype = K_QUERYREP;
	else return(EOF);
	break;
      }else if(packtype == K_NAVKEY){
	if((buf[1]&KEY_SIGMASK) != nav_key_desc[1].sig)
	  return(EOF);
	break;
      }else{ /* K_PBKEY */
	if((buf[i]&KEY_SIGMASK) != pb_key_desc[i].sig)
	  return(EOF);
	if(i==PB_KEY_LEN-1) break;
      }
    }
    i++;
    if (myread (buf+i, 1) != 1)
      return (EOF);
  }/* while */

  if(has_sw && must_init_oldstat){
    must_init_oldstat = 0;
    ignore_routing = 0;
    sw_howmany = 0;
    for(i=0;i<SW_MAXHORIZ; i++)
      sw_oldstat[i] = 0;
  }

  if(packtype == K_BATTERY){
    const char *text = "-><- Display battery is low";
    if(brl_cols < strlen(text)) text = "Display battery low";
    message(NULL, text, MSG_WAITKEY);
    return (EOF);
  }else if(packtype == K_QUERYREP){
    /* flush the last 10bytes of the reply. */
    LogPrint(LOG_DEBUG,"Got reply to idle ping");
    myread(buf, Q_REPLY_LENGTH - Q_HEADER_LENGTH);
    return(EOF);
  }else if(packtype == K_NAVKEY || packtype == K_PBKEY){
    /* construct code */
    int n;
    struct inbytedesc *desc;
    if(packtype == K_NAVKEY){
      n = NAV_KEY_LEN;
      desc = nav_key_desc;
    }else{
      n = PB_KEY_LEN;
      desc = pb_key_desc;
    }
    code = 0;
    for(i=0; i<n; i++)
      code |= (buf[i]&desc[i].mask) << desc[i].shift;
  }else if(packtype == K_SW){
    /* read the rest of the sequence */
    unsigned char cnt;
    unsigned char buf[SW_NVERT];

    code = 0; /* no normal (non-sensor switch) keys are pressed */

    /* read next byte: it indicates length of sequence */
    /* still VMIN = 0, VTIME = 1 */
    if (myread (&cnt, 1) != 1)
      return (EOF);
    
    /* if sw_bcnt and cnt disagree, then must be garbage??? */
    /* problematic for PB 65/80/81... not clear */
    if (cnt != sw_bcnt)
      return (EOF);

    if (myread (buf, SW_NVERT) != SW_NVERT)
      return (EOF);
    cnt -= SW_NVERT;
    /* cnt now gives the number of bytes describing horizontal
       routing keys only */
    
    if (myread (sw_stat, cnt) != cnt)
      return (EOF);

    /* if key press is maintained, then packet is resent by display
       every 0.5secs. When the key is released, then display sends a packet
       with all info bits at 0. */
    for(i=0; i<cnt; i++)
      sw_oldstat[i] |= sw_stat[i];

    for (sw_howmany = 0, i = 0; i < ncells; i++)
      if (SW_CHK (i))
	sw_which[sw_howmany++] = i;
    /* SW_CHK(i) tells if routing key i is pressed.
       sw_which[0] to sw_which[howmany-1] give the numbers of the keys
       that are pressed. */

    for(i=0; i<cnt; i++)
      if(sw_stat[i] != 0){
	return (EOF);
      }
    must_init_oldstat = 1;
    if(ignore_routing) return(EOF);
  }

  /* Now associate a command (in res) to the key(s) (in code and sw_...) */

  if(has_sw && code && sw_howmany){
    if(ignore_routing) return(EOF);
    ignore_routing = 1;
    if(sw_howmany == 1){
      switch(code){
	KEYAND(KEY_BUT3) KEY(KEY_BRIGHT, BRL_BLK_CUTBEGIN + sw_which[0]);
	KEYAND(KEY_BUT2) KEY(KEY_BLEFT, BRL_BLK_CUTRECT + sw_which[0]);
	KEYAND(KEY_R2DN) KEY (KEY_BDOWN, BRL_BLK_NXINDENT + sw_which[0]);
	KEYAND(KEY_R2UP) KEY (KEY_BUP, BRL_BLK_PRINDENT + sw_which[0]);
	KEY (KEY_CROUND, BRL_BLK_SETMARK + sw_which[0]);
	KEYAND(KEY_CNCV) KEY (KEY_BROUND, BRL_BLK_GOTOMARK + sw_which[0]);
	KEY (KEY_CUP, BRL_BLK_SETLEFT + sw_which[0]);
	KEY (KEY_CDOWN, BRL_BLK_SWITCHVT + sw_which[0]);
	KEYAND(KEY_CDOWN | KEY_BUP) KEY(KEY_CUP | KEY_CDOWN,
					BRL_BLK_DESCCHAR +sw_which[0]);
      }
    }else if(sw_howmany == 2) {
      if(sw_which[0]+1 == sw_which[1]
	 && (code == KEY_BRIGHT || code == KEY_BUT3))
	res = BRL_BLK_CUTAPPEND + sw_which[0];
      else if(sw_which[0]+1 == sw_which[1]
	 && (code == KEY_BLEFT || code == KEY_BUT2))
	res = BRL_BLK_CUTLINE + sw_which[1];
      else if(sw_which[0]==0 && sw_which[1]==1){
	switch(code){
	  KEYAND(KEY_R2DN) KEY (KEY_BDOWN, BRL_CMD_NXPGRPH);
	  KEYAND(KEY_R2UP) KEY (KEY_BUP, BRL_CMD_PRPGRPH);
	}
      }else if(sw_which[0]==1 && sw_which[1]==2){
	switch(code){
	  KEY (KEY_BDOWN, BRL_CMD_NXPROMPT);
	  KEY (KEY_BUP, BRL_CMD_PRPROMPT);
	}
      }else if(sw_which[0]==0 && sw_which[1]==2){
	switch(code){
	  KEYAND(KEY_R2DN) KEY (KEY_BDOWN, BRL_CMD_NXSEARCH);
	  KEYAND(KEY_R2UP) KEY (KEY_BUP, BRL_CMD_PRSEARCH);
	}
      }
    }
  }else if (has_sw && sw_howmany)	/* routing key */
    {
      if (sw_howmany == 1)
	res = BRL_BLK_ROUTE + sw_which[0];
#if 0
     else if (sw_howmany == 3 && sw_which[1] == sw_lastkey - 1
	       && sw_which[2] == sw_lastkey)
	res = BRL_BLK_CUTBEGIN + sw_which[0];
      else if (sw_howmany == 3 && sw_which[0] == 0 && sw_which[1] == 1)
 	res = BRL_BLK_CUTRECT + sw_which[2];
#endif /* 0 */
      else if(sw_howmany == 2 && sw_which[0] == 1 && sw_which[1] == 2)
 	res = BRL_CMD_PASTE;
      else if (sw_howmany == 2 && sw_which[0] == 0 && sw_which[1] == 1)
	res = BRL_CMD_CHRLT;
      else if (sw_howmany == 2 && sw_which[0] == sw_lastkey - 1
	       && sw_which[1] == sw_lastkey)
	res = BRL_CMD_CHRRT;
      else if (sw_howmany == 2 && sw_which[0] == 0 && sw_which[1] == 2)
	res = BRL_CMD_HWINLT;
      else if (sw_howmany == 2 && sw_which[0] == sw_lastkey - 2
	       && sw_which[1] == sw_lastkey)
	res = BRL_CMD_HWINRT;
      else if (sw_howmany == 2 && sw_which[0] == 0
	       && sw_which[1] == sw_lastkey)
	res = BRL_CMD_HELP;
      else if (sw_howmany == 4 && sw_which[0] == 0 && sw_which[1] == 1
	       && sw_which[2] == sw_lastkey-1 && sw_which[3] == sw_lastkey)
	res = BRL_CMD_LEARN;
#if 0
      else if (sw_howmany == 2 && sw_which[0] == 1
	       && sw_which[1] == sw_lastkey - 1)
	{
	  ResetTypematic ();
	  display_all (brl, prevdata);
	  /* Special: No res code... */
	}
#endif /* 0 */
      else if(sw_howmany == 3 && sw_which[0]+2 == sw_which[1]){
	  res = BRL_BLK_CUTBEGIN + sw_which[0];
	  pending_cmd = BRL_BLK_CUTRECT + sw_which[2];
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
    KEY (KEY_CROUND, (context == BRL_CTX_PREFS) ? BRL_CMD_MENU_PREV_SETTING
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
    KEY (KEY_CUP, (context == BRL_CTX_PREFS && displayType == PB40)
	 ? BRL_CMD_MENU_PREV_SETTING : BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP);
    KEY (KEY_CDOWN, (context == BRL_CTX_PREFS && displayType == PB40)
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
    KEY (KEY_CLEFT | KEY_BROUND, BRL_BLK_CUTBEGIN +0);
    KEY (KEY_CLEFT | KEY_BROUND | KEY_BUP, BRL_BLK_CUTAPPEND +0);
    KEY (KEY_CRIGHT | KEY_BROUND, BRL_BLK_CUTRECT +brl_cols-1);
    KEY (KEY_CRIGHT | KEY_BROUND | KEY_BUP, BRL_BLK_CUTLINE +brl_cols-1);
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

#if 0
  /* typematic reset */
    KEYSPECIAL (KEY_CLEFT | KEY_BRIGHT, ResetTypematic ();
		display_all (brl, prevdata);
    );
#endif /* 0 */
  };

  /* If this is a typematic repetition of some key other than movement keys */
  if (lastcmd == res && !is_repeat_cmd (res)){
    if(skip_this_cmd){
      gettimeofday (&lastcmd_time, NULL);
      res = EOF;
    }else{
      /* if to short a time has elapsed since last command, ignore this one */
      gettimeofday (&now, NULL);
      if (millisecondsBetween(&lastcmd_time, &now) < NONREPEAT_TIMEOUT)
	res = EOF;
    }
  }
  /* reset timer to avoid unwanted typematic */
  if (res != EOF){
    lastcmd = res;
    gettimeofday (&lastcmd_time, NULL);
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
