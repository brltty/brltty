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

/*
 * config.c - Everything configuration related.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <limits.h>

#ifdef ENABLE_PREFERENCES_MENU
#ifdef ENABLE_TABLE_SELECTION
#include <glob.h>
#endif /* ENABLE_TABLE_SELECTION */
#endif /* ENABLE_PREFERENCES_MENU */

#include "brl.h"
#ifdef ENABLE_SPEECH_SUPPORT
#include "spk.h"
#endif /* ENABLE_SPEECH_SUPPORT */
#include "scr.h"
#include "contract.h"
#include "tunes.h"
#include "message.h"
#include "misc.h"
#include "system.h"
#include "options.h"
#include "brltty.h"
#include "defaults.h"

char COPYRIGHT[] = "Copyright (C) 1995-2003 by The BRLTTY Team - all rights reserved.";

#define DEVICE_DIRECTORY "/dev"
#define TABLE_EXTENSION ".tbl"
#define TEXT_TABLE_PREFIX "text."
#define ATTRIBUTES_TABLE_PREFIX "attr."

#ifdef ENABLE_API
static char *opt_apiParameters = NULL;
#endif /* ENABLE_API */
static const char *opt_attributesTable = NULL;
static const char *opt_brailleDevice = NULL;
static const char *opt_brailleDriver = NULL;
static char *opt_brailleParameters = NULL;
static const char *opt_configurationFile = NULL;
#ifdef ENABLE_CONTRACTED_BRAILLE
static const char *opt_contractionsDirectory = NULL;
static const char *opt_contractionTable = NULL;
#endif /* ENABLE_CONTRACTED_BRAILLE */
static const char *opt_dataDirectory = NULL;
static short opt_environmentVariables = 0;
static const char *opt_libraryDirectory = NULL;
static short opt_logLevel = LOG_NOTICE;
static short opt_noDaemon = 0;
static short opt_noSpeech = 0;
static const char *opt_pidFile = NULL;
static const char *opt_preferencesFile = NULL;
static short opt_quiet = 0;
static char *opt_screenParameters = NULL;
#ifdef ENABLE_SPEECH_SUPPORT
static const char *opt_speechDriver = NULL;
static char *opt_speechParameters = NULL;
#endif /* ENABLE_SPEECH_SUPPORT */
static short opt_standardError = 0;
static const char *opt_tablesDirectory = NULL;
static const char *opt_textTable = NULL;
static short opt_version = 0;

static char *cfg_preferencesFile = NULL;
static char *cfg_tablesDirectory = NULL;
static char *cfg_textTable = NULL;
static char *cfg_attributesTable = NULL;
#ifdef ENABLE_CONTRACTED_BRAILLE
static char *cfg_contractionsDirectory = NULL;
static char *cfg_contractionTable = NULL;
#endif /* ENABLE_CONTRACTED_BRAILLE */
#ifdef ENABLE_API
static char *cfg_apiParameters = NULL;
#endif /* ENABLE_API */
static char *cfg_libraryDirectory = NULL;
static char *cfg_dataDirectory = NULL;
static char *cfg_brailleDriver = NULL;
static char *cfg_brailleDevice = NULL;
static char *cfg_brailleParameters = NULL;
#ifdef ENABLE_SPEECH_SUPPORT
static char *cfg_speechDriver = NULL;
static char *cfg_speechParameters = NULL;
#endif /* ENABLE_SPEECH_SUPPORT */
static char *cfg_screenParameters = NULL;

static const BrailleDriver *brailleDriver;
static char **brailleParameters = NULL;

#ifdef ENABLE_API
static char **apiParameters = NULL;
#endif /* ENABLE_API */

#ifdef ENABLE_SPEECH_SUPPORT
static const SpeechDriver *speechDriver;
static char **speechParameters = NULL;
#endif /* ENABLE_SPEECH_SUPPORT */

static char **screenParameters = NULL;

short homedir_found = 0;        /* CWD status */

static ConfigurationLineStatus
getConfigurationOperand (char **operandAddress, const char *delimiters, int extend) {
  char *operand = strtok(NULL, delimiters);

  if (!operand) return CFG_NoValue;
  if (strtok(NULL, delimiters)) return CFG_TooMany;

  {
    ConfigurationLineStatus status = CFG_OK;
    if (*operandAddress && !extend) {
      status = CFG_Duplicate;
      free(*operandAddress);
      *operandAddress = NULL;
    }
    if (*operandAddress) {
      int size = strlen(*operandAddress) + strlen(operand) + 2;
      char *buffer = mallocWrapper(size);
      snprintf(buffer, size, "%s,%s", *operandAddress, operand);
      free(*operandAddress);
      *operandAddress = buffer;
    } else {
      *operandAddress = strdupWrapper(operand);
    }
    return status;
  }
}

static ConfigurationLineStatus
configurePreferencesFile (const char *delimiters) {
  return getConfigurationOperand(&cfg_preferencesFile, delimiters, 0);
}

static ConfigurationLineStatus
configureTablesDirectory (const char *delimiters) {
  return getConfigurationOperand(&cfg_tablesDirectory, delimiters, 0);
}

static ConfigurationLineStatus
configureTextTable (const char *delimiters) {
  return getConfigurationOperand(&cfg_textTable, delimiters, 0);
}

static ConfigurationLineStatus
configureAttributesTable (const char *delimiters) {
  return getConfigurationOperand(&cfg_attributesTable, delimiters, 0);
}

#ifdef ENABLE_CONTRACTED_BRAILLE
static ConfigurationLineStatus
configureContractionsDirectory (const char *delimiters) {
  return getConfigurationOperand(&cfg_contractionsDirectory, delimiters, 0);
}

static ConfigurationLineStatus
configureContractionTable (const char *delimiters) {
  return getConfigurationOperand(&cfg_contractionTable, delimiters, 0);
}
#endif /* ENABLE_CONTRACTED_BRAILLE */

#ifdef ENABLE_API
static ConfigurationLineStatus
configureApiParameters (const char *delimiters) {
  return getConfigurationOperand(&cfg_apiParameters, delimiters, 1);
}
#endif /* ENABLE_API */

static ConfigurationLineStatus
configureLibraryDirectory (const char *delimiters) {
  return getConfigurationOperand(&cfg_libraryDirectory, delimiters, 0);
}

static ConfigurationLineStatus
configureDataDirectory (const char *delimiters) {
  return getConfigurationOperand(&cfg_dataDirectory, delimiters, 0);
}

static ConfigurationLineStatus
configureBrailleDriver (const char *delimiters) {
  return getConfigurationOperand(&cfg_brailleDriver, delimiters, 0);
}

static ConfigurationLineStatus
configureBrailleDevice (const char *delimiters) {
  return getConfigurationOperand(&cfg_brailleDevice, delimiters, 0);
}

static ConfigurationLineStatus
configureBrailleParameters (const char *delimiters) {
  return getConfigurationOperand(&cfg_brailleParameters, delimiters, 1);
}

#ifdef ENABLE_SPEECH_SUPPORT
static ConfigurationLineStatus
configureSpeechDriver (const char *delimiters) {
  return getConfigurationOperand(&cfg_speechDriver, delimiters, 0);
}

