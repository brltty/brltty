/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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
#include <glob.h>

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
#include "contract.h"
#include "tunes.h"
#include "message.h"
#include "misc.h"
#include "common.h"

char VERSION[] = "BRLTTY 3.0";
char COPYRIGHT[] = "Copyright (C) 1995-2002 by The BRLTTY Team - all rights reserved.";

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

static const char *opt_attributesTable = NULL;
static const char *opt_brailleDevice = NULL;
static const char *opt_brailleDriver = NULL;
static char *opt_brailleParameters = NULL;
static const char *opt_configurationFile = NULL;
static const char *opt_contractionTable = NULL;
static short opt_environmentVariables = 0;
static short opt_help = 0;
static short opt_helpAll = 0;
static short opt_logLevel = LOG_NOTICE;
static short opt_noDaemon = 0;
static short opt_noSpeech = 0;
static const char *opt_pidFile = NULL;
static const char *opt_preferencesFile = NULL;
static short opt_quiet = 0;
static char *opt_screenParameters = NULL;
static const char *opt_speechDriver = NULL;
static char *opt_speechParameters = NULL;
static short opt_standardError = 0;
static const char *opt_textTable = NULL;
static short opt_version = 0;

static char *cfg_preferencesFile = NULL;
static char *cfg_textTable = NULL;
static char *cfg_attributesTable = NULL;
static char *cfg_contractionTable = NULL;
static char *cfg_brailleDevice = NULL;
static char *cfg_brailleDriver = NULL;
static char *cfg_brailleParameters = NULL;
static char *cfg_speechDriver = NULL;
static char *cfg_speechParameters = NULL;
static char *cfg_screenParameters = NULL;

typedef struct {
  const char *pattern;
  const char *initial;
  char *current;
  unsigned none:1;
  glob_t glob;
  const char **paths;
  int count;
  unsigned char setting;
  const char *pathsArea[3];
} GlobData;
static GlobData glob_textTable;
static GlobData glob_attributesTable;
static GlobData glob_contractionTable;

// Define error codes for configuration file processing.
typedef enum {
   CFG_OK,                // No error.
   CFG_NoValue,                // Operand not specified.
   CFG_BadValue,        // Bad operand specified.
   CFG_TooMany,                // Too many operands.
   CFG_Duplicate        // Directive specified more than once.
} ConfigurationFileError;

static const BrailleDriver *brailleDriver;
static char **brailleParameters = NULL;

static const SpeechDriver *speechDriver;
static char **speechParameters = NULL;

static char **screenParameters = NULL;

short homedir_found = 0;        /* CWD status */

static int
getToken (char **val, const char *delimiters) {
  char *v = strtok(NULL, delimiters);

  if (!v) return CFG_NoValue;
  if (strtok(NULL, delimiters)) return CFG_TooMany;
  if (*val) return CFG_Duplicate;

  *val = strdupWrapper(v);
  return CFG_OK;
}

static int
configurePreferencesFile (const char *delimiters) {
  return getToken(&cfg_preferencesFile, delimiters);
}

static int
configureTextTable (const char *delimiters) {
  return getToken(&cfg_textTable, delimiters);
}

static int
configureAttributesTable (const char *delimiters) {
  return getToken(&cfg_attributesTable, delimiters);
}

static int
configureContractionTable (const char *delimiters) {
  return getToken(&cfg_contractionTable, delimiters);
}

static int
configureBrailleDevice (const char *delimiters) {
  return getToken(&cfg_brailleDevice, delimiters);
}

static int
configureBrailleDriver (const char *delimiters) {
  return getToken(&cfg_brailleDriver, delimiters);
}

static int
configureBrailleParameters (const char *delimiters) {
  return getToken(&cfg_brailleParameters, delimiters);
}

static int
configureSpeechDriver (const char *delimiters) {
  return getToken(&cfg_speechDriver, delimiters);
}

static int
configureSpeechParameters (const char *delimiters) {
  return getToken(&cfg_speechParameters, delimiters);
}

static int
configureScreenParameters (const char *delimiters) {
  return getToken(&cfg_screenParameters, delimiters);
}

typedef struct {
   char letter;
   const char *word;
   const char *argument;
   int (*configure) (const char *delimiters); 
   int flags;
   const char *description;
} OptionEntry;
#define OPT_Hidden 0X1
static OptionEntry optionTable[] = {
   {'a', "attributes-table", "file", configureAttributesTable, 0,
    "Path to attributes translation table file."},
   {'b', "braille-driver", "driver", configureBrailleDriver, 0,
    "Braille driver: full library path, or one of {" BRLLIBS "}"},
   {'c', "contraction-table", "file", configureContractionTable, 0,
    "Path to contraction table file."},
   {'d', "braille-device", "device", configureBrailleDevice, 0,
    "Path to device for accessing braille display."},
   {'e', "standard-error", NULL, NULL, 0,
    "Log to standard error instead of via syslog."},
   {'f', "configuration-file", "file", NULL, 0,
    "Path to default parameters file."},
   {'h', "help", NULL, NULL, 0,
    "Print this usage summary and exit."},
   {'l', "log-level", "level", NULL, 0,
    "Diagnostic logging level: 0-7 [5], or one of {emergency alert critical error warning [notice] information debug}"},
   {'n', "no-daemon", NULL, NULL, 0,
    "Remain a foreground process."},
   {'p', "preferences-file", "file", configurePreferencesFile, 0,
    "Path to preferences file."},
   {'q', "quiet", NULL, NULL, 0,
    "Suppress start-up messages."},
   {'s', "speech-driver", "driver", configureSpeechDriver, 0,
    "Speech driver: full library path, or one of {" SPKLIBS "}"},
   {'t', "text-table", "file", configureTextTable, 0,
    "Path to text translation table file."},
   {'v', "version", NULL, NULL, 0,
    "Print start-up messages and exit."},
   {'B', "braille-parameters", "arg,...", configureBrailleParameters, 0,
    "Parameters to braille driver."},
   {'E', "environment-variables", NULL, NULL, 0,
    "Recognize environment variables."},
   {'H', "full-help", NULL, NULL, OPT_Hidden,
    "Print this full usage summary and exit."},
   {'M', "message-delay", "csecs", NULL, 0,
    "Message hold time [400]."},
   {'N', "no-speech", NULL, NULL, 0,
    "Defer speech until restarted by command."},
   {'P', "pid-file", "file", NULL, 0,
    "Path to process identifier file."},
   {'R', "refresh-interval", "csecs", NULL, 0,
    "Braille window refresh interval [4]."},
   {'S', "speech-parameters", "arg,...", configureSpeechParameters, 0,
    "Parameters to speech driver."},
   {'X', "screen-parameters", "arg,...", configureScreenParameters, 0,
    "Parameters to screen driver."}
};
static unsigned int optionCount = sizeof(optionTable) / sizeof(optionTable[0]);

