/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 * Nicolas Pitre <nico@cam.org>
 * Stephane Doyon <doyons@jsp.umontreal.ca>
 *
 * Version 1.0.2, 17 September 1996
 *
 * Copyright (C) 1995, 1996 by Nikhil Nair and others.  All rights reserved.
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nikhil Nair <nn201@cus.cam.ac.uk>.
 */

/* brltty.c - main() plus signal handling and cursor routing
 * $Id: brltty.c,v 1.9 1996/10/01 15:55:10 nn201 Exp $
 */

#define BRLTTY_C 1

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "config.h"
#include "brl.h"
#include "scr.h"
#include "misc.h"
#include "beeps.h"
#include "cut-n-paste.h"
#include "speech.h"

#define VERSION "BRLTTY 1.0.2d"
#define COPYRIGHT "Copyright (C) 1995, 1996 by Nikhil Nair and others.  \
All rights reserved."
#define USAGE "Usage: %s [options]\n\
 -c config-file       use binary configuration file `config-file'\n\
 -d serial-device     use `serial-device' to access Braille terminal\n\
 -t text-trans-file   use translation table `text-trans-file'\n\
 -h, --help           print this usage message\n\
 -q, --quiet          suppress start-up messages\n\
 -v, --version        print start-up messages and exit\n"
#define ENV_MAGICNUM 0x4003

/* Some useful macros: */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define BRL_ISUPPER(c) (isupper (c) || (c) == '@' || (c) == '[' || (c) == '^' \
			|| (c) == ']' || (c) == '\\')
#define TOGGLEPLAY(var) play ((var) ? snd_toggleon : snd_toggleoff)

/* Argument for usetable() */
#define TBL_TEXT 0		/* use text translation table */
#define TBL_ATTRIB 1		/* use attribute translation table */

/* Global variables */
struct brltty_env		/* BRLTTY environment settings */
  {
    short magicnum;
    short csrvis;
    short csrblink;
    short capblink;
    short csrsize;
    short csroncnt;
    short csroffcnt;
    short caponcnt;
    short capoffcnt;
    short sixdots;
    short slidewin;
    short sound;
    short skpidlns;
    short stcellstyle;
  }
env, initenv =
{
  ENV_MAGICNUM, INIT_CSRVIS, INIT_CSRBLINK, INIT_CAPBLINK, INIT_CSRSIZE,
    INIT_CSR_ON_CNT, INIT_CSR_OFF_CNT, INIT_CAP_ON_CNT, INIT_CAP_OFF_CNT,
    INIT_SIXDOTS, INIT_SLIDEWIN, INIT_BEEPSON, INIT_SKPIDLNS,
#if defined (Alva_ABT3)
    ST_AlvaStyle
#elif defined (CombiBraille)
    ST_TiemanStyle
#else
    ST_None
#endif
};

short csrtrk = INIT_CSRTRK;
short dispmode = 0;		/* start by displaying text */
short dispmd = LIVE_SCRN;	/* freeze screen off */
short infmode = 0;		/* display screen image rather than info */
short csr_offright;		/* used for sliding window */
short winx, winy;		/* Start row and column of Braille window */
short cox, coy;			/* Old cursor position */
short hwinshift;		/* Half window horizontal distance */
short fwinshift;		/* Full window horizontal distance */
short vwinshift;		/* Window vertical distance */
brldim brl;			/* For the Braille routines */
scrstat scr;			/* For screen statistics */

/* Output translation tables - the files *.auto.h are generated at
 * compile-time:
 */
unsigned char texttrans[256] =
{
#include "text.auto.h"
};
unsigned char attribtrans[256] =
{
#include "attrib.auto.h"
};
unsigned char *curtbl = NULL;	/* currently active translation table */

volatile sig_atomic_t keep_going = 1;	/* controls program termination */
int tbl_fd;			/* Translation table filedescriptor */
short opt_h = 0, opt_q = 0, opt_v = 0;	/* -h, -q and -v options */
char *opt_c = NULL, *opt_d = NULL, *opt_t = NULL;	/* filename options */
short homedir_found = 0;	/* CWD status */

unsigned int TickCount = 0;	/* incremented each cycle */

/* status cells support */
unsigned char statcells[5];	/* status cell buffer */
unsigned char num[10] =
{14, 1, 5, 3, 11, 9, 7, 15, 13, 6};	/* number translation */

/* for csrjmp subprocess */
volatile int csr_active = 0;
pid_t csr_pid;