static ConfigurationLineStatus
configureSpeechParameters (const char *delimiters) {
  return getConfigurationOperand(&cfg_speechParameters, delimiters, 1);
}
#endif /* ENABLE_SPEECH_SUPPORT */

static ConfigurationLineStatus
configureScreenParameters (const char *delimiters) {
  return getConfigurationOperand(&cfg_screenParameters, delimiters, 1);
}

BEGIN_OPTION_TABLE
  {'a', "attributes-table", "file", configureAttributesTable, 0,
   "Path to attributes translation table file."},
  {'b', "braille-driver", "driver", configureBrailleDriver, 0,
   "Braille driver: full library path, or one of {" BRAILLE_DRIVERS "}"},
#ifdef ENABLE_CONTRACTED_BRAILLE
  {'c', "contraction-table", "file", configureContractionTable, 0,
   "Path to contraction table file."},
#endif /* ENABLE_CONTRACTED_BRAILLE */
  {'d', "braille-device", "device", configureBrailleDevice, 0,
   "Path to device for accessing braille display."},
  {'e', "standard-error", NULL, NULL, 0,
   "Log to standard error rather than to syslog."},
  {'f', "configuration-file", "file", NULL, 0,
   "Path to default parameters file."},
  {'l', "log-level", "level", NULL, 0,
   "Diagnostic logging level: 0-7 [5], or one of {emergency alert critical error warning [notice] information debug}"},
  {'n', "no-daemon", NULL, NULL, 0,
   "Remain a foreground process."},
  {'p', "preferences-file", "file", configurePreferencesFile, 0,
   "Path to preferences file."},
  {'q', "quiet", NULL, NULL, 0,
   "Suppress start-up messages."},
#ifdef ENABLE_SPEECH_SUPPORT
  {'s', "speech-driver", "driver", configureSpeechDriver, 0,
   "Speech driver: full library path, or one of {" SPEECH_DRIVERS "}"},
#endif /* ENABLE_SPEECH_SUPPORT */
  {'t', "text-table", "file", configureTextTable, 0,
   "Path to text translation table file."},
  {'v', "version", NULL, NULL, 0,
   "Print start-up messages and exit."},
#ifdef ENABLE_API
  {'A', "api-parameters", "arg,...", configureApiParameters, 0,
   "Parameters for the application programming interface."},
#endif /* ENABLE_API */
  {'B', "braille-parameters", "arg,...", configureBrailleParameters, 0,
   "Parameters for the braille driver."},
#ifdef ENABLE_CONTRACTED_BRAILLE
  {'C', "contractions-directory", "directory", configureContractionsDirectory, OPT_Hidden,
   "Path to directory for contractions tables."},
#endif /* ENABLE_CONTRACTED_BRAILLE */
  {'D', "data-directory", "directory", configureDataDirectory, OPT_Hidden,
   "Path to directory for driver help and configuration files."},
  {'E', "environment-variables", NULL, NULL, 0,
   "Recognize environment variables."},
  {'L', "library-directory", "directory", configureLibraryDirectory, OPT_Hidden,
   "Path to directory for loading drivers."},
  {'M', "message-delay", "csecs", NULL, 0,
   "Message hold time [400]."},
  {'N', "no-speech", NULL, NULL, 0,
   "Defer speech until restarted by command."},
  {'P', "pid-file", "file", NULL, 0,
   "Path to process identifier file."},
  {'R', "refresh-interval", "csecs", NULL, 0,
   "Braille window refresh interval [4]."},
#ifdef ENABLE_SPEECH_SUPPORT
  {'S', "speech-parameters", "arg,...", configureSpeechParameters, 0,
   "Parameters for the speech driver."},
#endif /* ENABLE_SPEECH_SUPPORT */
  {'T', "tables-directory", "directory", configureTablesDirectory, OPT_Hidden,
   "Path to directory for text and attributes tables."},
  {'X', "screen-parameters", "arg,...", configureScreenParameters, 0,
   "Parameters for the screen driver."},
END_OPTION_TABLE

static const char **bootParameters = NULL;
static int bootParameterCount = 0;
static char *
nextBootParameter (char **parameters) {
  const char delimiter = ',';
  char *parameter = *parameters;
  char *next;
  if (!*parameter) return NULL;
  if ((next = strchr(parameter, delimiter))) {
    *next = 0;
    parameter = strdupWrapper(parameter);
    *next = delimiter;
    *parameters = next + 1;
  } else {
    parameter = strdupWrapper(parameter);
    *parameters += strlen(parameter);
  }
  return parameter;
}

static void
prepareBootParameters (void) {
  char *string;
  int allocated = 0;
  if ((string = getBootParameters())) {
    allocated = 1;
  } else if (!(string = getenv("brltty"))) {
    return;
  }

  {
    int count = 0;
    char *parameters = string;
    char *parameter;
    while ((parameter = nextBootParameter(&parameters))) {
      ++count;
      if (*parameter) bootParameterCount = count;
      free(parameter);
    }
  }

  if (bootParameterCount) {
    int count = 0;
    char *parameters = string;
    char *parameter;
    bootParameters = mallocWrapper(bootParameterCount * sizeof(*bootParameters));
    while ((parameter = nextBootParameter(&parameters))) {
      if (*parameter) {
        bootParameters[count] = parameter;
      } else {
        bootParameters[count] = NULL;
        free(parameter);
      }
      if (++count == bootParameterCount) break;
    }
  }

  if (allocated) free(string);
}

