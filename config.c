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
-a file    --attributes-table=   Path to attributes translation table file.\n\
-b driver  --braille-driver=     Braille driver: full library path, or one of\n\
                                 {" BRLLIBS "}\n\
-B arg,... --braille-parameters= Parameters to braille driver.\n\
-d device  --braille-device=     Path to device for accessing braille display.\n\
-e         --errors              Log to standard error instead of via syslog.\n\
-f file    --configuration-file= Path to default parameters file.\n\
-h         --help                Print this usage summary and exit.\n\
-l level   --log-level=          Diagnostic logging level: 0-7 [5], or one of\n\
				 {emergency alert critical error warning\n\
				 [notice] information debug}\n\
-n         --no-daemon           Remain a foreground process.\n\
-p file    --preferences-file=   Path to preferences file.\n\
-q         --quiet               Suppress start-up messages.\n\
-s driver  --speech-driver=      Speech driver: full library path, or one of\n\
                                 {" SPKLIBS "}\n\
-S arg,... --speech-parameters=  Parameters to speech driver.\n\
-t file    --text-table=         Path to text translation table file.\n\
-v         --version             Print start-up messages and exit.\n";

static char *opt_attributesTable = NULL;
static char *opt_brailleDevice = NULL;
static char *opt_brailleParameters = NULL;
static char *opt_configurationFile = NULL;
static short opt_errors = 0;
static short opt_help = 0;
static short opt_logLevel = LOG_NOTICE;
static short opt_noDaemon = 0;
static char *opt_preferencesFile = NULL;
static short opt_quiet = 0;
static char *opt_speechParameters = NULL;
static char *opt_textTable = NULL;
static short opt_version = 0;

static char *cfg_preferencesFile = NULL;
static char *cfg_textTable = NULL;
static char *cfg_attributesTable = NULL;
static char *cfg_brailleDevice = NULL;
static char *cfg_brailleDriver = NULL;
static char *cfg_brailleParameters = NULL;
static char *cfg_speechDriver = NULL;
static char *cfg_speechParameters = NULL;

// Define error codes for configuration file processing.
typedef enum {
   CFG_OK,		// No error.
   CFG_NoValue,		// Operand not specified.
   CFG_BadValue,	// Bad operand specified.
   CFG_TooMany,		// Too many operands.
   CFG_Duplicate	// Directive specified more than once.
} ConfigurationFileError;

static char **brailleParameters = NULL;
static char **speechParameters = NULL;

short homedir_found = 0;	/* CWD status */

static void
extendParameters (char **parameters, char *operand) {
   if (*parameters) {
      size_t length = strlen(*parameters);
      *parameters = reallocWrapper(*parameters, length+1+strlen(operand)+1);
      sprintf((*parameters)+length, ",%s", operand);
   } else {
      *parameters = strdupWrapper(operand);
   }
}

static void
parseParameters (char ***values, char **names, char *parameters, char *description) {
   if (!names) {
      static char *noNames[] = {NULL};
      names = noNames;
   }
   if (!*values) {
      unsigned int count = 0;
      while (names[count]) ++count;
      *values = mallocWrapper((count + 1) * sizeof(**values));
      (*values)[count] = NULL;
      while (count--) (*values)[count] = strdupWrapper("");
   }
   if (parameters && *parameters) {
      char *name = (parameters = strdupWrapper(parameters));
      while (1) {
	 char *delimiter = strchr(name, ',');
	 int done = delimiter == NULL;
	 if (!done) *delimiter = 0;
	 if (*name) {
	    char *value = strchr(name, '=');
	    if (!value) {
	       LogPrint(LOG_WARNING, "Missing %s parameter value: %s", description, name);
	    } else if (value == name) {
	       LogPrint(LOG_WARNING, "Missing %s parameter name: %s", description, name);
	    } else {
	       unsigned int length = value - name;
	       unsigned int index = 0;
	       *value++ = 0;
	       while (names[index]) {
		  if (length <= strlen(names[index])) {
		     if (strncasecmp(name, names[index], length) == 0) {
			free((*values)[index]);
			(*values)[index] = strdupWrapper(value);
			break;
		     }
		  }
		  ++index;
	       }
	       if (!names[index]) {
		  LogPrint(LOG_WARNING, "Unsupported %s parameter: %s", description, name);
	       }
	    }
	 }
	 if (done) break;
	 name = delimiter + 1;
      }
      free(parameters);
   }
}

