/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

/*
 * config.c - Everything configuration related.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

// Not all systems have support for getopt_long, i.e. --name=value options.
// Our simple test is that we have it if we're using glibc.
// Experience may show that this test needs to be enhanced.
// The rest of the code should test one of the defines related to
// getopt_long, e.g. the presence of no_argument.
#ifdef __GLIBC__
  #include <getopt.h>
#endif

#include "config.h"
#include "brl.h"
#include "spk.h"
#include "scr.h"
#include "tunes.h"
#include "message.h"
#include "misc.h"
#include "common.h"


/*
 * Those should have been defined in the Makefile and passed along
 * with compiler arguments.
 */

#ifndef HOME_DIR
#warning HOME_DIR undefined
#define HOME_DIR "/dev/brltty"
#endif

#ifndef BRLDEV
#warning BRLDEV undefined
#define BRLDEV "/dev/ttyS0"
#endif

#ifndef BRLLIBS
#warning BRLLIBS undefined
#define BRLLIBS "???"
#endif

#ifndef SPKLIBS
#warning SPKLIBS undefined
#define SPKLIBS "???"
#endif


char USAGE[] = "\
Usage: %s [option ...]\n\
 -b driver  --braille=driver  Braille display driver to use: full library name\n\
                                or shortcut (" BRLLIBS ")\n\
 -c file    --config=file     Path to settings save/restore file.\n\
 -d device  --device=device   Path to device for accessing braille display.\n\
 -f file    --file=file       Path to default parameters file.\n\
 -l n       --log=n           Syslog debugging level (from 0 to 7, default 5)\n\
 -n         --nodaemon        Remain a foreground process.\n\
 -q         --quiet           Suppress start-up messages.\n\
 -s driver  --speech=driver   Speech interface driver to use: full library name\n\
                                or shortcut (" SPKLIBS ")\n\
 -S         --speechparm=arg  Parameter to speech driver.\n\
 -t file    --table=file      Path to dot translation table file.\n\
 -h         --help            Print this usage summary.\n\
 -v         --version         Print start-up messages and exit.\n";

/* -h, -l, -q and -v options */
short opt_h = 0, opt_n = 0, opt_q = 0, opt_v = 0, opt_l = LOG_INFO;
char *opt_c = NULL, *opt_d = NULL, *opt_f = NULL, *opt_t = NULL;	/* filename options */
short homedir_found = 0;	/* CWD status */

// Define error codes for configuration file processing.
#define CF_OK 0		// No error.
#define CF_NOVAL 1	// Operand not specified.
#define CF_INVAL 2	// Bad operand specified.
#define CF_EXTRA 3	// Too many operands.

static void process_options (int argc, char **argv)
{
  int option;

  const char *short_options = "+b:c:d:f:hl:nqs:S:t:v-:";
  #ifdef no_argument
    const struct option long_options[] =
      {
        {"braille" , required_argument, NULL, 'b'},
        {"config"  , required_argument, NULL, 'c'},
        {"device"  , required_argument, NULL, 'd'},
        {"file"    , required_argument, NULL, 'f'},
        {"help"    , no_argument      , NULL, 'h'},
        {"log"     , required_argument, NULL, 'l'},
        {"nodeamon", no_argument      , NULL, 'n'},
        {"quiet"   , no_argument      , NULL, 'q'},
        {"speech"  , required_argument, NULL, 's'},
        {"speechparm" , required_argument, NULL, 'S'},
        {"table"   , required_argument, NULL, 't'},
        {"version" , no_argument      , NULL, 'v'},
        {NULL      , 0                , NULL, 0  }
      };
    #define get_option() getopt_long(argc, argv, short_options, long_options, NULL)
  #else
    #define get_option() getopt(argc, argv, short_options)
  #endif

  /* Parse command line using getopt(): */
  while ((option = get_option()) != -1)
    /* continue on error as much as possible, as often we are typing blind
       and won't even see the error message unless the display come up. */
    switch (option)
      {
      case '?': // An invalid option has been specified.
	// An error message has already been displayed.
	return; /* not fatal */
      case 'b':			/* name of driver */
	braille_libname = optarg;
	break;
      case 'c':		/* configuration file path */
	opt_c = optarg;
	break;
      case 'd':		/* serial device path */
	opt_d = optarg;
	break;
      case 'f':		/* configuration file path */
	opt_f = optarg;
	break;
      case 'n':		/* configuration file path */
	opt_n = 1;
	break;
      case 's':			/* name of speech driver */
	speech_libname = optarg;
	break;
      case 'S':			/* parameter to speech driver */
	speech_drvparm = optarg;
	break;
      case 't':		/* text translation table file name */
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
	  /* not fatal */
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
	    /* not fatal */
	  }
	break;
      }
  #undef get_option

  if (opt_h)
    {
      printf (USAGE, argv[0]);
      exit(0);
    }
}

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

  if (!*val)
    {
      *val = strdup(v);
    }
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

