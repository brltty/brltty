/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Version 1.9.0, 06 April 1998
 *
 * Copyright (C) 1995-1998 by The BRLTTY Team, All rights reserved.
 *
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 * Nicolas Pitre <nico@cam.org>
 * Stephane Doyon <s.doyon@videotron.ca>
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
 */

/*
 * brltty.c - main() plus signal handling and cursor routing 
 */

#define BRLTTY_C 1

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "config.h"
#include "brl.h"
#include "scr.h"
#include "inskey.h"
#include "speech.h"
#include "beeps.h"
#include "cut-n-paste.h"
#include "misc.h"

#define VERSION "BRLTTY 1.9.1 (pre-release)"
#define COPYRIGHT "\
Copyright (C) 1995-1998 by The BRLTTY Team.  All rights reserved."
#define USAGE "\
Usage: %s [options]\n\
 -c config-file       use binary configuration file `config-file'\n\
 -d serial-device     use `serial-device' to access Braille terminal\n\
 -t text-trans-file   use translation table `text-trans-file'\n\
 -h, --help           print this usage message\n\
 -q, --quiet          suppress start-up messages\n\
 -v, --version        print start-up messages and exit\n"

#define ENV_MAGICNUM 0x4004

/*
 * Some useful macros: 
 */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define BRL_ISUPPER(c) (isupper (c) || (c) == '@' || (c) == '[' || (c) == '^' \
			|| (c) == ']' || (c) == '\\')
#define TOGGLEPLAY(var) play ((var) ? snd_toggleon : snd_toggleoff)
#define TOGGLE(var) \
  (var = (keypress & VAL_SWITCHON) ? 1 \
   : (keypress & VAL_SWITCHOFF) ? 0 \
   : var ^ 1)

/*
 * Argument for usetable() 
 */
#define TBL_TEXT 0		/* use text translation table */
#define TBL_ATTRIB 1		/* use attribute translation table */

/*
 * Global variables 
 */

/* struct definition of settings that are saveable */
struct brltty_env
  {				
    short magicnum;
    short csrvis;
    short attrvis;
    short csrblink;
    short capblink;
    short attrblink;
    short csrsize;
    short csroncnt;
    short csroffcnt;
    short caponcnt;
    short capoffcnt;
   short attroncnt;
    short attroffcnt;
    short sixdots;
    short slidewin;
    short sound;
    short skpidlns;
    short stcellstyle;
  }
env, initenv = {
    ENV_MAGICNUM, INIT_CSRVIS, INIT_ATTRVIS, INIT_CSRBLINK, INIT_CAPBLINK,
    INIT_ATTRBLINK, INIT_CSRSIZE, INIT_CSR_ON_CNT, INIT_CSR_OFF_CNT,
    INIT_CAP_ON_CNT, INIT_CAP_OFF_CNT, INIT_ATTR_ON_CNT, INIT_ATTR_OFF_CNT,
    INIT_SIXDOTS, INIT_SLIDEWIN, INIT_BEEPSON, INIT_SKPIDLNS,
#if defined (Alva_ABT3)
    ST_AlvaStyle
#elif defined (CombiBraille)
    ST_TiemanStyle
#elif defined (TSI)
    ST_PB80Style
#elif defined (Papenmeier)
    ST_Papenmeier
#else
    ST_None
#endif
};

/* struct definition for volatile parameters */
struct brltty_param
  {
    short csrtrk;		/* tracking mode */
    short dispmode;		/* text or attributes display */
    short winx, winy;		/* Start row and column of Braille window */
    short cox, coy;		/* Old cursor position */
  }
initparam = {
    INIT_CSRTRK, TBL_TEXT, 0, 0, 0, 0
};

/* Array definition containing pointers to brltty_param structures for 
 * each screen.  Those structures are dynamically allocated when a 
 * specific screen is used for the first time.
 * Screen 0 is reserved for help screen.
 */
struct brltty_param *scrparam[NBR_SCR+1];
struct brltty_param *p;		/* pointer to current param structure */
int curscr;			/* current screen number */

/* Misc param variables */
short dispmd = LIVE_SCRN;	/* freeze screen on/off */
short infmode = 0;		/* display screen image or info */
short csr_offright;		/* used for sliding window */
short hwinshift;		/* Half window horizontal distance */
short fwinshift;		/* Full window horizontal distance */
short vwinshift;		/* Window vertical distance */
brldim brl;			/* For the Braille routines */
scrstat scr;			/* For screen statistics */