/* Function prototypes: */
void csrjmp (int x, int y);	/* move cursor to (x,y) */
void csrjmp_sub (int x, int y);	/* cursor routing subprocess */
void setwinxy (int x, int y);	/* move window to include (x,y) */
void inskey (unsigned char *string);	/* insert char into input buffer */
void message (char *s, short silent);	/* write literal message on Braille display */
void clrbrlstat (void);
void termination_handler (int signum);	/* clean up before termination */
void stop_child (int signum);	/* called at end of cursor routing */
void usetable (int tbl);	/* set character translation table */
void configmenu (void);		/* configuration menu */
void loadconfig (void);
void saveconfig (void);
int nice (int);			/* should really be in a header file ... */


int
main (int argc, char *argv[])
{
  int keypress;			/* character received from braille display */
  int i;			/* loop counter */
  int old_winx = 0;		/* previous window position (used in help) */
  int old_winy = 0;
  char infbuf[20];		/* buffer for status information display */
  short csron = 1;		/* display cursor on (toggled during blink) */
  short csrcntr = 1;
  short capon = 0;		/* display caps off (toggled during blink) */
  short capcntr = 1;

  env = initenv;

  /* Parse command line using getopt(): */
  while ((i = getopt (argc, argv, "c:d:t:hqv-:")) != -1)
    switch (i)
      {
      case 'c':		/* configuration filename */
	opt_c = optarg;
	break;
      case 'd':		/* serial device name */
	opt_d = optarg;
	break;
      case 't':		/* text translation table filename */
	opt_t = optarg;
	break;
      case 'h':		/* help */
	opt_h = 1;
	break;
      case 'q':		/* quiet */
	opt_q = 1;
	break;
      case 'v':		/* version */
	opt_v = 1;
	break;
      case '-':		/* long options */
	if (strcmp (optarg, "help") == 0)
	  opt_h = 1;
	else if (strcmp (optarg, "quiet") == 0)
	  opt_q = 1;
	else if (strcmp (optarg, "version") == 0)
	  opt_v = 1;
	break;
      }

  if (opt_h)
    {
      printf (USAGE, argv[0]);
      return 0;
    }

  /* Print version and copyright information: */
  if (!(opt_q && !opt_v))	/* keep quiet only if -q and not -v */
    {
      puts (VERSION);
      puts (COPYRIGHT);

      /* Give the Braille library a chance to print start-up messages.
       * Pass the -d argument if present to override the default serial device.
       */
      identbrl (opt_d);
      identspk ();
    }
  if (opt_v)
    return 0;

  if (chdir (HOME_DIR))		/* change to directory containing data files */
    chdir ("/etc");		/* home directory not found, use backup */

  /* Load text translation table: */
  if (opt_t)
    {
      if ((tbl_fd = open (opt_t, O_RDONLY)) >= 0 && \
	  (curtbl = (unsigned char *) malloc (256)) && \
	  read (tbl_fd, curtbl, 256) == 256)
	memcpy (texttrans, curtbl, 256);
      else if (!opt_q)
	fprintf (stderr, "%s: Failed to read %s\n", argv[0], opt_t);
      if (curtbl)
	free (curtbl);
      if (tbl_fd >= 0)
	close (tbl_fd);
    }

  /* Initialize screen library */
  if (initscr ())		/* initialise screen reading */
    {
      if (!opt_q)
	fprintf (stderr, "%s: Cannot read screen\n", argv[0]);
      exit (2);
    }

  /* Become a daemon: */
  switch (fork ())
    {
    case -1:			/* can't fork */
      if (!opt_q)
	perror ("fork()");
      closescr ();
      exit (3);
    case 0:			/* child, process becomes a daemon: */
      close (STDIN_FILENO);
      close (STDOUT_FILENO);
      close (STDERR_FILENO);
      if (setsid () == -1)	/* request a new session (job control) */
	{
	  closescr ();
	  exit (4);
	}
      break;
    default:			/* parent returns to calling process: */
      return 0;
    }

  /* Initialise Braille and set text display: */
  brl = initbrl (opt_d);
  if (brl.x == -1)
    {
      closescr ();
      exit (5);
    }
  clrbrlstat ();

  /* Initialise speech */
  initspk ();

  /* Load configuration file: */
  loadconfig ();

  fwinshift = brl.x;
  hwinshift = fwinshift / 2;
  csr_offright = brl.x / 4;
  vwinshift = 5;

  /* Establish signal handler to clean up before termination: */
  if (signal (SIGTERM, termination_handler) == SIG_IGN)
    signal (SIGTERM, SIG_IGN);
  signal (SIGINT, SIG_IGN);
  signal (SIGHUP, SIG_IGN);
  signal (SIGCHLD, stop_child);


  usetable (TBL_TEXT);
  if (!opt_q)
    {
      message (VERSION, 0);	/* display initialisation message */
      delay (DISPDEL);		/* sleep for a while */
    }
  scr = getstat ();
  setwinxy (scr.posx, scr.posy);	/* set initial window position */
  cox = scr.posx;
  coy = scr.posy;
  soundstat (env.sound);	/* initialise beeps module */

  /* Main program loop */
  while (keep_going)
    {
      TickCount++;
      /* Process any Braille input */
      while ((keypress = readbrl (TBL_CMD)) != EOF)
	switch (keypress)
	  {
	  case CMD_TOP:
	    winy = 0;
	    break;
	  case CMD_TOP_LEFT:
	    winy = 0;
	    winx = 0;
	    break;
	  case CMD_BOT:
	    winy = scr.rows - brl.y;
	    break;
	  case CMD_BOT_LEFT:
	    winy = scr.rows - brl.y;
	    winx = 0;
	    break;
	  case CMD_WINUP:
	    if (winy == 0)
	      play (snd_bounce);
	    winy = MAX (winy - vwinshift, 0);
	    break;
	  case CMD_WINDN:
	    if (winy == scr.rows - brl.y)
	      play (snd_bounce);
	    winy = MIN (winy + vwinshift, scr.rows - brl.y);
	    break;
	  case CMD_LNUP:
	    if (winy == 0)
	      {
		play (snd_bounce);
		break;
	      }
	    if (!env.skpidlns)
	      {
		winy = MAX (winy - 1, 0);
		break;
	      }
	    /* no break here */
	  case CMD_PRDIFLN:
	    if (winy == 0)
	      play (snd_bounce);
	    else
	      {
		char buffer1[scr.cols], buffer2[scr.cols];
		scr = getstat ();
		getscr ((winpos)
			{
			0, winy, scr.cols, 1
			}
			,buffer1, dispmode ? SCR_ATTRIB : SCR_TEXT);
		while (winy > 0)
		  {
		    getscr ((winpos)
			    {
			    0, --winy, scr.cols, 1
			    }
			    ,buffer2, dispmode ? SCR_ATTRIB : SCR_TEXT);
		    if (memcmp (buffer1, buffer2, scr.cols) || \
			winy == scr.posy)
		      break;	/* lines are different */
		  }
	      }
	    break;
	  case CMD_LNDN:
	    if (winy == scr.rows - brl.y)
	      {
		play (snd_bounce);
		break;
	      }
	    if (!env.skpidlns)
	      {
		winy = MIN (winy + 1, scr.rows - brl.y);
		break;
	      }
	    /* no break here */
	  case CMD_NXDIFLN:
	    if (winy == scr.rows - brl.y)
	      play (snd_bounce);
	    else
	      {
		char buffer1[scr.cols], buffer2[scr.cols];
		scr = getstat ();
		getscr ((winpos)
			{
			0, winy, scr.cols, 1
			}
			,buffer1, dispmode ? SCR_ATTRIB : SCR_TEXT);
		while (winy < scr.rows - brl.y)
		  {
		    getscr ((winpos)
			    {
			    0, ++winy, scr.cols, 1
			    }
			    ,buffer2, dispmode ? SCR_ATTRIB : SCR_TEXT);
		    if (memcmp (buffer1, buffer2, scr.cols) || \
			winy == scr.posy)
		      break;	/* lines are different */
		  }
	      }
	    break;
	  case CMD_HOME:
	    scr = getstat ();
	    setwinxy (scr.posx, scr.posy);
	    break;
	  case CMD_LNBEG:
	    winx = 0;
	    break;
	  case CMD_LNEND:
	    winx = scr.cols - brl.x;
	    break;
	  case CMD_CHRLT:
	    if (winx == 0)
	      play (snd_bounce);
	    winx = MAX (winx - 1, 0);
	    break;
	  case CMD_CHRRT:
	    if (winx == scr.cols - brl.x)
	      play (snd_bounce);
	    winx = MIN (winx + 1, scr.cols - brl.x);
	    break;
	  case CMD_HWINLT:
	    if (winx == 0)
	      play (snd_bounce);
	    winx = MAX (winx - hwinshift, 0);
	    break;
	  case CMD_HWINRT:
	    if (winx == scr.cols - brl.x)
	      play (snd_bounce);
	    winx = MIN (winx + hwinshift, scr.cols - brl.x);
	    break;
	  case CMD_FWINLT:
	    if (winx == 0 && winy > 0)
	      {
		winx = scr.cols - brl.x;
		winy--;
		play (snd_wrap_up);
	      }
	    else if (winx == 0 && winy == 0)
	      play (snd_bounce);
	    else
	      winx = MAX (winx - fwinshift, 0);
	    break;
	  case CMD_FWINRT:
	    if (winx == scr.cols - brl.x && winy < scr.rows - brl.y)
	      {
		winx = 0;
		winy++;
		play (snd_wrap_down);
	      }
	    else if (winx == scr.cols - brl.x && winy == scr.rows - brl.y)
	      play (snd_bounce);
	    else
	      winx = MIN (winx + fwinshift, scr.cols - brl.x);
	    break;
	  case CMD_CSRJMP:
	    if ((dispmd & HELP_SCRN) != HELP_SCRN)
	      csrjmp (winx, winy);
	    break;
	  case CMD_KEY_UP:
	    if ((dispmd & HELP_SCRN) != HELP_SCRN)
	      inskey (UP_CSR);
	    break;
	  case CMD_KEY_DOWN:
	    if ((dispmd & HELP_SCRN) != HELP_SCRN)
	      inskey (DN_CSR);
	    break;
	  case CMD_KEY_RIGHT:
	    if ((dispmd & HELP_SCRN) != HELP_SCRN)
	      inskey (RT_CSR);
	    break;
	  case CMD_KEY_LEFT:
	    if ((dispmd & HELP_SCRN) != HELP_SCRN)
	      inskey (LT_CSR);
	    break;
	  case CMD_KEY_RETURN:
	    if ((dispmd & HELP_SCRN) != HELP_SCRN)
	      inskey (KEY_RETURN);
	    break;
	  case CMD_CSRVIS:
	    TOGGLEPLAY (env.csrvis ^= 1);
	    break;
	  case CMD_CSRBLINK:
	    csron = 1;
	    TOGGLEPLAY (env.csrblink ^= 1);
	    if (env.csrblink)
	      {
		csrcntr = env.csroncnt;
		capon = 0;
		capcntr = env.capoffcnt;
	      }
	    break;
	  case CMD_CSRTRK:
	    csrtrk ^= 1;
	    if (csrtrk)
	      {
		scr = getstat ();
		setwinxy (scr.posx, scr.posy);
		play (snd_link);
	      }
	    else
	      play (snd_unlink);
	    break;
	  case CMD_CUT_BEG:
	    cut_begin (winx, winy);
	    break;
	  case CMD_CUT_END:
	    cut_end (winx + brl.x - 1, winy + brl.y - 1);
	    break;
	  case CMD_PASTE:
	    if ((dispmd & HELP_SCRN) != HELP_SCRN)
	      cut_paste ();
	    break;
	  case CMD_SND:
	    soundstat (env.sound ^= 1);		/* toggle sound on/off */
	    play (snd_toggleon);	/* toggleoff wouldn't be played anyway :-) */
	    break;
	  case CMD_DISPMD:
	    usetable ((dispmode ^= 1) ? TBL_ATTRIB : TBL_TEXT);
	    break;
	  case CMD_FREEZE:
	    switch (dispmd & FROZ_SCRN)
	      {
	      case LIVE_SCRN:
		dispmd = selectdisp (dispmd | FROZ_SCRN);
		play (snd_freeze);
		break;
	      case FROZ_SCRN:
		dispmd = selectdisp (dispmd & ~FROZ_SCRN);
		play (snd_unfreeze);
		break;
	      }
	    break;
	  case CMD_HELP:
	    dispmode = 0;	/* always revert to text display */
	    infmode = 0;	/* ... and not in info mode */
	    usetable (TBL_TEXT);
	    switch (dispmd & HELP_SCRN)
	      {
	      case LIVE_SCRN:
		dispmd = selectdisp (dispmd | HELP_SCRN);
		if (dispmd & HELP_SCRN)
		  /* help screen selection successful */
		  {
		    old_winx = winx;	/* save window position */
		    old_winy = winy;
		    winx = winy = 0;	/* reset window position */
		  }
		else
		  /* help screen selection failed */
		  {
		    message ("can't find help", 0);
		    delay (DISPDEL);
		  }
		break;
	      case HELP_SCRN:
		dispmd = selectdisp (dispmd & ~HELP_SCRN);
		winx = old_winx;	/* restore previous window position */
		winy = old_winy;
		break;
	      }
	    break;
	  case CMD_CAPBLINK:
	    capon = 1;
	    TOGGLEPLAY (env.capblink ^= 1);
	    if (env.capblink)
	      {
		capcntr = env.caponcnt;
		csron = 0;
		csrcntr = env.csroffcnt;
	      }
	    break;
	  case CMD_INFO:
	    infmode ^= 1;
	    break;
	  case CMD_CSRSIZE:
	    TOGGLEPLAY (env.csrsize ^= 1);
	    break;
	  case CMD_SIXDOTS:
	    TOGGLEPLAY (env.sixdots ^= 1);
	    break;
	  case CMD_SLIDEWIN:
	    TOGGLEPLAY (env.slidewin ^= 1);
	    break;
	  case CMD_SKPIDLNS:
	    TOGGLEPLAY (env.skpidlns ^= 1);
	    break;
	  case CMD_SAVECONF:
	    saveconfig ();
	    play (snd_done);
	    break;
	  case CMD_CONFMENU:
	    configmenu ();
	    soundstat (env.sound);
	    break;
	  case CMD_RESET:
	    loadconfig ();
	    csron = 1;
	    capon = 0;
	    csrcntr = capcntr = 1;
	    play (snd_done);
	    break;
	  case CMD_SAY:
	    {
	      unsigned char buffer[scr.cols];
	      getscr ((winpos)
		      {
		      0, winy, scr.cols, 1
		      }
		      ,buffer, \
		      SCR_TEXT);
	      say (buffer, scr.cols);
	    }
	    break;
	  case CMD_MUTE:
	    mutespk ();
	    break;
	  default:
	    if (keypress >= CR_ROUTEOFFSET && keypress < \
		CR_ROUTEOFFSET + brl.x && (dispmd & HELP_SCRN) != HELP_SCRN)
	      {
		/* Cursor routing keys: */
		csrjmp (winx + keypress - CR_ROUTEOFFSET, winy);
		break;
	      }
	    else if (keypress >= CR_BEGBLKOFFSET && \
		     keypress < CR_BEGBLKOFFSET + brl.x)
	      cut_begin (winx + keypress - CR_BEGBLKOFFSET, winy);
	    else if (keypress >= CR_ENDBLKOFFSET && \
		     keypress < CR_ENDBLKOFFSET + brl.x)
	      cut_end (winx + keypress - CR_ENDBLKOFFSET, winy);
	    break;
	  }

      /* Update blink counters: */
      if (env.csrblink)
	if (!--csrcntr)
	  csrcntr = (csron ^= 1) ? env.csroncnt : env.csroffcnt;
      if (env.capblink)
	if (!--capcntr)
	  capcntr = (capon ^= 1) ? env.caponcnt : env.capoffcnt;

      /* Update Braille display. */

      scr = getstat ();

      /* cursor tracking */
      if (csrtrk)
	{
	  /* If cursor moves while blinking is on */
	  if (env.csrblink)
	    {
	      if (scr.posy != coy)
		{
		  /* turn off cursor to see what's under it while changing lines */
		  csron = 0;
		  csrcntr = env.csroffcnt;
		}
	      if (scr.posx != cox)
		{
		  /* turn on cursor to see it moving on the line */
		  csron = 1;
		  csrcntr = env.csroncnt;
		}
	    }

	  /* If the cursor moves in cursor tracking mode: */
	  if (!csr_active && (scr.posx != cox || scr.posy != coy))
	    {
	      setwinxy (scr.posx, scr.posy);
	      cox = scr.posx;
	      coy = scr.posy;
	    }
	}

      /* If not in info mode, get screen image: */
      if (!infmode)
	{
	  /* Update the Braille display.
	   * For the sake of displays on which the status cells and the
	   * main display have to be updated together, it is guaranteed
	   * that setbrlstat() will be called just *before* writebrl().
	   */
	  switch (env.stcellstyle)
	    {
	    case ST_AlvaStyle:
	      if ((dispmd & HELP_SCRN) == HELP_SCRN)
		{
		  statcells[0] = texttrans['h'];
		  statcells[1] = texttrans['l'];
		  statcells[2] = texttrans['p'];
		  statcells[3] = 0;
		  statcells[4] = 0;
		}
	      else
		{
		  /* The coords are given with letters as the DOS tsr */
		  statcells[0] = ((TickCount / 16) % (scr.posy / 25 + 1)) ? \
		    0 : texttrans[scr.posy % 25 + 'a'] | (scr.posx / brl.x) << 6;
		  statcells[1] = ((TickCount / 16) % (winy / 25 + 1)) ? \
		    0 : texttrans[winy % 25 + 'a'] | (winx / brl.x) << 6;
		  statcells[2] = texttrans[(dispmode) ? 'a' : \
			       ((dispmd & FROZ_SCRN) == FROZ_SCRN) ? 'f' : \
					   (csrtrk) ? 't' : ' '];
		  statcells[3] = 0;
		  statcells[4] = 0;
		}
	      break;
	    case ST_TiemanStyle:
	      statcells[0] = num[(winx / 10) % 10] << 4 | \
		num[(scr.posx / 10) % 10];
	      statcells[1] = num[winx % 10] << 4 | num[scr.posx % 10];
	      statcells[2] = num[(winy / 10) % 10] << 4 | \
		num[(scr.posy / 10) % 10];
	      statcells[3] = num[winy % 10] << 4 | num[scr.posy % 10];
	      statcells[4] = env.csrvis << 1 | env.csrsize << 3 | \
		env.csrblink << 5 | env.slidewin << 7 | csrtrk << 6 | \
		env.sound << 4 | dispmode << 2;
	      statcells[4] |= (dispmd & FROZ_SCRN) == FROZ_SCRN ? 1 : 0;
	      break;
	    default:
	      memset (statcells, 0, 5);
	      break;
	    }
	  setbrlstat (statcells);

	  getscr ((winpos)
		  {
		  winx, winy, brl.x, brl.y
		  }
		  ,brl.disp, \
		  dispmode ? SCR_ATTRIB : SCR_TEXT);
	  if (env.capblink && !capon)
	    for (i = 0; i < brl.x * brl.y; i++)
	      if (BRL_ISUPPER (brl.disp[i]))
		brl.disp[i] = ' ';

	  /* Do Braille translation using current table: */
	  if (env.sixdots && curtbl != attribtrans)
	    for (i = 0; i < brl.x * brl.y; brl.disp[i] = \
		 curtbl[brl.disp[i]] & 0x3f, i++);
	  else
	    for (i = 0; i < brl.x * brl.y; brl.disp[i] = \
		 curtbl[brl.disp[i]], i++);

	  /* If the cursor is visible and in range, and help is off: */
	  if (env.csrvis && (!env.csrblink || csron) && scr.posx >= winx && \
	      scr.posx < winx + brl.x && scr.posy >= winy && \
	      scr.posy < winy + brl.y && !(dispmd & HELP_SCRN))
	    brl.disp[(scr.posy - winy) * brl.x + scr.posx - winx] |= \
	      env.csrsize ? BIG_CSRCHAR : SMALL_CSRCHAR;
	  writebrl (brl);
	}

      /* If in info mode, send status information: */
      else
	{
	  sprintf (infbuf, "%02d:%02d %02d:%02d %c%c%c%c%c%c", \
		   winx, winy, scr.posx, scr.posy, csrtrk ? 't' : ' ', \
		   env.csrvis ? (env.csrblink ? 'B' : 'v') : \
		   (env.csrblink ? 'b' : ' '), dispmode ? 'a' : 't', \
		   (dispmd & FROZ_SCRN) == FROZ_SCRN ? 'f' : ' ', \
		   env.sixdots ? '6' : '8', env.capblink ? 'B' : ' ');

	  /* status cells */
	  statcells[0] = texttrans['i'];
	  statcells[1] = texttrans['n'];
	  statcells[2] = texttrans['f'];
	  statcells[3] = texttrans['o'];
	  statcells[4] = texttrans[' '];
	  setbrlstat (statcells);

	  message (infbuf, 1);
	}

      delay (DELAY_TIME);
    }

  clrbrlstat ();
  message ("BRLTTY terminating", 0);
  closescr ();

  /* Hard-wired delay to try and stop us being killed prematurely ... */
  delay (3000);
  closespk ();
  closebrl (brl);
  return 0;
}