static int process_configuration_file (char *path, int optional)
{
   FILE *file = fopen(path, "r");
   if (file != NULL)
     { // The configuration file has been successfully opened.
       process_lines(file, process_configuration_line, NULL);
       fclose(file);
     }
   else
     {
       if (optional && (errno == ENOENT))
         return 0;

       LogAndStderr(LOG_WARNING,
                    "Cannot open configuration file '%s': %s",
                    path, strerror(errno));
       // Just a warning, don't exit.
     }
   return 1;
}


/* 
 * Default definition for settings that are saveable.
 */
static struct brltty_env initenv = {
	{ENV_MAGICNUM&0XFF, ENV_MAGICNUM>>8},
	INIT_CSRVIS, 0,
	INIT_ATTRVIS, 0,
	INIT_CSRBLINK, 0,
	INIT_CAPBLINK, 0,
	INIT_ATTRBLINK, 0,
	INIT_CSRSIZE, 0,
	INIT_CSR_ON_CNT, 0,
	INIT_CSR_OFF_CNT, 0,
	INIT_CAP_ON_CNT, 0,
	INIT_CAP_OFF_CNT, 0,
	INIT_ATTR_ON_CNT, 0,
	INIT_ATTR_OFF_CNT, 0,
	INIT_SIXDOTS, 0,
	INIT_SLIDEWIN, 0,
	INIT_SOUND, INIT_TUNEDEV,
	INIT_SKPIDLNS,  0,
	INIT_SKPBLNKWINSMODE, 0,
	INIT_SKPBLNKWINS, 0,
	ST_None, INIT_WINOVLP
};

/* 
 * Default definition for volatile parameters
 */
struct brltty_param initparam = {
	INIT_CSRTRK, INIT_CSRHIDE, TBL_TEXT, 0, 0, 0, 0
};

void 
loadconfig (void)
{
  int fd, OK = 0;
  struct brltty_env newenv;

  fd = open (opt_c, O_RDONLY);
  if(fd < 0){
    LogPrint(LOG_WARNING, "Cannot open configuration file '%s'.", opt_c);
  }else{
      if (read (fd, &newenv, sizeof (newenv)) == sizeof (newenv))
	if ((newenv.magicnum[0] == (ENV_MAGICNUM&0XFF)) && (newenv.magicnum[1] == (ENV_MAGICNUM>>8)))
	  {
	    env = newenv;
	    OK = 1;
	  }
      close (fd);
    }
  if (!OK)
    env = initenv;
  setTuneDevice (env.tunedev);
}

void 
saveconfig (void)
{
  int fd;

  fd = open (opt_c, O_WRONLY | O_CREAT | O_TRUNC);
  if (fd < 0){
    LogPrint(LOG_WARNING, "Cannot save to configuration file '%s'.", opt_c);
    message ("can't save config", 0);
  }
  else{
    fchmod (fd, S_IRUSR | S_IWUSR);
    if (write (fd, &env, sizeof (struct brltty_env)) != \
	sizeof (struct brltty_env))
      {
	close (fd);
	fd = -2;
      }
    else
      close (fd);
  }
}


