/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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
#include "spk.h"
#include "scr.h"
#include "tbl.h"
#include "ctb.h"
#include "tunes.h"
#include "message.h"
#include "misc.h"
#include "system.h"
#include "options.h"
#include "brltty.h"
#include "defaults.h"
#include "serial.h"

#ifdef ENABLE_USB_SUPPORT
#include "usb.h"
#endif /* ENABLE_USB_SUPPORT */

#ifdef ENABLE_BLUETOOTH_SUPPORT
#include "bluez.h"
#endif /* ENABLE_BLUETOOTH_SUPPORT */

char COPYRIGHT[] = "Copyright (C) 1995-2004 by The BRLTTY Team - all rights reserved.";

static short opt_version = 0;
static short opt_quiet = 0;
static short opt_noDaemon = 0;
static short opt_standardError = 0;
static short opt_logLevel = LOG_NOTICE;
static short opt_environmentVariables = 0;

static const char *opt_configurationFile = NULL;
static const char *opt_preferencesFile = NULL;
static const char *opt_pidFile = NULL;
static const char *opt_dataDirectory = NULL;
static const char *opt_libraryDirectory = NULL;

static const char *opt_brailleDriver = NULL;
static const BrailleDriver *brailleDriver;
static char *opt_brailleParameters = NULL;
static char **brailleParameters = NULL;
static const char *opt_brailleDevice = NULL;

static const char *opt_tablesDirectory = NULL;
static const char *opt_textTable = NULL;
static const char *opt_attributesTable = NULL;

#ifdef ENABLE_CONTRACTED_BRAILLE
static const char *opt_contractionsDirectory = NULL;
static const char *opt_contractionTable = NULL;
#endif /* ENABLE_CONTRACTED_BRAILLE */

#ifdef ENABLE_API
static char *opt_apiParameters = NULL;
static char **apiParameters = NULL;
static int apiOpened;
#endif /* ENABLE_API */

#ifdef ENABLE_SPEECH_SUPPORT
static const char *opt_speechDriver = NULL;
static const SpeechDriver *speechDriver;
static char *opt_speechParameters = NULL;
static char **speechParameters = NULL;
static const char *opt_speechFifo = NULL;
static short opt_noSpeech = 0;
#endif /* ENABLE_SPEECH_SUPPORT */

static char *opt_screenParameters = NULL;
static char **screenParameters = NULL;

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

static char *cfg_preferencesFile = NULL;
static ConfigurationLineStatus
configurePreferencesFile (const char *delimiters) {
  return getConfigurationOperand(&cfg_preferencesFile, delimiters, 0);
}

static char *cfg_tablesDirectory = NULL;
static ConfigurationLineStatus
configureTablesDirectory (const char *delimiters) {
  return getConfigurationOperand(&cfg_tablesDirectory, delimiters, 0);
}

static char *cfg_textTable = NULL;
static ConfigurationLineStatus
configureTextTable (const char *delimiters) {
  return getConfigurationOperand(&cfg_textTable, delimiters, 0);
}

static char *cfg_attributesTable = NULL;
static ConfigurationLineStatus
configureAttributesTable (const char *delimiters) {
  return getConfigurationOperand(&cfg_attributesTable, delimiters, 0);
}

#ifdef ENABLE_CONTRACTED_BRAILLE
static char *cfg_contractionsDirectory = NULL;
static ConfigurationLineStatus
configureContractionsDirectory (const char *delimiters) {
  return getConfigurationOperand(&cfg_contractionsDirectory, delimiters, 0);
}

static char *cfg_contractionTable = NULL;
static ConfigurationLineStatus
configureContractionTable (const char *delimiters) {
  return getConfigurationOperand(&cfg_contractionTable, delimiters, 0);
}
#endif /* ENABLE_CONTRACTED_BRAILLE */

#ifdef ENABLE_API
static char *cfg_apiParameters = NULL;
static ConfigurationLineStatus
configureApiParameters (const char *delimiters) {
  return getConfigurationOperand(&cfg_apiParameters, delimiters, 1);
}
#endif /* ENABLE_API */

static char *cfg_libraryDirectory = NULL;
static ConfigurationLineStatus
configureLibraryDirectory (const char *delimiters) {
  return getConfigurationOperand(&cfg_libraryDirectory, delimiters, 0);
}

static char *cfg_dataDirectory = NULL;
static ConfigurationLineStatus
configureDataDirectory (const char *delimiters) {
  return getConfigurationOperand(&cfg_dataDirectory, delimiters, 0);
}

static char *cfg_brailleDriver = NULL;
static ConfigurationLineStatus
configureBrailleDriver (const char *delimiters) {
  return getConfigurationOperand(&cfg_brailleDriver, delimiters, 0);
}

static char *cfg_brailleDevice = NULL;
static ConfigurationLineStatus
configureBrailleDevice (const char *delimiters) {
  return getConfigurationOperand(&cfg_brailleDevice, delimiters, 0);
}

static char *cfg_brailleParameters = NULL;
static ConfigurationLineStatus
configureBrailleParameters (const char *delimiters) {
  return getConfigurationOperand(&cfg_brailleParameters, delimiters, 1);
}

#ifdef ENABLE_SPEECH_SUPPORT
static char *cfg_speechDriver = NULL;
static ConfigurationLineStatus
configureSpeechDriver (const char *delimiters) {
  return getConfigurationOperand(&cfg_speechDriver, delimiters, 0);
}

static char *cfg_speechParameters = NULL;
static ConfigurationLineStatus
configureSpeechParameters (const char *delimiters) {
  return getConfigurationOperand(&cfg_speechParameters, delimiters, 1);
}

static char *cfg_speechFifo = NULL;
static ConfigurationLineStatus
configureSpeechFifo (const char *delimiters) {
  return getConfigurationOperand(&cfg_speechFifo, delimiters, 0);
}
#endif /* ENABLE_SPEECH_SUPPORT */

static char *cfg_screenParameters = NULL;
static ConfigurationLineStatus
configureScreenParameters (const char *delimiters) {
  return getConfigurationOperand(&cfg_screenParameters, delimiters, 1);
}

BEGIN_OPTION_TABLE
  {'a', "attributes-table", "file", configureAttributesTable, 0,
   "Path to attributes translation table file."},
  {'b', "braille-driver", "driver", configureBrailleDriver, 0,
   "Braille driver: one of {" BRAILLE_DRIVER_CODES "}"},
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
   "Speech driver: one of {" SPEECH_DRIVER_CODES "}"},
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
#ifdef ENABLE_SPEECH_SUPPORT
  {'F', "speech-fifo", "file", configureSpeechFifo, 0,
   "Path to speech pass-through FIFO."},
#endif /* ENABLE_SPEECH_SUPPORT */
  {'L', "library-directory", "directory", configureLibraryDirectory, OPT_Hidden,
   "Path to directory for loading drivers."},
  {'M', "message-delay", "csecs", NULL, 0,
   "Message hold time [400]."},
#ifdef ENABLE_SPEECH_SUPPORT
  {'N', "no-speech", NULL, NULL, 0,
   "Defer speech until restarted by command."},