void
setwinxy (int x, int y)
{
  if (env.slidewin)
    {
      /* Change position only if the coordonates are not already displayed */
      if (x < winx || x >= winx + brl.x || y < winy || y >= winy + brl.y)
	{
	  winy = y < brl.y - 1 ? 0 : y - (brl.y - 1);
	  if (x < brl.x)
	    winx = 0;
	  else if (x >= scr.cols - csr_offright)
	    winx = scr.cols - brl.x;
	  else
	    winx = x - (brl.x - csr_offright);
	}
    }
  else
    {
      if (x < winx || x >= winx + brl.x)
	winx = x >= (scr.cols / brl.x) * brl.x ? scr.cols - brl.x : \
	  (x / brl.x) * brl.x;
      if (y < winy || y >= winy + brl.y)
	winy = y < brl.y - 1 ? 0 : y - (brl.y - 1);
    }
}


void
csrjmp (int x, int y)
{
  /* Fork cursor routing subprocess.
   * First, we must check if a subprocess is already running: if so, we
   * send it a SIGUSR1 and wait for it to die.
   */
  signal (SIGCHLD, SIG_IGN);	/* ignore SIGCHLD for the moment */
  if (csr_active)
    {
      kill (csr_pid, SIGUSR1);
      wait (NULL);
      csr_active--;
    }
  signal (SIGCHLD, stop_child);	/* re-establish handler */

  switch (csr_pid = fork ())
    {
    case -1:			/* fork failed */
      break;
    case 0:			/* child, cursor routing process */
      nice (CSRJMP_NICENESS);	/* reduce scheduling priority */
      csrjmp_sub (x, y);
      exit (0);			/* terminate child process */
    default:			/* parent waits for child to return */
      csr_active++;
      break;
    }
}


