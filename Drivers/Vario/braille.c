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

/* Vario/braille.c - Braille display driver for the Baum Vario 40 and 80 displays
 *
 * Written by Mario Lang <mlang@delysid.org>
 *
 * This is version 0.2 (August 2000) of the driver.
 * It attempts support for Baum Vario 40 and 80 in native Baum mode (Emul. 1).
 * It is designed to be compiled in BRLTTY versions 2.95.
 *
 * Most of the underlying code structure is borrowed from the TSI
 * driver written by Stéphane Doyon (s.doyon@videotron.ca), and credits go to
 * him for his nice work and especially for the well documented code.
 *
 * History:
 * Version 1.0: Initial patch submittion.
 */

#define VERSION "BRLTTY driver for BAUM Vario (Emul. 1) displays, version 1.0 (September 2000)"
#define COPYRIGHT "Copyright (C) 2000 by Mario Lang " \
                  "<mlang@delysid.org>"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/termios.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

#include "Programs/brl.h"
#include "Programs/misc.h"

#define BRLNAME	"Vario"
#define PREFSTYLE ST_TiemanStyle

#include "Programs/brl_driver.h"
#include "braille.h"

/* Braille display parameters that do not change */
#define BRLROWS 1		/* only one row on braille display */

#define V80BRLCOLS 80              /* The Vario 80 has 80 cells (+4 status) */
#define V80NCELLS 84
#define V80TSPDATACNT 11
#define V40BRLCOLS 40
#define V40NCELLS 40
#define V40TSPDATACNT 5
#define BAUDRATE B19200         /* But both run at 19k2 */

static int brlcols;
static int ncells;
static int tspdatacnt;   /* Number of bytes to await when getting TSP-data */
#ifdef USE_PING
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
#endif /* USE_PING */

/* for routing keys */
static int must_init_oldstat, must_init_code = 1;

/* Definitions to avoid typematic repetitions of function keys other
   than movement keys */
#define NONREPEAT_TIMEOUT 300
#define READBRL_SKIP_TIME 300
static int lastcmd = EOF;
static char typing_mode; /* For Vario80 - holds the mode of Command keys */
static char button_waitcount = 0;
static char ck;
static struct timeval lastcmd_time;
static struct timezone dum_tz;
/* Those functions it is OK to repeat */
static int repeat_list[] =
{CMD_FWINRT, CMD_FWINLT, CMD_LNUP, CMD_LNDN, CMD_WINUP, CMD_WINDN,
 VAL_PASSKEY+VPK_CURSOR_LEFT, VAL_PASSKEY+VPK_CURSOR_RIGHT, VAL_PASSKEY+VPK_CURSOR_UP, VAL_PASSKEY+VPK_CURSOR_DOWN,
 CMD_PRDIFLN, CMD_NXDIFLN, 0};