static void
processConfigurationLine (char *line, void *data)
{
  const char *delimiters = " \t"; // Characters which separate words.
  char *keyword; // Points to first word of each line.

  // Remove comment from end of line.
  {
    char *comment = strchr(line, '#');
    if (comment) *comment = 0;
  }

  keyword = strtok(line, delimiters);
  if (keyword) // Ignore blank lines.
    {
      int optionIndex;
      for (optionIndex=0; optionIndex<optionCount; ++optionIndex)
        {
          OptionEntry *option = &optionTable[optionIndex];
          if (option->configure) {
            if (strcasecmp(keyword, option->word) == 0) {
              int code = option->configure(delimiters);
              switch (code)
                {
                  case CFG_OK:
                    break;

                  case CFG_NoValue:
                    LogPrint(LOG_ERR,
                             "Operand not supplied for configuration item '%s'.",
                             keyword);
                    break;

                  case CFG_BadValue:
                    LogPrint(LOG_ERR,
                             "Invalid operand specified"
                             " for configuration item '%s'.",
                              keyword);
                    break;

                  case CFG_TooMany:
                    LogPrint(LOG_ERR,
                             "Too many operands supplied"
                             " for configuration item '%s'.",
                             keyword);
                    break;

                  case CFG_Duplicate:
                    LogPrint(LOG_ERR,
                             "Configuration item '%s' specified more than once.",
                             keyword);
                    break;

                  default:
                    LogPrint(LOG_ERR,
                             "Internal error: unsupported"
                             " configuration file error code: %d",
                             code);
                    break;
                }
              return;
            }
          }
        }
      LogPrint(LOG_ERR, "Unknown configuration item: '%s'.", keyword);
    }
}

static int
processConfigurationFile (const char *path, int optional)
{
   FILE *file = fopen(path, "r");
   if (file != NULL)
     { // The configuration file has been successfully opened.
       if (!processLines(file, processConfigurationLine, NULL))
         LogPrint(LOG_ERR, "File '%s' processing error.", path);
       fclose(file);
     }
   else
     {
       LogPrint((optional && (errno == ENOENT)? LOG_DEBUG: LOG_ERR),
                "Cannot open configuration file: %s: %s",
                path, strerror(errno));
       return 0;
     }
   return 1;
}

