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
#define VERSION \
"BRLTTY driver for Tieman Voyager, version 0.71 (January 2003)"
#define COPYRIGHT \
"   Copyright (C) 2001-2003 by Stéphane Doyon  <s.doyon@videotron.ca>\n" \
"                          and Stéphane Dalton <sdalton@videotron.ca>"
/* Voyager/braille.c - Braille display driver for Tieman Voyager displays.
 *
 * Written by:
 *   Stéphane Doyon  <s.doyon@videotron.ca>
 *   Stéphane Dalton <sdalton@videotron.ca>
 *
 * It is being tested on Voyager 44, should also support Voyager 70.
 * It is designed to be compiled in BRLTTY version 3.2.
 *
 * History:
 * 0.71, January 2003: brl->buffer now allocated by core.
 * 0.7, April 2002: The name of the kernel module changed from
 *   voyager to brlvger, so some stuff (header file name, structure
 *   name, constants...) were renamed. Note that the character device
 *   file changed from /dev/voyager major 180 minor 144 to /dev/brlvger
 *   major 180 minor 128 (now official). The kernel module is being
 *   integrated into the mainstream kernel tree. The Makefile
 *   causes us to use /usr/include/linux/brlvger.h if it exists,
 *   and kernel/linux/brlvger.h otherwise.
 * 0.6, February 2002: Added CMD_LEARN, CMD_NXPROMPT/CMD_PRPROMPT and
 *   CMD_SIXDITS. Some key bindings identification cleanups.
 * 0.5, January 2002: Added key bindings for CR_CUTAPPEND, CR_CUTLINE,
 *   CR_SETMARK, CR_GOTOMARK and CR_SETLEFT. Changed binding for NXSEARCH.
 * 0.4.1, November 2001: Added typematic repeat for braille dots typing.
 * 0.4, October 2001: First public release. Should be usable. Key bindings
 *   might benefit from some fine-tuning.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

#include "Programs/brl.h"
#include "Programs/misc.h"

typedef enum {
  PARM_REPEAT_INIT_DELAY=0,
  PARM_REPEAT_INTER_DELAY,
  PARM_DOTS_REPEAT_INIT_DELAY,
  PARM_DOTS_REPEAT_INTER_DELAY
} DriverParameter;
#define BRLPARMS "repeat_init_delay", "repeat_inter_delay", \
                 "dots_repeat_init_delay", "dots_repeat_inter_delay"

#define BRLSTAT ST_VoyagerStyle
#include "Programs/brl_driver.h"
#include "braille.h"

/* Kernel driver interface (symlink produced by Makefile) */
#include "brlvger.auto.h"

/* This defines the mapping from brltty coding to Voyager's dot coding. */
static TranslationTable outputTable;