static char DotsTable[256] =
{
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

/* delay between auto-detect attemps at initialization */
#define DETECT_DELAY (2500)	/* msec */

/* Communication codes */
#define ESC 0x1B
static char VARIO_DISPLAY[] = {ESC, 0x01};
#define VARIO_DISPLAY_LEN 2
static char VARIO_DEVICE_ID[] = {ESC, 0x84};
#define VARIO_DEVICE_ID_LEN 2
#define VARIO_DEVICE_ID_REPLY_LEN 18
#define MAXREAD 18
/* Global variables */

static int brl_fd;              /* file descriptor for comm port */
static struct termios oldtio,	/* old terminal settings for com port */
                      curtio;   /* current settings */
static unsigned char *rawdata,	/* translated data to send to display */
                     *prevdata, /* previous data sent */
                     *dispbuf;

static void brl_identify (void)
{
  LogPrint(LOG_NOTICE, VERSION);
  LogPrint(LOG_INFO, "   "COPYRIGHT);
}

static int myread(int fd, void *buf, unsigned len)
/* This is a replacement for read for use in nonblocking mode: when
   c_cc[VTIME] = 1, c_cc[VMIN] = 0. We want to read len bytes into buf, but
   return early if the timeout expires. */
{
  char *ptr = (char *)buf;
  int r, l = 0;
  while (l < len) {
    r = read(fd, ptr+l, len-l);
    if (r == 0)
      return(l);
    else if (r<0)
      return(r);
    l += r;
  }
  return(l);
}

static int QueryDisplay(int brl_fd, char *reply)
/* For auto-detect: send query, read response and validate response. */
{
  int r=-1;
  if (write(brl_fd, VARIO_DEVICE_ID, VARIO_DEVICE_ID_LEN) == VARIO_DEVICE_ID_LEN &&
      (r = myread (brl_fd, reply, VARIO_DEVICE_ID_REPLY_LEN)) == VARIO_DEVICE_ID_REPLY_LEN &&
      !memcmp (reply, VARIO_DEVICE_ID, VARIO_DEVICE_ID_LEN)) {
    reply[VARIO_DEVICE_ID_REPLY_LEN] = 0;
    LogPrint(LOG_DEBUG,"Valid reply received '%s'", reply+2);
    return 1;
  }
  LogPrint(LOG_DEBUG,"Invalid reply of %d bytes", r);
  return 0;
}

static int brl_open (BrailleDisplay *brl, char **parameters, const char *tty)
{
  int i = 0;
  char reply[VARIO_DEVICE_ID_REPLY_LEN+1];

  brl->buffer = rawdata = prevdata = NULL;

  /* Open the Braille display device for random access */
  brl_fd = open (tty, O_RDWR | O_NOCTTY);
  if (brl_fd < 0) {
    LogPrint(LOG_ERR, "Open failed on port %s: %s", tty, strerror(errno));
    goto failure;
  }
  if(!isatty(brl_fd)) {
    LogPrint(LOG_ERR,"Opened device %s is not a tty!", tty);
    goto failure;
  }
  LogPrint(LOG_DEBUG,"Tty %s opened", tty);

  tcgetattr (brl_fd, &oldtio);	/* save current settings */
  /* we don't check the return code. could only be EBADF or ENOTTY. */

  /* Construct new settings by working from current state */
  memcpy(&curtio, &oldtio, sizeof(struct termios));
  rawSerialDevice(&curtio);
  /* local */
  curtio.c_lflag &= ~TOSTOP; /* no SIGTTOU to backgrounded processes */
  /* control */
  curtio.c_cflag |= CLOCAL /* ignore status lines (carrier detect...) */
                  | CREAD; /* enable reading */
  curtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
#if 0
  curtio.c_cflag &= ~( CSTOPB /* 1 stop bit */
		      | CRTSCTS /* disable hardware flow control */
                      | PARENB /* disable parity generation and detection */
                     );
#endif /* 0 */
  /* input */
  curtio.c_iflag &= ~( INPCK /* no input parity check */
                      | ~IXOFF /* don't send XON/XOFF */
		     );

  /* noncanonical: for first operation */
  curtio.c_cc[VTIME] = 1;  /* 0.1sec timeout between chars on input */
  curtio.c_cc[VMIN] = 0; /* no minimum input. */

  /* Make the settings active and flush the input/output queues. This time we
     check the return code in case somehow the settings are invalid. */
  if(tcsetattr (brl_fd, TCSAFLUSH, &curtio) == -1) {
    LogPrint(LOG_ERR, "tcsetattr: %s", strerror(errno));
    goto failure;
  }

  /* Try to detect display by sending query */
  while(1) {
    /* Reset serial port config, in case some other program interfered and
       BRLTTY is resetting... */
    if(tcsetattr (brl_fd, TCSAFLUSH, &curtio) == -1) {
      LogPrint(LOG_ERR, "tcsetattr: %s", strerror(errno));
      goto failure;
    }
    LogPrint(LOG_DEBUG,"Sending query");
    if(QueryDisplay(brl_fd,reply)) break;
    delay(DETECT_DELAY);
  }
  LogPrint(LOG_DEBUG, "reply = '%s'", reply);
  if (reply[13] == '8') {
    brlcols = V80BRLCOLS;
    ncells = V80NCELLS;
    tspdatacnt = V80TSPDATACNT;
    brl->helpPage = 0;
  } else if (reply[13] == '4') {
    brlcols = V40BRLCOLS;
    ncells = V40NCELLS;
    tspdatacnt = V40TSPDATACNT;
    brl->helpPage = 1;
  } else { goto failure; };

  /* Mark time of last command to initialize typematic watch */
#ifdef USE_PING
  gettimeofday (&last_ping, &dum_tz);
  memcpy(&lastcmd_time, &last_ping, sizeof(struct timeval));
  pings=0;
#else /* USE_PING */
  gettimeofday (&lastcmd_time, &dum_tz);
#endif /* USE_PING */
  lastcmd = EOF;
  must_init_oldstat = must_init_code = 1;

  brl->x = brlcols;		/* initialise size of display */
  brl->y = BRLROWS;		/* always 1 (I want a 5 line display!!) */

  /* Allocate space for buffers */
  dispbuf = brl->buffer = (unsigned char *) malloc (ncells);
  prevdata = (unsigned char *) malloc (ncells);
  rawdata = (unsigned char *) malloc (2 * ncells + VARIO_DISPLAY_LEN);
  /* 2* to insert ESCs if char is ESC */
  if (!brl->buffer || !prevdata || !rawdata)
    goto failure;

  /* Initialize rawdata. It will be filled in and used directly to
     write to the display in writebrl(). */
  for (i = 0; i < VARIO_DISPLAY_LEN; i++)
    rawdata[i] = VARIO_DISPLAY[i];
  memset (rawdata + VARIO_DISPLAY_LEN, 0, 2 * ncells * BRLROWS);

  /* Force rewrite of display on first writebrl */
  memset(prevdata, 0x00, ncells);

  typing_mode = 0; /* sets CK keys to command mode - 1 = typing mode */
  return 1;

failure:;
  LogPrint(LOG_WARNING,"Baum Vario driver giving up");
  brl_close(brl);
  return 0;
}

static void brl_close (BrailleDisplay *brl)
{
  if (brl_fd >= 0) {
    tcsetattr (brl_fd, TCSADRAIN, &oldtio);
    close (brl_fd);
  }
  if (brl->buffer)
    free (brl->buffer);
  if (rawdata)
    free (rawdata);
  if (prevdata)
    free (prevdata);
}

/* Display a NCELLS long buffer on the display. 
   Vario 80 only supports a full update in native Baum mode. */
static void display(const unsigned char *buf)
{
  int i, escs = 0;
  for (i=0;i<ncells;i++) {
    rawdata[VARIO_DISPLAY_LEN+i+escs] = DotsTable[buf[i]];
    if (rawdata[VARIO_DISPLAY_LEN+i+escs] == ESC) {
      /* We need to take care of the ESC char, which is used for infotypess. */
      escs++;
      rawdata[VARIO_DISPLAY_LEN+i+escs] = ESC;
    }
  }
  write(brl_fd, rawdata, VARIO_DISPLAY_LEN+ncells+escs);
  tcdrain(brl_fd); /* Does this help? It seems at it made the scrolling
		      smoother */
}

static void brl_writeStatus (BrailleDisplay *brl, const unsigned char *s)
/* We have 4 status cells. Unclear how to handle. 
   I choose Tieman style. Any other suggestions? */
{
  int i;
  if (ncells > brlcols) {  /* When we have status cells */
    for (i=0;i<ncells-brlcols;i++) {
      dispbuf[brlcols+i] = s[i];
    }
  }
}

static void brl_writeWindow (BrailleDisplay *brl)
{
  int start, stop;
    
  if (brl->x != brlcols || brl->y != BRLROWS || brl->buffer != dispbuf)
    return;
    
  /* Determining start and stop for memcpy and prevdata */
  for (start=0;start<ncells;start++)
    if(brl->buffer[start] != prevdata[start]) break;
  if(start == ncells) return;

  for(stop = ncells-1; stop > start; stop--)
    if(brl->buffer[stop] != prevdata[stop]) break;
    
  /* Is it better to do it this way, or should we copy the whole display.
     Vario Emulation 1 doesnt support partial updates. So this kinda
     makes no sense */
  memcpy(prevdata+start, brl->buffer+start, stop-start+1);
  display (brl->buffer);
}


static int is_repeat_cmd (int cmd) {
  int *c = repeat_list;
  while (*c != 0)
    if (*(c++) == cmd)
      return (1);
  return (0);
}

static int brl_readCommand(BrailleDisplay *brl, DriverCommandContext cmds) {
#define TSP 0x22
#define BUTTON 0x24
#define KEY_TL1 (1<<0)
#define KEY_TL2 (1<<1)
#define KEY_TL3 (1<<2)
#define KEY_TR1 (1<<3)
#define KEY_TR2 (1<<4)
#define KEY_TR3 (1<<5)

#define FRONTKEY 0x28
#define KEY_LU (1<<8)
#define KEY_LD (1<<9)
#define KEY_MU (1<<10)
#define KEY_MD (1<<11)
#define KEY_RU (1<<12)
#define KEY_RD (1<<13)
/* Backkeys are not implemented zet. I don't see any use in them. */

#define COMMANDKEY 0x2B
#define KEY_CK1 (1<<16)
#define KEY_CK2 (1<<17)
#define KEY_CK3 (1<<18)
#define KEY_CK4 (1<<19)
#define KEY_CK5 (1<<20)
#define KEY_CK6 (1<<21)
#define KEY_CK7 (1<<22)

#define SW_CHK(swnum) \
      ( sw_oldstat[swnum/8] & (1 << (swnum % 8)) )
#define KEY(code,result) \
    case code: res = result; break;
#define KEYAND(code) \
    case code:

  static unsigned char sw_oldstat[V80TSPDATACNT];
  unsigned char sw_stat[tspdatacnt]; /* received switch data */
  static unsigned char sw_which[V80NCELLS], /* list of pressed keys */
                       sw_howmany = 0; /* length of that list */
  static unsigned char ignore_routing = 0;
     /* flag: after combo between routing and non-routing keys, don't act
	on any key until routing resets (releases). */
  static unsigned int code;  /* 32bits code representing pressed keys once the
			        input bytes are interpreted */

  int res = EOF; /* command code to return. code is mapped to res. */
  static int pending_cmd = EOF;
  char buf[MAXREAD], /* read buffer */
       infotype = 0; /* infotype being received (state) */
  unsigned i;
  struct timeval now;

  if (pending_cmd != EOF) {
    res = pending_cmd;
    lastcmd = EOF;
    pending_cmd = EOF;
    return(res);
  }

  if (must_init_code) {
    must_init_code = 0;
    sw_howmany = 0;
    code = 0;
    goto calcres;
  }

  /* reset to nonblocking */
  curtio.c_cc[VTIME] = 0;
  curtio.c_cc[VMIN] = 0;
  tcsetattr (brl_fd, TCSANOW, &curtio);
  gettimeofday (&now, &dum_tz);
  /* Check for first byte */
  if (!read (brl_fd, buf, 1)){
    if (button_waitcount >= BUTTON_STEP) { 
      goto calcres;
    }
    else if (button_waitcount > 0 &&
	     button_waitcount < BUTTON_STEP)
      button_waitcount++;
    else if (elapsedMilliseconds(&lastcmd_time, &now) > REPEAT_TIME &&
	     is_repeat_cmd(lastcmd)) {
      gettimeofday(&lastcmd_time, &dum_tz);
      return(lastcmd);
    }
#ifdef USE_PING
    else if ((i = elapsedMilliseconds(&last_ping, &now) > PING_INTRVL)){
      int ping_due = (pings==0 || (elapsedMilliseconds(&last_ping_sent, &now)
				   > PING_REPLY_DELAY));
      if(pings>=PING_MAXNQUERY && ping_due)
	return CMD_RESTARTBRL;
      else if (ping_due){
	LogPrint(LOG_DEBUG, "Display idle: sending query");
	write (brl_fd, VARIO_DEVICE_ID, VARIO_DEVICE_ID_LEN);
	tcdrain(brl_fd);
	pings++;
	gettimeofday(&last_ping_sent, &dum_tz);
      }
    }
#endif /* USE_PING */
    return (EOF);
  }
  /* there was some input, we heard something. */
#ifdef USE_PING
  gettimeofday(&last_ping, &dum_tz);
  pings=0;
#endif /* USE_PING */

  /* further reads will wait a bit to get a complete sequence */
  curtio.c_cc[VTIME] = 1;
  curtio.c_cc[VMIN] = 0;
  tcsetattr (brl_fd, TCSANOW, &curtio);

  /* read bytes */
  i=0;
  while(1){
    if(i==0){
      if(buf[0]==ESC) infotype = ESC;
      else return(EOF);
      /* unrecognized byte (garbage...?) */
    } else { /* i>0 */
      if(infotype==ESC){
	if(buf[1]==VARIO_DEVICE_ID[1]) infotype = VARIO_DEVICE_ID[1];
	else if (buf[1]==TSP) infotype = TSP;
	else if (buf[1]==BUTTON) infotype = BUTTON;
	else if (buf[1]==FRONTKEY) infotype = FRONTKEY;
	else if (buf[1]==COMMANDKEY) infotype = COMMANDKEY;
	else return(EOF);
	break;
      } /* else return(EOF) */;
    }
    i++;
    if (!myread (brl_fd, buf+i, 1))
      return (EOF);
  } /* while */
  if (must_init_oldstat) {
    must_init_oldstat = 0;
    ignore_routing = 0;
    sw_howmany = 0;
    for (i=0;i<tspdatacnt;i++)
      sw_oldstat[i] = 0;
  }


  if (infotype == TSP) {
    /* Read the TSP data */
    if (myread (brl_fd, sw_stat, tspdatacnt) != tspdatacnt) {
      return (EOF);
    }
    for (i=0; i<tspdatacnt; i++) {
      sw_oldstat[i] |= sw_stat[i];
    }
    for(sw_howmany=0, i=0; i<ncells; i++)
      if(SW_CHK (i))
	sw_which[sw_howmany++] = i;
    for(i=0; i<tspdatacnt; i++)
      if(sw_stat[i]!=0)
	return (EOF);
    must_init_oldstat = 1;
    if(ignore_routing) return(EOF);
  }
  else if (infotype == VARIO_DEVICE_ID[1]) {
    LogPrint(LOG_DEBUG,"Got reply to idle ping");
    myread(brl_fd, buf, VARIO_DEVICE_ID_REPLY_LEN - 2);
    return(EOF);
  } else if (infotype == BUTTON || infotype == FRONTKEY ||
	     infotype == COMMANDKEY) {
    char c;
    if (!read(brl_fd, &c, 1)) {
      return(EOF);
    }
    switch (infotype) {
    case BUTTON:
      code |= c;
      if (c == 0 && lastcmd == EOF) {
	must_init_code = 1;
      }
      else if (c == 0 && is_repeat_cmd(lastcmd)) {
	code = 0;
      }
      else if (button_waitcount < BUTTON_STEP) {
	button_waitcount++;
	return(EOF);
      }
      LogPrint(LOG_DEBUG, "case BUTTON: c = %d, code=%d", c, code);

      break;
    case FRONTKEY:   code = (c<<8);  break;
    case COMMANDKEY:
      ck |= c;
      if (c != 0) return(EOF);
      if (ck == 127) {
	typing_mode ^= 1;
	ck = 0;
	return(EOF);
      }
      if (typing_mode) {
	char t = 0;
	/* Moving bits around 
	   Bits should get changed like this:
	   87654321  ->  84716253
	*/
	if (ck & (1<<2)) t |= (1<<0);
	if (ck & (1<<4)) t |= (1<<1);
	if (ck & (1<<1)) t |= (1<<2);
	if (ck & (1<<5)) t |= (1<<3);
	if (ck & (1<<0)) t |= (1<<4);
	if (ck & (1<<6)) t |= (1<<5);
	if (ck & (1<<3)) t |= (1<<6);
	ck = 0;
	/* Define some special combinations. I only use dot 7 in combination
	   with dot 2, 3, 4, 5 and 6. */
	if (t == (1<<6)) t = 0;  /* Space is 00000000 */
	else if (t == ((1<<6)|(1<<1))) return(VAL_PASSKEY+VPK_RETURN);
	else if (t == ((1<<6)|(1<<2))) return(VAL_PASSCHAR+127);
	else if (t == ((1<<6)|(1<<3))) return(VAL_PASSCHAR+9);
	else if (t == ((1<<6)|(1<<4))) return(VAL_PASSKEY+VPK_CURSOR_LEFT);
	else if (t == ((1<<6)|(1<<5))) return(VAL_PASSKEY+VPK_CURSOR_RIGHT);
	return(VAL_PASSDOTS+t);
      } else {
	code |= (ck<<16);
	ck = 0;
	must_init_code = 1;
      }
      break;
    }
  } else return(EOF);

 calcres:;
  if (sw_howmany) {  /* Some Routing keys were pressed */
    /* Cut-n-paste: */
    if (sw_howmany == 3 && sw_which[0]+2 == sw_which[1]) { /* Vario80 */
      res = CR_CUTBEGIN + sw_which[0];
      pending_cmd = CR_CUTRECT + sw_which[2];
    } else if (sw_howmany == 3 && 
	       sw_which[2] == brlcols-1 && sw_which[1] == brlcols-2 &&
	       sw_which[0] < brlcols) {
      res = CR_CUTBEGIN + sw_which[0];
    } else if (sw_howmany == 3 &&
	       sw_which[0] == 0 && sw_which[1] == 1 &&
	       sw_which[2] < brlcols) {
      res = CR_CUTRECT + sw_which[2];
    } else if (sw_howmany == 2 && sw_which[1] == 80 && sw_which[0] < 80) {
      res = CR_CUTBEGIN + sw_which[0];
    } else if (sw_howmany == 2 && sw_which[1] == 81 && sw_which[0] < 80) {
      res = CR_CUTRECT + sw_which[0];
    } else if (sw_howmany == 2 && sw_which[0] == 0 && sw_which[1] == 1) {
      res = CMD_CHRLT;
    } else if (sw_howmany == 2 && sw_which[0] == brlcols-2 && sw_which[1] == brlcols-1) {
      res = CMD_CHRRT;
    } else if ((sw_howmany == 1 && sw_which[0] == 83) /* Vario80 */ ||
	       (sw_howmany == 2 && sw_which[0] == 1 && sw_which[1] == 2)) {
      res = CMD_PASTE;
    } else if (sw_howmany == 2 &&
	       sw_which[0] == 0 && sw_which[1] == brlcols-1) {
      res = CMD_HELP;
    } else if (sw_howmany == 1 && sw_which[0] < brlcols) {
      res = CR_ROUTE+sw_which[0];
    }
  } else if (code) {
    /* Due to ergonomic reasons, I decided to use
     * a non-standard setup of keys at the Vario 80.
     * The Vario40 keybindings are quite the same as in the DOS and Widnows drivers.
     * Feel free to change this to your needs. */
    switch (code) {
      KEY(KEY_TL1|KEY_TR1, CMD_PREFMENU);
      KEY(KEY_TL1|KEY_TL2|KEY_TR2, CMD_HELP);
      KEY(KEY_TL1|KEY_TL2|KEY_TR1, CMD_FREEZE);
      KEY(KEY_TL2|KEY_TR1, CMD_INFO);
      KEY(KEY_TL1|KEY_TL3, CMD_HOME);
      KEY(KEY_TL1 | KEY_TL2, CMD_TOP_LEFT);
      KEY(KEY_TL3 | KEY_TL2, CMD_BOT_LEFT);
      KEY(KEY_TL2|KEY_TL3|KEY_TR1|KEY_TR2, CMD_CSRTRK);
      KEY(KEY_TL1|KEY_TL3|KEY_TR3, CMD_ATTRVIS);
      KEY(KEY_TL1|KEY_TL2|KEY_TL3|KEY_TR3, CMD_DISPMD);
    }
    if (brlcols == 40) {
      switch (code) {
	KEY(KEY_TL1, CMD_LNUP);
	KEY(KEY_TL2, CMD_FWINLT);
	KEY(KEY_TL3, CMD_LNDN);
	KEY(KEY_TR1, VAL_PASSKEY+VPK_CURSOR_UP);
	KEY(KEY_TR2, CMD_FWINRT);
	KEY(KEY_TR3, VAL_PASSKEY+VPK_CURSOR_DOWN);
      }
    } else if (brlcols == 80) {
      switch (code) {
	KEY(KEY_TL1, CMD_TOP_LEFT);
	KEY(KEY_TL2, CMD_HOME);
	KEY(KEY_TL3, CMD_BOT_LEFT);
	KEY(KEY_TR1, CMD_LNUP);
	KEY(KEY_TR2, VAL_PASSKEY+VPK_RETURN);
	KEY(KEY_TR3, CMD_LNDN);
	KEY(KEY_MU, CMD_LNUP);
	KEY(KEY_MD, CMD_LNDN);
	KEY(KEY_RU, CMD_DISPMD);
	KEY(KEY_RD, CMD_CSRTRK);

	KEY(KEY_LU, VAL_PASSKEY+VPK_CURSOR_UP);
	KEY(KEY_LD, VAL_PASSKEY+VPK_CURSOR_DOWN);
	KEY(KEY_CK4, VAL_PASSKEY+VPK_RETURN);

	KEY(KEY_CK3 | KEY_CK5, CMD_PREFMENU);
	KEY(KEY_CK1 | KEY_CK3 | KEY_CK7, CMD_ATTRVIS);
	KEY(KEY_CK1, VAL_PASSKEY+VPK_CURSOR_LEFT);
	KEY(KEY_CK7, VAL_PASSKEY+VPK_CURSOR_RIGHT);
	KEY(KEY_CK2 | KEY_CK3 | KEY_CK5, CMD_FREEZE);
	KEY(KEY_CK2 | KEY_CK3 | KEY_CK6, CMD_HELP);
	KEY(KEY_CK2           | KEY_CK5, CMD_INFO);
      }
    }
  }

  lastcmd = res;
  gettimeofday (&lastcmd_time, &dum_tz);
  button_waitcount = 0;
  return(res);
}
