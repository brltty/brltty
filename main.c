/*
 * BrlTty - A daemon providing access to the Linux console (when in text
 *          mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BrlTty Team. All rights reserved.
 *
 * BrlTty comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/*
 * main.c - Main processing loop plus signal handling
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "brl.h"
#include "spk.h"
#include "scr.h"
#include "csrjmp.h"
#include "inskey.h"
#include "beeps.h"
#include "cut-n-paste.h"
#include "misc.h"
#include "message.h"
#include "config.h"
#include "common.h"


char VERSION[] = "BrlTty 2.99 (beta)";
char COPYRIGHT[] = "Copyright (C) 1995-2001 by The BrlTty Team.  All rights reserved.";

/*
 * Misc param variables
 */
short dispmd = LIVE_SCRN;	/* freeze screen on/off */
short infmode = 0;		/* display screen image or info */
short csr_offright;		/* used for sliding window */
short hwinshift;		/* Half window horizontal distance */
short fwinshift;		/* Full window horizontal distance */
short vwinshift;		/* Window vertical distance */
struct brltty_env env;		/* environment (i.e. global) parameters */
brldim brl;			/* For the Braille routines */
scrstat scr;			/* For screen state infos */
unsigned int TickCount = 0;	/* incremented each main loop cycle */


/*
 * useful macros
 */

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#define BRL_ISUPPER(c) \
	(isupper (c) || (c) == '@' || (c) == '[' || (c) == '^' || \
	 (c) == ']' || (c) == '\\')

#define TOGGLEPLAY(var)  play((var) ? snd_toggleon : snd_toggleoff)
#define TOGGLE(var) \
	(var = (keypress & VAL_SWITCHON) \
	       ? 1 : ((keypress & VAL_SWITCHOFF) \
		      ? 0 : (var ^ 1)))


/*
 * Output braille translation tables.
 * The files *.auto.h (the default tables) are generated at compile-time.
 */
unsigned char texttrans[256] =
{
  #include "text.auto.h"
};
unsigned char untexttrans[256];

unsigned char attribtrans[256] =
{
  #include "attrib.auto.h"
};

unsigned char *curtbl = texttrans;	/* active translation table */

void 
usetable (int tbl)
{
  curtbl = (tbl == TBL_ATTRIB) ? attribtrans : texttrans;
}


/* 
 * Array definition containing pointers to brltty_param structures for 
 * each screen.  Those structures are dynamically allocated when a 
 * specific screen is used for the first time.
 * Screen 0 is reserved for help screen.
 */
#define NBR_SCR 16		/* actual number of separate screens */

struct brltty_param scr0;	/* at least one is statically allocated */
struct brltty_param *scrparam[NBR_SCR+1] = { &scr0, };
struct brltty_param *p = &scr0;	/* pointer to current param structure */
int curscr;			/* current screen number */

void
switchto( unsigned int scrno )
{
  curscr = scrno;
  if (scrno > NBR_SCR)
      scrno = 0;
  if (!scrparam[scrno]){ 	/* if not already allocated... */
    {
      if (!(scrparam[scrno] = malloc (sizeof (*p))))
        scrno = 0; 	/* unable to allocate a new structure */
      else
        *scrparam[scrno] = initparam;
    }
  }
  p = scrparam[scrno];
  usetable (p->dispmode ? TBL_ATTRIB : TBL_TEXT);
}


/*
 * Status cells support 
 * remark: the Papenmeier has a column with 22 cells, 
 * all other terminals use up to 5 bytes
 */
unsigned char statcells[22];	/* status cell buffer */

/* Number dot translation for status cells */
const unsigned char num[10] = {14, 1, 5, 3, 11, 9, 7, 15, 13, 6};

void clrbrlstat (void)
{
  memset (statcells, 0, sizeof(statcells));
  braille->setstatus(statcells);
}


/*
 * controls program termination
 */
volatile int keep_going = 1;
	
void 
termination_handler (int signum)
{
  keep_going = 0;
  signal (signum, termination_handler);
}