static void
parseBrailleParameters (char *parameters) {
   parseParameters(&brailleParameters, braille->parameters, parameters, "braille driver");
}

static void
parseSpeechParameters (char *parameters) {
   parseParameters(&speechParameters, speech->parameters, parameters, "speech driver");
}

static void
logParameters (char **names, char **values, char *description) {
   if (names && values) {
      while (*names) {
         LogPrint(LOG_INFO, "%s Parameter: %s=%s", description, *names, *values);
	 ++names;
	 ++values;
      }
   }
}

static void processOptions (int argc, char **argv)
{
  int option;

  const char *short_options = "+a:b:d:ef:hl:np:qs:t:vB:C:M:R:S:";
  #ifdef no_argument
    const struct option long_options[] =
      {
        {"attributes-table"  , required_argument, NULL, 'a'},
        {"braille-driver"    , required_argument, NULL, 'b'},
        {"braille-device"    , required_argument, NULL, 'd'},
        {"errors"            , no_argument      , NULL, 'e'},
        {"configuration-file", required_argument, NULL, 'f'},
        {"help"              , no_argument      , NULL, 'h'},
        {"log-level"         , required_argument, NULL, 'l'},
        {"no-daemon"         , no_argument      , NULL, 'n'},
        {"preferences-file"  , required_argument, NULL, 'p'},
        {"quiet"             , no_argument      , NULL, 'q'},
        {"speech-driver"     , required_argument, NULL, 's'},
        {"text-table"        , required_argument, NULL, 't'},
        {"version"           , no_argument      , NULL, 'v'},
        {"braille-parameters", required_argument, NULL, 'B'},
        {"cycle-delay"       , required_argument, NULL, 'C'},
        {"message-delay"     , required_argument, NULL, 'M'},
        {"read-delay"        , required_argument, NULL, 'R'},
        {"speech-parameters" , required_argument, NULL, 'S'},
        {NULL                , 0                , NULL, ' '}
      };
    #define get_option() getopt_long(argc, argv, short_options, long_options, NULL)
  #else
    #define get_option() getopt(argc, argv, short_options)
  #endif

  /* Parse command line using getopt(): */
  opterr = 0;
  while ((option = get_option()) != -1) {
    /* continue on error as much as possible, as often we are typing blind
       and won't even see the error message unless the display come up. */
    switch (option) {
      default:
	LogPrint(LOG_WARNING, "Unimplemented invocation option: -%c", option);
	break;
      case '?': // An invalid option has been specified.
	LogPrint(LOG_WARNING, "Unknown invocation option: -%c", optopt);
	return; /* not fatal */
      case 'a':		/* text translation table file name */
	opt_attributesTable = optarg;
	break;
      case 'b':			/* name of driver */
	braille_libraryName = optarg;
	break;
      case 'd':		/* serial device path */
	opt_brailleDevice = optarg;
	break;
      case 'e':		/* help */
	opt_errors = 1;
	break;
      case 'f':		/* configuration file path */
	opt_configurationFile = optarg;
	break;
      case 'h':		/* help */
	opt_help = 1;
	break;
      case 'l':	{  /* log level */
        if (*optarg) {
	  static char *valueTable[] = {
	    "emergency", "alert", "critical", "error",
	    "warning", "notice", "information", "debug"
	  };
	  static unsigned int valueCount = sizeof(valueTable) / sizeof(valueTable[0]);
	  unsigned int valueLength = strlen(optarg);
	  int value;
	  for (value=0; value<valueCount; ++value) {
	    char *word = valueTable[value];
	    unsigned int wordLength = strlen(word);
	    if (valueLength <= wordLength) {
	      if (strncasecmp(optarg, word, valueLength) == 0) {
		break;
	      }
	    }
	  }
	  if (value < valueCount) {
	    opt_logLevel = value;
	    break;
	  }
	  {
	    char *endptr;
	    value = strtol(optarg, &endptr, 0);
	    if (!*endptr && value>=0 && value<valueCount) {
	      opt_logLevel = value;
	      break;
	    }
	  }
	}
	LogPrint(LOG_WARNING, "Invalid log priority: %s", optarg);
	break;
      }
      case 'n':		/* don't go into the background */
	opt_noDaemon = 1;
	break;
      case 'p':		/* preferences file path */
	opt_preferencesFile = optarg;
	break;
      case 'q':		/* quiet */
	opt_quiet = 1;
	break;
      case 's':			/* name of speech driver */
	speech_libraryName = optarg;
	break;
      case 't':		/* text translation table file name */
	opt_textTable = optarg;
	break;
      case 'v':		/* version */
	opt_version = 1;
	break;
      case 'B':			/* parameter to speech driver */
	extendParameters(&opt_brailleParameters, optarg);
	break;
      case 'C': {	/* cycle delay */
	int value;
	int minimum = 1;
	if (validateInteger(&value, "cycle delay", optarg, &minimum, NULL))
	  cycleDelay = value * 10;
	break;
      }
      case 'M': {	/* message delay */
	int value;
	int minimum = 1;
	if (validateInteger(&value, "message delay", optarg, &minimum, NULL))
	  messageDelay = value * 10;
	break;
      }
      case 'R': {	/* read delay */
	int value;
	int minimum = 1;
	if (validateInteger(&value, "read delay", optarg, &minimum, NULL))
	  readDelay = value * 10;
	break;
      }
      case 'S':			/* parameter to speech driver */
	extendParameters(&opt_speechParameters, optarg);
	break;
    }
  }
  #undef get_option

  if (opt_help)
    {
      printf(USAGE, argv[0]);
      exit(0);
    }
}