/* This is the reverse: mapping from Voyager to brltty dot cofing. */
static unsigned char voy2brlDotsTable[256] =
{
  0x0, 0x1, 0x4, 0x5, 0x10, 0x11, 0x14, 0x15, 
  0x2, 0x3, 0x6, 0x7, 0x12, 0x13, 0x16, 0x17, 
  0x8, 0x9, 0xc, 0xd, 0x18, 0x19, 0x1c, 0x1d, 
  0xa, 0xb, 0xe, 0xf, 0x1a, 0x1b, 0x1e, 0x1f, 
  0x20, 0x21, 0x24, 0x25, 0x30, 0x31, 0x34, 0x35, 
  0x22, 0x23, 0x26, 0x27, 0x32, 0x33, 0x36, 0x37, 
  0x28, 0x29, 0x2c, 0x2d, 0x38, 0x39, 0x3c, 0x3d, 
  0x2a, 0x2b, 0x2e, 0x2f, 0x3a, 0x3b, 0x3e, 0x3f, 
  0x40, 0x41, 0x44, 0x45, 0x50, 0x51, 0x54, 0x55, 
  0x42, 0x43, 0x46, 0x47, 0x52, 0x53, 0x56, 0x57, 
  0x48, 0x49, 0x4c, 0x4d, 0x58, 0x59, 0x5c, 0x5d, 
  0x4a, 0x4b, 0x4e, 0x4f, 0x5a, 0x5b, 0x5e, 0x5f, 
  0x60, 0x61, 0x64, 0x65, 0x70, 0x71, 0x74, 0x75, 
  0x62, 0x63, 0x66, 0x67, 0x72, 0x73, 0x76, 0x77, 
  0x68, 0x69, 0x6c, 0x6d, 0x78, 0x79, 0x7c, 0x7d, 
  0x6a, 0x6b, 0x6e, 0x6f, 0x7a, 0x7b, 0x7e, 0x7f, 
  0x80, 0x81, 0x84, 0x85, 0x90, 0x91, 0x94, 0x95, 
  0x82, 0x83, 0x86, 0x87, 0x92, 0x93, 0x96, 0x97, 
  0x88, 0x89, 0x8c, 0x8d, 0x98, 0x99, 0x9c, 0x9d, 
  0x8a, 0x8b, 0x8e, 0x8f, 0x9a, 0x9b, 0x9e, 0x9f, 
  0xa0, 0xa1, 0xa4, 0xa5, 0xb0, 0xb1, 0xb4, 0xb5, 
  0xa2, 0xa3, 0xa6, 0xa7, 0xb2, 0xb3, 0xb6, 0xb7, 
  0xa8, 0xa9, 0xac, 0xad, 0xb8, 0xb9, 0xbc, 0xbd, 
  0xaa, 0xab, 0xae, 0xaf, 0xba, 0xbb, 0xbe, 0xbf, 
  0xc0, 0xc1, 0xc4, 0xc5, 0xd0, 0xd1, 0xd4, 0xd5, 
  0xc2, 0xc3, 0xc6, 0xc7, 0xd2, 0xd3, 0xd6, 0xd7, 
  0xc8, 0xc9, 0xcc, 0xcd, 0xd8, 0xd9, 0xdc, 0xdd, 
  0xca, 0xcb, 0xce, 0xcf, 0xda, 0xdb, 0xde, 0xdf, 
  0xe0, 0xe1, 0xe4, 0xe5, 0xf0, 0xf1, 0xf4, 0xf5, 
  0xe2, 0xe3, 0xe6, 0xe7, 0xf2, 0xf3, 0xf6, 0xf7, 
  0xe8, 0xe9, 0xec, 0xed, 0xf8, 0xf9, 0xfc, 0xfd, 
  0xea, 0xeb, 0xee, 0xef, 0xfa, 0xfb, 0xfe, 0xff
};

/* Braille display parameters that do not change */
#define BRLROWS 1		/* only one row on braille display */

#define MAXNRCELLS 120 /* arbitrary max for allocations */

/* We'll use 4cells as status cells, both on Voyager 44 and 70. (3cells have
   content and the fourth is blank to mark the separation.)
   NB: You can't just change this constant to vary the number of status
   cells: some key bindings for cursor routing keys assign special
   functions to routing keys over status cells.
*/
#define NRSTATCELLS 4

/* Global variables */

static int brl_fd; /* to kernel driver */

static unsigned char *prevdata, /* previous pattern displayed */
                     *dispbuf; /* buffer to prepare new pattern */
static unsigned brl_cols, /* Number of cells available for text */
                ncells; /* total number of cells including status */
static char readbrl_init; /* Flag to reinitialize readbrl function state. */
static int repeat_init_delay, repeat_inter_delay; /* key repeat rate params */
/* repeat rate params for braile dots being typed */
static int dots_repeat_init_delay, dots_repeat_inter_delay;


