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
#define VERSION \
"BRLTTY user-space-only driver for Tieman Voyager, version 0.10 (March 2004)"
#define COPYRIGHT \
"   Copyright (C) 2004 by Stéphane Doyon  <s.doyon@videotron.ca>"

/* Voyager/braille.c - Braille display driver for Tieman Voyager displays.
 *
 * Written by Stéphane Doyon  <s.doyon@videotron.ca>
 *
 * It is being tested on Voyager 44, should also support Voyager 70.
 * It is designed to be compiled in BRLTTY version 3.4.2.
 *
 * History:
 * 0.10, March 2004: Use BRLTTY core repeat functions. Add brlinput parameter
 *   and toggle to disallow braille typing.
 * 0.01, January 2004: fork from the original driver which relied on an
 *   in-kernel USB driver.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

#include "Programs/usb.h"
#include "Programs/misc.h"
#include "Programs/message.h"

typedef enum {
  PARM_BRLINPUT=0
} DriverParameter;
#define BRLPARMS "brlinput"

#define BRLSTAT ST_VoyagerStyle
#define BRL_HAVE_FIRMNESS
#include "Programs/brl_driver.h"
#include "Programs/tbl.h"

static int brlinput = 1;

/* Workaround USB<->Voyager flakiness: repeat commands */
#define STALL_TRIES 3
#define SENDBRAILLE_REPEATS 2

/* Braille display parameters that do not change */
#define BRLROWS 1		/* only one row on braille display */

#define MAXNRCELLS 70 /* arbitrary max for allocations */

/* control message request codes */
#define BRLVGER_SET_DISPLAY_ON 0
#define BRLVGER_SET_DISPLAY_VOLTAGE 1
#define BRLVGER_GET_SERIAL 3
#define BRLVGER_GET_FWVERSION 5
#define BRLVGER_GET_LENGTH 6
#define BRLVGER_SEND_BRAILLE 7
#define BRLVGER_BEEP 9
#if 0 /* not used and not sure they're working */
/* hw ver is either 0.0 or unspecified on the prototype I have.
  Not sure how to decode it properly. */
#define BRLVGER_GET_DISPLAY_VOLTAGE 2
#define BRLVGER_GET_HWVERSION 4
#define BRLVGER_GET_CURRENT 8
#endif

/* We'll use 4cells as status cells, both on Voyager 44 and 70. (3cells have
   content and the fourth is blank to mark the separation.)
   NB: You can't just change this constant to vary the number of status
   cells: some key bindings for cursor routing keys assign special
   functions to routing keys over status cells.
*/
#define NRSTATCELLS 4

#define USB_VENDOR_ID 0X0798
#define USB_PRODUCT_ID 1

#define USB_INPUT_ENDPOINT 1

/* Global variables */

/* Mappings between Voyager's dot coding and brltty's coding. */
static TranslationTable inputTable, outputTable;

static UsbDevice *usbDevice;
static unsigned char usbInterface;

static unsigned char *prevdata, /* previous pattern displayed */
                     *dispbuf; /* buffer to prepare new pattern */
static unsigned brl_cols, /* Number of cells available for text */
                ncells; /* total number of cells including status */
static char readbrl_init; /* Flag to reinitialize readbrl function state. */


static int
chooseUsbDevice (UsbDevice *device, void *data)
{
  const char *serialNumber = data;
  const UsbDeviceDescriptor *descriptor = usbDeviceDescriptor(device);
  if ((descriptor->idVendor == USB_VENDOR_ID) 
      && (descriptor->idProduct == USB_PRODUCT_ID)) {
    if (!usbVerifySerialNumber(device, serialNumber))
      return 0;

    usbInterface = 0;
    if (usbClaimInterface(device, usbInterface) != -1) {
      if (usbSetConfiguration(device, 1) != -1)
        if (usbSetAlternative(device, usbInterface, 0) != -1)
          return 1;
      usbReleaseInterface(device, usbInterface);
    }
  }
  return 0;
}