static void
ensureOptionSetting (const char **setting, const char *defaultSetting, const char *configuredSetting, const char *environmentVariable) {
  if (!*setting) {
    if (opt_environmentVariables && environmentVariable) *setting = getenv(environmentVariable);
    if (!*setting) *setting = configuredSetting? configuredSetting: defaultSetting;
  }
}

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
parseParameters (char ***values, const char *const *names, const char *description, char *parameters) {
   if (!names) {
      static const char *const noNames[] = {NULL};
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
      const char *name = (parameters = strdupWrapper(parameters));
      while (1) {
         char *delimiter = strchr(name, ',');
         int done = delimiter == NULL;
         if (!done) *delimiter = 0;
         if (*name) {
            char *value = strchr(name, '=');
            if (!value) {
               LogPrint(LOG_ERR, "Missing %s parameter value: %s", description, name);
            } else if (value == name) {
               LogPrint(LOG_ERR, "Missing %s parameter name: %s", description, name);
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
                  LogPrint(LOG_ERR, "Unsupported %s parameter: %s", description, name);
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
processParameters (char ***values, const char *const *names, const char *description, char *optionParameters, char *configuredParameters, const char *environmentVariable) {
  parseParameters(values, names, description, configuredParameters);
  if (opt_environmentVariables && environmentVariable) {
    parseParameters(values, names, description, getenv(environmentVariable));
  }
  parseParameters(values, names, description, optionParameters);
}

static void
logParameters (const char *const *names, char **values, char *description) {
   if (names && values) {
      while (*names) {
         LogPrint(LOG_INFO, "%s Parameter: %s=%s", description, *names, *values);
         ++names;
         ++values;
      }
   }
}

static void
processOptions (int argc, char **argv)
{
  int option;

  char short_options[1 + (optionCount * 2) + 1];
  #ifdef no_argument
    struct option long_options[optionCount + 1];
    {
      struct option *opt = long_options;
      int index;
      for (index=0; index<optionCount; ++index) {
        OptionEntry *option = &optionTable[index];
        opt->name = option->word;
        opt->has_arg = option->argument? required_argument: no_argument;
        opt->flag = NULL;
        opt->val = option->letter;
        ++opt;
      }
      memset(opt, 0, sizeof(*opt));
    }
    #define get_option() getopt_long(argc, argv, short_options, long_options, NULL)
  #else
    #define get_option() getopt(argc, argv, short_options)
  #endif
  {
    char *opt = short_options;
    int index;
    *opt++ = '+';
    for (index=0; index<optionCount; ++index) {
      OptionEntry *option = &optionTable[index];
      *opt++ = option->letter;
      if (option->argument) *opt++ = ':';
    }
    *opt = 0;
  }

  /* Parse command line using getopt(): */
  opterr = 0;
  while ((option = get_option()) != -1) {
    /* continue on error as much as possible, as often we are typing blind
       and won't even see the error message unless the display come up. */
    switch (option) {
      default:
        LogPrint(LOG_ERR, "Unimplemented invocation option: -%c", option);
        break;
      case '?': // An invalid option has been specified.
        LogPrint(LOG_ERR, "Unknown invocation option: -%c", optopt);
        return; /* not fatal */
      case 'a':                /* text translation table file name */
        opt_attributesTable = optarg;
        break;
      case 'b':                        /* name of driver */
        opt_brailleDriver = optarg;
        break;
      case 'c':                        /* name of driver */
        opt_contractionTable = optarg;
        break;
      case 'd':                /* serial device path */
        opt_brailleDevice = optarg;
        break;
      case 'e':                /* help */
        opt_standardError = 1;
        break;
      case 'f':                /* configuration file path */
        opt_configurationFile = optarg;
        break;
      case 'H':                /* help */
        opt_helpAll = 1;
      case 'h':                /* help */
        opt_help = 1;
        break;
      case 'l':        {  /* log level */
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
        LogPrint(LOG_ERR, "Invalid log level: %s", optarg);
        break;
      }
      case 'n':                /* don't go into the background */
        opt_noDaemon = 1;
        break;
      case 'p':                /* preferences file path */
        opt_preferencesFile = optarg;
        break;
      case 'q':                /* quiet */
        opt_quiet = 1;
        break;
      case 's':                        /* name of speech driver */
        opt_speechDriver = optarg;
        break;
      case 't':                /* text translation table file name */
        opt_textTable = optarg;
        break;
      case 'v':                /* version */
        opt_version = 1;
        break;
      case 'B':                        /* parameter to speech driver */
        extendParameters(&opt_brailleParameters, optarg);
        break;
      case 'E':                        /* parameter to speech driver */
        opt_environmentVariables = 1;
        break;
      case 'M': {        /* message delay */
        int value;
        int minimum = 1;
        if (validateInteger(&value, "message delay", optarg, &minimum, NULL))
          messageDelay = value * 10;
        break;
      }
      case 'N':                /* defer speech until restarted by command */
        opt_noSpeech = 1;
        break;
      case 'P':                /* process identifier file */
        opt_pidFile = optarg;
        break;
      case 'R': {        /* read delay */
        int value;
        int minimum = 1;
        if (validateInteger(&value, "read delay", optarg, &minimum, NULL))
          refreshInterval = value * 10;
        break;
      }
      case 'S':                        /* parameter to speech driver */
        extendParameters(&opt_speechParameters, optarg);
        break;
      case 'X':                        /* parameter to speech driver */
        extendParameters(&opt_screenParameters, optarg);
        break;
    }
  }
  #undef get_option
}

static void
printHelp (FILE *outputStream, unsigned int lineWidth, char *programPath, int all) {
   char line[lineWidth+1];
   unsigned int wordWidth = 0;
   unsigned int argumentWidth = 0;
   int optionIndex;
   for (optionIndex=0; optionIndex<optionCount; ++optionIndex) {
      OptionEntry *option = &optionTable[optionIndex];
      if (option->word) wordWidth = MAX(wordWidth, strlen(option->word));
      if (option->argument) argumentWidth = MAX(argumentWidth, strlen(option->argument));
   }

   {
      char *programName = strrchr(programPath, '/');
      programName = programName? programName+1: programPath;
      fprintf(outputStream, "Usage: %s [option ...]\n", programName);
   }

   for (optionIndex=0; optionIndex<optionCount; ++optionIndex) {
      OptionEntry *option = &optionTable[optionIndex];
      unsigned int lineLength = 0;
      if (!all && (option->flags & OPT_Hidden)) continue;

      line[lineLength++] = '-';
      line[lineLength++] = option->letter;
      line[lineLength++] = ' ';

      {
         unsigned int end = lineLength + argumentWidth;
         if (option->argument) {
            size_t argumentLength = strlen(option->argument);
            memcpy(line+lineLength, option->argument, argumentLength);
            lineLength += argumentLength;
         }
         while (lineLength < end) line[lineLength++] = ' ';
      }
      line[lineLength++] = ' ';

      {
         unsigned int end = lineLength + 2 + wordWidth + 1;
         if (option->word) {
            size_t wordLength = strlen(option->word);
            line[lineLength++] = '-';
            line[lineLength++] = '-';
            memcpy(line+lineLength, option->word, wordLength);
            lineLength += wordLength;
            if (option->argument) line[lineLength++] = '=';
         }
         while (lineLength < end) line[lineLength++] = ' ';
      }
      line[lineLength++] = ' ';

      {
         unsigned int headerWidth = lineLength;
         unsigned int descriptionWidth = lineWidth - headerWidth;
         const char *description = option->description;
         unsigned int charsLeft = strlen(description);
         while (1) {
            unsigned int charCount = charsLeft;
            if (charCount > descriptionWidth) {
               charCount = descriptionWidth;
               while (description[charCount] != ' ') --charCount;
               while (description[charCount] == ' ') --charCount;
               ++charCount;
            }
            memcpy(line+lineLength, description, charCount);
            lineLength += charCount;

            line[lineLength] = 0;
            fprintf(outputStream, "%s\n", line);

            while (description[charCount] == ' ') ++charCount;
            if (!(charsLeft -= charCount)) break;
            description += charCount;

            lineLength = 0;
            while (lineLength < headerWidth) line[lineLength++] = ' ';
         }
      }
   }
}

static void
globPrepare (GlobData *data, const char *pattern, const char *initial, int none) {
  memset(data, 0, sizeof(*data));
  data->pattern = pattern;
  if (!initial) initial = "";
  data->current = strdupWrapper(data->initial = initial);
  data->none = (none != 0);
}

static void
globBegin (GlobData *data) {
  int index;
  memset(&data->glob, 0, sizeof(data->glob));
  data->glob.gl_offs = (sizeof(data->pathsArea) / sizeof(data->pathsArea[0])) - 1;
  if (glob(data->pattern, GLOB_DOOFFS, NULL, &data->glob) == 0) {
    data->paths = (const char **)data->glob.gl_pathv;
    if (data->glob.gl_flags & GLOB_DOOFFS) {
      /* The behaviour of gl_pathc is inconsistent. Some implementations
       * include the leading NULL pointers and some don't. Let's just
       * figure it out the hard way by finding the trailing NULL.
       */
      data->count = data->glob.gl_offs;
      while (data->paths[data->count]) ++data->count;
    } else {
      data->count = data->glob.gl_pathc;
    }
  } else {
    data->paths = data->pathsArea;
    data->paths[data->count = data->glob.gl_offs] = NULL;
  }

  index = data->glob.gl_offs;
  if (data->none) data->paths[--index] = "";
  data->paths[--index] = data->initial;
  data->paths += index;
  data->count -= index;
  data->setting = 0;

  for (index=1; index<data->count; ++index) {
    if (strcmp(data->paths[index], data->initial) == 0) {
      data->paths += 1;
      data->count -= 1;
      break;
    }
  }
  for (index=0; index<data->count; ++index) {
    if (strcmp(data->paths[index], data->current) == 0) {
      data->setting = index;
      break;
    }
  }
}

static void
globEnd (GlobData *data) {
  if (data->glob.gl_pathc) {
    if (data->glob.gl_flags & GLOB_DOOFFS) {
      int index;
      for (index=0; index<data->glob.gl_offs; ++index)
        data->glob.gl_pathv[index] = NULL;
    }
    globfree(&data->glob);
  }
}

static const char *
globChanged (GlobData *data) {
  char *path = strdupWrapper(data->paths[data->setting]);
  free(data->current);
  return data->current = path;
}

static void
loadTranslationTable (char *table, const char *path, const char *name) {
  int fd = open(path, O_RDONLY);
  if (fd >= 0) {
    char buffer[0X100];
    if (read(fd, buffer, sizeof(buffer)) == sizeof(buffer)) {
      memcpy(table, buffer, sizeof(buffer));
    } else {
      LogPrint(LOG_ERR, "Cannot read %s translation table: %s", name, path);
    }
    close(fd);
  } else {
    LogPrint(LOG_ERR, "Cannot open %s translation table: %s", name, path);
  }
}

static void
loadTextTable (const char *path) {
  loadTranslationTable(textTable, path, "text");
  reverseTable(textTable, untextTable);
}

static void
loadAttributesTable (const char *path) {
  loadTranslationTable(attributesTable, path, "attributes");
}

static void
changedWindowAttributes (void) {
  fwinshift = MAX(brl.x-prefs.winovlp, 1);
  hwinshift = brl.x / 2;
  vwinshift = 5;
}

static void
changedTuneDevice (void) {
  setTuneDevice(prefs.tunedev);
}

static void
changedTextTable (void) {
  const char *path = globChanged(&glob_textTable);
  loadTextTable(path);
}

static void
changedAttributesTable (void) {
  const char *path = globChanged(&glob_attributesTable);
  loadAttributesTable(path);
}

static void
changedContractionTable (void) {
  const char *path = globChanged(&glob_contractionTable);
  void *table = *path? compileContractionTable(path): NULL;
  if (contractionTable) destroyContractionTable(contractionTable);
  contractionTable = table;
}

static void
changedPreferences (void) {
  changedWindowAttributes();
  changedTuneDevice();
}

void
initializeBraille (void) {
   brl.disp = NULL;
   brl.x = brl.y = -1;
}

static void
getBrailleDriver (void) {
  if ((brailleDriver = loadBrailleDriver(&opt_brailleDriver))) {
    processParameters(&brailleParameters, brailleDriver->parameters, "braille driver",
                      opt_brailleParameters, cfg_brailleParameters, "BRLTTY_BRAILLE_PARAMETERS");
  } else {
    LogPrint(LOG_ERR, "Bad braille driver selection: %s", opt_brailleDriver);
    fprintf(stderr, "\n");
    listBrailleDrivers();
    fprintf(stderr, "\nUse -b to specify one, and -h for quick help.\n\n");

    /* not fatal */
    brailleDriver = &noBraille;
  }
  if (!opt_brailleDriver) opt_brailleDriver = "built-in";
}

void
startBrailleDriver (void) {
   brailleDriver->initialize(brailleParameters, &brl, opt_brailleDevice);
   if (brl.x == -1) {
      LogPrint(LOG_CRIT, "Braille driver initialization failed.");
      exit(6);
   }
   braille = brailleDriver;

   changedWindowAttributes();
   playTune(&tune_detected);

   LogPrint(LOG_DEBUG, "Braille display has %d %s of %d %s.",
            brl.y, (brl.y == 1)? "row": "rows",
            brl.x, (brl.x == 1)? "column": "columns");
   clearStatusCells();
}

void
stopBrailleDriver (void) {
   braille = &noBraille;
   brailleDriver->close(&brl);
   initializeBraille();
}

static void
exitContractionTable (void) {
   if (contractionTable) {
      destroyContractionTable(contractionTable);
      contractionTable = NULL;
   }
}

static void
exitBrailleDriver (void) {
   clearStatusCells();
   message("BRLTTY terminated.", MSG_NODELAY|MSG_SILENT);
   stopBrailleDriver();
}

void
initializeSpeech (void) {
}

static void
getSpeechDriver (void) {
  if ((speechDriver = loadSpeechDriver(&opt_speechDriver))) {
    processParameters(&speechParameters, speechDriver->parameters, "speech driver",
                      opt_speechParameters, cfg_speechParameters, "BRLTTY_SPEECH_PARAMETERS");
  } else {
    LogPrint(LOG_ERR, "Bad speech driver selection: %s", opt_speechDriver);
    fprintf(stderr, "\n");
    listSpeechDrivers();
    fprintf(stderr, "\nUse -s to specify one, and -h for quick help.\n\n");

    /* not fatal */
    speechDriver = &noSpeech;
  }
  if (!opt_speechDriver) opt_speechDriver = "built-in";
}

void
startSpeechDriver (void) {
   if (opt_noSpeech) {
      LogPrint(LOG_INFO, "Automatic speech driver initialization disabled.");
   } else {
      speechDriver->initialize(speechParameters);
      speech = speechDriver;
   }
}

void
stopSpeechDriver (void) {
   if (opt_noSpeech) {
      opt_noSpeech = 0;
   } else {
      speech->mute();
      speech = &noSpeech;
      speechDriver->close();
      initializeSpeech();
   }
}

static void
exitSpeechDriver (void) {
   stopSpeechDriver();
}

int
readKey (DriverCommandContext cmds) {
   while (1) {
      int key = braille->read(cmds);
      if (key != EOF) {
         LogPrint(LOG_DEBUG, "Command: %5.5X", key);
         if (key == CMD_NOOP) continue;
         return key;
      }
      delay(refreshInterval);
      closeTuneDevice(0);
   }
}

int
loadPreferences (int change)
{
  int ok = 0;
  int fd = open(opt_preferencesFile, O_RDONLY);
  if (fd != -1) {
    Preferences newPreferences;
    int count = read(fd, &newPreferences, sizeof(newPreferences));
    if (count == sizeof(newPreferences)) {
      if ((newPreferences.magicnum[0] == (PREFS_MAGICNUM&0XFF)) && (newPreferences.magicnum[1] == (PREFS_MAGICNUM>>8))) {
        prefs = newPreferences;
        ok = 1;
        if (change) changedPreferences();
      } else
        LogPrint(LOG_ERR, "Invalid preferences file: %s", opt_preferencesFile);
    } else if (count == -1)
      LogPrint(LOG_ERR, "Cannot read preferences file: %s: %s",
               opt_preferencesFile, strerror(errno));
    else
      LogPrint(LOG_ERR, "Preferences file '%s' has incorrect size %d (should be %d).",
               opt_preferencesFile, count, sizeof(newPreferences));
    close(fd);
  } else
    LogPrint((errno==ENOENT? LOG_DEBUG: LOG_ERR),
             "Cannot open preferences file: %s: %s",
             opt_preferencesFile, strerror(errno));
  return ok;
}

int 
savePreferences (void)
{
  int ok = 0;
  int fd = open(opt_preferencesFile, O_WRONLY | O_CREAT | O_TRUNC);
  if (fd != -1) {
    fchmod(fd, S_IRUSR | S_IWUSR);
    if (write(fd, &prefs, sizeof(prefs)) == sizeof(prefs)) {
      ok = 1;
    } else {
      LogPrint(LOG_ERR, "Cannot write to preferences file: %s: %s",
               opt_preferencesFile, strerror(errno));
    }
    close(fd);
  } else {
    LogPrint(LOG_ERR, "Cannot open preferences file: %s: %s",
             opt_preferencesFile, strerror(errno));
  }
  if (!ok)
    message("not saved", 0);
  return ok;
}

static int
testSkipBlankWindows () {
   return prefs.skpblnkwins;
}

static int
testSlidingWindow () {
   return prefs.slidewin;
}

static int
testShowCursor () {
   return prefs.csrvis;
}

static int
testBlinkingCursor () {
   return testShowCursor() && prefs.csrblink;
}

static int
testShowAttributes () {
   return prefs.attrvis;
}

static int
testBlinkingAttributes () {
   return testShowAttributes() && prefs.attrblink;
}

static int
testBlinkingCapitals () {
   return prefs.capblink;
}

static int
testTunes () {
   return prefs.tunes;
}

static int
testTunesMidi () {
   return testTunes() && (prefs.tunedev == tdSequencer);
}

void
updatePreferences (void) {
  globBegin(&glob_textTable);
  globBegin(&glob_attributesTable);
  globBegin(&glob_contractionTable);
  {
    static unsigned char exitSave = 0;                /* 1 == save preferences on exit */
    static const char *booleanValues[] = {"No", "Yes"};
    static const char *cursorStyles[] = {"Underline", "Block"};
    static const char *metaModes[] = {"Escape Prefix", "High-order Bit"};
    static const char *skipBlankWindowsModes[] = {"All", "End of Line", "Rest of Line"};
    static const char *statusStyles[] = {"None", "Alva", "Tieman", "PowerBraille 80", "Generic", "MDV", "Voyager"};
    static const char *textStyles[] = {"8-dot", "6-dot"};
    static const char *tuneDevices[] = {"PC Speaker", "Sound Card", "MIDI", "AdLib/OPL3/SB-FM"};
    typedef struct {
       unsigned char *setting;                        /* pointer to the item value */
       void (*changed) (void);
       int (*test) (void);
       const char *description;                        /* item description */
       const char *const*names;                        /* 0 == numeric, 1 == bolean */
       unsigned char minimum;                        /* minimum range */
       unsigned char maximum;                        /* maximum range */
    } MenuItem;
    #define MENU_ITEM(setting, changed, test, description, values, minimum, maximum) {&setting, changed, test, description, values, minimum, maximum}
    #define NUMERIC_ITEM(setting, changed, test, description, minimum, maximum) MENU_ITEM(setting, changed, test, description, NULL, minimum, maximum)
    #define TIMING_ITEM(setting, changed, test, description) NUMERIC_ITEM(setting, changed, test, description, 1, 16)
    #define TEXT_ITEM(setting, changed, test, description, names, count) MENU_ITEM(setting, changed, test, description, names, 0, count-1)
    #define SYMBOLIC_ITEM(setting, changed, test, description, names) TEXT_ITEM(setting, changed, test, description, names, ((sizeof(names) / sizeof(names[0]))))
    #define BOOLEAN_ITEM(setting, changed, test, description) SYMBOLIC_ITEM(setting, changed, test, description, booleanValues)
    #define GLOB_ITEM(data, changed, test, description) TEXT_ITEM(data.setting, changed, test, description, data.paths, data.count)
    MenuItem menu[] = {
       BOOLEAN_ITEM(exitSave, NULL, NULL, "Save on Exit"),
       SYMBOLIC_ITEM(prefs.sixdots, NULL, NULL, "Text Style", textStyles),
       SYMBOLIC_ITEM(prefs.metamode, NULL, NULL, "Meta Mode", metaModes),
       BOOLEAN_ITEM(prefs.skpidlns, NULL, NULL, "Skip Identical Lines"),
       BOOLEAN_ITEM(prefs.skpblnkwins, NULL, NULL, "Skip Blank Windows"),
       SYMBOLIC_ITEM(prefs.skpblnkwinsmode, NULL, testSkipBlankWindows, "Which Blank Windows", skipBlankWindowsModes),
       BOOLEAN_ITEM(prefs.slidewin, NULL, NULL, "Sliding Window"),
       BOOLEAN_ITEM(prefs.eager_slidewin, NULL, testSlidingWindow, "Eager Sliding Window"),
       NUMERIC_ITEM(prefs.winovlp, changedWindowAttributes, NULL, "Window Overlap", 0, 20),
       BOOLEAN_ITEM(prefs.csrvis, NULL, NULL, "Show Cursor"),
       SYMBOLIC_ITEM(prefs.csrsize, NULL, testShowCursor, "Cursor Style", cursorStyles),
       BOOLEAN_ITEM(prefs.csrblink, NULL, testShowCursor, "Blinking Cursor"),
       TIMING_ITEM(prefs.csroncnt, NULL, testBlinkingCursor, "Cursor Visible Period"),
       TIMING_ITEM(prefs.csroffcnt, NULL, testBlinkingCursor, "Cursor Invisible Period"),
       BOOLEAN_ITEM(prefs.attrvis, NULL, NULL, "Show Attributes"),
       BOOLEAN_ITEM(prefs.attrblink, NULL, testShowAttributes, "Blinking Attributes"),
       TIMING_ITEM(prefs.attroncnt, NULL, testBlinkingAttributes, "Attributes Visible Period"),
       TIMING_ITEM(prefs.attroffcnt, NULL, testBlinkingAttributes, "Attributes Invisible Period"),
       BOOLEAN_ITEM(prefs.capblink, NULL, NULL, "Blinking Capitals"),
       TIMING_ITEM(prefs.caponcnt, NULL, testBlinkingCapitals, "Capitals Visible Period"),
       TIMING_ITEM(prefs.capoffcnt, NULL, testBlinkingCapitals, "Capitals Invisible Period"),
       BOOLEAN_ITEM(prefs.tunes, NULL, NULL, "Alert Tunes"),
       SYMBOLIC_ITEM(prefs.tunedev, changedTuneDevice, testTunes, "Tune Device", tuneDevices),
       TEXT_ITEM(prefs.midiinstr, NULL, testTunesMidi, "MIDI Instrument", midiInstrumentTable, midiInstrumentCount),
       BOOLEAN_ITEM(prefs.dots, NULL, NULL, "Alert Dots"),
       SYMBOLIC_ITEM(prefs.stcellstyle, NULL, NULL, "Status Cells Style", statusStyles),
       GLOB_ITEM(glob_textTable, changedTextTable, NULL, "Text Table"),
       GLOB_ITEM(glob_attributesTable, changedAttributesTable, NULL, "Attributes Table"),
       GLOB_ITEM(glob_contractionTable, changedContractionTable, NULL, "Contraction Table")
    };
    int menuSize = sizeof(menu) / sizeof(menu[0]);
    static int menuIndex = 0;                        /* current menu item */

    unsigned char line[0X40];                /* display buffer */
    int lineIndent = 0;                                /* braille window pos in buffer */
    int settingChanged = 0;                        /* 1 when item's value has changed */

    Preferences oldPreferences = prefs;        /* backup preferences */
    int key;                                /* readbrl() value */

    /* status cells */
    setStatusText("prefs");
    message("Preferences Menu", 0);

    while (1) {
      int lineLength;                                /* current menu item length */
      int settingIndent;                                /* braille window pos in buffer */
      MenuItem *item = &menu[menuIndex];

      closeTuneDevice(0);

      /* First we draw the current menu item in the buffer */
      sprintf(line, "%s: ", item->description);
      settingIndent = strlen(line);
      if (item->names) {
         const char *name = item->names[*item->setting - item->minimum];
         if (!*name) name = "<off>";
         strcat(line, name);
      } else {
         sprintf(line+settingIndent, "%d", *item->setting);
      }
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
            brl.disp[index] = textTable[line[lineIndent+index]];
      }
      braille->writeWindow(&brl);
      delay(refreshInterval);

      /* Now process any user interaction */
      switch (key = readKey(CMDS_PREFS)) {
        case CMD_MENU_FIRST_ITEM:
        case CMD_TOP:
        case CMD_TOP_LEFT:
          menuIndex = lineIndent = 0;
          break;
        case CMD_MENU_LAST_ITEM:
        case CMD_BOT:
        case CMD_BOT_LEFT:
          menuIndex = menuSize - 1;
          lineIndent = 0;
          break;
        case CMD_MENU_PREV_ITEM:
        case CMD_LNUP:
        case VAL_PASSKEY+VPK_CURSOR_UP:
          do {
            if (menuIndex == 0)
              menuIndex = menuSize;
            --menuIndex;
          } while (menu[menuIndex].test && !menu[menuIndex].test());
          lineIndent = 0;
          break;
        case CMD_MENU_NEXT_ITEM:
        case CMD_LNDN:
        case VAL_PASSKEY+VPK_CURSOR_DOWN:
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
        case CMD_MENU_PREV_SETTING:
        case CMD_WINUP:
        case CMD_CHRLT:
        case VAL_PASSKEY+VPK_CURSOR_LEFT:
          if ((*item->setting)-- <= item->minimum)
            *item->setting = item->maximum;
          settingChanged = 1;
          break;
        case CMD_MENU_NEXT_SETTING:
        case CMD_WINDN:
        case CMD_CHRRT:
        case VAL_PASSKEY+VPK_CURSOR_RIGHT:
        case CMD_HOME:
        case VAL_PASSKEY+VPK_RETURN:
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
              "Press PREFS again to quit.", MSG_WAITKEY|MSG_NODELAY);
          break;
        case CMD_PREFLOAD:
          prefs = oldPreferences;
          changedPreferences();
          message("changes discarded", 0);
          break;
        case CMD_PREFSAVE:
          exitSave = 1;
          goto exitMenu;
        default:
          if (key >= CR_ROUTE && key < CR_ROUTE+brl.x) {
             /* Why not setting a value with routing keys... */
             key -= CR_ROUTE;
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
          if (exitSave) {
            if (savePreferences()) {
              playTune(&tune_done);
            }
          }
          globEnd(&glob_textTable);
          globEnd(&glob_attributesTable);
          globEnd(&glob_contractionTable);
          return;
      }

      if (settingChanged)
        if (item->changed)
          item->changed();
    }
  }
}

static void
exitTunes (void) {
   closeTuneDevice(1);
}

static void
exitScreen (void) {
   closeAllScreens();
}

static void
exitPidFile (void) {
   unlink(opt_pidFile);
}

static void
background (void) {
   switch (fork()) {
      case -1: // error
         LogPrint(LOG_CRIT, "process creation error: %s", strerror(errno));
         exit(10);
      case 0: // child
         break;
      default: // parent
         _exit(0);
   }
   if (!opt_standardError) {
      LogClose();
      LogOpen(1);
   }
}

void
startup(int argc, char *argv[]) {
  processOptions(argc, argv);

  if (opt_help) {
    printHelp(stdout, 79, argv[0], opt_helpAll);
    exit(0);
  }

  /* Set logging levels. */
  if (opt_standardError)
    LogClose();
  SetLogLevel(opt_logLevel);
  SetStderrLevel(opt_version?
                    (opt_quiet? LOG_NOTICE: LOG_INFO):
                    (opt_quiet? LOG_WARNING: LOG_NOTICE));

  LogPrint(LOG_NOTICE, "%s", VERSION);
  LogPrint(LOG_INFO, "%s", COPYRIGHT);

  /* Process the configuration file. */
  {
    int optional = opt_configurationFile == NULL;
    ensureOptionSetting(&opt_configurationFile, CONFIG_FILE, NULL, "BRLTTY_CONFIGURATION_FILE");
    processConfigurationFile(opt_configurationFile, optional);
  }
  ensureOptionSetting(&opt_preferencesFile, NULL, cfg_preferencesFile, "BRLTTY_PREFERENCES_FILE");
  ensureOptionSetting(&opt_textTable, NULL, cfg_textTable, "BRLTTY_TEXT_TABLE");
  ensureOptionSetting(&opt_attributesTable, NULL, cfg_attributesTable, "BRLTTY_ATTRIBUTES_TABLE");
  ensureOptionSetting(&opt_contractionTable, NULL, cfg_contractionTable, "BRLTTY_CONTRACTION_TABLE");
  ensureOptionSetting(&opt_brailleDevice, NULL, cfg_brailleDevice, "BRLTTY_BRAILLE_DEVICE");
  ensureOptionSetting(&opt_brailleDriver, NULL, cfg_brailleDriver, "BRLTTY_BRAILLE_DRIVER");
  ensureOptionSetting(&opt_speechDriver, NULL, cfg_speechDriver, "BRLTTY_SPEECH_DRIVER");

  if (!opt_brailleDevice) opt_brailleDevice = BRLDEV;
  if (*opt_brailleDevice == 0) {
    LogPrint(LOG_CRIT, "No braille device specified.");
    fprintf(stderr, "Use -d to specify one.\n");
    exit(4);
  }

  getBrailleDriver();
  getSpeechDriver();
  processParameters(&screenParameters, getScreenParameters(), "screen driver",
                    opt_screenParameters, cfg_screenParameters, "BRLTTY_SCREEN_PARAMETERS");

  if (!opt_preferencesFile) {
    const char *part1 = "brltty-";
    const char *part2 = brailleDriver->identifier;
    const char *part3 = ".prefs";
    char *path = mallocWrapper(strlen(part1) + strlen(part2) + strlen(part3) + 1);
    sprintf(path, "%s%s%s", part1, part2, part3);
    opt_preferencesFile = path;
  }

  if (chdir(HOME_DIR) == -1) {                /* * change to directory containing data files  */
    char *backup_dir = "/etc";
    LogPrint(LOG_ERR, "Cannot change directory to '%s': %s",
             HOME_DIR, strerror(errno));
    LogPrint(LOG_WARNING, "Using backup directory '%s' instead.",
             backup_dir);
    chdir(backup_dir);                /* home directory not found, use backup */
  }

  if (opt_textTable)
    loadTextTable(opt_textTable);
  else {
    opt_textTable = TEXTTRANS;
    reverseTable(textTable, untextTable);
  }
  globPrepare(&glob_textTable, "text.*.tbl", opt_textTable, 0);

  if (opt_attributesTable)
    loadAttributesTable(opt_attributesTable);
  else
    opt_attributesTable = ATTRTRANS;
  globPrepare(&glob_attributesTable, "attr*.tbl", opt_attributesTable, 0);

  globPrepare(&glob_contractionTable, "*.ctb", opt_contractionTable, 1);
  if (opt_contractionTable) {
    LogPrint(LOG_DEBUG, "Compiling contraction table: %s", opt_contractionTable);
    if ((contractionTable = compileContractionTable(opt_contractionTable))) {
      atexit(exitContractionTable);
    } else {
      LogPrint(LOG_ERR, "Cannot compile contraction table.");
    }
  }

  {
    char buffer[0X100];
    char *path = getcwd(buffer, sizeof(buffer));
    LogPrint(LOG_INFO, "Working Directory: %s",
             path? path: "path-too-long");
  }

  LogPrint(LOG_INFO, "Preferences File: %s", opt_preferencesFile);
  LogPrint(LOG_INFO, "Help Page: %s[%d]", brailleDriver->helpFile, getHelpPageNumber());
  LogPrint(LOG_INFO, "Text Table: %s", opt_textTable);
  LogPrint(LOG_INFO, "Attributes Table: %s", opt_attributesTable);
  LogPrint(LOG_INFO, "Braille Device: %s", opt_brailleDevice);
  LogPrint(LOG_INFO, "Braille Driver: %s (%s)",
           opt_brailleDriver, brailleDriver->name);
  logParameters(brailleDriver->parameters, brailleParameters, "Braille");
  LogPrint(LOG_INFO, "Speech Driver: %s (%s)",
           opt_speechDriver, speechDriver->name);
  logParameters(speechDriver->parameters, speechParameters, "Speech");
  logParameters(getScreenParameters(), screenParameters, "Screen");

  /*
   * Give braille and speech libraries a chance to introduce themselves.
   */
  brailleDriver->identify();
  speechDriver->identify();

  if (opt_version)
    exit(0);

  /* Load preferences file */
  if (!loadPreferences(0)) {
    memset(&prefs, 0, sizeof(prefs));

    prefs.magicnum[0] = PREFS_MAGICNUM&0XFF;
    prefs.magicnum[1] = PREFS_MAGICNUM>>8;

    prefs.csrvis = INIT_CSRVIS;
    prefs.csrsize = INIT_CSRSIZE;
    prefs.csrblink = INIT_CSRBLINK;
    prefs.csroncnt = INIT_CSR_ON_CNT;
    prefs.csroffcnt = INIT_CSR_OFF_CNT;

    prefs.attrvis = INIT_ATTRVIS;
    prefs.attrblink = INIT_ATTRBLINK;
    prefs.attroncnt = INIT_ATTR_ON_CNT;
    prefs.attroffcnt = INIT_ATTR_OFF_CNT;

    prefs.capblink = INIT_CAPBLINK;
    prefs.caponcnt = INIT_CAP_ON_CNT;
    prefs.capoffcnt = INIT_CAP_OFF_CNT;

    prefs.sixdots = INIT_SIXDOTS;
    prefs.metamode = INIT_METAMODE;
    prefs.winovlp = INIT_WINOVLP;
    prefs.slidewin = INIT_SLIDEWIN;
    prefs.eager_slidewin = INIT_EAGER_SLIDEWIN;

    prefs.skpidlns = INIT_SKPIDLNS;
    prefs.skpblnkwins = INIT_SKPBLNKWINS;
    prefs.skpblnkwinsmode = INIT_SKPBLNKWINSMODE;

    prefs.tunes = INIT_TUNES;
    prefs.tunedev = INIT_TUNEDEV;
    prefs.midiinstr = 0;
    prefs.dots = INIT_DOTS;

    prefs.stcellstyle = brailleDriver->statusStyle;
  }
  changedTuneDevice();
  atexit(exitTunes);

  /*
   * Initialize screen library 
   */
  if (!initializeLiveScreen(screenParameters)) {                                
    LogPrint(LOG_CRIT, "Cannot read screen.");
    exit(7);
  }
  atexit(exitScreen);
  
  if (!opt_noDaemon) {
    LogPrint(LOG_DEBUG, "Becoming daemon.");
    background();
    SetStderrOff();

    /* request a new session (job control) */
    if (setsid() == -1) {                        
      LogPrint(LOG_CRIT, "session creation error: %s", strerror(errno));
      exit(11);
    }

    {
      char *nullDevice = "/dev/null";
      freopen(nullDevice, "r", stdin);
      freopen(nullDevice, "a", stdout);
      if (opt_standardError) {
        fflush(stderr);
      } else {
        freopen(nullDevice, "a", stderr);
      }
    }

    background();
  }
  /*
   * From this point, all IO functions as printf, puts, perror, etc. can't be
   * used anymore since we are a daemon.  The LogPrint facility should 
   * be used instead.
   */

  /* Create the process identifier file. */
  if (opt_pidFile) {
    FILE *stream = fopen(opt_pidFile, "w");
    if (stream) {
      fprintf(stream, "%d\n", getpid());
      fclose(stream);
      atexit(exitPidFile);
    } else {
      LogPrint(LOG_ERR, "Cannot open process identifier file: %s: %s",
               opt_pidFile, strerror(errno));
    }
  }

  /* Initialise Braille display: */
  startBrailleDriver();
  atexit(exitBrailleDriver);

  /* Initialise speech */
  startSpeechDriver();
  atexit(exitSpeechDriver);

  /* Initialise help screen */
  if (!initializeHelpScreen(brailleDriver->helpFile))
    LogPrint(LOG_WARNING, "Cannot open help screen file: %s", brailleDriver->helpFile);

  if (!opt_quiet)
    message(VERSION, 0);        /* display initialisation message */
  if (ProblemCount) {
    char buffer[0X40];
    snprintf(buffer, sizeof(buffer), "%d startup problem%s",
             ProblemCount, (ProblemCount==1? "": "s"));
    message(buffer, MSG_WAITKEY);
  }
}