void
startbrl(void)
{
  braille->initialize(&brl, opt_d);
  if (brl.x == -1)
    {
      LogPrint(LOG_ERR, "Braille driver initialization failed.");
      closescr ();
      LogClose();
      exit (5);
    }
#ifdef ALLOW_OFFRIGHT_POSITIONS
  /* The braille display is allowed to stick out the right side of the
     screen by brl.x-offr. This allows the feature: */
  offr = 1;
#else
  /* This disallows it, as it was before. */
  offr = brl.x;
#endif
  fwinshift = brl.x;
  hwinshift = fwinshift / 2;
  csr_offright = brl.x / 4;
  vwinshift = 5;
  LogPrint(LOG_DEBUG, "Braille display has %d rows of %d cells.",
	   brl.y, brl.x);
  playTune(&tune_detected);
  clrbrlstat ();
}


void startup(int argc, char *argv[])
{
  process_options(argc, argv);

  /* Process the configuration file. */
  if (opt_f)
    {
      process_configuration_file(opt_f, 0);
    }
  else
    {
      process_configuration_file(CONFIG_FILE, 1);
    }

  /* Set logging priority levels. */
  SetLogPrio(opt_l);
  SetErrPrio(opt_q ? LOG_WARNING : LOG_INFO);

  if (opt_d == NULL)
    opt_d = BRLDEV;
  if (*opt_d == 0)
    {
      LogAndStderr(LOG_ERR, "No braille device specified - use -d.");
      /* this is fatal */
      exit(10);
    }

  if (!load_braille_driver())
    {
      fprintf(stderr, "%s braille driver selection -- use '-b XX' option to specify one.\n\n",
              braille_libname ? "Bad" : "No");
      list_braille_drivers();
      fprintf( stderr, "\nUse '%s -h' for quick help.\n\n", argv[0] );
      /* this is fatal */
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
      LogAndStderr(LOG_NOTICE, "Falling back to built-in speech driver.");
      /* not fatal */
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
      int tbl_fd = -1;
      unsigned char *tbl_p = NULL;
      if ((tbl_fd = open (opt_t, O_RDONLY)) >= 0 &&
	  (tbl_p = (unsigned char *) malloc (256)) && \
	  read (tbl_fd, tbl_p, 256) == 256)
	memcpy (texttrans, tbl_p, 256);
      else
	{
	  LogAndStderr(LOG_WARNING,
	               "Cannot read dot translation table '%s'.", opt_t);
	  opt_t = NULL;
	}
      if (tbl_p)
	  free (tbl_p);
      if (tbl_fd >= 0)
	  close (tbl_fd);
    }

  /* Find dots to char mapping by reversing braille translation table. */
  reverseTable(texttrans, untexttrans);

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
    LogAndStderr(LOG_INFO, "Working Directory: %s",
	         path ? path : "path-too-long");
  }

  LogAndStderr(LOG_INFO, "Settings Save/Restore File: %s", opt_c);
  LogAndStderr(LOG_INFO, "Help File: %s", braille->helpfile);
  LogAndStderr(LOG_INFO, "Translation Table: %s",
               opt_t ? opt_t : "built-in");
  LogAndStderr(LOG_INFO, "Braille Device: %s", opt_d);
  LogAndStderr(LOG_INFO, "Braille Driver: %s (%s)",
               braille_libname, braille->name);
  LogAndStderr(LOG_INFO, "Speech Driver: %s (%s)",
               speech_libname, speech->name);

  /*
   * Give braille and speech libraries a chance to introduce themselves.
   */
  braille->identify();
  speech->identify();

  if (opt_v)
    exit(0);

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
  
  if (!opt_n)
    {
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
    	exit(0);
          };
          break;
        default:			/* parent returns to calling process: */
          exit(0);
        }
    }

  /*
   * From this point, all IO functions as printf, puts, perror, etc. can't be
   * used anymore since we are a daemon.  The LogPrint facility should 
   * be used instead.
   */

  /* Initialise Braille display: */
  startbrl();

  /* Initialise speech */
  speech->initialize(speech_drvparm);

  /* Initialise help screen */
  if (inithlpscr (braille->helpfile))
    LogPrint(LOG_WARNING, "Cannot open help screen file '%s'.", braille->helpfile);

  if (!opt_q)
    message (VERSION, 0);	/* display initialisation message */
}


#define MIN(a, b)  (((a) < (b)) ? (a) : (b)) 
	