static int getToken (char **val, const char *delims)
{
  char *v = strtok(NULL, delims);

  if (!v) return CFG_NoValue;
  if (strtok(NULL, delims)) return CFG_TooMany;
  if (*val) return CFG_Duplicate;

  *val = strdupWrapper(v);
  return CFG_OK;
}

static int setPreferencesFile (const char *delims)
{
  return getToken(&cfg_preferencesFile, delims);
}

static int setTextTable (const char *delims)
{
  return getToken(&cfg_textTable, delims);
}

static int setAttributesTable (const char *delims)
{
  return getToken(&cfg_attributesTable, delims);
}

static int setBrailleDevice (const char *delims)
{
  return getToken(&cfg_brailleDevice, delims);
}

static int setBrailleDriver (const char *delims)
{
  return getToken(&cfg_brailleDriver, delims);
}

static int setBrailleParameters (const char *delims)
{
  return getToken(&cfg_brailleParameters, delims);
}

static int setSpeechDriver (const char *delims)
{
  return getToken(&cfg_speechDriver, delims);
}

static int setSpeechParameters (const char *delims)
{
  return getToken(&cfg_speechParameters, delims);
}

static void processConfigurationLine (char *line, void *data)
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
      {"preferences-file",      setPreferencesFile},
      {"text-table",            setTextTable},
      {"attributes-table",      setAttributesTable},
      {"braille-device",        setBrailleDevice},
      {"braille-driver",        setBrailleDriver},
      {"braille-parameters",    setBrailleParameters},
      {"speech-driver",         setSpeechDriver},
      {"speech-parameters",     setSpeechParameters},
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
		  case CFG_OK:
		    break;

		  case CFG_NoValue:
		    LogPrint(LOG_WARNING,
		             "Operand not supplied for configuration item '%s'.",
			     keyword);
		    break;

		  case CFG_BadValue:
		    LogPrint(LOG_WARNING,
		             "Invalid operand specified"
			     " for configuration item '%s'.",
			      keyword);
		    break;

		  case CFG_TooMany:
		    LogPrint(LOG_WARNING,
		             "Too many operands supplied"
			     " for configuration item '%s'.",
			     keyword);
		    break;

		  case CFG_Duplicate:
		    LogPrint(LOG_WARNING,
		             "Configuration item '%s' specified more than once.",
			     keyword);
		    break;

		  default:
		    LogPrint(LOG_WARNING,
		             "Internal error: unsupported"
			     " configuration file error code: %d",
			     code);
		    break;
		}
	      return;
	    }
	  ++kw;
	}
      LogPrint(LOG_WARNING, "Unknown configuration item: '%s'.", keyword);
    }
}