/*
 * Output translation tables - the files *.auto.h are generated at *
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

volatile sig_atomic_t keep_going = 1;	/*
					 * controls program termination 
					 */
int tbl_fd;			/* Translation table filedescriptor */
short opt_h = 0, opt_q = 0, opt_v = 0;	/* -h, -q and -v options */
char *opt_c = NULL, *opt_d = NULL, *opt_t = NULL;	/* filename options */
short homedir_found = 0;	/* CWD status */

unsigned int TickCount = 0;	/* incremented each cycle */

/*
 * Status cells support 
 * remark: the Papenmeier has a column with 22 cells, 
 * all other terminals use up to 5 bytes
 */
unsigned char statcells[22];	/* status cell buffer */

/* 
 * Number dot translation for status cells 
 */
const unsigned char num[10] = {14, 1, 5, 3, 11, 9, 7, 15, 13, 6};

/*
 * for csrjmp subprocess 
 */
volatile int csr_active = 0;
pid_t csr_pid;

/*
 * Function prototypes: 
 */
void switchto (unsigned int scrno); /* activate params for specified screen */
void csrjmp (int x, int y);	/* move cursor to (x,y) */
void csrjmp_sub (int x, int y);	/* cursor routing subprocess */
void setwinxy (int x, int y);	/* move window to include (x,y) */
void message (char *s, short silent);	/* write literal message on
					 * Braille display 
					 */
void clrbrlstat (void);
void termination_handler (int signum);	/* clean up before termination */
void stop_child (int signum);	/* called at end of cursor routing */
void usetable (int tbl);	/* set character translation table */
void configmenu (void);		/* configuration menu */
void loadconfig (void);
void saveconfig (void);
int nice (int);			/* should really be in a header file ... */

#if defined (Papenmeier)
int pm_num(int x);
int pm_stat(int line, int on);
#else
int pm_num(int x)
{ return 0; }

int pm_stat(int line, int on)
{ return 0; }

#endif