void 
configmenu (void)
{
  static unsigned char savecfg = 0;		/* 1 == save config on exit */
  char *booleanValues[] = {"No", "Yes"};
  char *cursorStyles[] = {"Underline", "Block"};
  char *skipBlankWindowsModes[] = {"All", "End of Line", "Rest of Line"};
  char *statusStyles[] = {"None", "Alva", "Tieman", "PowerBraille 80", "Papenmeier", "MDV"};
  char *textStyles[] = {"8 dot", "6 dot"};
  char *tuneDevices[] = {"PC Speaker", "Sound Card", "AdLib/OPL3/SB-FM"};
  typedef struct {
     unsigned char *setting;			/* pointer to the item value */
     char *description;			/* item description */
     char **names;			/* 0 == numeric, 1 == bolean */
     unsigned char minimum;			/* minimum range */
     unsigned char maximum;			/* maximum range */
  } MenuItem;
  #define MENU_ITEM(setting, description, values, minimum, maximum) {&setting, description, values, minimum, maximum}
  #define NUMERIC_ITEM(setting, description, minimum, maximum) MENU_ITEM(setting, description, NULL, minimum, maximum)
  #define TIMING_ITEM(setting, description) NUMERIC_ITEM(setting, description, 1, 16)
  #define SYMBOLIC_ITEM(setting, description, names) MENU_ITEM(setting, description, names, 0, ((sizeof(names) / sizeof(names[0])) - 1))
  #define BOOLEAN_ITEM(setting, description) SYMBOLIC_ITEM(setting, description, booleanValues)
  MenuItem menu[] = {
     BOOLEAN_ITEM(savecfg, "Save on Exit"),
     SYMBOLIC_ITEM(env.sixdots, "Text Style", textStyles),
     BOOLEAN_ITEM(env.skpidlns, "Skip Identical Lines"),
     BOOLEAN_ITEM(env.skpblnkwins, "Skip Blank Windows"),
     SYMBOLIC_ITEM(env.skpblnkwinsmode, "Which Blank Windows", skipBlankWindowsModes),
     BOOLEAN_ITEM(env.slidewin, "Sliding Window"),
     NUMERIC_ITEM(env.winovlp, "Window Overlap", 0, 20),
     BOOLEAN_ITEM(env.csrvis, "Show Cursor"),
     SYMBOLIC_ITEM(env.csrsize, "Cursor Style", cursorStyles),
     BOOLEAN_ITEM(env.csrblink, "Blinking Cursor"),
     TIMING_ITEM(env.csroncnt, "Cursor Visible Period"),
     TIMING_ITEM(env.csroffcnt, "Cursor Invisible Period"),
     BOOLEAN_ITEM(env.attrvis, "Show Attributes"),
     BOOLEAN_ITEM(env.attrblink, "Blinking Attributes"),
     TIMING_ITEM(env.attroncnt, "Attributes Visible Period"),
     TIMING_ITEM(env.attroffcnt, "Attributes Invisible Period"),
     BOOLEAN_ITEM(env.capblink, "Blinking Capitals"),
     TIMING_ITEM(env.caponcnt, "Capitals Visible Period"),
     TIMING_ITEM(env.capoffcnt, "Capitals Invisible Period"),
     BOOLEAN_ITEM(env.sound, "Sound"),
     SYMBOLIC_ITEM(env.tunedev, "Tune Device", tuneDevices),
     SYMBOLIC_ITEM(env.stcellstyle, "Status Cells Style", statusStyles)
  };
  int menuSize = sizeof(menu) / sizeof(menu[0]);
  static int menuIndex = 0;			/* current menu item */

  unsigned char line[40];		/* display buffer */
  int lineUpdated = 0;			/* 1 when item's value has changed */
  int lineLength;				/* current menu item length */
  int lineIndent = 0;				/* braille window pos in buffer */

  struct brltty_env oldEnvironment = env;	/* backup configuration */
  int key;				/* readbrl() value */

  /* status cells */
  memset(statcells, 0, sizeof(statcells));
  statcells[0] = texttrans['C'];
  statcells[1] = texttrans['n'];
  statcells[2] = texttrans['f'];
  statcells[3] = texttrans['i'];
  statcells[4] = texttrans['g'];
  braille->setstatus(statcells);
  message("Configuration Menu", 0);

  while (keep_going)
    {
      MenuItem *item = &menu[menuIndex];

      /* First we draw the current menu item in the buffer */
      sprintf(line, "%s: ", item->description);
      if (item->names)
         strcat(line, item->names[*item->setting - item->minimum]);
      else
         sprintf(line+strlen(line), "%d", *item->setting);
      lineLength = strlen(line);

      /* Next we deal with the braille window position in the buffer.
       * This is intended for small displays... or long item descriptions 
       */
      if (lineUpdated)
	{
	  lineUpdated = 0;
	  /* make sure the updated value is visible */
	  if (lineLength-lineIndent > brl.x*brl.y)
	    lineIndent = lineLength - brl.x*brl.y;
	}

      /* Then draw the braille window */
      memset(brl.disp, 0, brl.x*brl.y);
      {
	 int index;
	 for (index=0; index<MIN(brl.x*brl.y, lineLength-lineIndent); index++)
            brl.disp[index] = texttrans[line[lineIndent+index]];
      }
      braille->write(&brl);
      delay (DELAY_TIME);

      /* Now process any user interaction */
      while ((key = braille->read(CMDS_CONFIG)) != EOF)
	switch (key)
	  {
	  case CMD_NOOP:
	    continue;
	  case CMD_TOP:
	  case CMD_TOP_LEFT:
	    menuIndex = lineIndent = 0;
	    break;
	  case CMD_BOT:
	  case CMD_BOT_LEFT:
	    menuIndex = menuSize - 1;
	    lineIndent = 0;
	    break;
	  case CMD_LNUP:
	    if (--menuIndex < 0)
	      menuIndex = menuSize - 1;
	    lineIndent = 0;
	    break;
	  case CMD_LNDN:
	    if (++menuIndex >= menuSize)
	      menuIndex = 0;
	    lineIndent = 0;
	    break;
	  case CMD_FWINLT:
	    if (lineIndent > 0)
	      lineIndent -= MIN(brl.x*brl.y, lineIndent);
	    else
	      playTune(&tune_bounce);
	    break;
	  case CMD_FWINRT:
	    if (lineLength-lineIndent > brl.x*brl.y)
	      lineIndent += brl.x*brl.y;
	    else
	      playTune(&tune_bounce);
	    break;
	  case CMD_WINUP:
	  case CMD_CHRLT:
	  case CMD_KEY_LEFT:
	  case CMD_KEY_UP:
	    if (--*item->setting < item->minimum)
	      *item->setting = item->maximum;
	    lineUpdated = 1;
	    break;
	  case CMD_WINDN:
	  case CMD_CHRRT:
	  case CMD_KEY_RIGHT:
	  case CMD_KEY_DOWN:
	  case CMD_HOME:
	  case CMD_KEY_RETURN:
	    if (++*item->setting > item->maximum)
	      *item->setting = item->minimum;
	    lineUpdated = 1;
	    break;
	  case CMD_SAY:
	    speech->say(line, lineLength);
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
		"Press CONFIG again to quit.", MSG_WAITKEY |MSG_NODELAY);
	    break;
	  case CMD_RESET:
	    env = oldEnvironment;
	    message("Changes Discarded", 0);
	    break;
	  case CMD_SAVECONF:
	    savecfg |= 1;
	    /*break;*/
	  default:
	    if (key >= CR_ROUTEOFFSET && key < CR_ROUTEOFFSET+brl.x) {
               /* Why not setting a value with routing keys... */
	       key -= CR_ROUTEOFFSET;
	       if (item->names) {
	          *item->setting = key % (item->maximum + 1);
	       } else {
	          *item->setting = key;
		  if (*item->setting > item->maximum)
		     *item->setting = item->maximum;
		  if (*item->setting < item->minimum)
        	     *item->setting = item->minimum;
	       }
	       lineUpdated = 1;
               break;
	    }

	    /* For any other keystroke, we exit */
	    if (savecfg)
	      {
		saveconfig();
		playTune(&tune_done);
	      }
	    return;
	  }
    }
}