static void
ensureOptionSetting (const char **setting, const char *defaultSetting, const char *configuredSetting, const char *environmentVariable, int bootParameter) {
  if (!*setting) {
    if ((bootParameter >= 0) && (bootParameter < bootParameterCount)) *setting = bootParameters[bootParameter];
    if (!*setting) {
      if (opt_environmentVariables && environmentVariable) *setting = getenv(environmentVariable);
      if (!*setting) *setting = configuredSetting? configuredSetting: defaultSetting;
    }
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

static int
loadTranslationTable (TranslationTable *table, const char *file, const char *name) {
  int ok = 0;
  char *path = makePath(opt_tablesDirectory, file);
  if (path) {
    int fd = open(path, O_RDONLY);
    if (fd >= 0) {
      TranslationTable buffer;
      if (read(fd, &buffer, sizeof(buffer)) == sizeof(buffer)) {
        memcpy(table, &buffer, sizeof(buffer));
        ok = 1;
      } else {
        LogPrint(LOG_ERR, "Cannot read %s translation table: %s", name, path);
      }
      close(fd);
    } else {
      LogPrint(LOG_ERR, "Cannot open %s translation table: %s: %s",
               name, path, strerror(errno));
    }
    free(path);
  }
  return ok;
}

static int
loadTextTable (const char *file) {
  if (!loadTranslationTable(&textTable, file, "text")) return 0;
  reverseTable(&textTable, &untextTable);
  return 1;
}

static int
loadAttributesTable (const char *file) {
  return loadTranslationTable(&attributesTable, file, "attributes");
}

static void
fixTablePath (const char **path, const char *prefix) {
  const char *extension = TABLE_EXTENSION;
  const unsigned int prefixLength = strlen(prefix);
  const unsigned int pathLength = strlen(*path);
  const unsigned int extensionLength = strlen(extension);
  char buffer[prefixLength + pathLength + extensionLength + 1];
  unsigned int length = 0;

  if (prefixLength) {
    if (!strchr(*path, '/')) {
      if (strncmp(*path, prefix, prefixLength) != 0) {
        memcpy(&buffer[length], prefix, prefixLength);
        length += prefixLength;
      }
    }
  }

  memcpy(&buffer[length], *path, pathLength);
  length += pathLength;

  if ((pathLength < extensionLength) ||
      (memcmp(&buffer[length-extensionLength], extension, extensionLength) != 0)) {
    memcpy(&buffer[length], extension, extensionLength);
    length += extensionLength;
  }

  if (length > pathLength) {
    buffer[length] = 0;
    *path = strdupWrapper(buffer);
  }
}

static int
changedTuneDevice (void) {
  return setTuneDevice(prefs.tuneDevice);
}

static void
dimensionsChanged (int rows, int columns) {
  fwinshift = MAX(columns-prefs.windowOverlap, 1);
  hwinshift = columns / 2;
  vwinshift = 5;
  LogPrint(LOG_DEBUG, "fwin=%d hwin=%d vwin=%d",
           fwinshift, hwinshift, vwinshift);
}

static int
changedWindowAttributes (void) {
  dimensionsChanged(brl.y, brl.x);
  return 1;
}

static void
changedPreferences (void) {
  changedWindowAttributes();
  changedTuneDevice();
}

int
getBrailleCommand (DriverCommandContext cmds) {
   while (1) {
      int key = readBrailleCommand(&brl, cmds);
      if (key != EOF) {
         LogPrint(LOG_DEBUG, "Command: %5.5X", key);
         if (key == CMD_NOOP) continue;
         return key;
      }
      delay(refreshInterval);
      closeTuneDevice(0);
   }
}

void
initializeBraille (void) {
   initializeBrailleDisplay(&brl);
   brl.bufferResized = &dimensionsChanged;
   brl.dataDirectory = opt_dataDirectory;
}

static void
getBrailleDriver (void) {
  if ((brailleDriver = loadBrailleDriver(&opt_brailleDriver, opt_libraryDirectory))) {
    processParameters(&brailleParameters, brailleDriver->parameters, "braille driver",
                      opt_brailleParameters, cfg_brailleParameters, "BRLTTY_BRAILLE_PARAMETERS");
  } else {
    LogPrint(LOG_ERR, "Bad braille driver selection: %s", opt_brailleDriver);
    fprintf(stderr, "\n");
    listBrailleDrivers(opt_libraryDirectory);
    fprintf(stderr, "\nUse -b to specify one, and -h for quick help.\n\n");

    /* not fatal */
    brailleDriver = &noBraille;
  }
  if (!opt_brailleDriver) opt_brailleDriver = "built-in";
}

void
startBrailleDriver (void) {
   while (1) {
      if (brailleDriver->open(&brl, brailleParameters, opt_brailleDevice)) {
         if (allocateBrailleBuffer(&brl)) {
            braille = brailleDriver;
            clearStatusCells();
            setHelpPageNumber(brl.helpPage);
            playTune(&tune_detected);
            return;
         } else {
            LogPrint(LOG_DEBUG, "Braille buffer allocation failed.");
         }
         brailleDriver->close(&brl);
      } else {
         LogPrint(LOG_DEBUG, "Braille driver initialization failed.");
      }

      initializeBraille();
      delay(5000);
   }
}

void
stopBrailleDriver (void) {
   braille = &noBraille;
   if (brl.isCoreBuffer) free(brl.buffer);
   brailleDriver->close(&brl);
   initializeBraille();
}

static void
exitBrailleDriver (void) {
   clearStatusCells();
   message("BRLTTY terminated.", MSG_NODELAY|MSG_SILENT);
   stopBrailleDriver();
}

#ifdef ENABLE_API
static void
exitApi (void) {
   api_close(&brl);
}
#endif /* ENABLE_API */

#ifdef ENABLE_SPEECH_SUPPORT
void
initializeSpeech (void) {
}

static void
getSpeechDriver (void) {
  if ((speechDriver = loadSpeechDriver(&opt_speechDriver, opt_libraryDirectory))) {
    processParameters(&speechParameters, speechDriver->parameters, "speech driver",
                      opt_speechParameters, cfg_speechParameters, "BRLTTY_SPEECH_PARAMETERS");
  } else {
    LogPrint(LOG_ERR, "Bad speech driver selection: %s", opt_speechDriver);
    fprintf(stderr, "\n");
    listSpeechDrivers(opt_libraryDirectory);
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
      speechDriver->open(speechParameters);
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
#endif /* ENABLE_SPEECH_SUPPORT */

#ifdef ENABLE_CONTRACTED_BRAILLE
static void
exitContractionTable (void) {
   if (contractionTable) {
      destroyContractionTable(contractionTable);
      contractionTable = NULL;
   }
}

static int
loadContractionTable (const char *file) {
  void *table = NULL;
  if (*file) {
    char *path = makePath(opt_contractionsDirectory, file);
    LogPrint(LOG_DEBUG, "Compiling contraction table: %s", file);
    if (path) {
      if (!(table = compileContractionTable(path))) {
        LogPrint(LOG_ERR, "Cannot compile contraction table: %s", path);
      }
      free(path);
    }
    if (!table) return 0;
  }
  if (contractionTable) destroyContractionTable(contractionTable);
  contractionTable = table;
  return 1;
}
#endif /* ENABLE_CONTRACTED_BRAILLE */

int
loadPreferences (int change) {
  int ok = 0;
  int fd = open(opt_preferencesFile, O_RDONLY);
  if (fd != -1) {
    Preferences newPreferences;
    int count = read(fd, &newPreferences, sizeof(newPreferences));
    if (count == sizeof(newPreferences)) {
      if ((newPreferences.magic[0] == (PREFS_MAGIC_NUMBER & 0XFF)) &&
          (newPreferences.magic[1] == (PREFS_MAGIC_NUMBER >> 8))) {
        prefs = newPreferences;
        ok = 1;
        if (prefs.version == 0) {
          prefs.version++;
          prefs.pcmVolume = DEFAULT_PCM_VOLUME;
          prefs.midiVolume = DEFAULT_MIDI_VOLUME;
          prefs.fmVolume = DEFAULT_FM_VOLUME;
        }
        if (prefs.version == 1) {
          prefs.version++;
          prefs.sayLineMode = DEFAULT_SAY_LINE_MODE;
          prefs.autospeak = DEFAULT_AUTOSPEAK;
        }
        if (change) changedPreferences();
      } else
        LogPrint(LOG_ERR, "Invalid preferences file: %s", opt_preferencesFile);
    } else if (count == -1) {
      LogPrint(LOG_ERR, "Cannot read preferences file: %s: %s",
               opt_preferencesFile, strerror(errno));
    } else {
      long int actualSize = sizeof(newPreferences);
      LogPrint(LOG_ERR, "Preferences file '%s' has incorrect size %d (should be %ld).",
               opt_preferencesFile, count, actualSize);
    }
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

#ifdef ENABLE_PREFERENCES_MENU
static int
testTunes () {
   return prefs.alertTunes;
}

#if defined(ENABLE_PCM_TUNES) || defined(ENABLE_MIDI_TUNES) || defined(ENABLE_FM_TUNES)
static int
changedVolume (unsigned char volume) {
  return (volume % 5) == 0;
}
#endif /* defined(ENABLE_PCM_TUNES) || defined(ENABLE_MIDI_TUNES) || defined(ENABLE_FM_TUNES) */

#ifdef ENABLE_PCM_TUNES
static int
testTunesPcm () {
   return testTunes() && (prefs.tuneDevice == tdPcm);
}

static int
changedPcmVolume (void) {
  return changedVolume(prefs.pcmVolume);
}
#endif /* ENABLE_PCM_TUNES */

#ifdef ENABLE_MIDI_TUNES
static int
testTunesMidi () {
   return testTunes() && (prefs.tuneDevice == tdMidi);
}

static int
changedMidiVolume (void) {
  return changedVolume(prefs.midiVolume);
}
#endif /* ENABLE_MIDI_TUNES */

#ifdef ENABLE_FM_TUNES
static int
testTunesFm () {
   return testTunes() && (prefs.tuneDevice == tdFm);
}

static int
changedFmVolume (void) {
  return changedVolume(prefs.fmVolume);
}
#endif /* ENABLE_FM_TUNES */

#ifdef ENABLE_TABLE_SELECTION
typedef struct {
  const char *directory;
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

static void
globPrepare (GlobData *data, const char *directory, const char *pattern, const char *initial, int none) {
  memset(data, 0, sizeof(*data));
  data->directory = directory;
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

  {
    int originalDirectory = open(".", O_RDONLY);
    if (originalDirectory != -1) {
      if (chdir(data->directory) != -1) {
        if (glob(data->pattern, GLOB_DOOFFS, NULL, &data->glob) == 0) {
          data->paths = (const char **)data->glob.gl_pathv;
          /* The behaviour of gl_pathc is inconsistent. Some implementations
           * include the leading NULL pointers and some don't. Let's just
           * figure it out the hard way by finding the trailing NULL.
           */
          data->count = data->glob.gl_offs;
          while (data->paths[data->count]) ++data->count;
        } else {
          data->paths = data->pathsArea;
          data->paths[data->count = data->glob.gl_offs] = NULL;
        }
        if (fchdir(originalDirectory) == -1) {
          LogError("working directory restore");
        }
      } else {
        LogPrint(LOG_ERR, "Cannot set working directory: %s: %s",
                 data->directory, strerror(errno));
      }
      close(originalDirectory);
    } else {
      LogError("working directory open");
    }
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
    int index;
    for (index=0; index<data->glob.gl_offs; ++index)
      data->glob.gl_pathv[index] = NULL;
    globfree(&data->glob);
  }
}

static const char *
globChanged (GlobData *data) {
  char *path = strdup(data->paths[data->setting]);
  if (path) {
    free(data->current);
    return data->current = path;
  } else {
    LogError("Memory allocation");
  }
  return NULL;
}

static int
changedTextTable (void) {
  return loadTextTable(globChanged(&glob_textTable));
}

static int
changedAttributesTable (void) {
  return loadAttributesTable(globChanged(&glob_attributesTable));
}

#ifdef ENABLE_CONTRACTED_BRAILLE
static int
changedContractionTable (void) {
  return loadContractionTable(globChanged(&glob_contractionTable));
}
#endif /* ENABLE_CONTRACTED_BRAILLE */
#endif /* ENABLE_TABLE_SELECTION */

static int
testSkipBlankWindows () {
   return prefs.skipBlankWindows;
}

static int
testSlidingWindow () {
   return prefs.slidingWindow;
}

static int
testShowCursor () {
   return prefs.showCursor;
}

static int
testBlinkingCursor () {
   return testShowCursor() && prefs.blinkingCursor;
}

static int
testShowAttributes () {
   return prefs.showAttributes;
}

static int
testBlinkingAttributes () {
   return testShowAttributes() && prefs.blinkingAttributes;
}

static int
testBlinkingCapitals () {
   return prefs.blinkingCapitals;
}

void
updatePreferences (void) {
#ifdef ENABLE_TABLE_SELECTION
  globBegin(&glob_textTable);
  globBegin(&glob_attributesTable);
  globBegin(&glob_contractionTable);
#endif /* ENABLE_TABLE_SELECTION */

  {
    static unsigned char exitSave = 0;                /* 1 == save preferences on exit */
    static const char *booleanValues[] = {"No", "Yes"};
    static const char *cursorStyles[] = {"Underline", "Block"};
    static const char *metaModes[] = {"Escape Prefix", "High-order Bit"};
    static const char *skipBlankWindowsModes[] = {"All", "End of Line", "Rest of Line"};
    static const char *statusStyles[] = {"None", "Alva", "Tieman", "PowerBraille 80", "Generic", "MDV", "Voyager"};
    static const char *textStyles[] = {"8-dot", "6-dot"};
    static const char *tuneDevices[] = {
      "Beeper ("
#ifdef ENABLE_BEEPER_TUNES
        "console tone generator"
#else /* ENABLE_BEEPER_TUNES */
        "unsupported"
#endif /* ENABLE_BEEPER_TUNES */
        ")",

      "PCM ("
#ifdef ENABLE_PCM_TUNES
        "soundcard digital audio"
#else /* ENABLE_PCM_TUNES */
        "unsupported"
#endif /* ENABLE_PCM_TUNES */
        ")",

      "MIDI ("
#ifdef ENABLE_MIDI_TUNES
        "Musical Instrument Digital Interface"
#else /* ENABLE_MIDI_TUNES */
        "unsupported"
#endif /* ENABLE_MIDI_TUNES */
        ")",

      "FM ("
#ifdef ENABLE_FM_TUNES
        "soundcard synthesizer"
#else /* ENABLE_FM_TUNES */
        "unsupported"
#endif /* ENABLE_FM_TUNES */
        ")"
    };
#ifdef ENABLE_SPEECH_SUPPORT
    static const char *sayModes[] = {"Immediate", "Enqueue"};
#endif /* ENABLE_SPEECH_SUPPORT */
    typedef struct {
       unsigned char *setting;                        /* pointer to the item value */
       int (*changed) (void);
       int (*test) (void);
       const char *description;                        /* item description */
       const char *const*names;                        /* 0 == numeric, 1 == bolean */
       unsigned char minimum;                        /* minimum range */
       unsigned char maximum;                        /* maximum range */
    } MenuItem;
    #define MENU_ITEM(setting, changed, test, description, values, minimum, maximum) {&setting, changed, test, description, values, minimum, maximum}
    #define NUMERIC_ITEM(setting, changed, test, description, minimum, maximum) MENU_ITEM(setting, changed, test, description, NULL, minimum, maximum)
    #define TIMING_ITEM(setting, changed, test, description) NUMERIC_ITEM(setting, changed, test, description, 1, 16)
    #define VOLUME_ITEM(setting, changed, test, description) NUMERIC_ITEM(setting, changed, test, description, 0, 100)
    #define TEXT_ITEM(setting, changed, test, description, names, count) MENU_ITEM(setting, changed, test, description, names, 0, count-1)
    #define SYMBOLIC_ITEM(setting, changed, test, description, names) TEXT_ITEM(setting, changed, test, description, names, ((sizeof(names) / sizeof(names[0]))))
    #define BOOLEAN_ITEM(setting, changed, test, description) SYMBOLIC_ITEM(setting, changed, test, description, booleanValues)
    #define GLOB_ITEM(data, changed, test, description) TEXT_ITEM(data.setting, changed, test, description, data.paths, data.count)
    MenuItem menu[] = {
       BOOLEAN_ITEM(exitSave, NULL, NULL, "Save on Exit"),
       SYMBOLIC_ITEM(prefs.textStyle, NULL, NULL, "Text Style", textStyles),
       SYMBOLIC_ITEM(prefs.metaMode, NULL, NULL, "Meta Mode", metaModes),
       BOOLEAN_ITEM(prefs.skipIdenticalLines, NULL, NULL, "Skip Identical Lines"),
       BOOLEAN_ITEM(prefs.skipBlankWindows, NULL, NULL, "Skip Blank Windows"),
       SYMBOLIC_ITEM(prefs.blankWindowsSkipMode, NULL, testSkipBlankWindows, "Which Blank Windows", skipBlankWindowsModes),
       BOOLEAN_ITEM(prefs.slidingWindow, NULL, NULL, "Sliding Window"),
       BOOLEAN_ITEM(prefs.eagerSlidingWindow, NULL, testSlidingWindow, "Eager Sliding Window"),
       NUMERIC_ITEM(prefs.windowOverlap, changedWindowAttributes, NULL, "Window Overlap", 0, 20),
       BOOLEAN_ITEM(prefs.showCursor, NULL, NULL, "Show Cursor"),
       SYMBOLIC_ITEM(prefs.cursorStyle, NULL, testShowCursor, "Cursor Style", cursorStyles),
       BOOLEAN_ITEM(prefs.blinkingCursor, NULL, testShowCursor, "Blinking Cursor"),
       TIMING_ITEM(prefs.cursorVisiblePeriod, NULL, testBlinkingCursor, "Cursor Visible Period"),
       TIMING_ITEM(prefs.cursorInvisiblePeriod, NULL, testBlinkingCursor, "Cursor Invisible Period"),
       BOOLEAN_ITEM(prefs.showAttributes, NULL, NULL, "Show Attributes"),
       BOOLEAN_ITEM(prefs.blinkingAttributes, NULL, testShowAttributes, "Blinking Attributes"),
       TIMING_ITEM(prefs.attributesVisiblePeriod, NULL, testBlinkingAttributes, "Attributes Visible Period"),
       TIMING_ITEM(prefs.attributesInvisiblePeriod, NULL, testBlinkingAttributes, "Attributes Invisible Period"),
       BOOLEAN_ITEM(prefs.blinkingCapitals, NULL, NULL, "Blinking Capitals"),
       TIMING_ITEM(prefs.capitalsVisiblePeriod, NULL, testBlinkingCapitals, "Capitals Visible Period"),
       TIMING_ITEM(prefs.capitalsInvisiblePeriod, NULL, testBlinkingCapitals, "Capitals Invisible Period"),
#ifdef HAVE_LIBGPM
       BOOLEAN_ITEM(prefs.windowFollowsPointer, NULL, NULL, "Window Follows Pointer"),
       BOOLEAN_ITEM(prefs.pointerFollowsWindow, NULL, NULL, "Pointer Follows Window"),
#endif /* HAVE_LIBGPM */
       BOOLEAN_ITEM(prefs.alertTunes, NULL, NULL, "Alert Tunes"),
       SYMBOLIC_ITEM(prefs.tuneDevice, changedTuneDevice, testTunes, "Tune Device", tuneDevices),
#ifdef ENABLE_PCM_TUNES
       VOLUME_ITEM(prefs.pcmVolume, changedPcmVolume, testTunesPcm, "PCM Volume"),
#endif /* ENABLE_PCM_TUNES */
#ifdef ENABLE_MIDI_TUNES
       VOLUME_ITEM(prefs.midiVolume, changedMidiVolume, testTunesMidi, "MIDI Volume"),
       TEXT_ITEM(prefs.midiInstrument, NULL, testTunesMidi, "MIDI Instrument", midiInstrumentTable, midiInstrumentCount),
#endif /* ENABLE_MIDI_TUNES */
#ifdef ENABLE_FM_TUNES
       VOLUME_ITEM(prefs.fmVolume, changedFmVolume, testTunesFm, "FM Volume"),
#endif /* ENABLE_FM_TUNES */
       BOOLEAN_ITEM(prefs.alertDots, NULL, NULL, "Alert Dots"),
       BOOLEAN_ITEM(prefs.alertMessages, NULL, NULL, "Alert Messages"),
#ifdef ENABLE_SPEECH_SUPPORT
       SYMBOLIC_ITEM(prefs.sayLineMode, NULL, NULL, "Say-Line Mode", sayModes),
       BOOLEAN_ITEM(prefs.autospeak, NULL, NULL, "Autospeak"),
#endif /* ENABLE_SPEECH_SUPPORT */
       SYMBOLIC_ITEM(prefs.statusStyle, NULL, NULL, "Status Style", statusStyles),
#ifdef ENABLE_TABLE_SELECTION
       GLOB_ITEM(glob_textTable, changedTextTable, NULL, "Text Table"),
       GLOB_ITEM(glob_attributesTable, changedAttributesTable, NULL, "Attributes Table"),
#ifdef ENABLE_CONTRACTED_BRAILLE
       GLOB_ITEM(glob_contractionTable, changedContractionTable, NULL, "Contraction Table")
#endif /* ENABLE_CONTRACTED_BRAILLE */
#endif /* ENABLE_TABLE_SELECTION */
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
      if (settingChanged) {
        settingChanged = 0;
        /* make sure the updated value is visible */
        if ((lineLength-lineIndent > brl.x*brl.y) && (lineIndent < settingIndent))
          lineIndent = settingIndent;
      }

      /* Then draw the braille window */
      writeBrailleText(&brl, &line[lineIndent], MAX(0, lineLength-lineIndent));
      drainBrailleOutput(&brl, refreshInterval);

      /* Now process any user interaction */
      switch (key = getBrailleCommand(CMDS_PREFS)) {
        case CMD_TOP:
        case CMD_TOP_LEFT:
        case VAL_PASSKEY+VPK_PAGE_UP:
        case CMD_MENU_FIRST_ITEM:
          menuIndex = lineIndent = 0;
          break;
        case CMD_BOT:
        case CMD_BOT_LEFT:
        case VAL_PASSKEY+VPK_PAGE_DOWN:
        case CMD_MENU_LAST_ITEM:
          menuIndex = menuSize - 1;
          lineIndent = 0;
          break;
        case CMD_LNUP:
        case VAL_PASSKEY+VPK_CURSOR_UP:
        case CMD_MENU_PREV_ITEM:
          do {
            if (menuIndex == 0) menuIndex = menuSize;
            --menuIndex;
          } while (menu[menuIndex].test && !menu[menuIndex].test());
          lineIndent = 0;
          break;
        case CMD_LNDN:
        case VAL_PASSKEY+VPK_CURSOR_DOWN:
        case CMD_MENU_NEXT_ITEM:
          do {
            if (++menuIndex == menuSize) menuIndex = 0;
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
        case VAL_PASSKEY+VPK_CURSOR_LEFT:
        case CMD_MENU_PREV_SETTING: {
          int count = item->maximum - item->minimum + 1;
          do {
            if ((*item->setting)-- <= item->minimum) *item->setting = item->maximum;
            if (!--count) break;
          } while (item->changed && !item->changed());
          if (count)
            settingChanged = 1;
          else
            playTune(&tune_bad_command);
          break;
        }
        case CMD_WINDN:
        case CMD_CHRRT:
        case VAL_PASSKEY+VPK_CURSOR_RIGHT:
        case CMD_HOME:
        case VAL_PASSKEY+VPK_RETURN:
        case CMD_MENU_NEXT_SETTING: {
          int count = item->maximum - item->minimum + 1;
          do {
            if ((*item->setting)++ >= item->maximum) *item->setting = item->minimum;
            if (!--count) break;
          } while (item->changed && !item->changed());
          if (count)
            settingChanged = 1;
          else
            playTune(&tune_bad_command);
          break;
        }
#ifdef ENABLE_SPEECH_SUPPORT
        case CMD_SAY_LINE:
          speech->say(line, lineLength);
          break;
        case CMD_MUTE:
          speech->mute();
          break;
#endif /* ENABLE_SPEECH_SUPPORT */
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
            /* Why not support setting a value with routing keys. */
            unsigned char oldSetting = *item->setting;
            key -= CR_ROUTE;
            if (item->names) {
              *item->setting = key % (item->maximum + 1);
            } else {
              *item->setting = key;
              if (*item->setting > item->maximum) *item->setting = item->maximum;
              if (*item->setting < item->minimum) *item->setting = item->minimum;
            }
            if (*item->setting != oldSetting) {
              if (item->changed && !item->changed()) {
                *item->setting = oldSetting;
                playTune(&tune_bad_command);
              } else {
                settingChanged = 1;
              }
            }
            break;
          }

          /* For any other keystroke, we exit */
        exitMenu:
          if (exitSave) {
            if (savePreferences()) {
              playTune(&tune_done);
            }
          }

#ifdef ENABLE_TABLE_SELECTION
          globEnd(&glob_textTable);
          globEnd(&glob_attributesTable);
          globEnd(&glob_contractionTable);
#endif /* ENABLE_TABLE_SELECTION */

          return;
      }
    }
  }
}
#endif /* ENABLE_PREFERENCES_MENU */

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
parentExit0 (int signalNumber) {
  _exit(0);
}

static void
background (void) {
   pid_t child;

   fflush(stdout);
   fflush(stderr);

   switch (child = fork()) {
      case -1: /* error */
         LogPrint(LOG_CRIT, "Process creation error: %s", strerror(errno));
         exit(10);

      case 0: /* child */
         break;

      default: /* parent */
         while (1) {
            int status;
            if (waitpid(child, &status, 0) == -1) {
               if (errno == EINTR) continue;
               LogPrint(LOG_CRIT, "waitpid error: %s", strerror(errno));
               _exit(11);
            }
            if (WIFEXITED(status)) _exit(WEXITSTATUS(status));
            if (WIFSIGNALED(status)) _exit(WTERMSIG(status) | 0X80);
         }
   }

   if (!opt_standardError) {
      LogClose();
      LogOpen(1);
   }
}

static int
handleOption (const int option) {
  switch (option) {
    default:
      return 0;
    case 'a':                /* text translation table file name */
      opt_attributesTable = optarg;
      break;
    case 'b':                        /* name of driver */
      opt_brailleDriver = optarg;
      break;
#ifdef ENABLE_CONTRACTED_BRAILLE
    case 'c':                        /* name of driver */
      opt_contractionTable = optarg;
      break;
#endif /* ENABLE_CONTRACTED_BRAILLE */
    case 'd':                /* serial device path */
      opt_brailleDevice = optarg;
      break;
    case 'e':                /* help */
      opt_standardError = 1;
      break;
    case 'f':                /* configuration file path */
      opt_configurationFile = optarg;
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
#ifdef ENABLE_SPEECH_SUPPORT
    case 's':                        /* name of speech driver */
      opt_speechDriver = optarg;
      break;
#endif /* ENABLE_SPEECH_SUPPORT */
    case 't':                /* text translation table file name */
      opt_textTable = optarg;
      break;
    case 'v':                /* version */
      opt_version = 1;
      break;
#ifdef ENABLE_API
    case 'A':	/* parameters for application programming interface */
      extendParameters(&opt_apiParameters, optarg);
      break;
#endif /* ENABLE_API */
    case 'B':                        /* parameters for braille driver */
      extendParameters(&opt_brailleParameters, optarg);
      break;
#ifdef ENABLE_CONTRACTED_BRAILLE
    case 'C':                        /* path to contraction tables directory */
      opt_contractionsDirectory = optarg;
      break;
#endif /* ENABLE_CONTRACTED_BRAILLE */
    case 'D':                        /* path to driver help/configuration files directory */
      opt_dataDirectory = optarg;
      break;
    case 'E':                        /* parameter to speech driver */
      opt_environmentVariables = 1;
      break;
    case 'L':                        /* path to drivers directory */
      opt_libraryDirectory = optarg;
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
#ifdef ENABLE_SPEECH_SUPPORT
    case 'S':                        /* parameters for speech driver */
      extendParameters(&opt_speechParameters, optarg);
      break;
#endif /* ENABLE_SPEECH_SUPPORT */
    case 'T':                        /* path to text/attributes tables directory */
      opt_tablesDirectory = optarg;
      break;
    case 'X':                        /* parameters for screen driver */
      extendParameters(&opt_screenParameters, optarg);
      break;
  }
  return 1;
}

void
startup (int argc, char *argv[]) {
  processOptions(optionTable, optionCount, handleOption,
                 &argc, &argv, NULL);
  prepareBootParameters();
  initializeAllScreens();

  /* Set logging levels. */
  if (opt_standardError)
    LogClose();
  setLogLevel(opt_logLevel);
  setPrintLevel(opt_version?
                    (opt_quiet? LOG_NOTICE: LOG_INFO):
                    (opt_quiet? LOG_WARNING: LOG_NOTICE));

  LogPrint(LOG_NOTICE, "%s %s", PACKAGE_TITLE, PACKAGE_VERSION);
  LogPrint(LOG_INFO, "%s", COPYRIGHT);

  if (!opt_noDaemon) {
    const int signalNumber = SIGUSR1;
    struct sigaction newAction, oldAction;
    memset(&newAction, 0, sizeof(newAction));
    sigemptyset(&newAction.sa_mask);
    newAction.sa_handler = parentExit0;
    if (sigaction(signalNumber, &newAction, &oldAction) != -1) {
      sigset_t newMask, oldMask;
      sigemptyset(&newMask);
      sigaddset(&newMask, SIGINT);
      sigaddset(&newMask, SIGQUIT);
      sigaddset(&newMask, SIGHUP);
      sigprocmask(SIG_BLOCK, &newMask, &oldMask);
      background(); /* first fork */
      background(); /* second fork */
      sigprocmask(SIG_SETMASK, &oldMask, NULL);
      sigaction(signalNumber, &oldAction, NULL);
    } else {
      LogError("signal set");
      opt_noDaemon = 1;
    }
  }

  /* Create the process identifier file. */
  if (opt_pidFile) {
    FILE *stream = fopen(opt_pidFile, "w");
    if (stream) {
      long pid = getpid();
      fprintf(stream, "%ld\n", pid);
      fclose(stream);
      atexit(exitPidFile);
    } else {
      LogPrint(LOG_ERR, "Cannot open process identifier file: %s: %s",
               opt_pidFile, strerror(errno));
    }
  }

  /* Process the configuration file. */
  {
    int optional = opt_configurationFile == NULL;
    ensureOptionSetting(&opt_configurationFile,
                        CONFIGURATION_DIRECTORY "/" CONFIGURATION_FILE,
                        NULL, "BRLTTY_CONFIGURATION_FILE", -1);
    processConfigurationFile(optionTable, optionCount, opt_configurationFile, optional);
  }
  ensureOptionSetting(&opt_preferencesFile, NULL, cfg_preferencesFile, "BRLTTY_PREFERENCES_FILE", -1);
  ensureOptionSetting(&opt_tablesDirectory, DATA_DIRECTORY, cfg_tablesDirectory, "BRLTTY_TABLES_DIRECTORY", -1);
  ensureOptionSetting(&opt_textTable, NULL, cfg_textTable, "BRLTTY_TEXT_TABLE", 2);
  ensureOptionSetting(&opt_attributesTable, NULL, cfg_attributesTable, "BRLTTY_ATTRIBUTES_TABLE", -1);
#ifdef ENABLE_CONTRACTED_BRAILLE
  ensureOptionSetting(&opt_contractionsDirectory, DATA_DIRECTORY, cfg_contractionsDirectory, "BRLTTY_CONTRACTIONS_DIRECTORY", -1);
  ensureOptionSetting(&opt_contractionTable, NULL, cfg_contractionTable, "BRLTTY_CONTRACTION_TABLE", -1);
#endif /* ENABLE_CONTRACTED_BRAILLE */
  ensureOptionSetting(&opt_libraryDirectory, LIBRARY_DIRECTORY, cfg_libraryDirectory, "BRLTTY_LIBRARY_DIRECTORY", -1);
  ensureOptionSetting(&opt_dataDirectory, DATA_DIRECTORY, cfg_dataDirectory, "BRLTTY_DATA_DIRECTORY", -1);
  ensureOptionSetting(&opt_brailleDriver, NULL, cfg_brailleDriver, "BRLTTY_BRAILLE_DRIVER", 0);
  ensureOptionSetting(&opt_brailleDevice, NULL, cfg_brailleDevice, "BRLTTY_BRAILLE_DEVICE", 1);
#ifdef ENABLE_SPEECH_SUPPORT
  ensureOptionSetting(&opt_speechDriver, NULL, cfg_speechDriver, "BRLTTY_SPEECH_DRIVER", -1);
#endif /* ENABLE_SPEECH_SUPPORT */

  if (!opt_brailleDevice) opt_brailleDevice = BRAILLE_DEVICE;
  if (*opt_brailleDevice == 0) {
    LogPrint(LOG_CRIT, "No braille device specified.");
    fprintf(stderr, "Use -d to specify one.\n");
    exit(4);
  }
  if (*opt_brailleDevice != '/') {
    const char *directory = DEVICE_DIRECTORY;
    char buffer[strlen(directory) + 1 + strlen(opt_brailleDevice) + 1];
    sprintf(buffer, "%s/%s", directory, opt_brailleDevice);
    opt_brailleDevice = strdupWrapper(buffer);
  }

  getBrailleDriver();
#ifdef ENABLE_SPEECH_SUPPORT
  getSpeechDriver();
#endif /* ENABLE_SPEECH_SUPPORT */
#ifdef ENABLE_API
  processParameters(&apiParameters, api_parameters, "application programming interface",
                    opt_apiParameters, cfg_apiParameters, "BRLTTY_API_PARAMETERS");
#endif /* ENABLE_API */
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

  {
    const char *directories[] = {DATA_DIRECTORY, "/etc", "/", NULL};
    const char **directory = directories;
    while (*directory) {
      if (chdir(*directory) != -1) break;                /* * change to directory containing data files  */
      LogPrint(LOG_WARNING, "Cannot change directory to '%s': %s",
               *directory, strerror(errno));
      ++directory;
    }
  }

  if (opt_textTable) {
    fixTablePath(&opt_textTable, TEXT_TABLE_PREFIX);
    loadTextTable(opt_textTable);
  } else {
    opt_textTable = TEXT_TABLE;
    reverseTable(&textTable, &untextTable);
  }
#ifdef ENABLE_PREFERENCES_MENU
#ifdef ENABLE_TABLE_SELECTION
  globPrepare(&glob_textTable, opt_tablesDirectory, "text.*.tbl", opt_textTable, 0);
#endif /* ENABLE_TABLE_SELECTION */
#endif /* ENABLE_PREFERENCES_MENU */

  if (opt_attributesTable) {
    loadAttributesTable(opt_attributesTable);
  } else
    opt_attributesTable = ATTRIBUTES_TABLE;
#ifdef ENABLE_PREFERENCES_MENU
#ifdef ENABLE_TABLE_SELECTION
  globPrepare(&glob_attributesTable, opt_tablesDirectory, "attr*.tbl", opt_attributesTable, 0);
#endif /* ENABLE_TABLE_SELECTION */
#endif /* ENABLE_PREFERENCES_MENU */

#ifdef ENABLE_CONTRACTED_BRAILLE
#ifdef ENABLE_PREFERENCES_MENU
#ifdef ENABLE_TABLE_SELECTION
  globPrepare(&glob_contractionTable, opt_contractionsDirectory, "*.ctb", opt_contractionTable, 1);
#endif /* ENABLE_TABLE_SELECTION */
#endif /* ENABLE_PREFERENCES_MENU */
  if (opt_contractionTable) loadContractionTable(opt_contractionTable);
  atexit(exitContractionTable);
#endif /* ENABLE_CONTRACTED_BRAILLE */

  {
    char buffer[PATH_MAX+1];
    char *path = getcwd(buffer, sizeof(buffer));
    LogPrint(LOG_INFO, "Working Directory: %s",
             path? path: "path-too-long");
  }
  LogPrint(LOG_INFO, "Configuration File: %s", opt_configurationFile);
  LogPrint(LOG_INFO, "Preferences File: %s", opt_preferencesFile);
  LogPrint(LOG_INFO, "Tables Directory: %s", opt_tablesDirectory);
  LogPrint(LOG_INFO, "Text Table: %s", opt_textTable);
  LogPrint(LOG_INFO, "Attributes Table: %s", opt_attributesTable);
#ifdef ENABLE_CONTRACTED_BRAILLE
  LogPrint(LOG_INFO, "Contractions Directory: %s", opt_contractionsDirectory);
  LogPrint(LOG_INFO, "Contraction Table: %s",
           opt_contractionTable? opt_contractionTable: "none");
#endif /* ENABLE_CONTRACTED_BRAILLE */
#ifdef ENABLE_API
  logParameters(api_parameters, apiParameters, "API");
#endif /* ENABLE_API */
  LogPrint(LOG_INFO, "Library Directory: %s", opt_libraryDirectory);
  LogPrint(LOG_INFO, "Data Directory: %s", opt_dataDirectory);
  LogPrint(LOG_INFO, "Help File: %s", brailleDriver->helpFile);
  LogPrint(LOG_INFO, "Braille Driver: %s (%s)",
           opt_brailleDriver, brailleDriver->name);
  LogPrint(LOG_INFO, "Braille Device: %s", opt_brailleDevice);
  logParameters(brailleDriver->parameters, brailleParameters, "Braille");
#ifdef ENABLE_SPEECH_SUPPORT
  LogPrint(LOG_INFO, "Speech Driver: %s (%s)",
           opt_speechDriver, speechDriver->name);
  logParameters(speechDriver->parameters, speechParameters, "Speech");
#endif /* ENABLE_SPEECH_SUPPORT */
  logParameters(getScreenParameters(), screenParameters, "Screen");

#ifdef ENABLE_API
  api_identify();
#endif /* ENABLE_API */
  brailleDriver->identify();
#ifdef ENABLE_SPEECH_SUPPORT
  speechDriver->identify();
#endif /* ENABLE_SPEECH_SUPPORT */

  if (opt_version) exit(0);

  /* Load preferences file */
  if (!loadPreferences(0)) {
    memset(&prefs, 0, sizeof(prefs));

    prefs.magic[0] = PREFS_MAGIC_NUMBER & 0XFF;
    prefs.magic[1] = PREFS_MAGIC_NUMBER >> 8;
    prefs.version = 2;

    prefs.showCursor = DEFAULT_SHOW_CURSOR;
    prefs.cursorStyle = DEFAULT_CURSOR_STYLE;
    prefs.blinkingCursor = DEFAULT_BLINKING_CURSOR;
    prefs.cursorVisiblePeriod = DEFAULT_CURSOR_VISIBLE_PERIOD;
    prefs.cursorInvisiblePeriod = DEFAULT_CURSOR_INVISIBLE_PERIOD;

    prefs.showAttributes = DEFAULT_SHOW_ATTRIBUTES;
    prefs.blinkingAttributes = DEFAULT_BLINKING_ATTRIBUTES;
    prefs.attributesVisiblePeriod = DEFAULT_ATTRIBUTES_VISIBLE_PERIOD;
    prefs.attributesInvisiblePeriod = DEFAULT_ATTRIBUTES_INVISIBLE_PERIOD;

    prefs.blinkingCapitals = DEFAULT_BLINKING_CAPITALS;
    prefs.capitalsVisiblePeriod = DEFAULT_CAPITALS_VISIBLE_PERIOD;
    prefs.capitalsInvisiblePeriod = DEFAULT_CAPITALS_INVISIBLE_PERIOD;

    prefs.windowFollowsPointer = DEFAULT_WINDOW_FOLLOWS_POINTER;
    prefs.pointerFollowsWindow = DEFAULT_POINTER_FOLLOWS_WINDOW;

    prefs.textStyle = DEFAULT_TEXT_STYLE;
    prefs.metaMode = DEFAULT_META_MODE;

    prefs.windowOverlap = DEFAULT_WINDOW_OVERLAP;
    prefs.slidingWindow = DEFAULT_SLIDING_WINDOW;
    prefs.eagerSlidingWindow = DEFAULT_EAGER_SLIDING_WINDOW;

    prefs.skipIdenticalLines = DEFAULT_SKIP_IDENTICAL_LINES;
    prefs.skipBlankWindows = DEFAULT_SKIP_BLANK_WINDOWS;
    prefs.blankWindowsSkipMode = DEFAULT_BLANK_WINDOWS_SKIP_MODE;

    prefs.alertMessages = DEFAULT_ALERT_MESSAGES;
    prefs.alertDots = DEFAULT_ALERT_DOTS;
    prefs.alertTunes = DEFAULT_ALERT_TUNES;
    prefs.tuneDevice = getDefaultTuneDevice();
    prefs.pcmVolume = DEFAULT_PCM_VOLUME;
    prefs.midiVolume = DEFAULT_MIDI_VOLUME;
    prefs.midiInstrument = DEFAULT_MIDI_INSTRUMENT;
    prefs.fmVolume = DEFAULT_FM_VOLUME;

    prefs.sayLineMode = DEFAULT_SAY_LINE_MODE;
    prefs.autospeak = DEFAULT_AUTOSPEAK;

    prefs.statusStyle = brailleDriver->statusStyle;
  }
  changedTuneDevice();
  atexit(exitTunes);

  /*
   * Initialize screen library 
   */
  if (!openLiveScreen(screenParameters)) {                                
    LogPrint(LOG_CRIT, "Cannot read screen.");
    exit(7);
  }
  atexit(exitScreen);
  
  if (!opt_noDaemon) {
    setPrintOff();

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

    /* tell the parent process to exit */
    if (kill(getppid(), SIGUSR1) == -1) {
      LogPrint(LOG_CRIT, "Stop parent error: %s", strerror(errno));
      exit(12);
    }

    /* request a new session (job control) */
    if (setsid() == -1) {                        
      LogPrint(LOG_CRIT, "Session creation error: %s", strerror(errno));
      exit(13);
    }
  }
  /*
   * From this point, all IO functions as printf, puts, perror, etc. can't be
   * used anymore since we are a daemon.  The LogPrint facility should 
   * be used instead.
   */

  /* Activate the braille display. */
  initializeBraille();
  startBrailleDriver();
  atexit(exitBrailleDriver);

#ifdef ENABLE_API
  /* Activate the application programming interface. */
  api_open(&brl, apiParameters);
  atexit(exitApi);
#endif /* ENABLE_API */

#ifdef ENABLE_SPEECH_SUPPORT
  /* Activate the speech synthesizer. */
  initializeSpeech();
  startSpeechDriver();
  atexit(exitSpeechDriver);
#endif /* ENABLE_SPEECH_SUPPORT */

  /* Initialize the braille driver help screen. */
  {
    char *path = makePath(opt_dataDirectory, brailleDriver->helpFile);
    if (path) {
      if (openHelpScreen(path))
        LogPrint(LOG_INFO, "Help Page: %s[%d]", path, getHelpPageNumber());
      else
        LogPrint(LOG_WARNING, "Cannot open help file: %s", path);
      free(path);
    }
  }

  if (!opt_quiet) {
    char buffer[18 + 1];
    snprintf(buffer, sizeof(buffer), "%s %s", PACKAGE_TITLE, PACKAGE_VERSION);
    message(buffer, 0);        /* display initialization message */
  }

  if (loggedProblemCount) {
    char buffer[0X40];
    snprintf(buffer, sizeof(buffer), "%d startup problem%s",
             loggedProblemCount,
             (loggedProblemCount==1? "": "s"));
    message(buffer, MSG_WAITKEY);
  }
}