int 
main (int argc, char *argv[])
{
  int keypress;			/* character received from braille display */
  int i;			/* loop counter */
  short csron = 1;		/* display cursor on (toggled during blink) */
  short csrcntr = 1;
  short capon = 0;		/* display caps off (toggled during blink) */
  short capcntr = 1;
  short attron = 0;
  short attrcntr = 1;
  short oldwinx, oldwiny;

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

  /*
   * Print version and copyright information: 
   */
  if (!(opt_q && !opt_v))
    {	
      /* keep quiet only if -q and not -v */
      puts (VERSION);
      puts (COPYRIGHT);

      /*
       * Give the Braille library a chance to print start-up messages. * 
       * Pass the -d argument if present to override the default serial
       * device. 
       */
      identbrl (opt_d);
      identspk ();
    }
  if (opt_v)
    return 0;

  if (chdir (HOME_DIR))		/* * change to directory containing data files  */
    chdir ("/etc");		/* home directory not found, use backup */

  /*
   * Load text translation table: 
   */
  if (opt_t)
    {
      if ((tbl_fd = open (opt_t, O_RDONLY)) >= 0 && \
	  (curtbl = (unsigned char *) malloc (256)) && \
	  read (tbl_fd, curtbl, 256) == 256)
	{
	  memcpy (texttrans, curtbl, 256);
	}
      else if (!opt_q)
	fprintf (stderr, "%s: Failed to read %s\n", argv[0], opt_t);
      if (curtbl)
	free (curtbl);
      if (tbl_fd >= 0)
	close (tbl_fd);
    }

  /*
   * Initialize screen library 
   */
  if (initscr ())
    {				
      /* initialise screen reading */
      if (!opt_q)
	fprintf (stderr, "%s: Cannot read screen\n", argv[0]);
      exit (2);
    }

  /* allocate the first screen information structures */
  p = malloc (sizeof (*p));
  if (!p)
    {
      fprintf (stderr, "brltty: memory allocation error\n" );
      exit( -1 );
    }
  *p = initparam;
  scrparam[0] = p;
  for (i = 1; i <= NBR_SCR; i++)
    scrparam[i] = 0;
  curscr = 0;

  /*
   * Become a daemon: 
   */
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
      if (setsid () == -1)
	{			
	  /* request a new session (job control) */
	  closescr ();
	  exit (4);
	}
      break;
    default:			/* parent returns to calling process: */
      return 0;
    }
  /*
   * Initialise Braille and set text display: 
   */
  brl = initbrl (opt_d);
  if (brl.x == -1)
    {
      closescr ();
      free (p);
      exit (5);
    }
  clrbrlstat ();

  /*
   * Initialise speech 
   */
  initspk ();

  /*
   * Load configuration file: 
   */
  loadconfig ();

  fwinshift = brl.x;
  hwinshift = fwinshift / 2;
  csr_offright = brl.x / 4;
  vwinshift = 5;

  /*
   * Establish signal handler to clean up before termination: 
   */
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
  switchto( scr.no );			/* allocate current screen params */
  setwinxy (scr.posx, scr.posy);	/* set initial window position */
  oldwinx = p->winx; oldwiny = p->winy;
  p->cox = scr.posx;
  p->coy = scr.posy;

  /*
   * Main program loop 
   */
  while (keep_going)
    {
      TickCount++;
      /*
       * Process any Braille input 
       */
      while ((keypress = readbrl (TBL_CMD)) != EOF)
	switch (keypress & ~VAL_SWITCHMASK)
	  {
	  case CMD_TOP:
	    p->winy = 0;
	    break;
	  case CMD_TOP_LEFT:
	    p->winy = 0;
	    p->winx = 0;
	    break;
	  case CMD_BOT:
	    p->winy = scr.rows - brl.y;
	    break;
	  case CMD_BOT_LEFT:
	    p->winy = scr.rows - brl.y;
	    p->winx = 0;
	    break;
	  case CMD_WINUP:
	    if (p->winy == 0)
	      play (snd_bounce);
	    p->winy = MAX (p->winy - vwinshift, 0);
	    break;
	  case CMD_WINDN:
	    if (p->winy == scr.rows - brl.y)
	      play (snd_bounce);
	    p->winy = MIN (p->winy + vwinshift, scr.rows - brl.y);
	    break;
	  case CMD_LNUP:
	    if (p->winy == 0)
	      {
		play (snd_bounce);
		break;
	      }
	    if (!env.skpidlns)
	      {
		p->winy = MAX (p->winy - 1, 0);
		break;
	      }
	    /* no break here */
	  case CMD_PRDIFLN:
	  case CMD_ATTRUP:
	    if (p->winy == 0)
	      play (snd_bounce);
	    else
	      {
		char buffer1[scr.cols], buffer2[scr.cols];
		int scrtype = (keypress == CMD_ATTRUP) ? SCR_ATTRIB : \
			      p->dispmode ? SCR_ATTRIB : SCR_TEXT;
		int skipped = 0;
		getscr ((winpos)
			{
			0, p->winy, scr.cols, 1
			}
			,buffer1, scrtype);
		while (p->winy > 0)
		  {
		    getscr ((winpos)
			    {
			    0, --p->winy, scr.cols, 1
			    }
			    ,buffer2, scrtype);
		    if (memcmp (buffer1, buffer2, scr.cols) || \
			p->winy == scr.posy)
		      break;	/* lines are different */
                    if(p->winy == 0){
                      play(snd_bounce);
                      break;
                    }
		    /* lines are identical */
		    /* don't sound if it's the first time or we would have
		       beeped too many times already... */
		    if (skipped <= 4)
		      play (snd_skip);
		    else if (skipped % 4 == 0)
		      play (snd_skipmore);
		    skipped++;	/* will sound only if lines are skipped,
				   not if we just move by one line */
		  }
	      }
	    break;
	  case CMD_LNDN:
	    if (p->winy == scr.rows - brl.y)
	      {
		play (snd_bounce);
		break;
	      }
	    if (!env.skpidlns)
	      {
		p->winy = MIN (p->winy + 1, scr.rows - brl.y);
		break;
	      }
	    /* no break here */
	  case CMD_NXDIFLN:
	  case CMD_ATTRDN:
	    if (p->winy == scr.rows - brl.y)
	      play (snd_bounce);
	    else
	      {
		char buffer1[scr.cols], buffer2[scr.cols];
		int scrtype = (keypress == CMD_ATTRDN) ? SCR_ATTRIB : \
			      p->dispmode ? SCR_ATTRIB : SCR_TEXT;
		int skipped = 0;
		getscr ((winpos)
			{
			0, p->winy, scr.cols, 1
			}
			,buffer1, scrtype);
		while (p->winy < scr.rows - brl.y)
		  {
		    getscr ((winpos)
			    {
			    0, ++p->winy, scr.cols, 1
			    }
			    ,buffer2, scrtype);
		    if (memcmp (buffer1, buffer2, scr.cols) || \
			p->winy == scr.posy)
		      break;	/* lines are different */
		    if(p->winy == scr.rows-brl.y){
		      play(snd_bounce);
		      break;
		    }
		    /* lines are identical */
		    /* don't sound if it's the first time or we would have
		       beeped too many times already... */
		    if (skipped <= 4)
		      play (snd_skip);
		    else if (skipped % 4 == 0)
		      play (snd_skipmore);
		    skipped++;	/* will sound only if lines are skipped,
				   not if we just move by one line */
		  }
	      }
	    break;
	  case CMD_HOME:
	    setwinxy (scr.posx, scr.posy);
	    break;
	  case CMD_LNBEG:
	    p->winx = 0;
	    break;
	  case CMD_LNEND:
	    p->winx = scr.cols - brl.x;
	    break;
	  case CMD_CHRLT:
	    if (p->winx == 0)
	      play (snd_bounce);
	    p->winx = MAX (p->winx - 1, 0);
	    break;
	  case CMD_CHRRT:
	    if (p->winx == scr.cols - brl.x)
	      play (snd_bounce);
	    p->winx = MIN (p->winx + 1, scr.cols - brl.x);
	    break;
	  case CMD_HWINLT:
	    if (p->winx == 0)
	      play (snd_bounce);
	    p->winx = MAX (p->winx - hwinshift, 0);
	    break;
	  case CMD_HWINRT:
	    if (p->winx == scr.cols - brl.x)
	      play (snd_bounce);
	    p->winx = MIN (p->winx + hwinshift, scr.cols - brl.x);
	    break;
	  case CMD_FWINLT:
	    if (p->winx == 0 && p->winy > 0)
	      {
		p->winx = scr.cols - brl.x;
		p->winy--;
		play (snd_wrap_up);
	      }
	    else if (p->winx == 0 && p->winy == 0)
	      play (snd_bounce);
	    else
	      p->winx = MAX (p->winx - fwinshift, 0);
	    break;
	  case CMD_FWINRT:
	    if (p->winx == scr.cols - brl.x && p->winy < scr.rows - brl.y)
	      {
		p->winx = 0;
		p->winy++;
		play (snd_wrap_down);
	      }
	    else if (p->winx == scr.cols - brl.x && p->winy == scr.rows - brl.y)
	      play (snd_bounce);
	    else
	      p->winx = MIN (p->winx + fwinshift, scr.cols - brl.x);
	    break;
	  case CMD_CSRJMP:
	    if ((dispmd & HELP_SCRN) != HELP_SCRN)
	      csrjmp (p->winx, p->winy);
	    break;
	  case CMD_CSRJMP_VERT:
	    if ((dispmd & HELP_SCRN) != HELP_SCRN)
	      csrjmp (-1, p->winy);
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
	    TOGGLEPLAY ( TOGGLE(env.csrvis) );
	    break;
	  case CMD_ATTRVIS:
	    TOGGLEPLAY ( TOGGLE(env.attrvis) );
	    break;
	  case CMD_CSRBLINK:
	    csron = 1;
	    TOGGLEPLAY ( TOGGLE(env.csrblink) );
	    if (env.csrblink)
	      {
		csrcntr = env.csroncnt;
                attron = 1;
                attrcntr = env.attroncnt;
		capon = 0;
		capcntr = env.capoffcnt;
	      }
	    break;
	  case CMD_CSRTRK:
	    TOGGLE(p->csrtrk);
	    if (p->csrtrk)
	      {
		setwinxy (scr.posx, scr.posy);
		play (snd_link);
	      }
	    else
	      play (snd_unlink);
	    break;
	  case CMD_CUT_BEG:
	    cut_begin (p->winx, p->winy);
	    break;
	  case CMD_CUT_END:
	    cut_end (p->winx + brl.x - 1, p->winy + brl.y - 1);
	    break;
	  case CMD_PASTE:
	    if ((dispmd & HELP_SCRN) != HELP_SCRN)
	      cut_paste ();
	    break;
	  case CMD_SND:
	    soundstat ( TOGGLE(env.sound) );	/* toggle sound on/off */
	    play (snd_toggleon);  /* toggleoff wouldn't be played anyway :-) */
	    break;
	  case CMD_DISPMD:
	    usetable ((TOGGLE(p->dispmode)) ? TBL_ATTRIB : TBL_TEXT);
	    break;
	  case CMD_FREEZE:
	    { int v;
	      v = (dispmd & FROZ_SCRN) ? 1 : 0;
	      TOGGLE(v);
	      if(v){
		dispmd = selectdisp (dispmd | FROZ_SCRN);
		play (snd_freeze);
	      }else{
		dispmd = selectdisp (dispmd & ~FROZ_SCRN);
		play (snd_unfreeze);
	      }
	    }
	    break;
	  case CMD_HELP:
	    { int v;
	      infmode = 0;	/* ... and not in info mode */
	      v = (dispmd & HELP_SCRN) ? 1 : 0;
	      TOGGLE(v);
	      if(v){
		dispmd = selectdisp (dispmd | HELP_SCRN);
		if (dispmd & HELP_SCRN) /* help screen selection successful */
		  {
		    switchto( 0 );	/* screen 0 for help screen */
		    *p = initparam;	/* reset params for help screen */
		  }
		else	/* help screen selection failed */
		  {
		    message ("can't find help", 0);
		    delay (DISPDEL);
		  }
	      }else
		dispmd = selectdisp (dispmd & ~HELP_SCRN);
	    }
	    break;
	  case CMD_CAPBLINK:
	    capon = 1;
	    TOGGLEPLAY( TOGGLE(env.capblink) );
	    if (env.capblink)
	      {
		capcntr = env.caponcnt;
		attron = 0;
		attrcntr = env.attroffcnt;
		csron = 0;
		csrcntr = env.csroffcnt;
	      }
	    break;
	  case CMD_ATTRBLINK:
	    attron = 1;
	    TOGGLEPLAY( TOGGLE(env.attrblink) );
	    if (env.attrblink)
	      {
		attrcntr = env.attroncnt;
		capon = 1;
		capcntr = env.caponcnt;
		csron = 0;
		csrcntr = env.csroffcnt;
	      }
	    break;
	  case CMD_INFO:
	    TOGGLE(infmode);
	    break;
	  case CMD_CSRSIZE:
	    TOGGLEPLAY ( TOGGLE(env.csrsize) );
	    break;
	  case CMD_SIXDOTS:
	    TOGGLEPLAY ( TOGGLE(env.sixdots) );
	    break;
	  case CMD_SLIDEWIN:
	    TOGGLEPLAY ( TOGGLE(env.slidewin) );
	    break;
	  case CMD_SKPIDLNS:
	    TOGGLEPLAY ( TOGGLE(env.skpidlns) );
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
		      0, p->winy, scr.cols, 1
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
		csrjmp (p->winx + keypress - CR_ROUTEOFFSET, p->winy);
		break;
	      }
	    else if (keypress >= CR_BEGBLKOFFSET && \
		     keypress < CR_BEGBLKOFFSET + brl.x)
	      cut_begin (p->winx + keypress - CR_BEGBLKOFFSET, p->winy);
	    else if (keypress >= CR_ENDBLKOFFSET && \
		     keypress < CR_ENDBLKOFFSET + brl.x)
	      cut_end (p->winx + keypress - CR_ENDBLKOFFSET, p->winy);
	    break;
	  }

      /*
       * Update blink counters: 
       */
      if (env.csrblink)
	if (!--csrcntr)
	  csrcntr = (csron ^= 1) ? env.csroncnt : env.csroffcnt;
      if (env.capblink)
	if (!--capcntr)
	  capcntr = (capon ^= 1) ? env.caponcnt : env.capoffcnt;
      if (env.attrblink)
	if (!--attrcntr)
	  attrcntr = (attron ^= 1) ? env.attroncnt : env.attroffcnt;

      /*
       * Update Braille display and screen information.  Switch screen 
       * params if screen numver has changed.
       */
      scr = getstat ();
      if( !(dispmd & (HELP_SCRN|FROZ_SCRN)) && curscr != scr.no)
	switchto (scr.no);

      /* cursor tracking */
      if (p->csrtrk)
	{
	  /* If cursor moves while blinking is on */
	  if (env.csrblink)
	    {
	      if (scr.posy != p->coy)
		{
		  /* turn off cursor to see what's under it while changing lines */
		  csron = 0;
		  csrcntr = env.csroffcnt;
		}
	      if (scr.posx != p->cox)
		{
		  /* turn on cursor to see it moving on the line */
		  csron = 1;
		  csrcntr = env.csroncnt;
		}
	    }
	  /* If the cursor moves in cursor tracking mode: */
	  if (!csr_active && (scr.posx != p->cox || scr.posy != p->coy))
	    {
	      setwinxy (scr.posx, scr.posy);
	      p->cox = scr.posx;
	      p->coy = scr.posy;
	    }
	}
      /* If attribute underlining is blinking during display movement */
      if(env.attrvis && env.attrblink){
	/* We could check that to see if we changed screen, but that doesn't
	   really matter... this is mainly for when you are hunting up/down
	   for the line with attributes. */
	if(p->winx != oldwinx || p->winy != oldwiny){
	  attron = 1;
	  attrcntr = env.attroncnt;
	}
      }
      oldwinx = p->winx; oldwiny = p->winy;
      /* If not in info mode, get screen image: */
      if (!infmode)
	{
	  /* Update the Braille display. * For the sake of displays on 
	   * which the status cells and the * main display have to be
	   * updated together, it is guaranteed * that setbrlstat() will 
	   * be called just *before* writebrl(). 
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
		  statcells[1] = ((TickCount / 16) % (p->winy / 25 + 1)) ? \
		    0 : texttrans[p->winy % 25 + 'a'] | (p->winx / brl.x) << 6;
		  statcells[2] = texttrans[(p->dispmode) ? 'a' : \
			       ((dispmd & FROZ_SCRN) == FROZ_SCRN) ? 'f' : \
					   (p->csrtrk) ? 't' : ' '];
		  statcells[3] = 0;
		  statcells[4] = 0;
		}
	      break;
	    case ST_TiemanStyle:
	      statcells[0] = num[(p->winx / 10) % 10] << 4 | \
		num[(scr.posx / 10) % 10];
	      statcells[1] = num[p->winx % 10] << 4 | num[scr.posx % 10];
	      statcells[2] = num[(p->winy / 10) % 10] << 4 | \
		num[(scr.posy / 10) % 10];
	      statcells[3] = num[p->winy % 10] << 4 | num[scr.posy % 10];
	      statcells[4] = env.csrvis << 1 | env.csrsize << 3 | \
		env.csrblink << 5 | env.slidewin << 7 | p->csrtrk << 6 | \
		env.sound << 4 | p->dispmode << 2;
	      statcells[4] |= (dispmd & FROZ_SCRN) == FROZ_SCRN ? 1 : 0;
	      break;
	    case ST_PB80Style:
	      statcells[0] = num[(p->winy+1) % 10] << 4 | \
		num[((p->winy+1) / 10) % 10];
	      break;
	    case ST_Papenmeier:
	      statcells [0] = pm_num(p->winy+1);
	      statcells [1] = 0;  /* empty for easier reading */
	      statcells [2] =  pm_num(scr.posy+1);

	      statcells [3] =  pm_num(scr.posx+1);
	      statcells [4] = 0;     /* empty for easier reading */
	      statcells [5] = pm_stat(6, p->csrtrk);
	      statcells [6] = pm_stat(7, p->dispmode);
	      statcells [7] = 0;     /* empty for easier reading */
	      statcells [8] = pm_stat(9, (dispmd & FROZ_SCRN) == FROZ_SCRN);
	      statcells [9] = 0;
	      statcells [10] = 0;
	      statcells [11] = 0;
	      statcells [12] = pm_stat(3, env.csrvis);
	      statcells [13] = pm_stat(4, env.csrsize);
	      statcells [14] = pm_stat(5, env.csrblink);
	      statcells [15] = pm_stat(6, env.capblink);
	      statcells [16] = pm_stat(7, env.sixdots);
	      statcells [17] = pm_stat(8, env.sound);
	      statcells [18] = pm_stat(9, env.skpidlns);
	      statcells [19] = pm_stat(0, env.attrvis);
	      statcells [20] = pm_stat(1, env.attrblink);
              statcells [21] = 0;
	      break;
	    default:
	      memset (statcells, 0, sizeof(statcells));
	      break;
	    }
	  setbrlstat (statcells);

	  getscr ((winpos)
		  {
		  p->winx, p->winy, brl.x, brl.y
		  }
		  ,brl.disp, \
		  p->dispmode ? SCR_ATTRIB : SCR_TEXT);
	  if (env.capblink && !capon)
	    for (i = 0; i < brl.x * brl.y; i++)
	      if (BRL_ISUPPER (brl.disp[i]))
		brl.disp[i] = ' ';

	  /*
	   * Do Braille translation using current table: 
	   */
	  if (env.sixdots && curtbl != attribtrans)
	    for (i = 0; i < brl.x * brl.y; brl.disp[i] = \
		 curtbl[brl.disp[i]] & 0x3f, i++);
	  else
	    for (i = 0; i < brl.x * brl.y; brl.disp[i] = \
		 curtbl[brl.disp[i]], i++);

	  /* Attribute underlining: if viewing text (not attributes), attribute
	     underlining is active and visible and we're not in help, then we
	     get the attributes for the current region and OR the underline. */
	  if (!p->dispmode && env.attrvis && (!env.attrblink || attron)
	      && !(dispmd & HELP_SCRN)){
	    unsigned char attrbuf[brl.x*brl.y];
	    getscr ((winpos)
		    {
		    p->winx, p->winy, brl.x, brl.y
		    }
		    ,attrbuf, \
		    SCR_ATTRIB);
	    for (i = 0; i < brl.x * brl.y; i++)
	      switch(attrbuf[i]){
  	        case 0x07: 
  	        case 0x17: 
  	        case 0x30: 
		  break;
	        case 0x70:
		  brl.disp[i] |= ATTR1CHAR;
		  break;
	        case 0x0F:
	        default:
		  brl.disp[i] |= ATTR2CHAR;
		  break;
	      };
	  }

	  /*
	   * If the cursor is visible and in range, and help is off: 
	   */
	  if (env.csrvis && (!env.csrblink || csron) && scr.posx >= p->winx &&\
	      scr.posx < p->winx + brl.x && scr.posy >= p->winy && \
	      scr.posy < p->winy + brl.y && !(dispmd & HELP_SCRN))
	    brl.disp[(scr.posy - p->winy) * brl.x + scr.posx - p->winx] |= \
	      env.csrsize ? BIG_CSRCHAR : SMALL_CSRCHAR;
	  writebrl (brl);
	}
      /*
       * If in info mode, send status information: 
       */
      else
	{
	  char infbuf[20];
	  sprintf (infbuf, "%02d:%02d %02d:%02d %02d %c%c%c%c%c%c", \
		   p->winx, p->winy, scr.posx, scr.posy, curscr, 
		   p->csrtrk ? 't' : ' ', \
		   env.csrvis ? (env.csrblink ? 'B' : 'v') : \
		   (env.csrblink ? 'b' : ' '), p->dispmode ? 'a' : 't', \
		   (dispmd & FROZ_SCRN) == FROZ_SCRN ? 'f' : ' ', \
		   env.sixdots ? '6' : '8', env.capblink ? 'B' : ' ');

	  /*
	   * status cells 
	   */
	  memset (statcells, 0, sizeof(statcells));
	  statcells[0] = texttrans['I'];
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

  /*
   * Hard-wired delay to try and stop us being killed prematurely ... 
   */
  delay (1000);
  closespk ();
  closebrl (brl);
  for (i = 0; i <= NBR_SCR; i++) 
    free (scrparam[i]);
  return 0;
}