static int processConfigurationFile (char *path, int optional)
{
   FILE *file = fopen(path, "r");
   if (file != NULL)
     { // The configuration file has been successfully opened.
       processLines(file, processConfigurationLine, NULL);
       fclose(file);
     }
   else
     {
       LogPrint((optional && (errno == ENOENT)? LOG_DEBUG: LOG_WARNING),
                "Cannot open configuration file: %s: %s",
                path, strerror(errno));
       return 0;
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
	INIT_CSRTRK, INIT_CSRHIDE, TBL_TEXT, 0, 0, 0, 0, 0, 0
};

#define MIN(a, b)  (((a) < (b))? (a): (b)) 
#define MAX(a, b)  (((a) > (b))? (a): (b)) 
	
static void
changedWindowAttributes (void)
{
  fwinshift = MAX(brl.x-env.winovlp, 1);
  hwinshift = brl.x / 2;
  csr_offright = brl.x / 4;
  vwinshift = 5;
}

static void
changedTuneDevice (void)
{
  setTuneDevice (env.tunedev);
}

void
startBrailleDriver (void)
{
  braille->initialize(brailleParameters, &brl, opt_brailleDevice);
  if (brl.x == -1)
    {
      LogPrint(LOG_CRIT, "Braille driver initialization failed.");
      closescr();
      LogClose();
      exit(5);
    }
#ifdef ALLOW_OFFRIGHT_POSITIONS
  /* The braille display is allowed to stick out the right side of the
     screen by brl.x-offr. This allows the feature: */
  offr = 1;
#else
  /* This disallows it, as it was before. */
  offr = brl.x;
#endif
  changedWindowAttributes();
  LogPrint(LOG_DEBUG, "Braille display has %d rows of %d cells.",
	   brl.y, brl.x);
  playTune(&tune_detected);
  clrbrlstat ();
}

void
startSpeechDriver (void)
{
  speech->initialize(speechParameters);
}

int
readKey (DriverCommandContext cmds)
{
   while (1) {
      int key = braille->read(cmds);
      if (key == CMD_NOOP) continue;
      if (key != EOF) return key;
      delay(readDelay);
   }
}

static void
loadTranslationTable (char *table, char **path, char *name)
{
  if (*path) {
    int fd = open(*path, O_RDONLY);
    if (fd >= 0) {
      char buffer[0X100];
      if (read(fd, buffer, sizeof(buffer)) == sizeof(buffer)) {
	memcpy(table, buffer, sizeof(buffer));
      } else {
	LogPrint(LOG_WARNING, "Cannot read %s translation table: %s", name, *path);
	*path = NULL;
      }
      close(fd);
    } else {
      LogPrint(LOG_WARNING, "Cannot open %s translation table: %s", name, *path);
      *path = NULL;
    }
  }
}

int
loadPreferences (void)
{
  int ok = 0;
  int fd = open(opt_preferencesFile, O_RDONLY);
  if (fd != -1) {
    struct brltty_env newenv;
    if (read(fd, &newenv, sizeof(newenv)) == sizeof(newenv)) {
      if ((newenv.magicnum[0] == (ENV_MAGICNUM&0XFF)) && (newenv.magicnum[1] == (ENV_MAGICNUM>>8))) {
	env = newenv;
	ok = 1;
      } else
	LogPrint(LOG_WARNING, "Invalid preferences file: %s", opt_preferencesFile);
    } else
      LogPrint(LOG_WARNING, "Cannot read preferences file: %s: %s",
	       opt_preferencesFile, strerror(errno));
    close(fd);
  } else
    LogPrint((errno==ENOENT? LOG_DEBUG: LOG_WARNING),
             "Cannot open preferences file: %s: %s",
             opt_preferencesFile, strerror(errno));
  setTuneDevice (env.tunedev);
  return ok;
}

int 
savePreferences (void)
{
  int ok = 0;
  int fd = open(opt_preferencesFile, O_WRONLY | O_CREAT | O_TRUNC);
  if (fd != -1) {
    fchmod(fd, S_IRUSR | S_IWUSR);
    if (write(fd, &env, sizeof(env)) == sizeof(env)) {
      ok = 1;
    } else {
      LogPrint(LOG_WARNING, "Cannot write to preferences file: %s: %s",
	       opt_preferencesFile, strerror(errno));
    }
    close(fd);
  } else {
    LogPrint(LOG_WARNING, "Cannot open preferences file: %s: %s",
             opt_preferencesFile, strerror(errno));
  }
  if (!ok)
    message("not saved", 0);
  return ok;
}

static int
testSkipBlankWindows () {
   return env.skpblnkwins;
}

static int
testShowCursor () {
   return env.csrvis;
}

static int
testBlinkingCursor () {
   return testShowCursor() && env.csrblink;
}

static int
testShowAttributes () {
   return env.attrvis;
}

static int
testBlinkingAttributes () {
   return testShowAttributes() && env.attrblink;
}

static int
testBlinkingCapitals () {
   return env.capblink;
}

static int
testSound () {
   return env.sound;
}

static int
testSoundMidi () {
   return testSound() && (env.tunedev == tdSequencer);
}

void
updatePreferences (void)
{
  static unsigned char exitSave = 0;		/* 1 == save preferences on exit */
  static char *booleanValues[] = {"No", "Yes"};
  static char *cursorStyles[] = {"Underline", "Block"};
  static char *skipBlankWindowsModes[] = {"All", "End of Line", "Rest of Line"};
  static char *statusStyles[] = {"None", "Alva", "Tieman", "PowerBraille 80", "Papenmeier", "MDV"};
  static char *textStyles[] = {"8 dot", "6 dot"};
  static char *tuneDevices[] = {"PC Speaker", "Sound Card", "MIDI", "AdLib/OPL3/SB-FM"};
  typedef struct {
     unsigned char *setting;			/* pointer to the item value */
     void (*changed) (void);
     int (*test) (void);
     char *description;			/* item description */
     char **names;			/* 0 == numeric, 1 == bolean */
     unsigned char minimum;			/* minimum range */
     unsigned char maximum;			/* maximum range */
  } MenuItem;
  #define MENU_ITEM(setting, changed, test, description, values, minimum, maximum) {&setting, changed, test, description, values, minimum, maximum}
  #define NUMERIC_ITEM(setting, changed, test, description, minimum, maximum) MENU_ITEM(setting, changed, test, description, NULL, minimum, maximum)
  #define TIMING_ITEM(setting, changed, test, description) NUMERIC_ITEM(setting, changed, test, description, 1, 16)
  #define SYMBOLIC_ITEM(setting, changed, test, description, names) MENU_ITEM(setting, changed, test, description, names, 0, ((sizeof(names) / sizeof(names[0])) - 1))
  #define BOOLEAN_ITEM(setting, changed, test, description) SYMBOLIC_ITEM(setting, changed, test, description, booleanValues)
  MenuItem menu[] = {
     BOOLEAN_ITEM(exitSave, NULL, NULL, "Save on Exit"),
     SYMBOLIC_ITEM(env.sixdots, NULL, NULL, "Text Style", textStyles),
     BOOLEAN_ITEM(env.skpidlns, NULL, NULL, "Skip Identical Lines"),
     BOOLEAN_ITEM(env.skpblnkwins, NULL, NULL, "Skip Blank Windows"),
     SYMBOLIC_ITEM(env.skpblnkwinsmode, NULL, testSkipBlankWindows, "Which Blank Windows", skipBlankWindowsModes),
     BOOLEAN_ITEM(env.slidewin, NULL, NULL, "Sliding Window"),
     NUMERIC_ITEM(env.winovlp, changedWindowAttributes, NULL, "Window Overlap", 0, 20),
     BOOLEAN_ITEM(env.csrvis, NULL, NULL, "Show Cursor"),
     SYMBOLIC_ITEM(env.csrsize, NULL, testShowCursor, "Cursor Style", cursorStyles),
     BOOLEAN_ITEM(env.csrblink, NULL, testShowCursor, "Blinking Cursor"),
     TIMING_ITEM(env.csroncnt, NULL, testBlinkingCursor, "Cursor Visible Period"),
     TIMING_ITEM(env.csroffcnt, NULL, testBlinkingCursor, "Cursor Invisible Period"),
     BOOLEAN_ITEM(env.attrvis, NULL, NULL, "Show Attributes"),
     BOOLEAN_ITEM(env.attrblink, NULL, testShowAttributes, "Blinking Attributes"),
     TIMING_ITEM(env.attroncnt, NULL, testBlinkingAttributes, "Attributes Visible Period"),
     TIMING_ITEM(env.attroffcnt, NULL, testBlinkingAttributes, "Attributes Invisible Period"),
     BOOLEAN_ITEM(env.capblink, NULL, NULL, "Blinking Capitals"),
     TIMING_ITEM(env.caponcnt, NULL, testBlinkingCapitals, "Capitals Visible Period"),
     TIMING_ITEM(env.capoffcnt, NULL, testBlinkingCapitals, "Capitals Invisible Period"),
     BOOLEAN_ITEM(env.sound, NULL, NULL, "Sound"),
     SYMBOLIC_ITEM(env.tunedev, changedTuneDevice, testSound, "Tune Device", tuneDevices),
     MENU_ITEM(env.midiinstr, NULL, testSoundMidi, "MIDI Instrument", midiInstrumentTable, 0, midiInstrumentCount-1),
     SYMBOLIC_ITEM(env.stcellstyle, NULL, NULL, "Status Cells Style", statusStyles)
  };
  int menuSize = sizeof(menu) / sizeof(menu[0]);
  static int menuIndex = 0;			/* current menu item */

  unsigned char line[0X40];		/* display buffer */
  int lineIndent = 0;				/* braille window pos in buffer */
  int settingChanged = 0;			/* 1 when item's value has changed */

  struct brltty_env oldEnvironment = env;	/* backup preferences */
  int key;				/* readbrl() value */

  /* status cells */
  memset(statcells, 0, sizeof(statcells));
  statcells[0] = texttrans['C'];
  statcells[1] = texttrans['n'];
  statcells[2] = texttrans['f'];
  statcells[3] = texttrans['i'];
  statcells[4] = texttrans['g'];
  braille->setstatus(statcells);
  message("Preferences Menu", 0);

  while (1)
    {
      int lineLength;				/* current menu item length */
      int settingIndent;				/* braille window pos in buffer */
      MenuItem *item = &menu[menuIndex];

      closeTuneDevice();

      /* First we draw the current menu item in the buffer */
      sprintf(line, "%s: ", item->description);
      settingIndent = strlen(line);
      if (item->names)
         strcat(line, item->names[*item->setting - item->minimum]);
      else
         sprintf(line+settingIndent, "%d", *item->setting);
      lineLength = strlen(line);

      /* Next we deal with the braille window position in the buffer.
       * This is intended for small displays... or long item descriptions 
       */
      if (settingChanged)
	{
	  settingChanged = 0;
	  /* make sure the updated value is visible */
	  if (lineLength-lineIndent > brl.x*brl.y)
	    lineIndent = settingIndent;
	}

      /* Then draw the braille window */
      memset(brl.disp, 0, brl.x*brl.y);
      {
	 int index;
	 for (index=0; index<MIN(brl.x*brl.y, lineLength-lineIndent); index++)
            brl.disp[index] = texttrans[line[lineIndent+index]];
      }
      braille->write(&brl);
      delay(cycleDelay);

      /* Now process any user interaction */
      switch (key = readKey(CMDS_PREFS)) {
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
	  do {
	    if (menuIndex == 0)
	      menuIndex = menuSize;
	    --menuIndex;
	  } while (menu[menuIndex].test && !menu[menuIndex].test());
	  lineIndent = 0;
	  break;
	case CMD_LNDN:
	  do {
	    if (++menuIndex == menuSize)
	      menuIndex = 0;
	  } while (menu[menuIndex].test && !menu[menuIndex].test());
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
	  if ((*item->setting)-- <= item->minimum)
	    *item->setting = item->maximum;
	  settingChanged = 1;
	  break;
	case CMD_WINDN:
	case CMD_CHRRT:
	case CMD_KEY_RIGHT:
	case CMD_KEY_DOWN:
	case CMD_HOME:
	case CMD_KEY_RETURN:
	  if ((*item->setting)++ >= item->maximum)
	    *item->setting = item->minimum;
	  settingChanged = 1;
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
	      "Press PREFS again to quit.", MSG_WAITKEY |MSG_NODELAY);
	  break;
	case CMD_PREFLOAD: {
	  int index;
	  env = oldEnvironment;
	  for (index=0; index<menuSize; ++index) {
	    void (*changed) (void) = menu[index].changed;
	    if (changed) changed();
	  }
	  message("changes discarded", 0);
	  break;
	}
	case CMD_PREFSAVE:
	  exitSave = 1;
	  goto exitMenu;
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
	     settingChanged = 1;
	     break;
	  }

	  /* For any other keystroke, we exit */
	exitMenu:
	  if (exitSave)
	    {
	      if (savePreferences()) {
		playTune(&tune_done);
	      }
	    }
	  return;
      }

      if (settingChanged)
	if (item->changed)
	  item->changed();
    }
}