void 
child_stop_handler (int signum)
{
  pid_t pid;
  signal (signum, child_stop_handler);
  pid = wait(NULL);
  if (pid == csr_pid) 
    {
      csr_pid = 0;
      csr_active--;
    }
}


void 
setwinabsxy (int x, int y)
{
  if (x < p->winx || x >= p->winx + brl.x)
    p->winx = x >= (scr.cols / brl.x) * brl.x ? scr.cols - brl.x : \
      (x / brl.x) * brl.x;
  if (y < p->winy || y >= p->winy + brl.y)
    p->winy = y < brl.y - 1 ? 0 : y - (brl.y - 1);
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
    setwinabsxy(x,y);
}


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
  short speaking_scrno = -1, speaking_prev_inx = -1, speaking_start_line = 0;


  /* Open the system log. */
  LogOpen();
  LogPrint(LOG_NOTICE, "%s starting.", VERSION);

  /* Setup everything required on startup */
  startup(argc, argv);

  /*
   * Establish signal handler to clean up before termination:
   */
  if (signal (SIGTERM, termination_handler) == SIG_IGN)
      signal (SIGTERM, SIG_IGN);
  signal (SIGINT, SIG_IGN);
  signal (SIGHUP, SIG_IGN);
  signal (SIGCHLD, child_stop_handler);
  signal (SIGPIPE, SIG_IGN);

  if (!opt_q) {
    message (VERSION, 0);	/* display initialisation message */
    delay (DISPDEL);		/* sleep for a while */
  }

  /*
   * Initialize state variables 
   */
  *p = initparam;
  scrparam[0] = p;
  for (i = 1; i <= NBR_SCR; i++)
    scrparam[i] = 0;
  curscr = 0;
	
  getstat (&scr);
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
      while ((keypress = braille->read(TBL_CMD)) != EOF)
	switch (keypress & ~VAL_SWITCHMASK)
	  {
	  case CMD_NOOP:	/* do nothing but loop */
	    continue;
	  case CMD_RESTARTBRL:
	    braille->close(&brl);
	    play(snd_brloff);
	    LogPrint(LOG_INFO,"Reinitializing braille driver.");
	    startbrl();
	    break;
	  case CMD_RESTARTSPEECH:
	    speech->mute();
	    speech->close();
	    LogPrint(LOG_INFO,"Reinitializing speech driver.");
	    speech->initialize(speech_drvparm);
	    break;
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
	  case CMD_FWINLT:
	    if(env.skpblnkwins)
	      goto fwinltskip;
	    if(env.skpblnkeol && p->winx == scr.cols-brl.x
	       && p->winy > 0
	       && (scr.posy != p->winy || scr.posx >= p->winx)) {
	      char buffer[scr.cols];
	      int i;
	      getscr ((winpos)
	      {
		0, p->winy, scr.cols, 1
		  }
		      ,buffer, SCR_TEXT);

	      for(i=0; i<p->winx; i++)
		if(buffer[i] != ' ' && buffer[i] != 0)
		  break;
	      if(i == p->winx) {
		play (snd_wrap_up);
		goto skip_back;
	      }
	    }
	    if (p->winx == 0){
	      if(p->winy > 0){
		p->winx = scr.cols - brl.x;
		p->winy--;
		play (snd_wrap_up);
	      }else
		play (snd_bounce);
	    }else
	      p->winx = MAX (p->winx - fwinshift, 0);
	    break;
	    skip_back: /* go on through */
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
		int scrtype = (p->dispmode || keypress==CMD_ATTRUP)
		  ? SCR_ATTRIB : SCR_TEXT;
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
			(keypress!=CMD_ATTRUP && p->winy == scr.posy))
		      break;	/* lines are different */
                    if(p->winy == 0){
                      play(snd_bounce);
                      break;
                    }
		    /* lines are identical */
		    if (skipped == 0)
		      play (snd_skip_first);
		    else if (skipped <= 4)
		      play (snd_skip);
		    else if (skipped % 4 == 0)
		      play (snd_skipmore);
		    skipped++;	/* will sound only if lines are skipped,
				   not if we just move by one line */
		  }
	      }
	    break;
	  case CMD_FWINRT:
	    if(env.skpblnkwins)
	      goto fwinrtskip;
	    if(env.skpblnkeol && p->winx == 0
	       && p->winy < scr.rows - brl.y
	       && (scr.posy != p->winy || scr.posx < brl.x)) {
	      char buffer[scr.cols];
	      int i;
	      getscr ((winpos)
	      {
		0, p->winy, scr.cols, 1
		  }
		      ,buffer, SCR_TEXT);

	      for(i=brl.x; i<scr.cols; i++)
		if(buffer[i] != ' ' && buffer[i] != 0)
		  break;
	      if(i == scr.cols) {
		play (snd_wrap_down);
		goto skip_forw;
	      }
	    }
	    if (p->winx >= scr.cols - brl.x){
	      if (p->winy < scr.rows - brl.y){
		p->winx = 0;
		p->winy++;
		play (snd_wrap_down);
	      }else
		  play (snd_bounce);
	    }else
	      p->winx = MIN (p->winx + fwinshift, scr.cols - brl.x);
	    break;
	    skip_forw: /* go on through */
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
		int scrtype = (p->dispmode || keypress==CMD_ATTRDN)
		  ? SCR_ATTRIB : SCR_TEXT;
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
			(keypress!=CMD_ATTRDN && p->winy == scr.posy))
		      break;	/* lines are different */
		    if(p->winy == scr.rows-brl.y){
		      play(snd_bounce);
		      break;
		    }
		    /* lines are identical */
		    if (skipped == 0)
		      play (snd_skip_first);
		    else if (skipped <= 4)
		      play (snd_skip);
		    else if (skipped % 4 == 0)
		      play (snd_skipmore);
		    skipped++;	/* will sound only if lines are skipped,
				   not if we just move by one line */
		  }
	      }
	    break;
	  case CMD_NXBLNKLN:
	  case CMD_PRBLNKLN: {
	    int dir = (keypress == CMD_NXBLNKLN) ? +1 : -1;
	    int i;
	    int found = 0;
	    int l = p->winy +dir;
	    char buffer[scr.cols];
	    /* look for a blank line */
	    while(l>=0 && l <= scr.rows - brl.y) {
		getscr ((winpos)
			{0, l, scr.cols, 1}
			,buffer, SCR_TEXT);
		for(i=0; i<scr.cols; i++)
		  if(buffer[i] != ' ' && buffer[i] != 0)
		    break;
		if(i==scr.cols){
		  found = 1;
		  break;
		}
		l += dir;
	    }
	    if(found) {
	      /* look for a non-blank line */
	      found = 0;
	      while(l>=0 && l <= scr.rows - brl.y) {
		getscr ((winpos)
		        {0, l, scr.cols, 1}
			,buffer, SCR_TEXT);
		for(i=0; i<scr.cols; i++)
		  if(buffer[i] != ' ' && buffer[i] != 0)
		    break;
		if(i<scr.cols){
		  p->winy = l;
		  p->winx = 0;
		  found = 1;
		  break;
		}
		l += dir;
	      }
	    }
	    if(!found) {
	      play (snd_bounce);
	      play (snd_bounce);
	    }
	    break;
	  }
	  case CMD_FWINRTSKIP:
	  fwinrtskip: {
	    int y = p->winy, x = p->winx;
	    int i,l, cnt=0;
	    do {
	      char buffer[scr.cols];
	      if(p->winx >= scr.cols - brl.x) {
		if (p->winy < scr.rows - brl.y){
		  p->winx = 0;
		  p->winy++;
		  if(++cnt<=3)
		    play (snd_wrap_down);
		}else{
		  p->winy = y;
		  p->winx = x;
		  play (snd_bounce);
		  break;
		}
	      }else
		p->winx = MIN (p->winx + fwinshift, scr.cols - brl.x);
	      l = MAX(p->winx, scr.cols-brl.x);
	      getscr ((winpos)
	      {
		p->winx, p->winy, l, 1
		  }
		      ,buffer, SCR_TEXT);
	      for(i=0; i<l; i++)
		if(buffer[i] != ' ' && buffer[i] != 0)
		  break;
	    } while(i == l
		    && (p->winy != scr.posy
			|| scr.posx < p->winx || scr.posx > p->winx+brl.x));
	    break;
	  }
	  case CMD_FWINLTSKIP:
	  fwinltskip: {
	    int y = p->winy, x = p->winx;
	    int i,l, cnt=0;
	    do {
	      char buffer[scr.cols];
	      if(p->winx == 0) {
		if (p->winy > 0) {
		  p->winx = scr.cols - brl.x;
		  p->winy--;
		  if(++cnt<=3)
		    play (snd_wrap_down);
		}else{
		  p->winy = y;
		  p->winx = x;
		  play (snd_bounce);
		  break;
		}
	      }else
		p->winx = MAX (p->winx - fwinshift, 0);
	      l = MAX(p->winx, scr.cols-brl.x);
	      getscr ((winpos)
	      {
		p->winx, p->winy, l, 1
		  }
		      ,buffer, SCR_TEXT);
	      for(i=0; i<l; i++)
		if(buffer[i] != ' ' && buffer[i] != 0)
		  break;
	    } while(i == l
		    && (p->winy != scr.posy
			|| scr.posx < p->winx || scr.posx > p->winx+brl.x));
	    break;
	  }
	  case CMD_NXSEARCH:
	  case CMD_PRSEARCH: {
	    int dir = (keypress == CMD_NXSEARCH) ? +1 : -1;
	    int found = 0, caseSens = 0;
	    int l = p->winy;
	    char buffer[scr.cols+1];
	    char *ptr;
	    if(!cut_buffer || strlen(cut_buffer) > scr.cols)
	      break;
	    ptr = cut_buffer;
	    while(*ptr)
	      if(isupper(*(ptr++))) {
		caseSens = 1;
		break;
	      }
	    while(l>=0 && l <= scr.rows - brl.y) {
	      getscr ((winpos)
			{0, l, scr.cols, 1}
			,buffer, SCR_TEXT);
	      buffer[scr.cols] = 0;
	      if(!caseSens) {
		ptr = buffer;
		while(*ptr) {
		  *ptr = tolower(*ptr);
		  ptr++;
		}
	      }
	      if((ptr = strstr(buffer, cut_buffer))) {
		p->winy = l;
		p->winx = (ptr-buffer) / brl.x * brl.x;
		found = 1;
		break;
	      }
	      l += dir;
	    }
	    if(!found) {
	      play (snd_bounce);
	      play (snd_bounce);
	    }
	    break;
	  }
	  case CMD_HOME:
	    setwinxy (scr.posx, scr.posy);
	    break;
	  case CMD_SPKHOME:
	    if(scr.no == speaking_scrno) {
	      int inx = speech->track();
	      int y,x;
	      y = inx / scr.cols + speaking_start_line;
	      x = ((inx % scr.cols) / brl.x) * brl.x;
	      setwinabsxy( x,y );
	    }
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
	  case CMD_CSRJMP:
	    if ((dispmd & HELP_SCRN) != HELP_SCRN)
	      csrjmp (p->winx, p->winy, curscr);
	    break;
	  case CMD_CSRJMP_VERT:
	    if ((dispmd & HELP_SCRN) != HELP_SCRN)
	      csrjmp (-1, p->winy, curscr);
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
	    /* toggles the config option that decides whether cursor
	       is shown at all */
	    TOGGLEPLAY ( TOGGLE(env.csrvis) );
	    break;
	  case CMD_CSRHIDE_QK:
	    /* This is for briefly hiding the cursor */
	    TOGGLE(p->csrhide);
	    /* no sound */
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
		if(speech->isSpeaking())
		  speaking_prev_inx = -1;
		else setwinxy (scr.posx, scr.posy);
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
	    if ((dispmd & HELP_SCRN) != HELP_SCRN && !csr_active)
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
	  case CMD_SKPBLNKEOL:
	    TOGGLEPLAY ( TOGGLE(env.skpblnkeol) );
	    break;
	  case CMD_SKPBLNKWINS:
	    TOGGLEPLAY ( TOGGLE(env.skpblnkwins) );
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
	  case CMD_SAYALL:
	    {
	      /* OK heres a crazy idea: why not send the attributes with the
                 text, in case some inflection or marking can be added...! The
                 speech driver's say function will receive a buffer of text
                 and a length, but in reality the buffer will contain twice
                 len bytes: the text followed by the video attribs data. */
	      unsigned int i;
	      unsigned r = (keypress == CMD_SAYALL) ? scr.rows - p->winy :1;
	      unsigned char buffer[ 2*( r *scr.cols )];
	      getscr ((winpos)
		      {
			0, p->winy, scr.cols, r
		      }
		      ,buffer, \
		      SCR_TEXT);
	      i = r*scr.cols;
	      i--;
	      while(i>=0 && buffer[i] == 0) i--;
	      i++;
	      if(i==0) break;
	      if(speech->sayWithAttribs != NULL) {
		getscr ((winpos)
		{
		  0, p->winy, scr.cols, r
		    }
			,buffer+i, \
			SCR_ATTRIB);
		speech->sayWithAttribs(buffer, i);
	      }else speech->say(buffer, i);
	      speaking_scrno = scr.no;
	      speaking_start_line = p->winy;
	    }
	    break;
	  case CMD_MUTE:
	    speech->mute();
	    break;
	  default: {
	      int key = (keypress & ~0xFF);
	      int arg = keypress &0xFF;
	      switch(key) {
	      case VAL_PASSTHRU: {
		char buf[] = {arg, 0};
		inskey (buf);
		break;
	      }
	      case VAL_BRLKEY: {
		char buf[] = {untexttrans[arg], 0};
		inskey(buf);
		break;
	      }
	      case CR_ROUTEOFFSET:
		/* Cursor routing keys: */
		if(arg >= brl.x || (dispmd & HELP_SCRN) == HELP_SCRN)
		  break;
		csrjmp (p->winx + arg, p->winy, curscr);
		break;
	      case CR_BEGBLKOFFSET:
		if(arg >= brl.x) break;
		cut_begin (p->winx + arg, p->winy);
		break;
	      case CR_ENDBLKOFFSET:
		if(arg >= brl.x) break;
		cut_end (p->winx + arg, p->winy);
		break;
	      case CR_SWITCHVT:
		if(arg < 0X3F)
		  switchvt(arg+1);
		break;
	      case CR_NXINDENT:
	      case CR_PRINDENT: {
		int i;
		int dir = (key == CR_NXINDENT) ? +1 : -1;
		int n = arg+p->winx+1;
		int found = 0;
		int l = p->winy +dir;
		char buffer[scr.cols];
		if(n<=scr.cols) {
		  while(l>=0 && l <= scr.rows - brl.y) {
		    getscr ((winpos)
			{0, l, n, 1}
			,buffer, SCR_TEXT);
		    for(i=0; i<n; i++)
		      if(buffer[i] != ' ' && buffer[i] != 0)
			break;
		    if(i<n){
		      found = 1;
		      p->winy = l;
		      break;
		    }
		    l += dir;
		  }
		  if(!found) {
		    play (snd_bounce);
		    play (snd_bounce);
		  }
		}
		break;
	      }
	      default:
		LogPrint(LOG_DEBUG,
			 "Driver sent unrecognized command 0x%x.",
			 keypress);
	      };
	      break;
	    }
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
      getstat (&scr);
      if( !(dispmd & (HELP_SCRN|FROZ_SCRN)) && curscr != scr.no)
	switchto (scr.no);

      /* speech tracking */
      speech->processSpkTracking(); /* called continually even if we're not tracking
			       so that the pipe doesn't fill up. */
      if(p->csrtrk && speech->isSpeaking() && scr.no == speaking_scrno) {
	int inx = speech->track();
	if(inx != speaking_prev_inx) {
	  int y,x;
	  speaking_prev_inx = inx;
	  y = inx / scr.cols + speaking_start_line;
	  /* in case indexing data is wrong */
	  y %= scr.rows;
	  x = ((inx % scr.cols) / brl.x) * brl.x;
	  setwinabsxy( x,y );
	}
      }
      else /* cursor tracking */
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
	/* We could check to see if we changed screen, but that doesn't
	   really matter... this is mainly for when you are hunting up/down
	   for the line with attributes. */
	if(p->winx != oldwinx || p->winy != oldwiny){
	  attron = 1;
	  attrcntr = env.attroncnt;
	}
	/* problem: this still doesn't help when the braille window is
	   stationnary and the attributes themselves are moving
	   (example: tin). */
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
	    case ST_MDVStyle:
	      statcells[0] = num[((p->winy+1) / 10) % 10] | \
		(num[((p->winx+1) / 10) % 10] << 4);
	      statcells[1] = num[(p->winy+1) % 10]
		| (num[(p->winx+1) % 10] << 4);
	      break;
	    case ST_Papenmeier:
	      memset (statcells, 0, sizeof(statcells));
	      statcells [STAT_current] = p->winy+1;
	      statcells [STAT_row] = scr.posy+1;
	      statcells [STAT_col] = scr.posx+1;
	      statcells [STAT_tracking] = p->csrtrk;
	      statcells [STAT_dispmode] = p->dispmode;
	      statcells [STAT_frozen] = (dispmd & FROZ_SCRN) == FROZ_SCRN;
	      statcells [STAT_visible] = env.csrvis;
	      statcells [STAT_size] = env.csrsize;
	      statcells [STAT_blink] = env.csrblink;
	      statcells [STAT_capitalblink] = env.capblink;
	      statcells [STAT_dots] = env.sixdots;
	      statcells [STAT_sound] = env.sound;
	      statcells [STAT_skip] = env.skpidlns;
	      statcells [STAT_underline] = env.attrvis;
	      statcells [STAT_blinkattr] = env.attrblink;
	      break;
	    default:
	      memset (statcells, 0, sizeof(statcells));
	      break;
	    }
	  braille->setstatus(statcells);

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
		/* Experimental! Attribute values are hardcoded... */
  	        case 0x08: /* dark-gray on black */
  	        case 0x07: /* light-gray on black */
  	        case 0x17: /* light-gray on blue */
  	        case 0x30: /* black on cyan */
		  break;
	        case 0x70: /* black on light-gray */
		  brl.disp[i] |= ATTR1CHAR;
		  break;
	        case 0x0F: /* white on black */
	        default:
		  brl.disp[i] |= ATTR2CHAR;
		  break;
	      };
	  }

	  /*
	   * If the cursor is visible and in range, and help is off: 
	   */
	  if (env.csrvis && !p->csrhide
	      && (!env.csrblink || csron) && scr.posx >= p->winx &&\
	      scr.posx < p->winx + brl.x && scr.posy >= p->winy && \
	      scr.posy < p->winy + brl.y && !(dispmd & HELP_SCRN))
	    brl.disp[(scr.posy - p->winy) * brl.x + scr.posx - p->winx] |= \
	      env.csrsize ? BIG_CSRCHAR : SMALL_CSRCHAR;
	  braille->write(&brl);
	}
      /*
       * If in info mode, send status information: 
       */
      else
	{
	  /* Here we must be careful - some displays (e.g. Braille Lite 18)
	   * are very small ...
	   *
	   * NB: large displays use message(), small use writebrl()
	   * directly.
	   */
	  unsigned char infbuf[22];
	  if (brl.x * brl.y >= 21) /* 21 is current size of output ... */
	    {
	      sprintf (infbuf, "%02d:%02d %02d:%02d %02d %c%c%c%c%c%c", \
		       p->winx, p->winy, scr.posx, scr.posy, curscr, 
		       p->csrtrk ? 't' : ' ', \
		       env.csrvis ? (env.csrblink ? 'B' : 'v') : \
		       (env.csrblink ? 'b' : ' '), p->dispmode ? 'a' : 't', \
		       (dispmd & FROZ_SCRN) == FROZ_SCRN ? 'f' : ' ', \
		       env.sixdots ? '6' : '8', env.capblink ? 'B' : ' ');
	    }
	  else
	    {
	      /* Hmm, what to do for small displays ...
	       * Rather arbitrarily, we'll save 6 cells by replacing the
	       * cursor and display positions by a CombiBraille-style
	       * status display.  This assumes brl.x * brl.y > 14 ...
	       */
	      sprintf (infbuf, "xxxxx %02d %c%c%c%c%c%c     ", \
		       curscr, p->csrtrk ? 't' : ' ', \
		       env.csrvis ? (env.csrblink ? 'B' : 'v') : \
		       (env.csrblink ? 'b' : ' '), p->dispmode ? 'a' : 't', \
		       (dispmd & FROZ_SCRN) == FROZ_SCRN ? 'f' : ' ', \
		       env.sixdots ? '6' : '8', env.capblink ? 'B' : ' ');
	      infbuf[0] = num[(p->winx / 10) % 10] << 4 | \
		num[(scr.posx / 10) % 10];
	      infbuf[1] = num[p->winx % 10] << 4 | num[scr.posx % 10];
	      infbuf[2] = num[(p->winy / 10) % 10] << 4 | \
		num[(scr.posy / 10) % 10];
	      infbuf[3] = num[p->winy % 10] << 4 | num[scr.posy % 10];
	      infbuf[4] = env.csrvis << 1 | env.csrsize << 3 | \
		env.csrblink << 5 | env.slidewin << 7 | p->csrtrk << 6 | \
		env.sound << 4 | p->dispmode << 2;
	      infbuf[4] |= (dispmd & FROZ_SCRN) == FROZ_SCRN ? 1 : 0;

	      /* We have to do the Braille translation ourselves, since
	       * we don't want the first five characters translated ...
	       */
	      for (i = 5; infbuf[i]; i++)
		infbuf[i] = texttrans[infbuf[i]];
	    }

	  /*
	   * status cells 
	   */
	  memset (statcells, 0, sizeof(statcells));
	  statcells[0] = texttrans['I'];
	  statcells[1] = texttrans['n'];
	  statcells[2] = texttrans['f'];
	  statcells[3] = texttrans['o'];
	  statcells[4] = texttrans[' '];
	  braille->setstatus(statcells);

	  if (brl.x * brl.y >= 21)
	    message (infbuf, MSG_SILENT);
	  else
	    {
	      memcpy (brl.disp, infbuf, brl.x * brl.y);
	      braille->write(&brl);
	    }
	}

      delay (DELAY_TIME);
    }

  clrbrlstat ();
  message ("BrlTty terminating.", 0);
  closescr ();

  /*
   * Hard-wired delay to try and stop us being killed prematurely ... 
   */
  delay (1000);
  speech->close();
  braille->close(&brl);
  play(snd_brloff);
  /* don't forget that scrparam[0] is staticaly allocated */
  for (i = 1; i <= NBR_SCR; i++) 
    free (scrparam[i]);
  LogPrint(LOG_NOTICE,"Terminating.");
  LogClose();
  return 0;
}