void
switchto( unsigned int scrno )
{
  curscr = scrno;
  if (scrno > NBR_SCR)
      scrno = 0;
  if (!scrparam[scrno]) 	/* if not already allocated... */
    if (!(scrparam[scrno] = malloc (sizeof (*p))))
      scrno = 0; 	/* unable to allocate a new structure */
    else
      *scrparam[scrno] = initparam;
  p = scrparam[scrno];
  usetable (p->dispmode ? TBL_ATTRIB : TBL_TEXT);
}


void 
setwinxy (int x, int y)
{
  if (env.slidewin)
    {
      /* Change position only if the coordonates are not already displayed */
      if (x < p->winx || x >= p->winx + brl.x || 
	  y < p->winy || y >= p->winy + brl.y)
	{
	  p->winy = y < brl.y - 1 ? 0 : y - (brl.y - 1);
	  if (x < brl.x)
	    p->winx = 0;
	  else if (x >= scr.cols - csr_offright)
	    p->winx = scr.cols - brl.x;
	  else
	    p->winx = x - (brl.x - csr_offright);
	}
    }
  else
    {
      if (x < p->winx || x >= p->winx + brl.x)
	p->winx = x >= (scr.cols / brl.x) * brl.x ? scr.cols - brl.x : \
	  (x / brl.x) * brl.x;
      if (y < p->winy || y >= p->winy + brl.y)
	p->winy = y < brl.y - 1 ? 0 : y - (brl.y - 1);
    }
}