static void 
brl_identify (void)
{
  LogPrint(LOG_NOTICE, VERSION);
  LogPrint(LOG_INFO, COPYRIGHT);
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *dev)
{
  struct brlvger_info vi;

  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(&dots, &outputTable);
  }

  /* use user parameters */
  {
    int min = 0, max = 5000;
    if(!*parameters[PARM_REPEAT_INIT_DELAY]
       || !validateInteger(&repeat_init_delay,
			   "Delay before key repeat begins",
			   parameters[PARM_REPEAT_INIT_DELAY], &min, &max))
      repeat_init_delay = DEFAULT_REPEAT_INIT_DELAY;
    if(!*parameters[PARM_REPEAT_INTER_DELAY]
       || !validateInteger(&repeat_inter_delay, 
			   "Delay between key repeatitions",
			   parameters[PARM_REPEAT_INTER_DELAY], &min, &max))
      repeat_inter_delay = DEFAULT_REPEAT_INTER_DELAY;
    if(!*parameters[PARM_DOTS_REPEAT_INIT_DELAY]
       || !validateInteger(&dots_repeat_init_delay, 
			   "Delay before typed dots repeat begins",
			   parameters[PARM_DOTS_REPEAT_INIT_DELAY],
			   &min, &max))
      dots_repeat_init_delay = DEFAULT_DOTS_REPEAT_INIT_DELAY;
    if(!*parameters[PARM_DOTS_REPEAT_INTER_DELAY]
       || !validateInteger(&dots_repeat_inter_delay, 
			   "Delay between typed dots repeatitions",
			   parameters[PARM_DOTS_REPEAT_INTER_DELAY],
			   &min, &max))
      dots_repeat_inter_delay = DEFAULT_DOTS_REPEAT_INTER_DELAY;
  }

  dispbuf = prevdata = NULL;

  brl_fd = open (dev, O_RDWR);
  /* Kernel driver will block until a display is connected. */
  if (brl_fd < 0){
    LogPrint(LOG_ERR, "Open failed on device %s: %s", dev, strerror(errno));
    goto failure;
  }
  LogPrint(LOG_DEBUG,"Device %s opened", dev);

  /* Get display and USB kernel driver info */
  if(ioctl(brl_fd, BRLVGER_GET_INFO, &vi) <0) {
    LogPrint(LOG_ERR, "ioctl BRLVGER_GET_INFO failed on device %s: %s",
	     dev, strerror(errno));
    goto failure;
  }
  vi.driver_version[sizeof(vi.driver_version)-1] = 0;
  vi.driver_banner[sizeof(vi.driver_banner)-1] = 0;
  LogPrint(LOG_INFO, "Kernel driver version: %s", vi.driver_version);
  LogPrint(LOG_DEBUG, "Kernel driver identification: %s", vi.driver_banner);
  vi.hwver[sizeof(vi.hwver)-1] = 0;
  vi.fwver[sizeof(vi.fwver)-1] = 0;
  vi.serialnum[sizeof(vi.serialnum)-1] = 0;
  LogPrint(LOG_DEBUG, "Display hardware version: %u.%u",
	   vi.hwver[0],vi.hwver[1]);
  LogPrint(LOG_DEBUG, "Display firmware version: %s", vi.fwver);
  LogPrint(LOG_DEBUG, "Display serial number: %s", vi.serialnum);

  ncells = vi.display_length;
  if(ncells < NRSTATCELLS +5 || ncells > MAXNRCELLS) {
    LogPrint(LOG_ERR, "Returned unlikely number of cells %u", ncells);
    goto failure;
  }
  LogPrint(LOG_INFO,"Display has %u cells", ncells);
  if(ncells == 44)
    brl->helpPage = 0;
  else if(ncells == 70)
    brl->helpPage = 1;
  else{
    LogPrint(LOG_NOTICE, "Unexpected display length, unknown model, "
	     "using Voyager 44 help file.");
    brl->helpPage = 0;
  }

  brl_cols = ncells -NRSTATCELLS;

  /* cause the display to beep */
  {
    __u16 duration = 200;
    if(ioctl(brl_fd, BRLVGER_BUZZ, &duration) <0) {
      LogError("ioctl BRLVGER_BUZZ");
      goto failure;
    }
  }

  /* readbrl will want to do non-blocking reads. */
  if(fcntl(brl_fd, F_SETFL, O_NONBLOCK) <0) {
    LogError("fcntl F_SETFL O_NONBLOCK");
    goto failure;
  }

  if(!(dispbuf = (unsigned char *)malloc(ncells))
     || !(prevdata = (unsigned char *) malloc (ncells)))
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
  if (brl_fd >= 0)
    close(brl_fd);
  brl_fd = -1;
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
  unsigned char buf[ncells];
  int i;
  int start, stop, len;

  /* assert:   brl->x == brl_cols */
    
  memcpy(dispbuf +NRSTATCELLS, brl->buffer, brl_cols);

  /* If content hasn't changed, do nothing. */
  if(memcmp(prevdata, dispbuf, ncells) == 0)
    return;

  start = 0;
  stop = ncells-1;
  /* Whether or not to do partial updates... Not clear to me that it
     is worth the cycles. */