#endif /* ENABLE_SPEECH_SUPPORT */
  {'P', "pid-file", "file", NULL, 0,
   "Path to process identifier file."},
#ifdef ENABLE_SPEECH_SUPPORT
  {'S', "speech-parameters", "arg,...", configureSpeechParameters, 0,
   "Parameters for the speech driver."},
#endif /* ENABLE_SPEECH_SUPPORT */
  {'T', "tables-directory", "directory", configureTablesDirectory, OPT_Hidden,
   "Path to directory for text and attributes tables."},
  {'U', "update-interval", "csecs", NULL, 0,
   "Braille window update interval [4]."},
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
parseParameters (
  char **values,
  const char *const *names,
  const char *description,
  const char *qualifier,
  const char *parameters
) {
  if (parameters && *parameters) {
    char *copy = strdupWrapper(parameters);
    char *name = copy;

    while (1) {
      char *end = strchr(name, ',');
      int done = end == NULL;
      if (!done) *end = 0;

      if (*name) {
        char *value = strchr(name, '=');
        if (!value) {
          LogPrint(LOG_ERR, "Missing %s parameter value: %s",
                   description, name);
        } else if (value == name) {
        noName:
          LogPrint(LOG_ERR, "Missing %s parameter name: %s",
                   description, name);
        } else {
          int nameLength = value++ - name;
          int eligible = 1;

          if (qualifier) {
            char *colon = memchr(name, ':', nameLength);
            if (colon) {
              int qualifierLength = colon - name;
              int nameAdjustment = qualifierLength + 1;
              eligible = 0;
              if (!qualifierLength) {
                LogPrint(LOG_ERR, "Missing %s identifier: %s",
                         description, name);
              } else if (!(nameLength -= nameAdjustment)) {
                goto noName;
              } else if ((qualifierLength == strlen(qualifier)) &&
                         (memcmp(name, qualifier, qualifierLength) == 0)) {
                name += nameAdjustment;
                eligible = 1;
              }
            }
          }

          if (eligible) {
            unsigned int index = 0;
            while (names[index]) {
              if (nameLength <= strlen(names[index])) {
                if (strncasecmp(name, names[index], nameLength) == 0) {
                  free(values[index]);
                  values[index] = strdupWrapper(value);
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
      }

      if (done) break;
      name = end + 1;
    }

    free(copy);
  }
}

static char **
processParameters (
  const char *const *names,
  const char *description,
  const char *qualifier,
  const char *optionParameters,
  const char *configuredParameters,
  const char *environmentVariable,
  const char *defaultParameters
) {
  char **values;

  if (!names) {
    static const char *const noNames[] = {NULL};
    names = noNames;
  }

  {
    unsigned int count = 0;
    while (names[count]) ++count;
    values = mallocWrapper((count + 1) * sizeof(*values));
    values[count] = NULL;
    while (count--) values[count] = strdupWrapper("");
  }

  parseParameters(values, names, description, qualifier, defaultParameters);
  parseParameters(values, names, description, qualifier, configuredParameters);
  if (opt_environmentVariables && environmentVariable) {
    parseParameters(values, names, description, qualifier, getenv(environmentVariable));
  }
  parseParameters(values, names, description, qualifier, optionParameters);
  return values;
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
reportTranslationTableMessage (const char *message) {
  LogPrint(LOG_WARNING, "%s", message);
}

static int
replaceTranslationTable (TranslationTable *table, const char *file, const char *type) {
  int ok = 0;
  char *path = makePath(opt_tablesDirectory, file);
  if (path) {
    if (loadTranslationTable(path, table, reportTranslationTableMessage, 0)) {
      ok = 1;
    }
    free(path);
  }
  if (!ok) LogPrint(LOG_ERR, "Cannot load %s table: %s", type, file);
  return ok;
}

static int
replaceTextTable (const char *file) {
  if (!replaceTranslationTable(&textTable, file, "text")) return 0;
  reverseTranslationTable(&textTable, &untextTable);
  return 1;
}

static int
replaceAttributesTable (const char *file) {
  return replaceTranslationTable(&attributesTable, file, "attributes");
}

static void
fixFilePath (const char **path, const char *extension, const char *prefix) {
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

  if (extensionLength) {
    if ((pathLength < extensionLength) ||
        (memcmp(&buffer[length-extensionLength], extension, extensionLength) != 0)) {
      memcpy(&buffer[length], extension, extensionLength);
      length += extensionLength;
    }
  }

  if (length > pathLength) {
    buffer[length] = 0;
    *path = strdupWrapper(buffer);
  }
}

static void
fixTranslationTablePath (const char **path, const char *prefix) {
  fixFilePath(path, TRANSLATION_TABLE_EXTENSION, prefix);
}

static void
dimensionsChanged (int rows, int columns) {
  fwinshift = MAX(columns-prefs.windowOverlap, 1);
  hwinshift = columns / 2;
  vwinshift = (rows > 1)? rows: 5;
  LogPrint(LOG_DEBUG, "Shifts: fwin=%d hwin=%d vwin=%d",
           fwinshift, hwinshift, vwinshift);
}

int
readCommand (BRL_DriverCommandContext context) {
   int command = readBrailleCommand(&brl, context);
   if (command != EOF) {
      LogPrint(LOG_DEBUG, "Command: %06X", command);
      if (IS_DELAYED_COMMAND(command)) command = BRL_CMD_NOOP;
      command &= BRL_MSK_CMD;
   }
   return command;
}

int
getCommand (BRL_DriverCommandContext context) {
   while (1) {
      int command = readCommand(context);
      if (command == EOF) {
         delay(updateInterval);
         closeTuneDevice(0);
      } else if (command != BRL_CMD_NOOP) {
         return command;
      }
   }
}

void
initializeBraille (void) {
  initializeBrailleDisplay(&brl);
  brl.bufferResized = &dimensionsChanged;
  brl.dataDirectory = opt_dataDirectory;
}

static char **
processBrailleParameters (const BrailleDriver *driver) {
  return processParameters(driver->parameters,
                           "braille driver", driver->identifier,
                           opt_brailleParameters, cfg_brailleParameters,
                           "BRLTTY_BRAILLE_PARAMETERS", BRAILLE_PARAMETERS);
}

static const BrailleDriver *
findBrailleDriver (int *internal) {
  char **devices;
  if ((devices = splitString(opt_brailleDevice, ','))) {
    char **device = devices;
    while (*device) {
      const char *const *identifier;

      {
        const char *type;
        const char *dev = *device;

        if (isSerialDevice(&dev)) {
          static const char *serialIdentifiers[] = {
            "pm",
            NULL
          };
          identifier = serialIdentifiers;
          type = "serial";

#ifdef ENABLE_USB_SUPPORT
        } else if (isUsbDevice(&dev)) {
          static const char *usbIdentifiers[] = {
            "al", "fs", "ht", "pm", "vo",
            NULL
          };
          identifier = usbIdentifiers;
          type = "USB";
#endif /* ENABLE_USB_SUPPORT */

#ifdef ENABLE_BLUETOOTH_SUPPORT
        } else if (isBluetoothDevice(&dev)) {
          static const char *bluetoothIdentifiers[] = {
            "ht",
            NULL
          };
          identifier = bluetoothIdentifiers;
          type = "bluetooth";
#endif /* ENABLE_BLUETOOTH_SUPPORT */

        } else {
          LogPrint(LOG_WARNING, "Braille display autodetection not supported for '%s'.", dev);
          goto nextDevice;
        }
        LogPrint(LOG_NOTICE, "Looking for %s braille display on '%s'.", type, *device);
      }

      while (*identifier) {
        const BrailleDriver *driver;
        LogPrint(LOG_INFO, "Checking for '%s' braille display.", *identifier);
        if ((driver = loadBrailleDriver(*identifier, internal, opt_libraryDirectory))) {
          char **parameters = processBrailleParameters(driver);
          if (parameters) {
            initializeBrailleDisplay(&brl);
            if (driver->open(&brl, parameters, *device)) {
              driver->close(&brl);
              LogPrint(LOG_DEBUG, "Braille display found: %s[%s]",
                       driver->identifier, driver->name);

              opt_brailleDriver = driver->identifier;
              opt_brailleDevice = strdupWrapper(*device);

              deallocateStrings(parameters);
              deallocateStrings(devices);
              if (!*internal) unloadSharedObject(driver);
              return driver;
            }

            deallocateStrings(parameters);
          }

          if (!*internal) unloadSharedObject(driver);
        }

        ++identifier;
      }

    nextDevice:
      ++device;
    }

    deallocateStrings(devices);
  }

  LogPrint(LOG_WARNING, "No braille display found.");
  return NULL;
}

static void
getBrailleDriver (void) {
  int internal;
  if (!opt_brailleDriver || (strcmp(opt_brailleDriver, "auto") != 0)) {
    brailleDriver = loadBrailleDriver(opt_brailleDriver, &internal, opt_libraryDirectory);
  } else {
    brailleDriver = findBrailleDriver(&internal);
  }

  if (brailleDriver) {
    brailleParameters = processBrailleParameters(brailleDriver);
  } else {
    LogPrint(LOG_ERR, "Bad braille driver selection: %s", opt_brailleDriver);
    fprintf(stderr, "\n");
    listBrailleDrivers(opt_libraryDirectory);
    fprintf(stderr, "\nUse -b to specify one, and -h for quick help.\n\n");

    /* not fatal */
    brailleDriver = &noBraille;
  }
  if (!opt_brailleDriver) opt_brailleDriver = brailleDriver->identifier;
}

static void
startBrailleDriver (void) {
  char **devices;
  if ((devices = splitString(opt_brailleDevice, ','))) {
    char **device = devices;

    while (1) {
      LogPrint(LOG_DEBUG, "Starting braille driver: %s -> %s",
               brailleDriver->identifier, *device);
      if (brailleDriver->open(&brl, brailleParameters, *device)) {
        if (allocateBrailleBuffer(&brl)) {
          braille = brailleDriver;
          if (braille->firmness) braille->firmness(&brl, prefs.brailleFirmness);

          clearStatusCells(&brl);
          setHelpPageNumber(brl.helpPage);
          playTune(&tune_braille_on);
          return;
        } else {
          LogPrint(LOG_DEBUG, "Braille buffer allocation failed.");
        }
        brailleDriver->close(&brl);
      } else {
        LogPrint(LOG_DEBUG, "Braille driver initialization failed: %s -> %s",
                 brailleDriver->identifier, *device);
      }

      initializeBraille();
      if (!*++device) {
        device = devices;
        delay(5000);
      }
    }
  }
}

static void
stopBrailleDriver (void) {
  braille = &noBraille;
  if (brl.isCoreBuffer) free(brl.buffer);
  brailleDriver->close(&brl);
  initializeBraille();
}

void
restartBrailleDriver (void) {
#ifdef ENABLE_API
  if (apiOpened) api_unlink();
#endif /* ENABLE_API */

  stopBrailleDriver();
  playTune(&tune_braille_off);
  LogPrint(LOG_INFO, "Reinitializing braille driver.");
  startBrailleDriver();

#ifdef ENABLE_API
  if (apiOpened) api_link();
#endif /* ENABLE_API */
}

static void
exitBrailleDriver (void) {
   clearStatusCells(&brl);
   message("BRLTTY terminated.", MSG_NODELAY|MSG_SILENT);
   stopBrailleDriver();
}

static int
testBrailleFirmness (void) {
  return braille->firmness != NULL;
}

static int
changedBrailleFirmness (unsigned char setting) {
  setBrailleFirmness(&brl, setting);
  return 1;
}

#ifdef ENABLE_API
static void
exitApi (void) {
  api_close(&brl);
  apiOpened = 0;
}
#endif /* ENABLE_API */

#ifdef ENABLE_SPEECH_SUPPORT
void
initializeSpeech (void) {
}

static char **
processSpeechParameters (const SpeechDriver *driver) {
  return processParameters(driver->parameters,
                           "speech driver", driver->identifier,
                           opt_speechParameters, cfg_speechParameters,
                           "BRLTTY_SPEECH_PARAMETERS", SPEECH_PARAMETERS);
}

static void
getSpeechDriver (void) {
  int internal;
  if ((speechDriver = loadSpeechDriver(opt_speechDriver, &internal, opt_libraryDirectory))) {
    speechParameters = processSpeechParameters(speechDriver);
  } else {
    LogPrint(LOG_ERR, "Bad speech driver selection: %s", opt_speechDriver);
    fprintf(stderr, "\n");
    listSpeechDrivers(opt_libraryDirectory);
    fprintf(stderr, "\nUse -s to specify one, and -h for quick help.\n\n");

    /* not fatal */
    speechDriver = &noSpeech;
  }
  if (!opt_speechDriver) opt_speechDriver = speechDriver->identifier;
}

static void
startSpeechDriver (void) {
   if (opt_noSpeech) {
      LogPrint(LOG_INFO, "Automatic speech driver initialization disabled.");
   } else if (speechDriver->open(speechParameters)) {
      speech = speechDriver;
      if (speech->rate) speech->rate(prefs.speechRate);
      if (speech->volume) speech->volume(prefs.speechVolume);
   }
}

static void
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

void
restartSpeechDriver (void) {
  stopSpeechDriver();
  LogPrint(LOG_INFO, "Reinitializing speech driver.");
  startSpeechDriver();
}

static void
exitSpeechDriver (void) {
   stopSpeechDriver();
}

static int
testSpeechRate (void) {
  return speech->rate != NULL;
}

static int
changedSpeechRate (unsigned char setting) {
  setSpeechRate(setting);
  return 1;
}

static int
testSpeechVolume (void) {
  return speech->volume != NULL;
}

static int
changedSpeechVolume (unsigned char setting) {
  setSpeechVolume(setting);
  return 1;
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

static int
changedWindowAttributes (void) {
  dimensionsChanged(brl.y, brl.x);
  return 1;
}

static void
changedPreferences (void) {
  changedWindowAttributes();
  setTuneDevice(prefs.tuneDevice);
  if (braille->firmness) braille->firmness(&brl, prefs.brailleFirmness);
#ifdef ENABLE_SPEECH_SUPPORT
  if (speech->rate) speech->rate(prefs.speechRate);
  if (speech->volume) speech->volume(prefs.speechVolume);
#endif /* ENABLE_SPEECH_SUPPORT */
}

int
loadPreferences (int change) {
  int ok = 0;
  int fd = open(opt_preferencesFile, O_RDONLY);
  if (fd != -1) {
    Preferences newPreferences;
    int length = read(fd, &newPreferences, sizeof(newPreferences));
    if (length >= 40) {
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

        if (prefs.version == 2) {
          prefs.version++;
          prefs.autorepeat = DEFAULT_AUTOREPEAT;
          prefs.autorepeatDelay = DEFAULT_AUTOREPEAT_DELAY;
          prefs.autorepeatInterval = DEFAULT_AUTOREPEAT_INTERVAL;

          prefs.cursorVisibleTime *= 4;
          prefs.cursorInvisibleTime *= 4;
          prefs.attributesVisibleTime *= 4;
          prefs.attributesInvisibleTime *= 4;
          prefs.capitalsVisibleTime *= 4;
          prefs.capitalsInvisibleTime *= 4;
        }

        if (length == 40) {
          length++;
          prefs.speechRate = SPK_DEFAULT_RATE;
        }

        if (length == 41) {
          length++;
          prefs.speechVolume = SPK_DEFAULT_VOLUME;
        }

        if (length == 42) {
          length++;
          prefs.brailleFirmness = DEFAULT_BRAILLE_FIRMNESS;
        }

        if (change) changedPreferences();
      } else
        LogPrint(LOG_ERR, "Invalid preferences file: %s", opt_preferencesFile);
    } else if (length == -1) {
      LogPrint(LOG_ERR, "Cannot read preferences file: %s: %s",
               opt_preferencesFile, strerror(errno));
    } else {
      long int actualSize = sizeof(newPreferences);
      LogPrint(LOG_ERR, "Preferences file '%s' has incorrect size %d (should be %ld).",
               opt_preferencesFile, length, actualSize);
    }
    close(fd);
  } else
    LogPrint((errno==ENOENT? LOG_DEBUG: LOG_ERR),
             "Cannot open preferences file: %s: %s",
             opt_preferencesFile, strerror(errno));
  return ok;
}

#ifdef ENABLE_PREFERENCES_MENU
int 
savePreferences (void) {
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
testTunes (void) {
   return prefs.alertTunes;
}

static int
changedTuneDevice (unsigned char setting) {
  return setTuneDevice(setting);
}

#ifdef ENABLE_PCM_SUPPORT
static int
testTunesPcm (void) {
  return testTunes() && (prefs.tuneDevice == tdPcm);
}
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
static int
testTunesMidi (void) {
  return testTunes() && (prefs.tuneDevice == tdMidi);
}
#endif /* ENABLE_MIDI_SUPPORT */

#ifdef ENABLE_FM_SUPPORT
static int
testTunesFm (void) {
  return testTunes() && (prefs.tuneDevice == tdFm);
}
#endif /* ENABLE_FM_SUPPORT */

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
  data->paths = data->pathsArea;
  data->paths[data->count = data->glob.gl_offs] = NULL;

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
          while (data->paths[data->count]) ++data->count;
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
changedTextTable (unsigned char setting) {
  return replaceTextTable(globChanged(&glob_textTable));
}

static int
changedAttributesTable (unsigned char setting) {
  return replaceAttributesTable(globChanged(&glob_attributesTable));
}

#ifdef ENABLE_CONTRACTED_BRAILLE
static GlobData glob_contractionTable;

static int
changedContractionTable (unsigned char setting) {
  return loadContractionTable(globChanged(&glob_contractionTable));
}
#endif /* ENABLE_CONTRACTED_BRAILLE */
#endif /* ENABLE_TABLE_SELECTION */

static int
testSkipBlankWindows (void) {
  return prefs.skipBlankWindows;
}

static int
testSlidingWindow (void) {
  return prefs.slidingWindow;
}

static int
changedWindowOverlap (unsigned char setting) {
  return changedWindowAttributes();
}

static int
testAutorepeat (void) {
  return prefs.autorepeat;
}

static int
testShowCursor (void) {
  return prefs.showCursor;
}

static int
testBlinkingCursor (void) {
  return testShowCursor() && prefs.blinkingCursor;
}

static int
testShowAttributes (void) {
  return prefs.showAttributes;
}

static int
testBlinkingAttributes (void) {
  return testShowAttributes() && prefs.blinkingAttributes;
}

static int
testBlinkingCapitals (void) {
  return prefs.blinkingCapitals;
}

typedef struct {
  unsigned char *setting;                 /* pointer to current value */
  int (*changed) (unsigned char setting); /* called when value changes */
  int (*test) (void);                     /* returns true if item should be presented */
  const char *label;                      /* item name for presentation */
  const char *const *names;               /* symbolic names of values */
  unsigned char minimum;                  /* lowest valid value */
  unsigned char maximum;                  /* highest valid value */
  unsigned char divisor;                  /* present only multiples of this value */
} MenuItem;

static void
previousSetting (MenuItem *item) {
  if ((*item->setting)-- <= item->minimum) *item->setting = item->maximum;
}

static void
nextSetting (MenuItem *item) {
  if ((*item->setting)++ >= item->maximum) *item->setting = item->minimum;
}

void
updatePreferences (void) {
#ifdef ENABLE_TABLE_SELECTION
  globBegin(&glob_textTable);
  globBegin(&glob_attributesTable);
#ifdef ENABLE_CONTRACTED_BRAILLE
  globBegin(&glob_contractionTable);
#endif /* ENABLE_CONTRACTED_BRAILLE */
#endif /* ENABLE_TABLE_SELECTION */

  {
    static unsigned char exitSave = 0;                /* 1 == save preferences on exit */
    static const char *booleanValues[] = {"No", "Yes"};
    static const char *cursorStyles[] = {"Underline", "Block"};
    static const char *firmnessLevels[] = {"Minimum", "Low", "Medium", "High", "Maximum"};
    static const char *skipBlankWindowsModes[] = {"All", "End of Line", "Rest of Line"};
    static const char *statusStyles[] = {"None", "Alva", "Tieman", "PowerBraille 80", "Generic", "MDV", "Voyager"};
    static const char *textStyles[] = {"8-dot", "6-dot"};
    static const char *tuneDevices[] = {
      "Beeper ("
#ifdef ENABLE_BEEPER_SUPPORT
        "console tone generator"
#else /* ENABLE_BEEPER_SUPPORT */
        "unsupported"
#endif /* ENABLE_BEEPER_SUPPORT */
        ")",

      "PCM ("
#ifdef ENABLE_PCM_SUPPORT
        "soundcard digital audio"
#else /* ENABLE_PCM_SUPPORT */
        "unsupported"
#endif /* ENABLE_PCM_SUPPORT */
        ")",

      "MIDI ("
#ifdef ENABLE_MIDI_SUPPORT
        "Musical Instrument Digital Interface"
#else /* ENABLE_MIDI_SUPPORT */
        "unsupported"
#endif /* ENABLE_MIDI_SUPPORT */
        ")",

      "FM ("
#ifdef ENABLE_FM_SUPPORT
        "soundcard synthesizer"
#else /* ENABLE_FM_SUPPORT */
        "unsupported"
#endif /* ENABLE_FM_SUPPORT */
        ")"
    };
#ifdef ENABLE_SPEECH_SUPPORT
    static const char *sayModes[] = {"Immediate", "Enqueue"};
#endif /* ENABLE_SPEECH_SUPPORT */
    #define MENU_ITEM(setting, changed, test, label, values, minimum, maximum, divisor) {&setting, changed, test, label, values, minimum, maximum, divisor}
    #define NUMERIC_ITEM(setting, changed, test, label, minimum, maximum, divisor) MENU_ITEM(setting, changed, test, label, NULL, minimum, maximum, divisor)
    #define TIME_ITEM(setting, changed, test, label) NUMERIC_ITEM(setting, changed, test, label, 1, 100, updateInterval/10)
    #define VOLUME_ITEM(setting, changed, test, label) NUMERIC_ITEM(setting, changed, test, label, 0, 100, 5)
    #define TEXT_ITEM(setting, changed, test, label, names, count) MENU_ITEM(setting, changed, test, label, names, 0, count-1, 1)
    #define SYMBOLIC_ITEM(setting, changed, test, label, names) TEXT_ITEM(setting, changed, test, label, names, ((sizeof(names) / sizeof(names[0]))))
    #define BOOLEAN_ITEM(setting, changed, test, label) SYMBOLIC_ITEM(setting, changed, test, label, booleanValues)
    #define GLOB_ITEM(data, changed, test, label) TEXT_ITEM(data.setting, changed, test, label, data.paths, data.count)
    MenuItem menu[] = {
       BOOLEAN_ITEM(exitSave, NULL, NULL, "Save on Exit"),
       SYMBOLIC_ITEM(prefs.textStyle, NULL, NULL, "Text Style", textStyles),
       BOOLEAN_ITEM(prefs.skipIdenticalLines, NULL, NULL, "Skip Identical Lines"),
       BOOLEAN_ITEM(prefs.skipBlankWindows, NULL, NULL, "Skip Blank Windows"),
       SYMBOLIC_ITEM(prefs.blankWindowsSkipMode, NULL, testSkipBlankWindows, "Which Blank Windows", skipBlankWindowsModes),
       BOOLEAN_ITEM(prefs.slidingWindow, NULL, NULL, "Sliding Window"),
       BOOLEAN_ITEM(prefs.eagerSlidingWindow, NULL, testSlidingWindow, "Eager Sliding Window"),
       NUMERIC_ITEM(prefs.windowOverlap, changedWindowOverlap, NULL, "Window Overlap", 0, 20, 1),
       BOOLEAN_ITEM(prefs.autorepeat, NULL, NULL, "Autorepeat"),
       TIME_ITEM(prefs.autorepeatDelay, NULL, testAutorepeat, "Autorepeat Delay"),
       TIME_ITEM(prefs.autorepeatInterval, NULL, testAutorepeat, "Autorepeat Interval"),
       BOOLEAN_ITEM(prefs.showCursor, NULL, NULL, "Show Cursor"),
       SYMBOLIC_ITEM(prefs.cursorStyle, NULL, testShowCursor, "Cursor Style", cursorStyles),
       BOOLEAN_ITEM(prefs.blinkingCursor, NULL, testShowCursor, "Blinking Cursor"),
       TIME_ITEM(prefs.cursorVisibleTime, NULL, testBlinkingCursor, "Cursor Visible Time"),
       TIME_ITEM(prefs.cursorInvisibleTime, NULL, testBlinkingCursor, "Cursor Invisible Time"),
       BOOLEAN_ITEM(prefs.showAttributes, NULL, NULL, "Show Attributes"),
       BOOLEAN_ITEM(prefs.blinkingAttributes, NULL, testShowAttributes, "Blinking Attributes"),
       TIME_ITEM(prefs.attributesVisibleTime, NULL, testBlinkingAttributes, "Attributes Visible Time"),
       TIME_ITEM(prefs.attributesInvisibleTime, NULL, testBlinkingAttributes, "Attributes Invisible Time"),
       BOOLEAN_ITEM(prefs.blinkingCapitals, NULL, NULL, "Blinking Capitals"),
       TIME_ITEM(prefs.capitalsVisibleTime, NULL, testBlinkingCapitals, "Capitals Visible Time"),
       TIME_ITEM(prefs.capitalsInvisibleTime, NULL, testBlinkingCapitals, "Capitals Invisible Time"),
       SYMBOLIC_ITEM(prefs.brailleFirmness, changedBrailleFirmness, testBrailleFirmness, "Braille Firmness", firmnessLevels),
#ifdef HAVE_LIBGPM
       BOOLEAN_ITEM(prefs.windowFollowsPointer, NULL, NULL, "Window Follows Pointer"),
       BOOLEAN_ITEM(prefs.pointerFollowsWindow, NULL, NULL, "Pointer Follows Window"),
#endif /* HAVE_LIBGPM */
       BOOLEAN_ITEM(prefs.alertTunes, NULL, NULL, "Alert Tunes"),
       SYMBOLIC_ITEM(prefs.tuneDevice, changedTuneDevice, testTunes, "Tune Device", tuneDevices),
#ifdef ENABLE_PCM_SUPPORT
       VOLUME_ITEM(prefs.pcmVolume, NULL, testTunesPcm, "PCM Volume"),
#endif /* ENABLE_PCM_SUPPORT */
#ifdef ENABLE_MIDI_SUPPORT
       VOLUME_ITEM(prefs.midiVolume, NULL, testTunesMidi, "MIDI Volume"),
       TEXT_ITEM(prefs.midiInstrument, NULL, testTunesMidi, "MIDI Instrument", midiInstrumentTable, midiInstrumentCount),
#endif /* ENABLE_MIDI_SUPPORT */
#ifdef ENABLE_FM_SUPPORT
       VOLUME_ITEM(prefs.fmVolume, NULL, testTunesFm, "FM Volume"),
#endif /* ENABLE_FM_SUPPORT */
       BOOLEAN_ITEM(prefs.alertDots, NULL, NULL, "Alert Dots"),
       BOOLEAN_ITEM(prefs.alertMessages, NULL, NULL, "Alert Messages"),
#ifdef ENABLE_SPEECH_SUPPORT
       SYMBOLIC_ITEM(prefs.sayLineMode, NULL, NULL, "Say-Line Mode", sayModes),
       BOOLEAN_ITEM(prefs.autospeak, NULL, NULL, "Autospeak"),
       NUMERIC_ITEM(prefs.speechRate, changedSpeechRate, testSpeechRate, "Speech Rate", 0, SPK_MAXIMUM_RATE, 1),
       NUMERIC_ITEM(prefs.speechVolume, changedSpeechVolume, testSpeechVolume, "Speech Volume", 0, SPK_MAXIMUM_VOLUME, 1),
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

    int lineIndent = 0;                                /* braille window pos in buffer */
    int settingChanged = 0;                        /* 1 when item's value has changed */

    Preferences oldPreferences = prefs;        /* backup preferences */
    int command;                                /* readbrl() value */

    /* status cells */
    setStatusText(&brl, "prf");
    message("Preferences Menu", 0);

    while (1) {
      MenuItem *item = &menu[menuIndex];
      char valueBuffer[0X10];
      const char *value;

      closeTuneDevice(0);

      if (!item->names) {
        snprintf(valueBuffer, sizeof(valueBuffer),
                 "%d", *item->setting);
        value = valueBuffer;
      } else if (!*(value = item->names[*item->setting - item->minimum])) {
        value = "<off>";
      }

      {
        const char *label = item->label;
        const char *delimiter = ": ";
        int settingIndent = strlen(label) + strlen(delimiter);
        int valueLength = strlen(value);
        int lineLength = settingIndent + valueLength;
        char line[lineLength + 1];

        /* First we draw the current menu item in the buffer */
        snprintf(line,  sizeof(line), "%s%s%s",
                 label, delimiter, value);

        /* Next we deal with the braille window position in the buffer.
         * This is intended for small displays and/or long item descriptions 
         */
        if (settingChanged) {
          settingChanged = 0;
          /* make sure the updated value is visible */
          if ((lineLength-lineIndent > brl.x*brl.y) && (lineIndent < settingIndent))
            lineIndent = settingIndent;
        }

        /* Then draw the braille window */
        writeBrailleText(&brl, &line[lineIndent], MAX(0, lineLength-lineIndent));
        drainBrailleOutput(&brl, updateInterval);

        /* Now process any user interaction */
        switch (command = getCommand(BRL_CTX_PREFS)) {
          case BRL_CMD_HELP:
            /* This is quick and dirty... Something more intelligent 
             * and friendly needs to be done here...
             */
            message( 
                "Press UP and DOWN to select an item, "
                "HOME to toggle the setting. "
                "Routing keys are available too! "
                "Press PREFS again to quit.", MSG_WAITKEY|MSG_NODELAY);
            break;

          case BRL_BLK_PASSKEY+BRL_KEY_HOME:
          case BRL_CMD_PREFLOAD:
            prefs = oldPreferences;
            changedPreferences();
            message("changes discarded", 0);
            break;
          case BRL_BLK_PASSKEY+BRL_KEY_ENTER:
          case BRL_CMD_PREFSAVE:
            exitSave = 1;
            goto exitMenu;

          case BRL_CMD_TOP:
          case BRL_CMD_TOP_LEFT:
          case BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP:
          case BRL_CMD_MENU_FIRST_ITEM:
            menuIndex = lineIndent = 0;
            break;
          case BRL_CMD_BOT:
          case BRL_CMD_BOT_LEFT:
          case BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN:
          case BRL_CMD_MENU_LAST_ITEM:
            menuIndex = menuSize - 1;
            lineIndent = 0;
            break;

          case BRL_CMD_LNUP:
          case BRL_CMD_PRDIFLN:
          case BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP:
          case BRL_CMD_MENU_PREV_ITEM:
            do {
              if (menuIndex == 0) menuIndex = menuSize;
              --menuIndex;
            } while (menu[menuIndex].test && !menu[menuIndex].test());
            lineIndent = 0;
            break;
          case BRL_CMD_LNDN:
          case BRL_CMD_NXDIFLN:
          case BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN:
          case BRL_CMD_MENU_NEXT_ITEM:
            do {
              if (++menuIndex == menuSize) menuIndex = 0;
            } while (menu[menuIndex].test && !menu[menuIndex].test());
            lineIndent = 0;
            break;

          case BRL_CMD_FWINLT:
            if (lineIndent > 0)
              lineIndent -= MIN(brl.x*brl.y, lineIndent);
            else
              playTune(&tune_bounce);
            break;
          case BRL_CMD_FWINRT:
            if (lineLength-lineIndent > brl.x*brl.y)
              lineIndent += brl.x*brl.y;
            else
              playTune(&tune_bounce);
            break;

          {
            void (*adjust) (MenuItem *item);
            int count;
          case BRL_CMD_WINUP:
          case BRL_CMD_CHRLT:
          case BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT:
          case BRL_CMD_BACK:
          case BRL_CMD_MENU_PREV_SETTING:
            adjust = previousSetting;
            goto adjustSetting;
          case BRL_CMD_WINDN:
          case BRL_CMD_CHRRT:
          case BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT:
          case BRL_CMD_HOME:
          case BRL_CMD_RETURN:
          case BRL_CMD_MENU_NEXT_SETTING:
            adjust = nextSetting;
          adjustSetting:
            count = item->maximum - item->minimum + 1;
            do {
              adjust(item);
              if (!--count) break;
            } while ((*item->setting % item->divisor) || (item->changed && !item->changed(*item->setting)));
            if (count)
              settingChanged = 1;
            else
              playTune(&tune_command_rejected);
            break;
          }

  #ifdef ENABLE_SPEECH_SUPPORT
          case BRL_CMD_SAY_LINE:
            speech->say(line, lineLength);
            break;
          case BRL_CMD_MUTE:
            speech->mute();
            break;
  #endif /* ENABLE_SPEECH_SUPPORT */

          default:
            if (command >= BRL_BLK_ROUTE && command < BRL_BLK_ROUTE+brl.x) {
              unsigned char oldSetting = *item->setting;
              int key = command - BRL_BLK_ROUTE;
              if (item->names) {
                *item->setting = key % (item->maximum + 1);
              } else {
                unsigned char highestKey = brl.x - 1;
                unsigned char highestValue = item->maximum - item->minimum;
                *item->setting = (highestValue * (key + (highestKey / (highestValue * 2))) / highestKey) + item->minimum;
              }
              if (*item->setting != oldSetting) {
                if (item->changed && !item->changed(*item->setting)) {
                  *item->setting = oldSetting;
                  playTune(&tune_command_rejected);
                } else {
                  settingChanged = 1;
                }
              }
              break;
            }

            /* For any other keystroke, we exit */
            playTune(&tune_command_rejected);
          case BRL_BLK_PASSKEY+BRL_KEY_ESCAPE:
          case BRL_BLK_PASSKEY+BRL_KEY_END:
          case BRL_CMD_PREFMENU:
          exitMenu:
            if (exitSave) {
              if (savePreferences()) {
                playTune(&tune_command_done);
              }
            }

  #ifdef ENABLE_TABLE_SELECTION
            globEnd(&glob_textTable);
            globEnd(&glob_attributesTable);
  #ifdef ENABLE_CONTRACTED_BRAILLE
            globEnd(&glob_contractionTable);
  #endif /* ENABLE_CONTRACTED_BRAILLE */
  #endif /* ENABLE_TABLE_SELECTION */

            return;
        }
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
validateInterval (int *value, const char *description, const char *word) {
  static const int minimum = 1;
  int ok = validateInteger(value, description, word, &minimum, NULL);
  if (ok) *value *= 10;
  return ok;
}

static int
handleOption (const int option) {
  switch (option) {
    default:
      return 0;

    case 'v':	/* --version */
      opt_version = 1;
      break;
    case 'q':	/* --quiet */
      opt_quiet = 1;
      break;
    case 'n':	/* --no-daemon */
      opt_noDaemon = 1;
      break;
    case 'e':	/* --standard-error */
      opt_standardError = 1;
      break;
    case 'l': {	/* --log-level */
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
    case 'E':	/* --environment-variables */
      opt_environmentVariables = 1;
      break;

    case 'f':	/* --configuration-file */
      opt_configurationFile = optarg;
      break;
    case 'p':	/* --preferences-file */
      opt_preferencesFile = optarg;
      break;
    case 'P':	/* --pid-file */
      opt_pidFile = optarg;
      break;
    case 'D':	/* --data-directory */
      opt_dataDirectory = optarg;
      break;
    case 'L':	/* --library-directory */
      opt_libraryDirectory = optarg;
      break;

    case 'b':	/* --braille-driver */
      opt_brailleDriver = optarg;
      break;
    case 'B':	/* --braille-parameters */
      extendParameters(&opt_brailleParameters, optarg);
      break;
    case 'd':	/* --braille-device */
      opt_brailleDevice = optarg;
      break;

    case 'T':	/* --tables-directory */
      opt_tablesDirectory = optarg;
      break;
    case 't':	/* --text-table */
      opt_textTable = optarg;
      break;
    case 'a':	/* --attributes-table */
      opt_attributesTable = optarg;
      break;

#ifdef ENABLE_CONTRACTED_BRAILLE
    case 'C':	/* --contractions-directory */
      opt_contractionsDirectory = optarg;
      break;
    case 'c':	/* --contractgion-table */
      opt_contractionTable = optarg;
      break;
#endif /* ENABLE_CONTRACTED_BRAILLE */

#ifdef ENABLE_API
    case 'A':	/* --api-parameters */
      extendParameters(&opt_apiParameters, optarg);
      break;
#endif /* ENABLE_API */

#ifdef ENABLE_SPEECH_SUPPORT
    case 's':	/* --speech-driver */
      opt_speechDriver = optarg;
      break;
    case 'S':	/* --speech-parameters */
      extendParameters(&opt_speechParameters, optarg);
      break;
    case 'F':	/* --speech-fifo */
      opt_speechFifo = optarg;
      break;
    case 'N':	/* --no-speech */
      opt_noSpeech = 1;
      break;
#endif /* ENABLE_SPEECH_SUPPORT */

    case 'X':	/* --screen-parameters */
      extendParameters(&opt_screenParameters, optarg);
      break;

    case 'U':	/* --update-interval */
      validateInterval(&updateInterval, "update interval", optarg);
      break;
    case 'M':	/* --message-delay */
      validateInterval(&messageDelay, "message delay", optarg);
      break;
  }
  return 1;
}

void
startup (int argc, char *argv[]) {
  processOptions(optionTable, optionCount, handleOption,
                 &argc, &argv, NULL);
  if (argc) LogPrint(LOG_ERR, "Excess parameter: %s", argv[0]);

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
  ensureOptionSetting(&opt_dataDirectory, DATA_DIRECTORY, cfg_dataDirectory, "BRLTTY_DATA_DIRECTORY", -1);
  ensureOptionSetting(&opt_libraryDirectory, LIBRARY_DIRECTORY, cfg_libraryDirectory, "BRLTTY_LIBRARY_DIRECTORY", -1);
  ensureOptionSetting(&opt_tablesDirectory, DATA_DIRECTORY, cfg_tablesDirectory, "BRLTTY_TABLES_DIRECTORY", -1);
  ensureOptionSetting(&opt_textTable, NULL, cfg_textTable, "BRLTTY_TEXT_TABLE", 2);
  ensureOptionSetting(&opt_attributesTable, NULL, cfg_attributesTable, "BRLTTY_ATTRIBUTES_TABLE", -1);
#ifdef ENABLE_CONTRACTED_BRAILLE
  ensureOptionSetting(&opt_contractionsDirectory, DATA_DIRECTORY, cfg_contractionsDirectory, "BRLTTY_CONTRACTIONS_DIRECTORY", -1);
  ensureOptionSetting(&opt_contractionTable, NULL, cfg_contractionTable, "BRLTTY_CONTRACTION_TABLE", -1);
#endif /* ENABLE_CONTRACTED_BRAILLE */
  ensureOptionSetting(&opt_brailleDriver, NULL, cfg_brailleDriver, "BRLTTY_BRAILLE_DRIVER", 0);
  ensureOptionSetting(&opt_brailleDevice, BRAILLE_DEVICE, cfg_brailleDevice, "BRLTTY_BRAILLE_DEVICE", 1);
#ifdef ENABLE_SPEECH_SUPPORT
  ensureOptionSetting(&opt_speechDriver, NULL, cfg_speechDriver, "BRLTTY_SPEECH_DRIVER", -1);
  ensureOptionSetting(&opt_speechFifo, NULL, cfg_speechFifo, "BRLTTY_SPEECH_FIFO", -1);
#endif /* ENABLE_SPEECH_SUPPORT */

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

  {
    char buffer[PATH_MAX+1];
    char *path = getcwd(buffer, sizeof(buffer));
    LogPrint(LOG_INFO, "Working Directory: %s",
             path? path: "path-too-long");
  }

  LogPrint(LOG_INFO, "Configuration File: %s", opt_configurationFile);
  LogPrint(LOG_INFO, "Data Directory: %s", opt_dataDirectory);
  LogPrint(LOG_INFO, "Library Directory: %s", opt_libraryDirectory);
  LogPrint(LOG_INFO, "Tables Directory: %s", opt_tablesDirectory);

  if (opt_textTable) {
    fixTranslationTablePath(&opt_textTable, TEXT_TABLE_PREFIX);
    if (!replaceTextTable(opt_textTable)) opt_textTable = NULL;
  }
  if (!opt_textTable) {
    opt_textTable = TEXT_TABLE;
    reverseTranslationTable(&textTable, &untextTable);
  }
  LogPrint(LOG_INFO, "Text Table: %s", opt_textTable);
#ifdef ENABLE_PREFERENCES_MENU
#ifdef ENABLE_TABLE_SELECTION
  globPrepare(&glob_textTable, opt_tablesDirectory,
              TEXT_TABLE_PREFIX "*" TRANSLATION_TABLE_EXTENSION,
              opt_textTable, 0);
#endif /* ENABLE_TABLE_SELECTION */
#endif /* ENABLE_PREFERENCES_MENU */

  if (opt_attributesTable) {
    fixTranslationTablePath(&opt_attributesTable, ATTRIBUTES_TABLE_PREFIX);
    replaceAttributesTable(opt_attributesTable);
  } else {
    opt_attributesTable = ATTRIBUTES_TABLE;
  }
  LogPrint(LOG_INFO, "Attributes Table: %s", opt_attributesTable);
#ifdef ENABLE_PREFERENCES_MENU
#ifdef ENABLE_TABLE_SELECTION
  globPrepare(&glob_attributesTable, opt_tablesDirectory,
              "attr*" TRANSLATION_TABLE_EXTENSION,
              opt_attributesTable, 0);
#endif /* ENABLE_TABLE_SELECTION */
#endif /* ENABLE_PREFERENCES_MENU */

#ifdef ENABLE_CONTRACTED_BRAILLE
  LogPrint(LOG_INFO, "Contractions Directory: %s", opt_contractionsDirectory);
  if (opt_contractionTable) {
    fixFilePath(&opt_contractionTable, CONTRACTION_TABLE_EXTENSION, CONTRACTION_TABLE_PREFIX);
    loadContractionTable(opt_contractionTable);
  }
  atexit(exitContractionTable);
  LogPrint(LOG_INFO, "Contraction Table: %s",
           opt_contractionTable? opt_contractionTable: "none");
#ifdef ENABLE_PREFERENCES_MENU
#ifdef ENABLE_TABLE_SELECTION
  globPrepare(&glob_contractionTable, opt_contractionsDirectory,
              CONTRACTION_TABLE_PREFIX "*" CONTRACTION_TABLE_EXTENSION,
              opt_contractionTable, 1);
#endif /* ENABLE_TABLE_SELECTION */
#endif /* ENABLE_PREFERENCES_MENU */
#endif /* ENABLE_CONTRACTED_BRAILLE */

  screenParameters = processParameters(getScreenParameters(),
                                       "screen driver", NULL,
                                       opt_screenParameters, cfg_screenParameters,
                                       "BRLTTY_SCREEN_PARAMETERS", SCREEN_PARAMETERS);
  logParameters(getScreenParameters(), screenParameters, "Screen");

#ifdef ENABLE_API
  api_identify();
  apiParameters = processParameters(api_parameters,
                                    "application programming interface", NULL,
                                    opt_apiParameters, cfg_apiParameters,
                                    "BRLTTY_API_PARAMETERS", API_PARAMETERS);
  logParameters(api_parameters, apiParameters, "API");
#endif /* ENABLE_API */

  if (!*opt_brailleDevice) {
    LogPrint(LOG_CRIT, "No braille device specified.");
    fprintf(stderr, "Use -d to specify one.\n");
    exit(4);
  }
  LogPrint(LOG_INFO, "Braille Device: %s", opt_brailleDevice);

  getBrailleDriver();
  LogPrint(LOG_INFO, "Braille Driver: %s [%s] (compiled on %s at %s)",
           opt_brailleDriver, brailleDriver->name,
           brailleDriver->date, brailleDriver->time);
  brailleDriver->identify();
  logParameters(brailleDriver->parameters, brailleParameters, "Braille");
  LogPrint(LOG_INFO, "Help File: %s",
           brailleDriver->helpFile? brailleDriver->helpFile: "<NONE>");

  if (!opt_preferencesFile) {
    const char *part1 = "brltty-";
    const char *part2 = brailleDriver->identifier;
    const char *part3 = ".prefs";
    char *path = mallocWrapper(strlen(part1) + strlen(part2) + strlen(part3) + 1);
    sprintf(path, "%s%s%s", part1, part2, part3);
    opt_preferencesFile = path;
  }
  LogPrint(LOG_INFO, "Preferences File: %s", opt_preferencesFile);

#ifdef ENABLE_SPEECH_SUPPORT
  getSpeechDriver();
  LogPrint(LOG_INFO, "Speech Driver: %s [%s] (compiled on %s at %s)",
           opt_speechDriver, speechDriver->name,
           speechDriver->date, speechDriver->time);
  speechDriver->identify();
  logParameters(speechDriver->parameters, speechParameters, "Speech");
#endif /* ENABLE_SPEECH_SUPPORT */

  if (opt_version) exit(0);
  if (brailleDriver == &noBraille)
#ifdef ENABLE_SPEECH_SUPPORT
    if (speechDriver == &noSpeech)
#endif /* ENABLE_SPEECH_SUPPORT */
      exit(0);

  /* Load preferences file */
  if (!loadPreferences(0)) {
    memset(&prefs, 0, sizeof(prefs));

    prefs.magic[0] = PREFS_MAGIC_NUMBER & 0XFF;
    prefs.magic[1] = PREFS_MAGIC_NUMBER >> 8;
    prefs.version = 3;

    prefs.autorepeat = DEFAULT_AUTOREPEAT;
    prefs.autorepeatDelay = DEFAULT_AUTOREPEAT_DELAY;
    prefs.autorepeatInterval = DEFAULT_AUTOREPEAT_INTERVAL;

    prefs.showCursor = DEFAULT_SHOW_CURSOR;
    prefs.cursorStyle = DEFAULT_CURSOR_STYLE;
    prefs.blinkingCursor = DEFAULT_BLINKING_CURSOR;
    prefs.cursorVisibleTime = DEFAULT_CURSOR_VISIBLE_TIME;
    prefs.cursorInvisibleTime = DEFAULT_CURSOR_INVISIBLE_TIME;

    prefs.showAttributes = DEFAULT_SHOW_ATTRIBUTES;
    prefs.blinkingAttributes = DEFAULT_BLINKING_ATTRIBUTES;
    prefs.attributesVisibleTime = DEFAULT_ATTRIBUTES_VISIBLE_TIME;
    prefs.attributesInvisibleTime = DEFAULT_ATTRIBUTES_INVISIBLE_TIME;

    prefs.blinkingCapitals = DEFAULT_BLINKING_CAPITALS;
    prefs.capitalsVisibleTime = DEFAULT_CAPITALS_VISIBLE_TIME;
    prefs.capitalsInvisibleTime = DEFAULT_CAPITALS_INVISIBLE_TIME;

    prefs.windowFollowsPointer = DEFAULT_WINDOW_FOLLOWS_POINTER;
    prefs.pointerFollowsWindow = DEFAULT_POINTER_FOLLOWS_WINDOW;

    prefs.textStyle = DEFAULT_TEXT_STYLE;
    prefs.brailleFirmness = DEFAULT_BRAILLE_FIRMNESS;

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
    prefs.speechRate = SPK_DEFAULT_RATE;
    prefs.speechVolume = SPK_DEFAULT_VOLUME;

    prefs.statusStyle = brailleDriver->statusStyle;
  }
  setTuneDevice(prefs.tuneDevice);
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
  if ((apiOpened = api_open(&brl, apiParameters))) {
    atexit(exitApi);
  }
#endif /* ENABLE_API */

#ifdef ENABLE_SPEECH_SUPPORT
  /* Activate the speech synthesizer. */
  initializeSpeech();
  startSpeechDriver();
  atexit(exitSpeechDriver);

  /* Create the speech pass-through FIFO. */
  if (opt_speechFifo) openSpeechFifo(opt_dataDirectory, opt_speechFifo);
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