void
csrjmp_sub (int x, int y)
{
  int curx, cury;		/* current cursor position */
  int difx, dify;		/* initial displacement to target */
  sigset_t mask;		/* for blocking of SIGUSR1 */

  /* Set up signal mask: */
  sigemptyset (&mask);
  sigaddset (&mask, SIGUSR1);

  /* Initialise second thread of screen reading: */
  if (initscr_phys ())
    return;

  timeout_yet (0);		/* initialise stop-watch */
  scr = getstat_phys ();

  /* Deal with vertical movement first, ignoring horizontal jumping ... */
  dify = y - scr.posy;
  while (dify * (y - scr.posy) > 0 && !timeout_yet (CSRJMP_TIMEOUT))
    {
      sigprocmask (SIG_BLOCK, &mask, NULL);	/* block SIGUSR1 */
      inskey (dify > 0 ? DN_CSR : UP_CSR);
      timeout_yet (0);		/* initialise stop-watch */
      do
	{
#if CSRJMP_LOOP_DELAY > 0
	  delay (CSRJMP_LOOP_DELAY);	/* sleep a while ... */
#endif
	  cury = scr.posy;
	  scr = getstat_phys ();
	}
      while ((scr.posy - cury) * dify <= 0 && !timeout_yet (CSRJMP_TIMEOUT));
      sigprocmask (SIG_UNBLOCK, &mask, NULL);	/* killed here if SIGUSR1 */
    }

  /* Now horizontal movement, quitting if the vertical position is wrong: */
  difx = x - scr.posx;
  while (difx * (x - scr.posx) > 0 && scr.posy == y && \
	 !timeout_yet (CSRJMP_TIMEOUT))
    {
      sigprocmask (SIG_BLOCK, &mask, NULL);	/* block SIGUSR1 */
      inskey (difx > 0 ? RT_CSR : LT_CSR);
      timeout_yet (0);		/* initialise stop-watch */
      do
	{
#if CSRJMP_LOOP_DELAY > 0
	  delay (CSRJMP_LOOP_DELAY);	/* sleep a while ... */
#endif
	  curx = scr.posx;
	  scr = getstat_phys ();
	}
      while ((scr.posx - curx) * difx <= 0 && scr.posy == y && \
	     !timeout_yet (CSRJMP_TIMEOUT));
      sigprocmask (SIG_UNBLOCK, &mask, NULL);	/* killed here if SIGUSR1 */
    }

  closescr_phys ();		/* close second thread of screen reading */
}


