/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
 *
 * Web Page: http://www.cam.org/~nico/brltty
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
 * brltty.c - Main loop plus signal handling and cursor routing 
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
#include <errno.h>

// Not all sysems have support for getopt_long, i.e. --name=value options.
// Our simple test is that we have it if we're using glibc.
// Experience may show that this test needs to be enhanced.
// The rest of the code should test one of the defines related to
// getopt_long, e.g. the presence of no_argument.
#ifdef __GLIBC__
  #include <getopt.h>
#endif

#include "config.h"
#include "brl.h"
#include "scr.h"
#include "inskey.h"
#include "spk.h"
#include "beeps.h"
#include "cut-n-paste.h"
#include "misc.h"
#include "message.h"

#define VERSION "BRLTTY 2.95 (beta)"
#define COPYRIGHT "\
Copyright (C) 1995-2000 by The BRLTTY Team.  All rights reserved."
#define USAGE "\
Usage: %s [option ...]\n\
 -b driver  --braille=driver  Braille display driver to use: full library name\n\
                                or shortcut ("##BRLLIBS##")\n\
 -c file    --config=file     Path to configuration save/restore file.\n\
 -d device  --device=device   Path to device for accessing braille display.\n\
 -l n       --log=n           Syslog debugging level (from 0 to 7, default 5)\n\
 -q         --quiet           Suppress start-up messages.\n\
 -s driver  --speech=driver   Speech interface driver to use: full library name\n\
                                or shortcut ("##SPKLIBS##")\n\
 -p         --speechparm     Parameter to the speech driver
 -t file    --table=file      Path to dot translation table file.\n\
 -h         --help            Print this usage information.\n\
 -v         --version         Print start-up messages and exit.\n"

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
    short skpblnkeol;
    short stcellstyle;
  }
env, initenv = {
    ENV_MAGICNUM, INIT_CSRVIS, INIT_ATTRVIS, INIT_CSRBLINK, INIT_CAPBLINK,
    INIT_ATTRBLINK, INIT_CSRSIZE, INIT_CSR_ON_CNT, INIT_CSR_OFF_CNT,
    INIT_CAP_ON_CNT, INIT_CAP_OFF_CNT, INIT_ATTR_ON_CNT, INIT_ATTR_OFF_CNT,
    INIT_SIXDOTS, INIT_SLIDEWIN, INIT_BEEPSON, INIT_SKPIDLNS, INIT_SKPBLNKEOL,
    ST_None
};

/* struct definition for volatile parameters */
struct brltty_param
  {
    short csrtrk;		/* tracking mode */
    short csrhide;              /* For temporarily hiding cursor */
    short dispmode;		/* text or attributes display */
    short winx, winy;		/* Start row and column of Braille window */
    short cox, coy;		/* Old cursor position */
  }