void 
csrjmp (int x, int y)
{
  /*
   * Fork cursor routing subprocess. * First, we must check if a
   * subprocess is already running: if so, we * send it a SIGUSR1 and
   * wait for it to die. 
   */
  signal (SIGCHLD, SIG_IGN);	/* ignore SIGCHLD for the moment */
  if (csr_active)
    {
      kill (csr_pid, SIGUSR1);
      wait (NULL);
      csr_active--;
    }
  signal (SIGCHLD, stop_child);	/* re-establish handler */

  csr_active++;
  switch (csr_pid = fork ())
    {
    case -1:			/* fork failed */
      csr_active--;
      break;
    case 0:			/* child, cursor routing process */
      nice (CSRJMP_NICENESS);	/* reduce scheduling priority */
      csrjmp_sub (x, y);
      exit (0);			/* terminate child process */
    default:			/* parent waits for child to return */
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
  while (dify * (y - scr.posy) > 0 && 
	 curscr == scr.no &&
	 !timeout_yet (CSRJMP_TIMEOUT))
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
	  curx = scr.posx;
	  scr = getstat_phys ();
	}
      while ((scr.posy - cury) * dify <= 0
	     && !((scr.posx - curx) * dify <= 0)
	     && !timeout_yet (CSRJMP_TIMEOUT));
      sigprocmask (SIG_UNBLOCK, &mask, NULL);	/* killed here if SIGUSR1 */
    }

  if(x<0) return; /* vertical routing only */

  /* Now horizontal movement, quitting if the vertical position is wrong: */
  difx = x - scr.posx;
  while (difx * (x - scr.posx) > 0 && scr.posy == y && 
	 curscr == scr.no && 
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

      /*
       * Do Braille translation using current table. * Six-dot mode is
       * ignored, since case can be important, and * blinking caps won't 
       * work ... 
       */
      for (i = 0; i < brl.x * brl.y; brl.disp[i] = curtbl[brl.disp[i]], i++);

      writebrl (brl);
      if (l)
	while (readbrl (TBL_ARG) == EOF)
	  delay (KEYDEL);
    }
  usetable (p->dispmode ? TBL_ATTRIB : TBL_TEXT);
}