void
inskey (unsigned char *string)
{
  int ins_fd;			/* file descriptor for current terminal */

  ins_fd = open (CONSOLE, O_RDONLY);
  if (ins_fd == -1)
    return;
  while (*string)
    if (ioctl (ins_fd, TIOCSTI, string++))
      break;
  close (ins_fd);
}


void
message (char *s, short silent)
{
  int i, j, l;

  if (!silent && env.sound)
    {
      mutespk ();
      say (s, strlen (s));
    }

  usetable (TBL_TEXT);
  memset (brl.disp, ' ', brl.x * brl.y);
  l = strlen (s);
  while (l)
    {
      j = l <= brl.x * brl.y ? l : brl.x * brl.y - 1;
      for (i = 0; i < j; brl.disp[i++] = *s++);
      if (l -= j)
	brl.disp[brl.x * brl.y - 1] = '-';

      /* Do Braille translation using current table.
       * Six-dot mode is ignored, since case can be important, and
       * blinking caps won't work ...
       */
      for (i = 0; i < brl.x * brl.y; brl.disp[i] = curtbl[brl.disp[i]], i++);

      writebrl (brl);
      if (l)
	while (readbrl (TBL_ARG) == EOF)
	  delay (KEYDEL);
    }
  usetable (dispmode ? TBL_ATTRIB : TBL_TEXT);
}