initparam = {
    INIT_CSRTRK, INIT_CSRHIDE, TBL_TEXT, 0, 0, 0, 0
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
 * Output braille translation tables - the files *.auto.h are generated at *
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
/* -h, -l, -q and -v options */
short opt_h = 0, opt_q = 0, opt_v = 0, opt_l = LOG_NOTICE;
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
pid_t csr_pid = 0;

/*
 * Function prototypes: 
 */
void startbrl();
void switchto (unsigned int scrno); /* activate params for specified screen */
void csrjmp (int x, int y);	/* move cursor to (x,y) */
void csrjmp_sub (int x, int y);	/* cursor routing subprocess */
void setwinabsxy (int x, int y);	/* move window to include (x,y) */
void setwinxy (int x, int y);	/* move window to include (x,y) */
void clrbrlstat (void);
void termination_handler (int signum);	/* clean up before termination */
void stop_child (int signum);	/* called at end of cursor routing */
void usetable (int tbl);	/* set character translation table */
void configmenu (void);		/* configuration menu */
void loadconfig (void);
void saveconfig (void);
int nice (int);			/* should really be in a header file ... */

// Define error codes for configuration file processing.
#define CF_OK 0		// No error.
#define CF_NOVAL 1	// Operand not specified.
#define CF_INVAL 2	// Bad operand specified.
#define CF_EXTRA 3	// Too many operands.

static int get_token (char **val, const char *delims)
{
  char *v = strtok(NULL, delims);

  if (v == NULL)
    {
      return CF_NOVAL;
    }
    
  if (strtok(NULL, delims) != NULL)
    {
      return CF_EXTRA;
    }

  *val = strdup(v);
  return CF_OK;
}

static int set_braille_configuration (const char *delims)
{
  return get_token(&opt_c, delims);
}

static int set_braille_device (const char *delims)
{
  return get_token(&opt_d, delims);
}

static int set_braille_driver (const char *delims)
{
  return get_token(&braille_libname, delims);
}

static int set_speech_driver (const char *delims)
{
  return get_token(&speech_libname, delims);
}

static int set_speech_driverparm (const char *delims)
{
  return get_token(&speech_drvparm, delims);
}

static int set_dot_translation (const char *delims)
{
  return get_token(&opt_t, delims);
}

static void process_configuration_line (char *line, void *data)
{
  const char *word_delims = " \t"; // Characters which separate words.
  char *keyword; // Points to first word of each line.

  // Relate each keyword to its handler via a table.
  typedef struct
    {
      const char *name;
      int (*handler) (const char *delims); 
    } keyword_entry;
  const keyword_entry keyword_table[] =
    {
      {"braille-configuration", set_braille_configuration},
      {"braille-device",        set_braille_device},
      {"braille-driver",        set_braille_driver},
      {"speech-driver",         set_speech_driver},
      {"speech-driverparm",         set_speech_driverparm},
      {"dot-translation",       set_dot_translation},
      {NULL,                    NULL}
    };

  // Remove comment from end of line.
  {
    char *comment = strchr(line, '#');
    if (comment != NULL)
      *comment = '\0';
  }

  keyword = strtok(line, word_delims);
  if (keyword != NULL) // Ignore blank lines.
    {
      const keyword_entry *kw = keyword_table;
      while (kw->name != NULL)
        {
	  if (strcasecmp(keyword, kw->name) == 0)
	    {
	      int code = kw->handler(word_delims);
	      switch (code)
	        {
		  case CF_OK:
		    break;

		  case CF_NOVAL:
		    LogAndStderr(LOG_WARNING,
		                 "Operand not supplied for"
				 " configuration item '%s'.",
				 keyword);
		    break;

		  case CF_INVAL:
		    LogAndStderr(LOG_WARNING,
		                 "Invalid operand specified"
				 " for configuration item '%s'.",
				 keyword);
		    break;

		  case CF_EXTRA:
		    LogAndStderr(LOG_WARNING,
		                 "Too many operands supplied"
				 " for configuration item '%s'.",
				 keyword);
		    break;

		  default:
		    LogAndStderr(LOG_WARNING,
		                 "Internal error: unsupported"
				 " configuration file error code: %d",
				 code);
		    break;
		}
	      return;
	    }
	  ++kw;
	}
      LogAndStderr(LOG_WARNING,
                   "Unknown configuration item: '%s'.",
		   keyword);
    }
}

static void process_configuration_file (char *path)
{
   FILE *file = fopen(path, "r");
   if (file != NULL)
     { // The configuration file has been successfully opened.
       process_lines(file, process_configuration_line, NULL);
       fclose(file);
     }
   else if (errno != ENOENT)
     {
       LogAndStderr(LOG_WARNING,
                    "Cannot open configuration file '%s': %s",
                    path, strerror(errno));
       // Just a warning, don't exit.
     }
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

  const char *short_options = "+b:c:d:hl:qs:p:t:v-:";
  #ifdef no_argument
    const struct option long_options[] =
      {
        {"braille", required_argument, NULL, 'b'},
        {"config" , required_argument, NULL, 'c'},
        {"device" , required_argument, NULL, 'd'},
        {"help"   , no_argument      , NULL, 'h'},
        {"log"    , required_argument, NULL, 'l'},
        {"quiet"  , no_argument      , NULL, 'q'},
        {"speech" , required_argument, NULL, 's'},
        {"speechparm" , required_argument, NULL, 'p'},
        {"table"  , required_argument, NULL, 't'},
        {"version", no_argument      , NULL, 'v'},
        {NULL     , 0                , NULL, 0  }
      };
    #define get_option() getopt_long(argc, argv, short_options, long_options, NULL)
  #else
    #define get_option() getopt(argc, argv, short_options)
  #endif

  /* Open the system log. */
  LogOpen();
  LogPrint(LOG_NOTICE, "%s starting.", VERSION);

  /* Process the configuration file. */
  process_configuration_file(CONFIG_FILE);

  /* Parse command line using getopt(): */
  while ((i = get_option()) != -1)
    switch (i)
      {
      case '?': // An invalid option has been specified.
	// An error message has already been displayed.
        exit(2);
      case 'b':			/* name of driver */
	braille_libname = optarg;
	break;
      case 'c':		/* configuration filename */
	opt_c = optarg;
	break;
      case 'd':		/* serial device name */
	opt_d = optarg;
	break;
      case 's':			/* name of driver */
	speech_libname = optarg;
	break;
      case 'p':			/* name of driver */
	speech_drvparm = optarg;
	break;
      case 't':		/* text translation table filename */
	opt_t = optarg;
	break;
      case 'h':		/* help */
	opt_h = 1;
	break;
      case 'l':	{  /* log level */
	char *endptr;
	opt_l = strtol(optarg,&endptr,0);
	if(endptr==optarg || *endptr != 0 || opt_l<0 || opt_l>7){
	  fprintf(stderr,"Invalid log level... ignored\n");
	  opt_l = 4;
	}
      }
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
	else
	  {
	    fprintf(stderr, "Unknown option -- %s\n", optarg);
	    exit(2);
	  }
	break;
      }
  #undef get_option

  if (opt_h)
    {
      printf (USAGE, argv[0]);
      return 0;
    }

  /* Set logging priority levels. */
  SetLogPrio(opt_l);
  SetErrPrio(opt_q ? LOG_WARNING : LOG_INFO);

  if (opt_d == NULL)
    opt_d = BRLDEV;
  if (*opt_d == 0)
    {
      LogAndStderr(LOG_ERR, "No braille device specified - use -d.");
      exit(10);
    }

  if (!load_braille_driver())
    {
      fprintf(stderr, "%s braille driver selection -- use '-b XX' option to specify one.\n\n",
              braille_libname ? "Bad" : "No");
      list_braille_drivers();
      fprintf( stderr, "\nUse '%s -h' for quick help.\n\n", argv[0] );
      exit(10);
    }

  if (!opt_c)
    {
      char *part1 = "brltty-";
      char *part2 = braille->identifier;
      char *part3 = ".dat";
      opt_c = malloc(strlen(part1) + strlen(part2) + strlen(part3) + 1);
      sprintf(opt_c, "%s%s%s", part1, part2, part3);
    }

  if (!load_speech_driver())
    {
      fprintf(stderr, "%s speech driver selection -- use '-s XX' option to specify one.\n\n",
              speech_libname ? "Bad" : "No");
      list_speech_drivers();
      fprintf( stderr, "\nUse '%s -h' for quick help.\n\n", argv[0] );
      exit(10);
    }

  /* copy default mode for status display */
  initenv.stcellstyle = braille->pref_style;

  if (chdir (HOME_DIR))		/* * change to directory containing data files  */
    {
      char *backup_dir = "/etc";
      LogAndStderr(LOG_WARNING,
                   "Cannot change directory to '%s': %s",
                   HOME_DIR, strerror(errno));
      LogAndStderr(LOG_WARNING,
                   "Using backup directory '%s' instead.",
                   backup_dir);
      chdir (backup_dir);		/* home directory not found, use backup */
    }

  /*
   * Load text translation table: 
   */
  if (opt_t)
    {
      if ((tbl_fd = open (opt_t, O_RDONLY)) >= 0 && \
	  (curtbl = (unsigned char *) malloc (256)) && \
	  read (tbl_fd, curtbl, 256) == 256)
	memcpy (texttrans, curtbl, 256);
      else
	{
	  if (!opt_q)
	    LogAndStderr(LOG_WARNING,
	                 "Cannot read dot translation table '%s'.",
		         opt_t);
	  opt_t = NULL;
	}
      if (curtbl)
	{
	  free (curtbl);
	  curtbl = NULL;
        }
      if (tbl_fd >= 0)
	{
	  close (tbl_fd);
	  tbl_fd = -1;
	}
    }

  /*
   * Print version and copyright information: 
   */
  if (opt_v || !opt_q)
    {	
      /* keep quiet only if -q and not -v */
      puts (VERSION);
      puts (COPYRIGHT);
    }

  {
    char buffer[0X100];
    char *path = getcwd(buffer, sizeof(buffer));
    LogAndStderr(LOG_NOTICE, "Working Directory: %s",
	         path ? path : "path-too-long");
  }

  LogAndStderr(LOG_NOTICE, "Dot Translation Table: %s",
               opt_t ? opt_t : "built-in");
  LogAndStderr(LOG_NOTICE, "Braille Device: %s", opt_d);
  LogAndStderr(LOG_NOTICE, "Braille Driver: %s (%s)",
               braille_libname, braille->name);
  LogAndStderr(LOG_NOTICE, "Braille Help File: %s", braille->helpfile);
  LogAndStderr(LOG_NOTICE, "Braille Configuration File: %s", opt_c);
  LogAndStderr(LOG_NOTICE, "Speech Driver: %s (%s)",
               speech_libname, speech->name);

  /*
   * Give braille and speech libraries a chance to introduce themselves.
   */
  braille->identify();
  speech->identify();

  if (opt_v)
    return 0;

  /* Load configuration file */
  loadconfig ();

  /*
   * Initialize screen library 
   */
  if (initscr ())
    {				
      LogAndStderr(LOG_ERR, "Cannot read screen.");
      LogClose();
      exit (2);
    }
  
  /* allocate the first screen information structures */
  p = malloc (sizeof (*p));
  if (!p)
    {
      LogAndStderr(LOG_ERR, "Memory allocation error.");
      closescr();
      LogClose();
      exit(2);
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
      LogAndStderr(LOG_ERR, "fork: %s", strerror(errno));
      closescr ();
      LogClose();
      exit (3);
    case 0:			/* child, process becomes a daemon: */
      close (STDIN_FILENO);
      close (STDOUT_FILENO);
      close (STDERR_FILENO);
      LogPrint(LOG_DEBUG, "Becoming daemon.");
      if (setsid () == -1)
	{			
	  /* request a new session (job control) */
	  closescr ();
	  LogPrint(LOG_ERR, "setsid: %s", strerror(errno));
	  LogClose();
	  exit (4);
	}
      /* I read somewhere that the correct thing to do here is to fork
	 again, so we are not a group leader and then can never be associated
	 a controlling tty. Well... No harm I suppose. */
      switch(fork()) {
      case -1:
	LogAndStderr(LOG_ERR, "second fork: %s", strerror(errno));
	closescr ();
	LogClose();
	exit (3);
      case 0:
	break;
      default:
	return 0;
      };
      break;
    default:			/* parent returns to calling process: */
      return 0;
    }

  /* From this point, all IO functions as printf, puts, perror, etc. can't be
   * used anymore since we are a daemon.  The LogPrint facility should 
   * be used instead.
   */

  /* Initialise Braille and set text display: */
  startbrl();

  /* Initialise speech */
  speech->initialize(speech_drvparm);

  /* Initialise help screen */
  if (inithlpscr (braille->helpfile))
    LogPrint(LOG_WARNING, "Cannot open help screen file '%s'.", braille->helpfile);

  /*
   * Establish signal handler to clean up before termination: 
   */
  if (signal (SIGTERM, termination_handler) == SIG_IGN)
    signal (SIGTERM, SIG_IGN);
  signal (SIGINT, SIG_IGN);
  signal (SIGHUP, SIG_IGN);
  signal (SIGCHLD, stop_child);
  signal (SIGPIPE, SIG_IGN);

  usetable (TBL_TEXT);
  if (!opt_q)
    {
      message (VERSION, 0);	/* display initialisation message */
      delay (DISPDEL);		/* sleep for a while */
    }

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
	  case CMD_SKPBLNKEOL:
	    TOGGLEPLAY ( TOGGLE(env.skpblnkeol) );
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
	      while(buffer[--i] == 0);
	      i++;
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
	  default:
	    if (keypress & VAL_PASSTHRU) 
	      {
		char buf[2] = { keypress&0xFF, 0 };
		inskey (buf);
	      }
	    else if (keypress >= CR_ROUTEOFFSET && 
		     keypress < CR_ROUTEOFFSET + brl.x && 
		     (dispmd & HELP_SCRN) != HELP_SCRN)
	      {
		/* Cursor routing keys: */
		csrjmp (p->winx + keypress - CR_ROUTEOFFSET, p->winy);
	      }
	    else if (keypress >= CR_BEGBLKOFFSET && 
		     keypress < CR_BEGBLKOFFSET + brl.x)
	      {
		cut_begin (p->winx + keypress - CR_BEGBLKOFFSET, p->winy);
	      }
	    else if (keypress >= CR_ENDBLKOFFSET && 
		     keypress < CR_ENDBLKOFFSET + brl.x)
	      {
		cut_end (p->winx + keypress - CR_ENDBLKOFFSET, p->winy);
	      }
	    else
	      LogPrint(LOG_DEBUG,
		       "Driver sent unrecognized command 0x%x.",
		       keypress);
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
	      statcells [STAT_visible] = 0;
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
  message ("BRLTTY terminating", 0);
  closescr ();

  /*
   * Hard-wired delay to try and stop us being killed prematurely ... 
   */
  delay (1000);
  speech->close();
  braille->close(&brl);
  play(snd_brloff);
  for (i = 0; i <= NBR_SCR; i++) 
    free (scrparam[i]);
  LogPrint(LOG_NOTICE,"Terminating.");
  LogClose();
  return 0;
}


void
startbrl()
{
  braille->initialize(&brl, opt_d);
  if (brl.x == -1)
    {
      LogPrint(LOG_ERR, "Braille driver initialization failed.");
      closescr ();
      LogClose();
      exit (5);
    }
  fwinshift = brl.x;
  hwinshift = fwinshift / 2;
  csr_offright = brl.x / 4;
  vwinshift = 5;
  LogPrint(LOG_DEBUG, "Braille display has %d rows of %d cells.",
	   brl.y, brl.x);
  play(snd_detected);
  clrbrlstat ();
}


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
      while (wait(NULL) != csr_pid);
      csr_pid = 0;
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
  int dif, t = 0;
  sigset_t mask;		/* for blocking of SIGUSR1 */

  /* Set up signal mask: */
  sigemptyset (&mask);
  sigaddset (&mask, SIGUSR1);

  /* Initialise second thread of screen reading: */
  if (initscr_phys ())
    return;

  getstat_phys (&scr);

  /* Deal with vertical movement first, ignoring horizontal jumping ... */
  dif = y - scr.posy;
  while (dif != 0 && curscr == scr.no)
    {
      timeout_yet (0);		/* initialise stop-watch */
      sigprocmask (SIG_BLOCK, &mask, NULL);	/* block SIGUSR1 */
      inskey (dif > 0 ? DN_CSR : UP_CSR);
      sigprocmask (SIG_UNBLOCK, &mask, NULL);	/* unblock SIGUSR1 */
      do
	{
#if CSRJMP_LOOP_DELAY > 0
	  delay (CSRJMP_LOOP_DELAY);	/* sleep a while ... */
#endif
	  cury = scr.posy;
	  curx = scr.posx;
	  getstat_phys (&scr);
	}
      while (!(t = timeout_yet (CSRJMP_TIMEOUT)) &&
	     scr.posy==cury && scr.posx==curx);
      if(t) break;
      if((scr.posy==cury && (scr.posx-curx)*dif <= 0) ||
	 (scr.posy!=cury && (y-scr.posy)*(y-scr.posy) >= dif*dif))
      {
	delay(CSRJMP_SETTLE_DELAY);
	getstat_phys (&scr);
	if((scr.posy==cury && (scr.posx-curx)*dif <= 0) ||
	   (scr.posy!=cury && (y-scr.posy)*(y-scr.posy) >= dif*dif))
	{
	  /* We are getting farther from our target... Let's try to go back 
	   * to the previous position wich was obviously the nearest ever 
	   * reached before gibing up.
	   */
	  sigprocmask (SIG_BLOCK, &mask, NULL);	/* block SIGUSR1 */
	  inskey (dif < 0 ? DN_CSR : UP_CSR);
	  sigprocmask (SIG_UNBLOCK, &mask, NULL);	/* unblock SIGUSR1 */
	  break;
	}
      }
      dif = y - scr.posy;
    }

  if(x>=0){ /* don't do this for vertical-only routing (x=-1) */
    /* Now horizontal movement, quitting if the vertical position is wrong: */
    dif = x - scr.posx;
    while (dif != 0 && scr.posy == y && curscr == scr.no)
      {
	timeout_yet (0);		/* initialise stop-watch */
	sigprocmask (SIG_BLOCK, &mask, NULL);	/* block SIGUSR1 */
	inskey (dif > 0 ? RT_CSR : LT_CSR);
	sigprocmask (SIG_UNBLOCK, &mask, NULL);	/* unblock SIGUSR1 */
	do
	  {
#if CSRJMP_LOOP_DELAY > 0
	    delay (CSRJMP_LOOP_DELAY);	/* sleep a while ... */
#endif
	    curx = scr.posx;
	    getstat_phys (&scr);
	  }
	while (!(t = timeout_yet (CSRJMP_TIMEOUT)) &&
	       scr.posx==curx && scr.posy == y);
	if(t) break;
	if(scr.posy != y || (x-scr.posx)*(x-scr.posx) >= dif*dif){
	  delay(CSRJMP_SETTLE_DELAY);
	  getstat_phys (&scr);
	  if(scr.posy != y || (x-scr.posx)*(x-scr.posx) >= dif*dif){
	    /* We probably wrapped on a short line... or are getting farther 
	     * from our target. Try to get back to the previous position which
	     * was obviously the nearest ever reached before we exit.
	     */
	    sigprocmask (SIG_BLOCK, &mask, NULL);	/* block SIGUSR1 */
	    inskey (dif > 0 ? LT_CSR : RT_CSR);
	    sigprocmask (SIG_UNBLOCK, &mask, NULL);	/* unblock SIGUSR1 */
	    break;
	  }
	}
	dif = x - scr.posx;
      }
  }

  closescr_phys ();		/* close second thread of screen reading */
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


void 
clrbrlstat (void)
{
  memset (statcells, 0, sizeof(statcells));
  braille->setstatus(statcells);
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
  pid_t pid;
  signal (signum, stop_child);
  pid = wait(NULL);
  if (pid == csr_pid) 
    {
      csr_pid = 0;
      csr_active--;
    }
}


void 
usetable (int tbl)
{
  curtbl = (tbl == TBL_ATTRIB) ? attribtrans : texttrans;
}


void 
configmenu (void)
{
  static short savecfg = 0;		/* 1 == save config on exit */
  static struct item
    {
      short *ptr;			/* pointer to the item value */
      char *desc;			/* item description */
      short bool;			/* 0 == numeric, 1 == bolean */
      short min;			/* minimum range */
      short max;			/* maximum range */
    }
  menu[] =
  {
    {
      &savecfg, "save config on exit", 1, 0, 1
    }
    ,
    {
      &env.csrvis, "cursor is visible", 1, 0, 1
    }
    ,
    {
      &env.csrsize, "block cursor", 1, 0, 1
    }
    ,
    {
      &env.csrblink, "blinking cursor", 1, 0, 1
    }
    ,
    {
      &env.capblink, "blinking capitals", 1, 0, 1
    }
    ,
    {
      &env.attrvis, "attributes are visible", 1, 0, 1
    }
    ,
    {
      &env.attrblink, "blinking attributes", 1, 0, 1
    }
    ,
    {
      &env.csroncnt, "blinking cursor visible period", 0, 1, 16
    }
    ,
    {
      &env.csroffcnt, "blinking cursor invisible period", 0, 1, 16
    }
    ,
    {
      &env.caponcnt, "blinking caps visible period", 0, 1, 16
    }
    ,
    {
      &env.capoffcnt, "blinking caps invisible period", 0, 1, 16
    }
    ,
    {
      &env.attroncnt, "blinking attributes visible period", 0, 1, 16
    }
    ,
    {
      &env.attroffcnt, "blinking attributes invisible period", 0, 1, 16
    }
    ,
    {
      &env.sixdots, "six dot text mode", 1, 0, 1
    }
    ,
    {
      &env.slidewin, "sliding window", 1, 0, 1
    }
    ,
    {
      &env.skpidlns, "skip identical lines", 1, 0, 1
    }
    ,
    {
      &env.skpblnkeol, "skip blank end-of-lines", 1, 0, 1
    }
    ,
    {
      &env.sound, "audio signals", 1, 0, 1
    }
    ,
    {
      &env.stcellstyle, "status cells style", 0, 0, NB_STCELLSTYLES
    }
  };
  struct brltty_env oldenv = env;	/* backup configuration */
  int i;				/* for loops */
  int k;				/* readbrl() value */
  int l;				/* current menu item length */
  int x = 0;				/* braille window pos in buffer */
  static int n = 0;			/* current menu item */
  int maxn = sizeof(menu)/sizeof(struct item) - 1;
					/* hiest menu item index */
  int updated = 0;			/* 1 when item's value has changed */
  unsigned char buffer[40];		/* display buffer */

  /* status cells */
  memset (statcells, 0, sizeof(statcells));
  statcells[0] = texttrans['C'];
  statcells[1] = texttrans['n'];
  statcells[2] = texttrans['f'];
  statcells[3] = texttrans['i'];
  statcells[4] = texttrans['g'];
  braille->setstatus(statcells);

  message ("Configuration menu", 0);
  delay (DISPDEL);

  while (keep_going)
    {
      /* First we draw the current menu item in the buffer */
      strcpy (buffer, menu[n].desc);
      if (menu[n].bool)
	sprintf (buffer + strlen(buffer), ": %s", *menu[n].ptr ? "on" : "off");
      else
	sprintf (buffer + strlen(buffer), ": %d", *menu[n].ptr);

      /* Next we deal with the braille window position in the buffer.
       * This is intended for small displays... or long item descriptions 
       */
      l = strlen (buffer);
      if (updated) 
	{
	  updated = 0;
	  /* make sure the updated value is visible */
	  if (l - x > brl.x * brl.y)
	    x = l - brl.x * brl.y;
	}

      /* Then draw the braille window */
      memset (brl.disp, 0, brl.x * brl.y);
      for (i = 0; i < MIN(brl.x * brl.y, l - x); i++)
	brl.disp[i] = texttrans[buffer[i+x]];
      braille->write(&brl);
      delay (DELAY_TIME);

      /* Now process any user interaction */
      while ((k = braille->read(TBL_CMD)) != EOF)
	switch (k)
	  {
	  case CMD_NOOP:
	    continue;
	  case CMD_TOP:
	  case CMD_TOP_LEFT:
	    n = x = 0;
	    break;
	  case CMD_BOT:
	  case CMD_BOT_LEFT:
	    n = maxn;
	    x = 0;
	    break;
	  case CMD_LNUP:
	    if (--n < 0)
	      n = maxn;
	    x = 0;
	    break;
	  case CMD_LNDN:
	    if (++n > maxn)
	      n = 0;
	    x = 0;
	    break;
	  case CMD_FWINLT:
	    if( x > 0 )
	      x -= MIN( brl.x * brl.y, x );
	    else
	      play (snd_bounce);
	    break;
	  case CMD_FWINRT:
	    if( l - x > brl.x * brl.y )
	      x += brl.x * brl.y;
	    else
	      play (snd_bounce);
	    break;
	  case CMD_WINUP:
	  case CMD_CHRLT:
	  case CMD_KEY_LEFT:
	  case CMD_KEY_UP:
	    if (--*menu[n].ptr < menu[n].min)
	      *menu[n].ptr = menu[n].max;
	    updated = 1;
	    break;
	  case CMD_WINDN:
	  case CMD_CHRRT:
	  case CMD_KEY_RIGHT:
	  case CMD_KEY_DOWN:
	  case CMD_HOME:
	  case CMD_KEY_RETURN:
	    if (++*menu[n].ptr > menu[n].max)
	      *menu[n].ptr = menu[n].min;
	    updated = 1;
	    break;
	  case CMD_SAY:
	    speech->say(buffer, l);
	    break;
	  case CMD_MUTE:
	    speech->mute();
	    break;
	  case CMD_HELP:
	    /* This is quick and dirty... Something more intelligent 
	     * and friendly need to be done here...
	     */
	    message( 
		"Press UP and DOWN to select an item, "
		"HOME to toggle the setting. "
		"Routing keys are available too! "
		"Press CONFIG again to quit.", MSG_WAITKEY);
	    break;
	  case CMD_RESET:
	    env = oldenv;
	    message ("changes undone", 0);
	    delay (DISPDEL);
	    break;
	  case CMD_SAVECONF:
	    savecfg |= 1;
	    /*break;*/
	  default:
	    if( k >= CR_ROUTEOFFSET && k < CR_ROUTEOFFSET + brl.x ) {
		/* Why not setting a value with routing keys... */
		if( menu[n].bool ) {
		    *menu[n].ptr = k % 2;
		}else{
		    *menu[n].ptr = k - CR_ROUTEOFFSET;
		    if( *menu[n].ptr > menu[n].max ) *menu[n].ptr = menu[n].max;
		    if( *menu[n].ptr < menu[n].min ) *menu[n].ptr = menu[n].min;
		}
		updated = 1;
		break;
	    }

	    /* For any other keystroke, we exit */
	    if (savecfg)
	      {
		saveconfig ();
		play (snd_done);
	      }
	    return;
	  }
    }
}


void 
loadconfig (void)
{
  int i = 1;
  struct brltty_env newenv;

  tbl_fd = open (opt_c, O_RDONLY);
  if(tbl_fd < 0){
    LogPrint(LOG_WARNING, "Cannot open configuration file '%s'.", opt_c);
  }else{
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
  tbl_fd = open (opt_c, O_WRONLY | O_CREAT | O_TRUNC);
  if (tbl_fd < 0){
    LogPrint(LOG_WARNING, "Cannot save to configuration file '%s'.", opt_c);
    message ("can't save config", 0);
    delay (DISPDEL);
  }
  else{
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