static int
_sndcontrolmsg(char *reqname, uint8_t request, uint16_t value, uint16_t index,
	      unsigned char *buffer, uint16_t size)
{
  int ret, repeats = STALL_TRIES;
  do {
    if(repeats == STALL_TRIES)
      LogPrint(LOG_DEBUG, "control req %s 0x%x", reqname, request);
    else
      LogPrint(LOG_DEBUG, "control req 0x%x, got error %s, try %d",
	       request, strerror(errno), STALL_TRIES+1-repeats);
    ret = usbControlTransfer(usbDevice, USB_DIRECTION_OUTPUT, 
			     USB_RECIPIENT_ENDPOINT, USB_TYPE_VENDOR,
                             request, value, index, buffer, size, 100);
  } while(ret<0 && errno==EPIPE && --repeats);
  return ret;
}

#define sndcontrolmsg(request, value, index, buffer, size) \
    ({ int ret = _sndcontrolmsg(#request, request, value, index, buffer, size); \
       if(ret<0) LogPrint(LOG_ERR, "%s control op " \
			  "got error %s", #request, strerror(errno)); \
       ret; \
    })

static int
_rcvcontrolmsg(char *reqname, uint8_t request, uint16_t value, uint16_t index,
	      unsigned char *buffer, uint16_t size)
{
  int ret, repeats = STALL_TRIES;
  do {
    if(repeats == STALL_TRIES)
      LogPrint(LOG_DEBUG, "control req %s 0x%x", reqname, request);
    else
      LogPrint(LOG_DEBUG, "control req 0x%x, got error %s, try %d",
	       request, strerror(errno), STALL_TRIES+1-repeats);
    ret = usbControlTransfer(usbDevice, USB_DIRECTION_INPUT, 
			     USB_RECIPIENT_ENDPOINT, USB_TYPE_VENDOR,
                             request, value, index, buffer, size, 100);
  } while(ret<0 && errno==EPIPE && --repeats);
  return ret;
}

#define rcvcontrolmsg(request, value, index, buffer, size) \
    ({ int ret = _rcvcontrolmsg(#request, request, value, index, buffer, size); \
       if(ret<0) LogPrint(LOG_ERR, "%s query op " \
			  "got error %s", #request, strerror(errno)); \
       ret; \
    })

#define RAW_STRING_SIZE 500
#define STRING_SIZE (2*RAW_STRING_SIZE +1)
static unsigned char *
decodeString(char *rawbuf)
{
  static unsigned char str[STRING_SIZE];
  int i, len = (rawbuf[0]-2)/2;
  if(len<0)
    len = 0;
  else if(len+1 > STRING_SIZE)
    len = STRING_SIZE-1;
  for(i=0; i<len; i++)
    str[i] = rawbuf[2+2*i];
  str[i] = 0;
  return str;
}


static void 
brl_identify (void)
{
  LogPrint(LOG_NOTICE, VERSION);
  LogPrint(LOG_INFO, COPYRIGHT);
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *dev)
{
  int ret;

  if (*parameters[PARM_BRLINPUT])
    validateYesNo(&brlinput, "Allow braille input",
                  parameters[PARM_BRLINPUT]);
  brlinput = !!brlinput;

  if (!isUsbDevice(&dev)) {
    LogPrint(LOG_ERR,"Unsupported port type. Must be USB.");
    goto failure;
  }
  /* Open the Braille display device */
  LogPrint(LOG_DEBUG,"Attempting open");
  if (!(usbDevice = usbFindDevice(chooseUsbDevice, (void *)dev))) {
    LogPrint(LOG_INFO, "USB device not found.");
    goto failure;
  }
  LogPrint(LOG_DEBUG, "USB device opened.");

  {
    static const DotsTable dots
      = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(&dots, &outputTable);
    reverseTranslationTable(&outputTable, &inputTable);
  }

  dispbuf = prevdata = NULL;

  {
    /* query for length */
    unsigned char data[2];
    ret = rcvcontrolmsg(BRLVGER_GET_LENGTH, 0, 0, data, 2);
    if(ret<0) goto failure;
    switch(data[1]) {
    case 48:
      ncells = 44;
      brl->helpPage = 0;
      break;
    case 72:
      ncells = 70;
      brl->helpPage = 1;
      break;
    default:
      LogPrint(LOG_ERR,"Unsupported display length code %u", data[1]);
      goto failure;
    };
  }
  LogPrint(LOG_INFO,"Display has %u cells", ncells);
  brl_cols = ncells -NRSTATCELLS;

  {
    unsigned char rawbuf[RAW_STRING_SIZE];
    ret = rcvcontrolmsg(BRLVGER_GET_SERIAL, 0, 0,
                        rawbuf, sizeof(rawbuf));
    if (ret != -1) {
      LogPrint(LOG_INFO, "Voyager Serial Number: %s", decodeString(rawbuf));
    }
  }

  {
    unsigned char rawbuf[RAW_STRING_SIZE];
    ret = rcvcontrolmsg(BRLVGER_GET_FWVERSION, 0, 0,
                        rawbuf, sizeof(rawbuf));
    if (ret != -1) {
      LogPrint(LOG_INFO, "Voyager Firmware Version: %s", decodeString(rawbuf));
    }
  }

  ret = sndcontrolmsg(BRLVGER_SET_DISPLAY_ON, 1, 0, NULL, 0);
  if(ret<0) goto failure;

  /* cause the display to beep */
  ret = sndcontrolmsg(BRLVGER_BEEP, 200, 0, NULL, 0);

  LogPrint(LOG_DEBUG, "Starting input");
  if (!usbBeginInput(usbDevice, USB_INPUT_ENDPOINT, USB_ENDPOINT_TRANSFER_INTERRUPT, 8, 8)) goto failure;
  LogPrint(LOG_DEBUG, "input started");

  if(!(dispbuf = malloc(ncells))
     || !(prevdata = malloc(ncells)))
    goto failure;

  /* dispbuf will hold the 4 status cells followed by the text cells.
     We export directly to BRLTTY only the text cells. */
  brl->x = brl_cols;		/* initialize size of display */
  brl->y = BRLROWS;		/* always 1 */

  /* Force rewrite of display on first writebrl */
  memset(prevdata, 0xFF, ncells); /* all dots */

  readbrl_init = 1; /* init state on first readbrl */

  return 1;

failure:;
  LogPrint(LOG_WARNING,"Voyager driver giving up");
  brl_close(brl);
  return 0;
}

static void 
brl_close (BrailleDisplay *brl)
{
  if (usbDevice) {
    usbReleaseInterface(usbDevice, usbInterface);
    usbCloseDevice(usbDevice);
    usbDevice = NULL;
  }

  free(dispbuf);
  free(prevdata);
  dispbuf = prevdata = NULL;
}


static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *s)
{
  if(dispbuf)
    memcpy(dispbuf, s, NRSTATCELLS);
}


static void 
brl_writeWindow (BrailleDisplay *brl)
{
  int repeats;
  unsigned char buf[ncells];
  int i;

  memcpy(dispbuf +NRSTATCELLS, brl->buffer, brl_cols);

  /* If content hasn't changed, do nothing. */
  if(memcmp(prevdata, dispbuf, ncells) == 0)
    return;

  /* remember current content */
  memcpy(prevdata, dispbuf, ncells);

  /* translate to voyager dot pattern coding */
  for(i=0; i<=ncells; i++)
    buf[i] = outputTable[dispbuf[i]];

  /* Firmware supports multiples of 8cells, so there are extra cells
     in the firmware's imagination that don't actually exist physically.
     And for some reason there actually are holes! euurkkk! */
  /* Old kernel driver had this dirty hack: sometimes some of the dots are
     not updated somehow. Repeat the command and they right themselves.
     I don't see this behavior anymore though... */
  repeats = SENDBRAILLE_REPEATS;
  switch(ncells) {
  case 44: {
    /* Two ghost cells at the beginning of the display, plus
       two more after the sixth physical cell. */
    unsigned char hbuf[46];
    hbuf[6] = hbuf[7] = 0;
    memcpy(hbuf, buf, 6);
    memcpy(hbuf+8, buf+6, 38);
    while(repeats--)
      sndcontrolmsg(BRLVGER_SEND_BRAILLE, 0,
                    2, hbuf, 46);
    break;
  }
  case 70:
    /* Two ghost cells at the beginning of the display. */
    while(repeats--)
      sndcontrolmsg(BRLVGER_SEND_BRAILLE, 0,
		    2, buf, 70);
    break;
  };
}


/* Names and codes for display keys */

/* Top round keys behind the routing keys, numbered assuming they are
   in a configuration to type Braille on. */
#define DOT1 0x01
#define DOT2 0x02
#define DOT3 0x04
#define DOT4 0x08
#define DOT5 0x10
#define DOT6 0x20
#define DOT7 0x40
#define DOT8 0x80

/* Front keys. Codes are shifted by 8bits so they can be combined with
   DOT key codes. */
/* Leftmost */
#define K_A     0x0100
/* The next one */
#define K_B     0x0200
/* The round key to the left of the central pad */
#define K_RL  0x0400
/* Up position of central pad */
#define K_UP    0x0800
#define K_DOWN  0x1000
#define K_RR 0x2000
/* Second from the right */
#define K_C     0x4000
/* Rightmost */
#define K_D     0x8000

/* Convenience */
#define KEY(v, rcmd) \
    case v: cmd = rcmd; break;

/* OK what follows is pretty hairy. I got tired of individually maintaining
   the sources and help files so here's my first attempt at "automatic"
   generation of help files. This is my first shot at it, so be kind with
   me. */
/* These macros include an ordering hint for the help file and the help
   text. GENHLP is not defined during compilation, so at compilation the
   macros are expanded in a way that just drops the help-related
   information. */
#ifndef GENHLP
#define HKEY(n, kc, hlptxt, cmd) \
    KEY(kc, cmd);
#define PHKEY(n, prfx, kc, hlptxt, cmd) \
    KEY(kc, cmd)
#define CKEY(n, kc, hlptxt, cmd) \
    KEY(kc, cmd)
/* For pairs of symmetric commands */
#define HKEY2(n, kc1,kc2, hlptxt, cmd1,cmd2) \
    KEY(kc1, cmd1); \
    KEY(kc2, cmd2);
#define PHKEY2(n, prfx, kc1,kc2, hlptxt, cmd1,cmd2) \
    KEY(kc1, cmd1); \
    KEY(kc2, cmd2);
/* Help text only, no code */
#define HLP0(n, hlptxt)
/* Watch out: HLP0 vanishes from code, but don't put a trailing semicolon! */

#else /* GENHLP */

/* To generate the help files we do gcc -DGENHLP -E (and heavily post-process
   the result). So these macros expand to something that is easily
   searched/grepped for and "easily" post-processed. */
/* Parameters are: ordering hint, keycode, help text, and command code. */
#define HKEY(n, kc, hlptxt, cmd) \
   <HLP> n: #kc : hlptxt </HLP>
/* Add a prefix parameter, will be prepended to the key code. */
#define PHKEY(n, prfx, kc, hlptxt, cmd) \
   <HLP> n: prfx #kc : hlptxt </HLP>
/* A special case of the above for chords. */
#define CKEY(n, kc, hlptxt, cmd) \
   <HLP> n: "Chord-" #kc : hlptxt </HLP>
/* Now for pairs of symmetric commands */
#define HKEY2(n, kc1,kc2, hlptxt, cmd1,cmd2) \
   <HLP> n: #kc1 / #kc2 : hlptxt </HLP>
#define PHKEY2(n, prfx, kc1,kc2, hlptxt, cmd1,cmd2) \
   <HLP> n: prfx #kc1 / #kc2 : hlptxt </HLP>
/* Just the text, no key code */
#define HLP0(n, hlptxt) \
   <HLP> n: : hlptxt </HLP>
#endif /* GENHLP */

static int 
brl_readCommand (BrailleDisplay *brl, DriverCommandContext cmds)
{
  /* State: */
  /* For a key binding that triggers two cmds */
  static int pending_cmd = EOF;
  /* OR of all display keys pressed since last time all keys were released. */
  static unsigned keystate = 0;
  /* a flag for each routing key indicating if it was pressed since last time
     all keys were released. */
  static unsigned char rtk_pressed[MAXNRCELLS];

  /* Non-static: */
  /* ordered list of pressed routing keys by number */
  unsigned char rtk_which[MAXNRCELLS];
  /* number of entries in rtk_which */
  int howmanykeys = 0;
  /* read buffer: buf[0] for DOT keys, buf[1] for keys A B C D UP DOWN RL RR,
     buf[2]-buf[7] list pressed routing keys by number, maximum 6 keys,
     list ends with 0.
     All 0s is sent when all keys released. */
  unsigned char buf[8];
  /* recognized command */
  int cmd = CMD_NOOP;
  int i, r, release, repeat = 0;

  if(readbrl_init) {
    /* initialize state */
    readbrl_init = 0;
    pending_cmd = EOF;
    memset(rtk_pressed, 0, sizeof(rtk_pressed));
  }

  if(pending_cmd != EOF){
    cmd = pending_cmd;
    pending_cmd = EOF;
    return cmd;
  }

  r = usbReapInput(usbDevice, buf, 8, 0);
  if(r<0) {
    if(errno == EAGAIN) {
      /* no input */
      r = 0;
    }else if(errno == ENODEV) {
      /* Display was disconnected */
      return CMD_RESTARTBRL;
    }else{
      LogPrint(LOG_NOTICE,"Read error: %s.", strerror(errno));
      readbrl_init = 1;
      return EOF;
    }
  }else if(r>0 && r<8) {
    /* The display handles read requests of only and exactly 8bytes */
    LogPrint(LOG_NOTICE,"Short read %d", r);
    readbrl_init = 1;
    return EOF;
  }

  if(r==0) /* no new key */
    return EOF;
  /* one or more keys were pressed or released */

  /* We combine dot and front key info in keystate */
  keystate |= (buf[1]<<8) | buf[0];
  
  for(i=2; i<8; i++) {
    unsigned key = buf[i];
    if(!key)
      break;
    if(key < 1 || key > ncells) {
      LogPrint(LOG_NOTICE, "Invalid routing key number %u", key);
      continue;
    }
    key -= 1; /* start counting at 0 */
    rtk_pressed[key] = 1;
  }

  /* build rtk_which */
  for(howmanykeys = 0, i = 0; i < ncells; i++)
    if(rtk_pressed[i])
      rtk_which[howmanykeys++] = i;
  /* rtk_pressed[i] tells if routing key i is pressed.
     rtk_which[0] to rtk_which[howmanykeys-1] lists
     the numbers of the keys that are pressed. */

  release = (!buf[0] && !buf[1] && !buf[2]);

  /* A few keys trigger the repeat behavior: B, C, UP and DOWN,
     B+C (space bar), type dots patterns, or a dots pattern + B or C or both.
  */
  if(howmanykeys==0
     && (keystate == K_B || keystate == K_C
	 || keystate == K_UP || keystate == K_DOWN
	 || (keystate &(0xFF |K_B|K_C)) == keystate)) {
    /* Stand by to begin repeating */
    repeat = VAL_REPEAT_DELAY;
  }

  if(!repeat && !release)
    /* wait until all keys are released to decide the effect */
    return EOF;

  /* Key effect */

  if(howmanykeys == 0) {
    if(!(keystate & 0xFF)) {
      /* No routing keys, no dots, only front keys (or possibly a spurious
         release) */
      if(cmds == CMDS_PREFS) {
	switch(keystate) {
	case K_UP:
	case K_RL:
	  cmd = CMD_MENU_PREV_SETTING;
	  break;
	case K_DOWN:
	case K_RR:
	  cmd = CMD_MENU_NEXT_SETTING;
	  break;
	}
      }
      if(cmd == CMD_NOOP) {
	switch(keystate) {
	  HKEY2(101, K_A,K_D, "Move backward/forward", CMD_FWINLT,CMD_FWINRT );
	  HKEY2(101, K_B,K_C, "Move up/down", CMD_LNUP,CMD_LNDN );
	  HKEY2(101, K_A|K_B,K_A|K_C, "Goto top-left / bottom-left", 
		CMD_TOP_LEFT,CMD_BOT_LEFT );
	  HKEY(101, K_RR, "Goto cursor", CMD_HOME );
	  HKEY(101, K_RL, "Cursor tracking toggle", CMD_CSRTRK );
	  HKEY2(101, K_UP,K_DOWN, "Move cursor up/down (arrow keys)",
		VAL_PASSKEY+VPK_CURSOR_UP, VAL_PASSKEY+VPK_CURSOR_DOWN );
	  HKEY(205, K_RL|K_RR, "Freeze screen (toggle)", CMD_FREEZE);
	  HKEY(205, K_RL|K_UP, "Show attributes (toggle)", CMD_DISPMD);
	  HKEY(205, K_RR|K_UP,
	       "Show position and status info (toggle)", CMD_INFO);
	  HKEY2(501, K_RL|K_B,K_RL|K_C, 
		"Previous/next line with different attributes",
		CMD_ATTRUP, CMD_ATTRDN);
	  HKEY2(501, K_RR|K_B,K_RR|K_C, "Previous/next different line",
		CMD_PRDIFLN, CMD_NXDIFLN);
	  HKEY2(501, K_RR|K_A,K_RR|K_D, "Previous/next non-blank window",
		CMD_FWINLTSKIP, CMD_FWINRTSKIP);
	  /* typing */
	  HLP0(602, "B+C: Space (spacebar)")
	  case K_B|K_C: cmd = VAL_PASSDOTS +0; /* space: no dots */ break;
	}
      }
    }else if(!(keystate &~0xFF)) {
      /* no routing keys, some dots, no front keys */
      /* This is a character typed in braille */
      cmd = VAL_PASSDOTS | inputTable[keystate];
    }else if((keystate & (K_B|K_C)) && !(keystate & 0xFF00 & ~(K_B|K_C))) {
      /* no routing keys, some dots, combined with B or C or both but
	 no other front key */
      /* This is a chorded character typed in braille */
      switch(keystate &0xFF) {
        HLP0(601, "Chord-1478: Toggle braille input on/off")
        case DOT1|DOT4|DOT7|DOT8:
          if(release) {
            brlinput ^= 1;
            /* ouch! message() will recurse in here. */
            keystate = 0;
            message((brlinput) ? "Braille input on" : "Braille input off",
                     MSG_SILENT);
            return CMD_NOOP;
          }
          break;
	CKEY(610, DOT4|DOT6, "Return", VAL_PASSKEY + VPK_RETURN );
	CKEY(610, DOT2|DOT3|DOT4|DOT5, "Tab", VAL_PASSKEY + VPK_TAB );
	CKEY(610, DOT1|DOT2, "Backspace", VAL_PASSKEY + VPK_BACKSPACE );
	CKEY(610, DOT2|DOT4|DOT6, "Escape", VAL_PASSKEY + VPK_ESCAPE );
	CKEY(610, DOT1|DOT4|DOT5, "Delete", VAL_PASSKEY + VPK_DELETE );
	CKEY(610, DOT7, "Left arrow", VAL_PASSKEY+VPK_CURSOR_LEFT);
	CKEY(610, DOT8, "Right arrow", VAL_PASSKEY+VPK_CURSOR_RIGHT);
      }
    }
  }else{ /* Some routing keys */
    if(!keystate) {
      /* routing keys, no other keys */
      if (howmanykeys == 1) {
	switch(rtk_which[0]) {
	  HLP0(201, "CRs1: Help screen (toggle)")
	    KEY( 0, CMD_HELP );
	  HLP0(205, "CRs2: Preferences menu (and again to exit)")
	    KEY( 1, CMD_PREFMENU );
	  HLP0(501, "CRs3: Go back to previous reading location "
	       "(undo cursor tracking motion).")
	    KEY( 2, CMD_BACK );
	  HLP0(301, "CRs4: Route cursor to current line")
	    KEY( 3, CMD_CSRJMP_VERT );
	default:
	  HLP0(301,"CRt#: Route cursor to cell")
	    cmd = CR_ROUTE + rtk_which[0] -NRSTATCELLS;
	}
      }else if(howmanykeys == 3
	       && rtk_which[0] >= NRSTATCELLS
	       && rtk_which[0]+2 == rtk_which[1]){
	HLP0(405,"CRtx + CRt(x+2) + CRty : Cut text from x to y")
	  cmd = CR_CUTBEGIN + rtk_which[0] -NRSTATCELLS;
          pending_cmd = CR_CUTRECT + rtk_which[2] -NRSTATCELLS;
      }else if (howmanykeys == 2 && rtk_which[0] == 0 && rtk_which[1] == 1)
	HLP0(201,"CRs1+CRs2: Learn mode (key describer) (toggle)")
	  cmd = CMD_LEARN;
      else if (howmanykeys == 2	&& rtk_which[0] == NRSTATCELLS+1
	       && rtk_which[1] == NRSTATCELLS+2)
	HLP0(408,"CRt2+CRt3: Paste cut text")
	  cmd = CMD_PASTE;
      else if (howmanykeys == 2 && rtk_which[0] == NRSTATCELLS
	       && rtk_which[1] == NRSTATCELLS+1)
	HLP0(501,"CRt1+CRt2 / CRt<COLS-1>+CRt<COLS>: Move window left/right "
	     "one character")
	  cmd = CMD_CHRLT;
      else if (howmanykeys == 2 && rtk_which[0] == ncells-2
	       && rtk_which[1] == ncells-1)
	cmd = CMD_CHRRT;
    }
    /* routing keys and other keys */
    else if(keystate & (K_UP|K_RL|K_RR)) {
      /* Some routing keys combined with UP RL or RR (actually any key
	 combo that has at least one of those) */
      /* Treated special because we use absolute routing key numbers
	 (counting the status cell keys) */
      if(howmanykeys == 1)
	switch(keystate) {
	  PHKEY(205,"CRa#+", K_UP, "Switch to virtual console #",
		CR_SWITCHVT + rtk_which[0]);
	  PHKEY(501,"CRa#+", K_RL, "Remember current position as mark #",
		CR_SETMARK + rtk_which[0]);
	  PHKEY(501,"CRa#+", K_RR, "Goto mark #",
		CR_GOTOMARK + rtk_which[0]);
	}
      else if(howmanykeys == 2 && rtk_which[0] == 0 && rtk_which[1] == 1) {
	switch(keystate) {
	  PHKEY2(205,"CRa1+CRa2+", K_RL,K_RR, "Switch to previous/next "
		 "virtual console",
		 CMD_SWITCHVT_PREV, CMD_SWITCHVT_NEXT);
	}
      }
    }
    else if(howmanykeys == 1 && keystate == (K_A|K_D)) {
      /* One absolute routing key with A+D */
      switch(rtk_which[0]) {
	HLP0(205, "A+D +CRa1: Six dots mode (toggle)")
	  KEY( 0, CMD_SIXDOTS );
      };
    }
    else if(howmanykeys == 1 && rtk_which[0] >= NRSTATCELLS) {
      /* one text routing key with some other keys */
      switch(keystate){
	PHKEY(501,"CRt#+",K_DOWN,"Move right # cells",
	      CR_SETLEFT + rtk_which[0] -NRSTATCELLS);
	PHKEY(401, "CRt#+",K_D, "Mark beginning of region to cut",
	      CR_CUTBEGIN + rtk_which[0] -NRSTATCELLS);
	PHKEY(401, "CRt#+",K_A, "Mark bottom-right of rectangular "
	      "region and cut", 
	      CR_CUTRECT + rtk_which[0] -NRSTATCELLS);
	PHKEY2(501, "CRt#+",K_B,K_C, "Move to previous/next line indented "
	       "no more than #",
	       CR_PRINDENT + rtk_which[0] -NRSTATCELLS,
	       CR_NXINDENT + rtk_which[0] -NRSTATCELLS);
      }
    }
    else if(howmanykeys == 2 && (keystate & (K_A|K_D))
	    && rtk_which[0] >= NRSTATCELLS
	    && rtk_which[0]+1 == rtk_which[1])
      /* two consecutive text routing keys combined with A or D */
      switch(keystate){
	PHKEY(401, "CRt# + CRt(#+1) + ",K_D,
	      "Mark beginning of cut region for append",
	      CR_CUTAPPEND +rtk_which[0] -NRSTATCELLS);
	PHKEY(401, "CRt# + CRt(#-1) + ",K_A,
	      "Mark end of linear region and cut",
	       CR_CUTLINE +rtk_which[1] -NRSTATCELLS);
      }
    else if(howmanykeys == 2
	    && rtk_which[0] == NRSTATCELLS && rtk_which[1] == NRSTATCELLS+1)
      /* text routing keys 1 and 2, with B or C */
      switch(keystate){
	PHKEY2(501, "CRt1+CRt2+",K_B,K_C, "Move to previous/next "
	       "paragraph (blank line separation)",
	       CMD_PRPGRPH, CMD_NXPGRPH);
      }
    else if(howmanykeys == 2
	    && rtk_which[0] == NRSTATCELLS && rtk_which[1] == NRSTATCELLS+2) {
      /* text routing keys 1 and 3, with some other keys */
      switch(keystate){
	PHKEY2(501, "CRt1+CRt3+",K_B,K_C, "Search screen "
	       "backward/forward for cut text",
	       CMD_PRSEARCH, CMD_NXSEARCH);
      }
    }
    else if(howmanykeys == 2
	    && rtk_which[0] == NRSTATCELLS+1 && rtk_which[1] == NRSTATCELLS+2) {
      /* text routing keys 2 and 3, with some other keys */
      switch(keystate){
	PHKEY2(501, "CRt2+CRt3+",K_B,K_C, "Previous/next prompt "
	       "(same prompt as current line)",
	       CMD_PRPROMPT, CMD_NXPROMPT);
      }
    }
  }

  if(!brlinput &&
     ((keystate & 0xFF) || (keystate & (K_B|K_C)) == (K_B|K_C)))
    /* braille dot keys or spacebar is disallowed. */
    cmd = CMD_NOOP;

  if(release)
    repeat = 0;
  cmd |= repeat;

  if(release) {
    /* keys were released, clear state */
    keystate = 0;
    memset(rtk_pressed, 0, ncells);
  }

  return cmd;
}

/* Voltage: from 0->300V to 255->200V.
 * Presumably this is voltage for dot firmness.
 * Presumably 0 makes dots hardest, 255 makes them softest.
 * We are told 265V is normal operating voltage but we don't know the scale.
 */
static void
brl_firmness (BrailleDisplay *brl, int setting) {
  sndcontrolmsg(BRLVGER_SET_DISPLAY_VOLTAGE,
                      0XFF - (setting * 0XFF / BRL_MAXIMUM_FIRMNESS),
                      0, NULL, 0);
}