#define PARTIAL_UPDATE
#ifdef PARTIAL_UPDATE
  while(start <= stop && dispbuf[start] == prevdata[start])
    start++;
  while(stop >= start && dispbuf[stop] == prevdata[stop])
    stop--;
#endif /* PARTIAL_UPDATE */

  len = stop-start+1;
  /* remember current content */
  memcpy(prevdata+start, dispbuf+start, len);

  /* translate to voyager dot pattern coding */
  for(i=start; i<=stop; i++)
    buf[i] = outputTable[dispbuf[i]];

#ifdef PARTIAL_UPDATE
  lseek(brl_fd, start, SEEK_SET);
#endif /* PARTIAL_UPDATE */
  write(brl_fd, buf+start, len);
  /* The kernel driver currently never returns EAGAIN. If it did it would be
     wiser to select(). We don't bother to report failed writes because then
     we'd have to do rate limiting. Failures are caught in readbrl anyway. */
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
   the sources and help files so here's might first attempt at "automatic"
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
  /* Reference time for fastkey (typematic / key repeat) */
  static struct timeval presstime;
  /* key repeat state: 0 not a fastkey (not a key that repeats), 
     1 waiting for initial delay to begin repeating (can still be combined
     with other keys),
     2 key effect occured at least once and now waiting for next repeat,
     3 during repeat another key was pressed, so lock up and do nothing
     until keys are released. */
  static int fastkey = 0;
  /* type of key being repeated: valid when fastkey is 1 or 2. fastdots=0
     means movement keys (fast), fastdots=1 means brialle dots or space
     (longer delay and slower repeat). */
  static int fastdots = 0;
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
  int i, r, cmd = EOF;
  int ignore_release = 0;

  if(readbrl_init) {
    /* initialize state */
    readbrl_init = 0;
    pending_cmd = EOF;
    keystate = 0;
    fastkey = 0;
    memset(rtk_pressed, 0, sizeof(rtk_pressed));
  }

  if(pending_cmd != EOF){
    cmd = pending_cmd;
    pending_cmd = EOF;
    return cmd;
  }

  r = read(brl_fd, buf, 8);
  if(r<0) {
    if(errno == ENOLINK)
      /* Display was disconnected */
      return CMD_RESTARTBRL;
    if(errno != EAGAIN && errno != EINTR) {
      LogPrint(LOG_NOTICE,"Read error: %s", strerror(errno));
      readbrl_init = 1;
      return EOF;
      /* If some errors are discovered to occur and are fatal we should
	 return CMD_RESTARTBRL for those. For now, this shouldn't happen. */
    }
  }else if(r==0) {
    /* Should not happen */
    LogPrint(LOG_NOTICE,"Read returns EOF!");
    readbrl_init = 1;
    return EOF;
  }else if(r<8) {
    /* The driver wants and handles read requests of only and exactly 8bytes */
    LogPrint(LOG_NOTICE,"Read returns short count %d", r);
    readbrl_init = 1;
    return EOF;
  }

  if(r<0) { /* no new key */
    /* handle key repetition */
    struct timeval now;
    /* If no repeatable keys are pressed then do nothing. */
    if(!fastkey)
      return EOF;
    /* If a key repeat was interrupted by the press of another key, do
       nothing and wait for the keys to be released. */
    if(fastkey == 3)
      return EOF;
    gettimeofday(&now, NULL);
    if(!((fastkey == 1 && elapsedMilliseconds(&presstime, &now)
	  > ((fastdots) ? dots_repeat_init_delay : repeat_init_delay))
	 || (fastkey > 1 && elapsedMilliseconds(&presstime, &now)
	     > ((fastdots) ? dots_repeat_inter_delay : repeat_inter_delay))))
      return EOF;
    fastkey = 2;
    memcpy(&presstime, &now, sizeof(presstime));
  }else{ /* one or more keys were pressed or released */
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

    /* A few keys trigger the repeat behavior: B, C, UP and DOWN,
       B+C (space bar), type dots patterns, or a dots pattern + B or C or both.
    */
    if(fastkey <= 1 && howmanykeys==0 && (buf[0] || buf[1] /*press event*/)
       && (keystate == K_B || keystate == K_C
	   || keystate == K_UP || keystate == K_DOWN
	   || (keystate &(0xFF |K_B|K_C)) == keystate)) {
      /* Stand by to begin repeating */
      gettimeofday(&presstime, NULL);
      fastkey = 1;
      fastdots = ((keystate & 0xFF) || keystate == (K_B|K_C));
      return EOF;
    }else{
      if(fastkey == 2 || fastkey == 3) {
	/* A key was repeating and its effect has occured at least once. */
	if(buf[0] || buf[1] || buf[2]) {
	  /* wait for release */
	  fastkey = 3;
	  return EOF;
	}
	/* ignore release (goto clear state) */
	ignore_release = 1;
	fastkey = 0;
      }else{
	/* If there was any key waiting to repeat it is stil within
	   repeat_init_delay timeout, so allow combination. */
	fastkey = 0;
      }
    }

    if(buf[0] || buf[1] || buf[2])
      /* wait until all keys are released to decide the effect */
      return EOF;
  }

  /* Key effect */

  if(ignore_release); /* do nothing */
  else if(howmanykeys == 0) {
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
      if(cmd == EOF) {
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
	  HLP0(601, "B+C: Space (spacebar)")
	  case K_B|K_C: cmd = VAL_PASSDOTS +0; /* space: no dots */ break;
	}
      }
    }else if(!(keystate &~0xFF)) {
      /* no routing keys, some dots, no front keys */
      /* This is a character typed in braille */
      cmd = VAL_PASSDOTS | voy2brlDotsTable[keystate];
    }else if((keystate & (K_B|K_C)) && !(keystate & 0xFF00 & ~(K_B|K_C))) {
      /* no routing keys, some dots, combined with B or C or both but
	 no other front keys. */
      /* This is a chorded character typed in braille */
      switch(keystate &0xFF) {
	CKEY(601, DOT4|DOT6, "Return", VAL_PASSKEY + VPK_RETURN );
	CKEY(601, DOT2|DOT3|DOT4|DOT5, "Tab", VAL_PASSKEY + VPK_TAB );
	CKEY(601, DOT1|DOT2, "Backspace", VAL_PASSKEY + VPK_BACKSPACE );
	CKEY(601, DOT2|DOT4|DOT6, "Escape", VAL_PASSKEY + VPK_ESCAPE );
	CKEY(601, DOT1|DOT4|DOT5, "Delete", VAL_PASSKEY + VPK_DELETE );
	CKEY(601, DOT7, "Left arrow", VAL_PASSKEY+VPK_CURSOR_LEFT);
	CKEY(601, DOT8, "Right arrow", VAL_PASSKEY+VPK_CURSOR_RIGHT);
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

  if(!fastkey) {
    /* keys were released, clear state */
    keystate = 0;
    fastkey = 0;
    memset(rtk_pressed, 0, ncells);
  }

  return cmd;
}