void
clrbrlstat (void)
{
  memset (statcells, 0, 5);
  setbrlstat (statcells);
}


void
termination_handler (int signum)
{
  keep_going = 0;
  signal (signum, termination_handler);
}

void
stop_child (int signum)
{
  signal (signum, stop_child);
  wait (NULL);
  csr_active--;
}


void
usetable (int tbl)
{
  curtbl = (tbl == TBL_ATTRIB) ? attribtrans : texttrans;
}


void
configmenu (void)
{
  static short savecfg = 0;
  struct item
    {
      short *ptr;
      char desc[16];
      short bool;
      short min;
      short max;
    }
  menu[14] =
  {
    {
      &savecfg, "save config   ", 1, 0, 1
    }
    ,
    {
      &env.csrvis, "csr is visible", 1, 0, 1
    }
    ,
    {
      &env.csrblink, "csr blink     ", 1, 0, 1
    }
    ,
    {
      &env.capblink, "cap blink     ", 1, 0, 1
    }
    ,
    {
      &env.csrsize, "block cursor  ", 1, 0, 1
    }
    ,
    {
      &env.csroncnt, "csr blink on  ", 0, 1, 9
    }
    ,
    {
      &env.csroffcnt, "csr blink off ", 0, 1, 9
    }
    ,
    {
      &env.caponcnt, "cap blink on  ", 0, 1, 9
    }
    ,
    {
      &env.capoffcnt, "cap blink off ", 0, 1, 9
    }
    ,
    {
      &env.sixdots, "six dot text  ", 1, 0, 1
    }
    ,
    {
      &env.slidewin, "sliding window", 1, 0, 1
    }
    ,
    {
      &env.skpidlns, "skip ident lns", 1, 0, 1
    }
    ,
    {
      &env.sound, "audio signals ", 1, 0, 1
    }
    ,
    {
      &env.stcellstyle, "st cells style", 0, 0, NB_STCELLSTYLES
    }
  };
  struct brltty_env oldenv = env;
  int k;
  static short n = 0;
  short maxn = 13;
  char buffer[20];

  /* status cells */
  statcells[0] = texttrans['c'];
  statcells[1] = texttrans['n'];
  statcells[2] = texttrans['f'];
  statcells[3] = texttrans['i'];
  statcells[4] = texttrans['g'];
  setbrlstat (statcells);
  message ("Configuration menu", 0);
  delay (DISPDEL);

  while (keep_going)
    {
      while ((k = readbrl (TBL_CMD)) != EOF)
	switch (k)
	  {
	  case CMD_TOP:
	  case CMD_HOME:
	    n = 0;
	    break;
	  case CMD_BOT:
	    n = maxn;
	    break;
	  case CMD_LNUP:
	    if (--n < 0)
	      n = maxn;
	    break;
	  case CMD_LNDN:
	    if (++n > maxn)
	      n = 0;
	    break;
	  case CMD_FWINLT:
	    if (--*menu[n].ptr < menu[n].min)
	      *menu[n].ptr = menu[n].max;
	    break;
	  case CMD_FWINRT:
	    if (++*menu[n].ptr > menu[n].max)
	      *menu[n].ptr = menu[n].min;
	    break;
	  case CMD_RESET:
	    env = oldenv;
	    break;
	  default:
	    if (savecfg)
	      {
		saveconfig ();
		play (snd_done);
	      }
	    return;
	  }

      memcpy (buffer, menu[n].desc, 14);
      if (menu[n].bool)
	sprintf (buffer + 14, ": %s", *menu[n].ptr ? "on" : "off");
      else
	sprintf (buffer + 14, ": %d", *menu[n].ptr);
      message (buffer, 1);
      delay (DELAY_TIME);
    }
}


void
loadconfig (void)
{
  int i = 1;
  struct brltty_env newenv;

  tbl_fd = open (opt_c ? opt_c : CONFFILE_NAME, O_RDONLY);
  if (tbl_fd >= 0)
    {
      if (read (tbl_fd, &newenv, sizeof (struct brltty_env)) == \
	  sizeof (struct brltty_env))
	if (newenv.magicnum == ENV_MAGICNUM)
	  {
	    env = newenv;
	    soundstat (env.sound);
	    i = 0;
	  }
      close (tbl_fd);
    }
  if (i)
    env = initenv;
}


void
saveconfig (void)
{
  tbl_fd = open (opt_c ? opt_c : CONFFILE_NAME, O_WRONLY | O_CREAT | O_TRUNC);
  if (tbl_fd >= 0)
    {
      fchmod (tbl_fd, S_IRUSR | S_IWUSR);
      if (write (tbl_fd, &env, sizeof (struct brltty_env)) != \
	  sizeof (struct brltty_env))
	{
	  close (tbl_fd);
	  tbl_fd = -2;
	}
      else
	close (tbl_fd);
    }
}