void 
clrbrlstat (void)
{
  memset (statcells, 0, sizeof(statcells));
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
  menu[18] =
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
      &env.csrsize, "block cursor  ", 1, 0, 1
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
      &env.attrvis, "attr visible  ", 1, 0, 1
    }
    ,
    {
      &env.attrblink, "attr blink    ", 1, 0, 1
    }
    ,
    {
      &env.csroncnt, "csr blink on  ", 0, 1, 16
    }
    ,
    {
      &env.csroffcnt, "csr blink off ", 0, 1, 16
    }
    ,
    {
      &env.caponcnt, "cap blink on  ", 0, 1, 16
    }
    ,
    {
      &env.capoffcnt, "cap blink off ", 0, 1, 16
    }
    ,
    {
      &env.attroncnt, "attr blink on ", 0, 1, 16
    }
    ,
    {
      &env.attroffcnt, "attr blink off", 0, 1, 16
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
  short maxn = 17;
  char buffer[20];

  /* status cells */
  memset (statcells, 0, sizeof(statcells));
  statcells[0] = texttrans['C'];
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
	  case CMD_WINUP:
	  case CMD_FWINLT:
	    if (--*menu[n].ptr < menu[n].min)
	      *menu[n].ptr = menu[n].max;
	    break;
	  case CMD_WINDN:
	  case CMD_FWINRT:
	    if (++*menu[n].ptr > menu[n].max)
	      *menu[n].ptr = menu[n].min;
	    break;
	  case CMD_RESET:
	    env = oldenv;
	    break;
	  case CMD_SAVECONF:
	    savecfg |= 1;
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
	    i = 0;
	  }
      close (tbl_fd);
    }
  if (i)
    env = initenv;
  soundstat (env.sound);
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