void 
message (unsigned char *s, short flags)
{
  int i, j, l, silent;

  silent = (flags & MSG_SILENT);
  l = strlen (s);

  if (!silent && env.sound)
    {
      speech->mute();
      speech->say(s, l);
    }

  while (l)
    {
      memset (brl.disp, ' ', brl.x * brl.y);

      /* strip leading spaces */
      while( *s == ' ' )  s++, l--;

      if (l <= brl.x * brl.y) {
	 j = l;	/* the whole message fits on the braille window */
      }else{
	 /* split the message on multiple window on space characters */
	 for( j = (brl.x * brl.y - 2); j > 0 && s[j] != ' '; j-- );
	 if( j == 0 ) j = brl.x * brl.y - 1;
      }
      for (i = 0; i < j; brl.disp[i++] = *s++);
      if (l -= j) {
	 brl.disp[brl.x * brl.y - 1] = '-';
      }

      /*
       * Do Braille translation using text table. * Six-dot mode is
       * ignored, since case can be important, and * blinking caps won't 
       * work ... 
       */
      for (i = 0; i < brl.x * brl.y; brl.disp[i] = texttrans[brl.disp[i]], i++);
      braille->write( &brl );

      if ( l || (flags & MSG_WAITKEY) )
	while (braille->read(TBL_ARG) == EOF)
	  delay (KEYDEL);
    }
}