void startup(int argc, char *argv[])
{
  processOptions(argc, argv);

  /* Set logging priority levels. */
  if (opt_errors)
    LogClose();
  SetLogPriority(opt_logLevel);
  SetStderrPriority(opt_version?
                       (opt_quiet? LOG_NOTICE: LOG_INFO):
                       (opt_quiet? LOG_WARNING: LOG_NOTICE));

  LogPrint(LOG_NOTICE, "%s", VERSION);
  LogPrint(LOG_INFO, "%s", COPYRIGHT);

  /* Process the configuration file. */
  if (opt_configurationFile)
    processConfigurationFile(opt_configurationFile, 0);
  else
    processConfigurationFile(CONFIG_FILE, 1);
  if (!opt_preferencesFile) opt_preferencesFile = cfg_preferencesFile;
  if (!opt_textTable) opt_textTable = cfg_textTable;
  if (!opt_attributesTable) opt_attributesTable = cfg_attributesTable;
  if (!opt_brailleDevice) opt_brailleDevice = cfg_brailleDevice;
  if (!braille_libraryName) braille_libraryName = cfg_brailleDriver;
  if (!speech_libraryName) speech_libraryName = cfg_speechDriver;

  if (opt_brailleDevice == NULL)
    opt_brailleDevice = BRLDEV;
  if (*opt_brailleDevice == 0)
    {
      LogPrint(LOG_CRIT, "No braille device specified.");
      fprintf(stderr, "Use -d to specify one.\n");
      /* this is fatal */
      exit(10);
    }

  if (!load_braille_driver())
    {
      LogPrint(LOG_CRIT, "%s braille driver selection.",
               braille_libraryName? "Bad": "No");
      fprintf(stderr, "\n");
      list_braille_drivers();
      fprintf(stderr, "\nUse -b to specify one, and -h for quick help.\n\n");
      /* this is fatal */
      exit(10);
    }
  parseBrailleParameters(cfg_brailleParameters);
  parseBrailleParameters(opt_brailleParameters);

  if (!load_speech_driver())
    {
      LogPrint(LOG_ERR, "%s speech driver selection.",
               speech_libraryName? "Bad": "No");
      fprintf(stderr, "\n");
      list_speech_drivers();
      fprintf(stderr, "\nUse -s to specify one, and -h for quick help.\n\n");
      LogPrint(LOG_WARNING, "Falling back to built-in speech driver.");
      /* not fatal */
    }
  parseSpeechParameters(cfg_speechParameters);
  parseSpeechParameters(opt_speechParameters);

  if (!opt_preferencesFile)
    {
      char *part1 = "brltty-";
      char *part2 = braille->identifier;
      char *part3 = ".prefs";
      opt_preferencesFile = mallocWrapper(strlen(part1) + strlen(part2) + strlen(part3) + 1);
      sprintf(opt_preferencesFile, "%s%s%s", part1, part2, part3);
    }

  /* copy default mode for status display */
  initenv.stcellstyle = braille->status_style;

  if (chdir (HOME_DIR))		/* * change to directory containing data files  */
    {
      char *backup_dir = "/etc";
      LogPrint(LOG_WARNING, "Cannot change directory to '%s': %s",
               HOME_DIR, strerror(errno));
      LogPrint(LOG_WARNING, "Using backup directory '%s' instead.",
               backup_dir);
      chdir (backup_dir);		/* home directory not found, use backup */
    }

  /*
   * Load translation tables: 
   */
  loadTranslationTable(attribtrans, &opt_attributesTable, "attributes");
  loadTranslationTable(texttrans, &opt_textTable, "text");
  reverseTable(texttrans, untexttrans);

  {
    char buffer[0X100];
    char *path = getcwd(buffer, sizeof(buffer));
    LogPrint(LOG_INFO, "Working Directory: %s",
	     path? path: "path-too-long");
  }

  LogPrint(LOG_INFO, "Preferences File: %s", opt_preferencesFile);
  LogPrint(LOG_INFO, "Help File: %s", braille->help_file);
  LogPrint(LOG_INFO, "Text Table: %s",
           opt_textTable? opt_textTable: "built-in");
  LogPrint(LOG_INFO, "Attributes Table: %s",
           opt_attributesTable? opt_attributesTable: "built-in");
  LogPrint(LOG_INFO, "Braille Device: %s", opt_brailleDevice);
  LogPrint(LOG_INFO, "Braille Driver: %s (%s)",
           braille_libraryName, braille->name);
  logParameters(braille->parameters, brailleParameters, "Braille");
  LogPrint(LOG_INFO, "Speech Driver: %s (%s)",
           speech_libraryName, speech->name);
  logParameters(speech->parameters, speechParameters, "Speech");

  /*
   * Give braille and speech libraries a chance to introduce themselves.
   */
  braille->identify();
  speech->identify();

  if (opt_version)
    exit(0);

  /* Load preferences file */
  if (!loadPreferences())
    env = initenv;

  /*
   * Initialize screen library 
   */
  if (initscr())
    {				
      LogPrint(LOG_CRIT, "Cannot read screen.");
      LogClose();
      exit(2);
    }
  
  if (!opt_noDaemon)
    {
      /*
       * Become a daemon:
       */
      LogPrint(LOG_DEBUG, "Becoming daemon.");
      switch (fork())
        {
        case -1:			/* can't fork */
          LogPrint(LOG_CRIT, "fork: %s", strerror(errno));
          closescr();
          LogClose();
          exit(3);
        case 0: /* child, process becomes a daemon */
	  {
	    char *nullDevice = "/dev/null";
	    SetStderrOff();
	    freopen(nullDevice, "r", stdin);
	    freopen(nullDevice, "a", stdout);
	    if (opt_errors)
	      fflush(stderr);
	    else
	      freopen(nullDevice, "a", stderr);
	  }

	  /* request a new session (job control) */
	  if (setsid() == -1) {			
	    closescr();
	    LogPrint(LOG_CRIT, "setsid: %s", strerror(errno));
	    LogClose();
	    exit(4);
	  }

          /* I read somewhere that the correct thing to do here is to fork
	     again, so we are not a group leader and then can never be associated
	     a controlling tty. Well... No harm I suppose. */
          switch (fork()) {
	    case -1:
	      LogPrint(LOG_CRIT, "second fork: %s", strerror(errno));
	      closescr();
	      LogClose();
	      exit(3);
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
  if (brailleParameters)
    startBrailleDriver();
  else
    LogPrint(LOG_ERR, "Braille driver not started.");

  /* Initialise speech */
  if (speechParameters)
    startSpeechDriver();
  else
    LogPrint(LOG_ERR, "Speech driver not started.");

  /* Initialise help screen */
  if (inithlpscr (braille->help_file))
    LogPrint(LOG_WARNING, "Cannot open help screen file '%s'.", braille->help_file);

  if (!opt_quiet)
    message(VERSION, 0);	/* display initialisation message */
  if (ProblemCount) {
    char buffer[0X40];
    snprintf(buffer, sizeof(buffer), "%d startup problem%s",
             ProblemCount, (ProblemCount==1? "": "s"));
    message(buffer, MSG_WAITKEY);
  }
}


