/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/*
 * config.c - Everything configuration related.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#include "cmd.h"
#include "brl.h"
#include "spk.h"
#include "scr.h"
#include "status.h"
#include "datafile.h"
#include "ttb.h"
#include "atb.h"
#include "ctb.h"
#include "ktb.h"
#include "ktb_keyboard.h"
#include "kbd.h"
#include "tunes.h"
#include "message.h"
#include "log.h"
#include "file.h"
#include "parse.h"
#include "system.h"
#include "async.h"
#include "program.h"
#include "options.h"
#include "brltty.h"
#include "prefs.h"
#include "charset.h"

#include "io_serial.h"
#include "io_usb.h"
#include "io_bluetooth.h"

#ifdef __MINGW32__
#include "sys_windows.h"

int isWindowsService = 0;

#define SERVICE_NAME "BrlAPI"
#define SERVICE_DESCRIPTION "Braille API (BrlAPI)"

static int opt_installService;
static const char *const optionStrings_InstallService[] = {
  SERVICE_NAME,
  NULL
};

static int opt_removeService;
static const char *const optionStrings_RemoveService[] = {
  SERVICE_NAME,
  NULL
};
#endif /* __MINGW32__ */

#ifdef __MSDOS__
#include "sys_msdos.h"
#endif /* __MSDOS__ */

static int opt_version;
static int opt_verify;
static int opt_quiet;
static int opt_noDaemon;
static int opt_standardError;
static char *opt_logLevel;
static char *opt_logFile;
static int opt_bootParameters = 1;
static int opt_environmentVariables;
static char *opt_updateInterval;
static char *opt_messageDelay;

static int opt_cancelExecution;
static const char *const optionStrings_CancelExecution[] = {
  PACKAGE_NAME,
  NULL
};

static char *opt_configurationFile;
static char *opt_preferencesFile;
static char *opt_pidFile;
static char *opt_writableDirectory;
static char *opt_driversDirectory;

static char *opt_brailleDevice;
int opt_releaseDevice;
static char **brailleDevices;
static const char *brailleDevice = NULL;
static int brailleConstructed;

static char *opt_brailleDriver;
static char **brailleDrivers;
static const BrailleDriver *brailleDriver = NULL;
static void *brailleObject;
static char *opt_brailleParameters;
static char **brailleParameters = NULL;
static char *oldPreferencesFile = NULL;
static int oldPreferencesEnabled = 1;

char *opt_tablesDirectory;
char *opt_textTable;
char *opt_attributesTable;

#ifdef ENABLE_CONTRACTED_BRAILLE
char *opt_contractionTable;
ContractionTable *contractionTable = NULL;
#endif /* ENABLE_CONTRACTED_BRAILLE */

static char *opt_keyTable;
KeyTable *keyboardKeyTable = NULL;

static char *opt_keyboardProperties;
static KeyboardProperties keyboardProperties;

#ifdef ENABLE_API
static int opt_noApi;
static char *opt_apiParameters;
static char **apiParameters = NULL;
int apiStarted;
#endif /* ENABLE_API */

#ifdef ENABLE_SPEECH_SUPPORT
static char *opt_speechDriver;
static char **speechDrivers = NULL;
static const SpeechDriver *speechDriver = NULL;
static void *speechObject;
static char *opt_speechParameters;
static char **speechParameters = NULL;
static char *opt_speechInput;
#endif /* ENABLE_SPEECH_SUPPORT */

static char *opt_screenDriver;
static char **screenDrivers;
static const ScreenDriver *screenDriver = NULL;
static void *screenObject;
static char *opt_screenParameters;
static char **screenParameters = NULL;

#ifdef ENABLE_PCM_SUPPORT
char *opt_pcmDevice;
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
char *opt_midiDevice;
#endif /* ENABLE_MIDI_SUPPORT */

static const char *const optionStrings_LogLevel[] = {
  "0-7 [5]",
  "emergency alert critical error warning [notice] information debug",
  NULL
};

static const char *const optionStrings_BrailleDriver[] = {
  BRAILLE_DRIVER_CODES,
  NULL
};

static const char *const optionStrings_ScreenDriver[] = {
  SCREEN_DRIVER_CODES,
  NULL
};

#ifdef ENABLE_SPEECH_SUPPORT
static const char *const optionStrings_SpeechDriver[] = {
  SPEECH_DRIVER_CODES,
  NULL
};
#endif /* ENABLE_SPEECH_SUPPORT */

BEGIN_OPTION_TABLE(programOptions)
#ifdef __MINGW32__
  { .letter = 'I',
    .word = "install-service",
    .flags = OPT_Hidden,
    .setting.flag = &opt_installService,
    .description = strtext("Install the %s service, and then exit."),
    .strings = optionStrings_InstallService
  },

  { .letter = 'R',
    .word = "remove-service",
    .flags = OPT_Hidden,
    .setting.flag = &opt_removeService,
    .description = strtext("Remove the %s service, and then exit."),
    .strings = optionStrings_RemoveService
  },
#endif /* __MINGW32__ */

  { .letter = 'C',
    .word = "cancel-execution",
    .flags = OPT_Hidden,
    .setting.flag = &opt_cancelExecution,
    .description = strtext("Stop an existing instance of %s, and then exit."),
    .strings = optionStrings_CancelExecution
  },

  { .letter = 'P',
    .word = "pid-file",
    .flags = OPT_Hidden | OPT_Config | OPT_Environ,
    .argument = strtext("file"),
    .setting.string = &opt_pidFile,
    .description = strtext("Path to process identifier file.")
  },

  { .letter = 'D',
    .word = "drivers-directory",
    .flags = OPT_Hidden | OPT_Config | OPT_Environ,
    .argument = strtext("directory"),
    .setting.string = &opt_driversDirectory,
    .defaultSetting = DRIVERS_DIRECTORY,
    .description = strtext("Path to directory for loading drivers.")
  },

  { .letter = 'W',
    .word = "writable-directory",
    .flags = OPT_Hidden | OPT_Config | OPT_Environ,
    .argument = strtext("directory"),
    .setting.string = &opt_writableDirectory,
    .defaultSetting = WRITABLE_DIRECTORY,
    .description = strtext("Path to directory which can be written to.")
  },

  { .letter = 'f',
    .word = "configuration-file",
    .flags = OPT_Environ,
    .argument = strtext("file"),
    .setting.string = &opt_configurationFile,
    .defaultSetting = CONFIGURATION_DIRECTORY "/" CONFIGURATION_FILE,
    .description = strtext("Path to default settings file.")
  },

  { .letter = 'F',
    .word = "preferences-file",
    .flags = OPT_Hidden | OPT_Config | OPT_Environ,
    .argument = strtext("file"),
    .setting.string = &opt_preferencesFile,
    .defaultSetting = PREFERENCES_FILE,
    .description = strtext("Path to default preferences file.")
  },

  { .letter = 'E',
    .word = "environment-variables",
    .flags = OPT_Hidden,
    .setting.flag = &opt_environmentVariables,
    .description = strtext("Recognize environment variables.")
  },

#ifdef ENABLE_API
  { .letter = 'A',
    .word = "api-parameters",
    .flags = OPT_Extend | OPT_Config | OPT_Environ,
    .argument = strtext("arg,..."),
    .setting.string = &opt_apiParameters,
    .defaultSetting = API_PARAMETERS,
    .description = strtext("Parameters for the application programming interface.")
  },

  { .letter = 'N',
    .word = "no-api",
    .flags = OPT_Hidden,
    .setting.flag = &opt_noApi,
    .description = strtext("Disable the application programming interface.")
  },
#endif /* ENABLE_API */

  { .letter = 'b',
    .word = "braille-driver",
    .bootParameter = 1,
    .flags = OPT_Config | OPT_Environ,
    .argument = strtext("driver"),
    .setting.string = &opt_brailleDriver,
    .defaultSetting = "auto",
    .description = strtext("Braille driver: one of {%s}"),
    .strings = optionStrings_BrailleDriver
  },

  { .letter = 'B',
    .word = "braille-parameters",
    .flags = OPT_Extend | OPT_Config | OPT_Environ,
    .argument = strtext("arg,..."),
    .setting.string = &opt_brailleParameters,
    .defaultSetting = BRAILLE_PARAMETERS,
    .description = strtext("Parameters for the braille driver.")
  },

  { .letter = 'd',
    .word = "braille-device",
    .bootParameter = 2,
    .flags = OPT_Config | OPT_Environ,
    .argument = strtext("device"),
    .setting.string = &opt_brailleDevice,
    .defaultSetting = BRAILLE_DEVICE,
    .description = strtext("Path to device for accessing braille display.")
  },

  { .letter = 'r',
    .word = "release-device",
    .flags = OPT_Hidden | OPT_Config | OPT_Environ,
    .setting.flag = &opt_releaseDevice,
#ifdef WINDOWS
    .defaultSetting = FLAG_TRUE_WORD,
#else /* WINDOWS */
    .defaultSetting = FLAG_FALSE_WORD,
#endif /* WINDOWS */
    .description = strtext("Release braille device when screen or window is unreadable.")
  },

  { .letter = 'T',
    .word = "tables-directory",
    .flags = OPT_Hidden | OPT_Config | OPT_Environ,
    .argument = strtext("directory"),
    .setting.string = &opt_tablesDirectory,
    .defaultSetting = TABLES_DIRECTORY,
    .description = strtext("Path to directory for text and attributes tables.")
  },

  { .letter = 't',
    .word = "text-table",
    .bootParameter = 3,
    .flags = OPT_Config | OPT_Environ,
    .argument = strtext("file"),
    .setting.string = &opt_textTable,
    .defaultSetting = "auto",
    .description = strtext("Path to text translation table file.")
  },

  { .letter = 'a',
    .word = "attributes-table",
    .flags = OPT_Config | OPT_Environ,
    .argument = strtext("file"),
    .setting.string = &opt_attributesTable,
    .description = strtext("Path to attributes translation table file.")
  },

#ifdef ENABLE_CONTRACTED_BRAILLE
  { .letter = 'c',
    .word = "contraction-table",
    .flags = OPT_Config | OPT_Environ,
    .argument = strtext("file"),
    .setting.string = &opt_contractionTable,
    .description = strtext("Path to contraction table file.")
  },
#endif /* ENABLE_CONTRACTED_BRAILLE */

  { .letter = 'k',
    .word = "key-table",
    .flags = OPT_Config | OPT_Environ,
    .argument = strtext("file"),
    .setting.string = &opt_keyTable,
    .description = strtext("Path to key table file.")
  },

  { .letter = 'K',
    .word = "keyboard-properties",
    .flags = OPT_Hidden | OPT_Extend | OPT_Config | OPT_Environ,
    .argument = strtext("arg,..."),
    .setting.string = &opt_keyboardProperties,
    .description = strtext("Properties of the keyboard.")
  },

#ifdef ENABLE_SPEECH_SUPPORT
  { .letter = 's',
    .word = "speech-driver",
    .flags = OPT_Config | OPT_Environ,
    .argument = strtext("driver"),
    .setting.string = &opt_speechDriver,
    .defaultSetting = "auto",
    .description = strtext("Speech driver: one of {%s}"),
    .strings = optionStrings_SpeechDriver
  },

  { .letter = 'S',
    .word = "speech-parameters",
    .flags = OPT_Extend | OPT_Config | OPT_Environ,
    .argument = strtext("arg,..."),
    .setting.string = &opt_speechParameters,
    .defaultSetting = SPEECH_PARAMETERS,
    .description = strtext("Parameters for the speech driver.")
  },

  { .letter = 'i',
    .word = "speech-input",
    .flags = OPT_Hidden | OPT_Config | OPT_Environ,
    .argument = strtext("file"),
    .setting.string = &opt_speechInput,
    .description = strtext("Path to speech input file system object.")
  },
#endif /* ENABLE_SPEECH_SUPPORT */

  { .letter = 'x',
    .word = "screen-driver",
    .flags = OPT_Config | OPT_Environ,
    .argument = strtext("driver"),
    .setting.string = &opt_screenDriver,
    .defaultSetting = SCREEN_DRIVER,
    .description = strtext("Screen driver: one of {%s}"),
    .strings = optionStrings_ScreenDriver
  },

  { .letter = 'X',
    .word = "screen-parameters",
    .flags = OPT_Extend | OPT_Config | OPT_Environ,
    .argument = strtext("arg,..."),
    .setting.string = &opt_screenParameters,
    .defaultSetting = SCREEN_PARAMETERS,
    .description = strtext("Parameters for the screen driver.")
  },

#ifdef ENABLE_PCM_SUPPORT
  { .letter = 'p',
    .word = "pcm-device",
    .flags = OPT_Hidden | OPT_Config | OPT_Environ,
    .argument = strtext("device"),
    .setting.string = &opt_pcmDevice,
    .description = strtext("Device specifier for soundcard digital audio.")
  },
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
  { .letter = 'm',
    .word = "midi-device",
    .flags = OPT_Hidden | OPT_Config | OPT_Environ,
    .argument = strtext("device"),
    .setting.string = &opt_midiDevice,
    .description = strtext("Device specifier for the Musical Instrument Digital Interface.")
  },
#endif /* ENABLE_MIDI_SUPPORT */

  { .letter = 'U',
    .word = "update-interval",
    .flags = OPT_Hidden,
    .argument = strtext("csecs"),
    .setting.string = &opt_updateInterval,
    .description = strtext("Braille window update interval [4].")
  },

  { .letter = 'M',
    .word = "message-delay",
    .flags = OPT_Hidden,
    .argument = strtext("csecs"),
    .setting.string = &opt_messageDelay,
    .description = strtext("Message hold time [400].")
  },

  { .letter = 'q',
    .word = "quiet",
    .setting.flag = &opt_quiet,
    .description = strtext("Suppress start-up messages.")
  },

  { .letter = 'l',
    .word = "log-level",
    .flags = OPT_Hidden | OPT_Config | OPT_Environ,
    .argument = strtext("level"),
    .setting.string = &opt_logLevel,
    .description = strtext("Diagnostic logging level: %s, or one of {%s}"),
    .strings = optionStrings_LogLevel
  },

  { .letter = 'L',
    .word = "log-file",
    .flags = OPT_Hidden | OPT_Config | OPT_Environ,
    .argument = strtext("file"),
    .setting.string = &opt_logFile,
    .description = strtext("Path to log file.")
  },

  { .letter = 'e',
    .word = "standard-error",
    .flags = OPT_Hidden,
    .setting.flag = &opt_standardError,
    .description = strtext("Log to standard error rather than to the system log.")
  },

  { .letter = 'n',
    .word = "no-daemon",
    .flags = OPT_Hidden,
    .setting.flag = &opt_noDaemon,
    .description = strtext("Remain a foreground process.")
  },

  { .letter = 'v',
    .word = "verify",
    .setting.flag = &opt_verify,
    .description = strtext("Write the start-up logs, and then exit.")
  },

  { .letter = 'V',
    .word = "version",
    .setting.flag = &opt_version,
    .description = strtext("Log the versions of the core, API, and built-in drivers, and then exit.")
  },
END_OPTION_TABLE

#ifdef ENABLE_CONTRACTED_BRAILLE
static void
exitContractionTable (void) {
  if (contractionTable) {
    destroyContractionTable(contractionTable);
    contractionTable = NULL;
  }
}

int
loadContractionTable (const char *name) {
  ContractionTable *table = NULL;

  if (*name) {
    char *path;

    if ((path = makeContractionTablePath(opt_tablesDirectory, name))) {
      logMessage(LOG_DEBUG, "compiling contraction table: %s", path);

      if (!(table = compileContractionTable(path))) {
        logMessage(LOG_ERR, "%s: %s", gettext("cannot compile contraction table"), path);
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

static unsigned int brailleHelpPageNumber = 0;
static unsigned int keyboardHelpPageNumber = 0;

static int
enableHelpPage (unsigned int *pageNumber) {
  if (!*pageNumber) {
    if (!constructHelpScreen()) return 0;
    if (!(*pageNumber = addHelpPage())) return 0;
  }

  return setHelpPageNumber(*pageNumber);
}

static int
enableBrailleHelpPage (void) {
  return enableHelpPage(&brailleHelpPageNumber);
}

static int
enableKeyboardHelpPage (void) {
  return enableHelpPage(&keyboardHelpPageNumber);
}

static void
disableHelpPage (unsigned int pageNumber) {
  if (pageNumber) {
    if (setHelpPageNumber(pageNumber)) {
      clearHelpPage();
    }
  }
}

static int
handleWcharHelpLine (const wchar_t *line, void *data UNUSED) {
  return addHelpLine(line);
}

static int
handleUtf8HelpLine (char *line, void *data) {
  const char *utf8 = line;
  size_t count = strlen(utf8) + 1;
  wchar_t buffer[count];
  wchar_t *characters = buffer;

  convertUtf8ToWchars(&utf8, &characters, count);
  return handleWcharHelpLine(buffer, data);
}

static int
loadHelpFile (const char *file) {
  int loaded = 0;
  FILE *stream;

  if ((stream = openDataFile(file, "r", 0))) {
    if (processLines(stream, handleUtf8HelpLine, NULL)) loaded = 1;

    fclose(stream);
  }

  return loaded;
}

static void
exitKeyTable (void) {
  if (keyboardKeyTable) {
    destroyKeyTable(keyboardKeyTable);
    keyboardKeyTable = NULL;
  }

  disableHelpPage(keyboardHelpPageNumber);
}

static int
replaceKeyboardKeyTable (const char *name) {
  KeyTable *table = NULL;

  if (*name) {
    char *file;

    if ((file = ensureKeyTableExtension(name))) {
      char *path;

      if (file == locatePathName(file)) {
        const char *prefix = "kbd-";

        if (strncmp(file, prefix, strlen(prefix)) != 0) {
          const char *strings[] = {prefix, file};
          char *newFile = joinStrings(strings, ARRAY_COUNT(strings));

          if (newFile) {
            free(file);
            file = newFile;
          }
        }
      }

      if ((path = makePath(opt_tablesDirectory, file))) {
        logMessage(LOG_DEBUG, "compiling key table: %s", path);

        if (!(table = compileKeyTable(path, KEY_NAME_TABLES(keyboard)))) {
          logMessage(LOG_ERR, "%s: %s", gettext("cannot compile key table"), path);
        }

        free(path);
      }

      free(file);
    }

    if (!table) return 0;
  }

  if (keyboardKeyTable) {
    destroyKeyTable(keyboardKeyTable);
    disableHelpPage(keyboardHelpPageNumber);
  }

  if ((keyboardKeyTable = table)) {
    setKeyEventLoggingFlag(keyboardKeyTable, &LOG_CATEGORY_FLAG(KEYBOARD_KEY_EVENTS));

    if (enableKeyboardHelpPage()) {
      listKeyTable(keyboardKeyTable, handleWcharHelpLine, NULL);
    }
  }

  return 1;
}

static KeyTableState
handleKeyboardKeyEvent (unsigned char set, unsigned char key, int press) {
  if (scr.unreadable) {
    resetKeyTable(keyboardKeyTable);
    return KTS_UNBOUND;
  }

  return processKeyEvent(keyboardKeyTable, getCurrentCommandContext(), set, key, press);
}

static void scheduleKeyboardMonitor (int interval);

static void
tryKeyboardMonitor (void *data UNUSED) {
  logMessage(LOG_DEBUG, "starting keyboard monitor");
  if (!startKeyboardMonitor(&keyboardProperties, handleKeyboardKeyEvent)) {
    logMessage(LOG_DEBUG, "keyboard monitor failed");
    scheduleKeyboardMonitor(5000);
  }
}

static void
scheduleKeyboardMonitor (int interval) {
  asyncRelativeAlarm(interval, tryKeyboardMonitor, NULL);
}

int
haveStatusCells (void) {
  return brl.statusColumns > 0;
}

static void
windowConfigurationChanged (unsigned int rows, unsigned int columns) {
  textStart = 0;
  textCount = columns;
  statusStart = 0;
  statusCount = 0;

  if (!(textMaximized || haveStatusCells())) {
    unsigned int separatorWidth = (prefs.statusSeparator == ssNone)? 0: 1;
    unsigned int reserved = 1 + separatorWidth;

    if (brl.textColumns > reserved) {
      unsigned int statusWidth = prefs.statusCount;

      if (!statusWidth) statusWidth = getStatusFieldsLength(prefs.statusFields);
      statusWidth = MIN(statusWidth, brl.textColumns-reserved);

      if (statusWidth > 0) {
        switch (prefs.statusPosition) {
          case spLeft:
            statusStart = 0;
            statusCount = statusWidth;
            textStart = statusCount + separatorWidth;
            textCount = columns - textStart;
            break;

          case spRight:
            statusCount = statusWidth;
            statusStart = columns - statusCount;
            textCount = statusStart - separatorWidth;
            textStart = 0;
            break;
        }
      }
    }
  }

  logMessage(LOG_DEBUG, "regions: text=%u.%u status=%u.%u",
             textStart, textCount, statusStart, statusCount);

  fullWindowShift = MAX(textCount-prefs.windowOverlap, 1);
  halfWindowShift = textCount / 2;
  verticalWindowShift = (rows > 1)? rows: 5;
  logMessage(LOG_DEBUG, "shifts: full=%u half=%u vertical=%u",
             fullWindowShift, halfWindowShift, verticalWindowShift);
}

void
reconfigureWindow (void) {
  windowConfigurationChanged(brl.textRows, brl.textColumns);
}

static void
applyBraillePreferences (void) {
  reconfigureWindow();
  setBrailleFirmness(&brl, prefs.brailleFirmness);
  setBrailleSensitivity(&brl, prefs.brailleSensitivity);
}

#ifdef ENABLE_SPEECH_SUPPORT
static void
applySpeechPreferences (void) {
  setSpeechVolume(&spk, prefs.speechVolume, 0);
  setSpeechRate(&spk, prefs.speechRate, 0);
  setSpeechPitch(&spk, prefs.speechPitch, 0);
  setSpeechPunctuation(&spk, prefs.speechPunctuation, 0);
}
#endif /* ENABLE_SPEECH_SUPPORT */

static void
applyAllPreferences (void) {
  setTuneDevice(prefs.tuneDevice);
  applyBraillePreferences();

#ifdef ENABLE_SPEECH_SUPPORT
  applySpeechPreferences();
#endif /* ENABLE_SPEECH_SUPPORT */
}

void
setPreferences (const Preferences *newPreferences) {
  prefs = *newPreferences;
  applyAllPreferences();
}

static void
ensureStatusFields (void) {
  const unsigned char *fields = braille->statusFields;
  unsigned int count = brl.statusColumns * brl.statusRows;

  if (!fields && count) {
    static const unsigned char fields1[] = {
      sfWindowRow, sfEnd
    };

    static const unsigned char fields2[] = {
      sfWindowRow, sfCursorRow, sfEnd
    };

    static const unsigned char fields3[] = {
      sfWindowRow, sfCursorRow, sfCursorColumn, sfEnd
    };

    static const unsigned char fields4[] = {
      sfWindowCoordinates, sfCursorCoordinates, sfEnd
    };

    static const unsigned char fields5[] = {
      sfWindowCoordinates, sfCursorCoordinates, sfStateDots, sfEnd
    };

    static const unsigned char fields6[] = {
      sfWindowCoordinates, sfCursorCoordinates, sfStateDots, sfScreenNumber,
      sfEnd
    };

    static const unsigned char fields7[] = {
      sfWindowCoordinates, sfCursorCoordinates, sfStateDots, sfTime,
      sfEnd
    };

    static const unsigned char *const fieldsTable[] = {
      fields1, fields2, fields3, fields4, fields5, fields6, fields7
    };
    static const unsigned char fieldsCount = ARRAY_COUNT(fieldsTable);

    if (count > fieldsCount) count = fieldsCount;
    fields = fieldsTable[count - 1];
  }

  setStatusFields(fields);
}

int
loadPreferences (void) {
  int ok = 0;

  {
    char *path = makePreferencesFilePath(opt_preferencesFile);

    if (path) {
      if (testFilePath(path)) {
        oldPreferencesEnabled = 0;
        if (loadPreferencesFile(path)) ok = 1;
      }

      free(path);
    }
  }

  if (oldPreferencesEnabled && oldPreferencesFile) {
    if (loadPreferencesFile(oldPreferencesFile)) ok = 1;
  }

  if (!ok) resetPreferences();
  applyAllPreferences();
  return ok;
}

int 
savePreferences (void) {
  int ok = 0;
  char *path = makePreferencesFilePath(opt_preferencesFile);

  if (path) {
    if (savePreferencesFile(path)) {
      ok = 1;
      oldPreferencesEnabled = 0;
    }

    free(path);
  }

  if (!ok) message(NULL, gettext("not saved"), 0);
  return ok;
}

typedef struct {
  const char *driverType;
  const char *const *requestedDrivers;
  const char *const *autodetectableDrivers;
  const char * (*getDefaultDriver) (void);
  int (*haveDriver) (const char *code);
  int (*initializeDriver) (const char *code, int verify);
} DriverActivationData;

static int
activateDriver (const DriverActivationData *data, int verify) {
  int oneDriver = data->requestedDrivers[0] && !data->requestedDrivers[1];
  int autodetect = oneDriver && (strcmp(data->requestedDrivers[0], "auto") == 0);
  const char *const defaultDrivers[] = {data->getDefaultDriver(), NULL};
  const char *const *driver;

  if (!oneDriver || autodetect) verify = 0;

  if (!autodetect) {
    driver = data->requestedDrivers;
  } else if (defaultDrivers[0]) {
    driver = defaultDrivers;
  } else if (*(driver = data->autodetectableDrivers)) {
    logMessage(LOG_DEBUG, "performing %s driver autodetection", data->driverType);
  } else {
    logMessage(LOG_DEBUG, "no autodetectable %s drivers", data->driverType);
  }

  if (!*driver) {
    static const char *const fallbackDrivers[] = {"no", NULL};
    driver = fallbackDrivers;
    autodetect = 0;
  }

  while (*driver) {
    if (!autodetect || data->haveDriver(*driver)) {
      logMessage(LOG_DEBUG, "checking for %s driver: %s", data->driverType, *driver);
      if (data->initializeDriver(*driver, verify)) return 1;
    }

    ++driver;
  }

  logMessage(LOG_DEBUG, "%s driver not found", data->driverType);
  return 0;
}

static void
unloadDriverObject (void **object) {
#ifdef ENABLE_SHARED_OBJECTS
  if (*object) {
    unloadSharedObject(*object);
    *object = NULL;
  }
#endif /* ENABLE_SHARED_OBJECTS */
}

void
initializeBraille (void) {
  initializeBrailleDisplay(&brl);
  brl.bufferResized = &windowConfigurationChanged;
}

int
constructBrailleDriver (void) {
  initializeBraille();

  if (braille->construct(&brl, brailleParameters, brailleDevice)) {
    if (ensureBrailleBuffer(&brl, LOG_INFO)) {
      brailleConstructed = 1;

      /* Initialize the braille driver's help screen. */
      if (brl.keyBindings) {
        logMessage(LOG_INFO, "%s: %s", gettext("Key Bindings"), brl.keyBindings);

        if (brl.keyNameTables) {
          char *file;

          {
            const char *strings[] = {
              "brl-", braille->definition.code, "-", brl.keyBindings, KEY_TABLE_EXTENSION
            };
            file = joinStrings(strings, ARRAY_COUNT(strings));
          }

          if (file) {
            char *path;

            if ((path = makePath(opt_tablesDirectory, file))) {
              if ((brl.keyTable = compileKeyTable(path, brl.keyNameTables))) {
                setKeyEventLoggingFlag(brl.keyTable, &LOG_CATEGORY_FLAG(BRAILLE_KEY_EVENTS));
                logMessage(LOG_INFO, "%s: %s", gettext("Key Table"), path);

                if (enableBrailleHelpPage()) {
                  listKeyTable(brl.keyTable, handleWcharHelpLine, NULL);
                }
              } else {
                logMessage(LOG_WARNING, "%s: %s", gettext("cannot open key table"), path);
              }

              free(path);
            }

            free(file);
          }
        }

        if (!brl.keyTable) {
          char *file;

          {
            const char *strings[] = {
              "brl-", braille->definition.code, "-", brl.keyBindings, ".txt"
            };
            file = joinStrings(strings, ARRAY_COUNT(strings));
          }

          if (file) {
            char *path;

            if ((path = makePath(opt_tablesDirectory, file))) {
              int loaded = 0;

              if (enableBrailleHelpPage())
                if (loadHelpFile(path))
                  loaded = 1;

              if (loaded) {
                logMessage(LOG_INFO, "%s: %s", gettext("Help Path"), path);
              } else {
                logMessage(LOG_WARNING, "%s: %s", gettext("cannot open help file"), path);
              }

              free(path);
            }

            free(file);
          }
        }

        if (!getHelpLineCount()) {
          addHelpLine(WS_C("help not available"));
        }
      }

      return 1;
    }

    braille->destruct(&brl);
  } else {
    logMessage(LOG_DEBUG, "%s: %s -> %s",
               gettext("braille driver initialization failed"),
               braille->definition.code, brailleDevice);
  }

  return 0;
}

void
destructBrailleDriver (void) {
  brailleConstructed = 0;
  drainBrailleOutput(&brl, 0);
  braille->destruct(&brl);
  disableHelpPage(brailleHelpPageNumber);

  if (brl.keyTable) {
    destroyKeyTable(brl.keyTable);
    brl.keyTable = NULL;
  }
}

static int
initializeBrailleDriver (const char *code, int verify) {
  if ((braille = loadBrailleDriver(code, &brailleObject, opt_driversDirectory))) {
    brailleParameters = getParameters(braille->parameters,
                                      braille->definition.code,
                                      opt_brailleParameters);
    if (brailleParameters) {
      int constructed = verify;

      if (!constructed) {
        logMessage(LOG_DEBUG, "initializing braille driver: %s -> %s",
                   braille->definition.code, brailleDevice);

        if (constructBrailleDriver()) {
#ifdef ENABLE_API
          if (apiStarted) api_link(&brl);
#endif /* ENABLE_API */

          brailleDriver = braille;
          constructed = 1;
        }
      }

      if (constructed) {
        logMessage(LOG_INFO, "%s: %s [%s]",
                   gettext("Braille Driver"), braille->definition.code, braille->definition.name);
        identifyBrailleDriver(braille, 0);
        logParameters(braille->parameters, brailleParameters,
                      gettext("Braille Parameter"));
        logMessage(LOG_INFO, "%s: %s", gettext("Braille Device"), brailleDevice);

        {
          const char *strings[] = {
            CONFIGURATION_DIRECTORY, "/",
            PACKAGE_NAME, "-",
            braille->definition.code, ".prefs"
          };

          oldPreferencesFile = joinStrings(strings, ARRAY_COUNT(strings));
        }

        if (oldPreferencesFile) {
          logMessage(LOG_INFO, "%s: %s", gettext("Old Preferences File"), oldPreferencesFile);
          return 1;
        } else {
          logMallocError();
        }
      }

      deallocateStrings(brailleParameters);
      brailleParameters = NULL;
    }

    unloadDriverObject(&brailleObject);
  } else {
    logMessage(LOG_ERR, "%s: %s", gettext("braille driver not loadable"), code);
  }

  braille = &noBraille;
  return 0;
}

static int
activateBrailleDriver (int verify) {
  int oneDevice = brailleDevices[0] && !brailleDevices[1];
  const char *const *device = (const char *const *)brailleDevices;

  if (!oneDevice) verify = 0;

  while (*device) {
    const char *const *autodetectableDrivers;

    brailleDevice = *device;
    logMessage(LOG_DEBUG, "checking braille device: %s", brailleDevice);

    {
      const char *dev = brailleDevice;

      if (isSerialDevice(&dev)) {
        static const char *const serialDrivers[] = {
          "md", "pm", "ts", "ht", "bn", "al", "bm", "pg", "sk",
          NULL
        };
        autodetectableDrivers = serialDrivers;
      } else if (isUsbDevice(&dev)) {
        static const char *const usbDrivers[] = {
          "al", "bm", "eu", "fs", "ht", "hm", "hw", "mt", "pg", "pm", "sk", "vo",
          NULL
        };
        autodetectableDrivers = usbDrivers;
      } else if (isBluetoothDevice(&dev)) {
        static const char *bluetoothDrivers[] = {
          "np", "ht", "al", "bm",
          NULL
        };
        autodetectableDrivers = bluetoothDrivers;
      } else {
        static const char *noDrivers[] = {NULL};
        autodetectableDrivers = noDrivers;
      }
    }

    {
      const DriverActivationData data = {
        .driverType = "braille",
        .requestedDrivers = (const char *const *)brailleDrivers,
        .autodetectableDrivers = autodetectableDrivers,
        .getDefaultDriver = getDefaultBrailleDriver,
        .haveDriver = haveBrailleDriver,
        .initializeDriver = initializeBrailleDriver
      };
      if (activateDriver(&data, verify)) return 1;
    }

    device += 1;
  }

  brailleDevice = NULL;
  return 0;
}

static void
deactivateBrailleDriver (void) {
  if (brailleDriver) {
#ifdef ENABLE_API
    if (apiStarted) api_unlink(&brl);
#endif /* ENABLE_API */

    if (brailleConstructed) destructBrailleDriver();
    braille = &noBraille;
    brailleDevice = NULL;
    brailleDriver = NULL;
  }

  unloadDriverObject(&brailleObject);

  if (brailleParameters) {
    deallocateStrings(brailleParameters);
    brailleParameters = NULL;
  }

  if (oldPreferencesFile) {
    free(oldPreferencesFile);
    oldPreferencesFile = NULL;
  }
}

static int
startBrailleDriver (void) {
  usbForgetDevices();
  bthForgetConnectErrors();

  if (activateBrailleDriver(0)) {
    if (oldPreferencesEnabled) {
      if (!loadPreferencesFile(oldPreferencesFile)) resetPreferences();
      applyAllPreferences();
    } else {
      applyBraillePreferences();
    }

    ensureStatusFields();
    playTune(&tune_braille_on);

    if (clearStatusCells(&brl)) {
      if (opt_quiet) return 1;

      {
        int flags = MSG_SILENT;
        char banner[0X100];

        makeProgramBanner(banner, sizeof(banner));
        if (message(NULL, banner, flags)) return 1;
      }
    }

    deactivateBrailleDriver();
  }

  return 0;
}

static int tryBrailleDriver (void);

static void
retryBrailleDriver (void *data UNUSED) {
  if (!brailleDriver) tryBrailleDriver();
}

static int
tryBrailleDriver (void) {
  if (startBrailleDriver()) return 1;
  asyncRelativeAlarm(5000, retryBrailleDriver, NULL);
  initializeBraille();
  ensureBrailleBuffer(&brl, LOG_DEBUG);
  return 0;
}

static void
stopBrailleDriver (void) {
  deactivateBrailleDriver();
  playTune(&tune_braille_off);
}

void
restartBrailleDriver (void) {
  stopBrailleDriver();
  logMessage(LOG_INFO, gettext("reinitializing braille driver"));
  tryBrailleDriver();
}

static void
exitBrailleDriver (void) {
  if (brailleConstructed) {
    clearStatusCells(&brl);
    message(NULL, gettext("BRLTTY terminated"), MSG_NODELAY|MSG_SILENT);
  }

  stopBrailleDriver();
}

#ifdef ENABLE_API
static void
exitApi (void) {
  api_stop(&brl);
  apiStarted = 0;
}
#endif /* ENABLE_API */

#ifdef ENABLE_SPEECH_SUPPORT
void
initializeSpeech (void) {
  initializeSpeechSynthesizer(&spk);
}

int
constructSpeechDriver (void) {
  initializeSpeech();

  if (speech->construct(&spk, speechParameters)) {
    return 1;
  } else {
    logMessage(LOG_DEBUG, "speech driver initialization failed: %s",
               speech->definition.code);
  }

  return 0;
}

void
destructSpeechDriver (void) {
  speech->destruct(&spk);
}

static int
initializeSpeechDriver (const char *code, int verify) {
  if ((speech = loadSpeechDriver(code, &speechObject, opt_driversDirectory))) {
    speechParameters = getParameters(speech->parameters,
                                     speech->definition.code,
                                     opt_speechParameters);
    if (speechParameters) {
      int constructed = verify;

      if (!constructed) {
        logMessage(LOG_DEBUG, "initializing speech driver: %s",
                   speech->definition.code);

        if (constructSpeechDriver()) {
          constructed = 1;
          speechDriver = speech;
        }
      }

      if (constructed) {
        logMessage(LOG_INFO, "%s: %s [%s]",
                   gettext("Speech Driver"), speech->definition.code, speech->definition.name);
        identifySpeechDriver(speech, 0);
        logParameters(speech->parameters, speechParameters,
                      gettext("Speech Parameter"));

        return 1;
      }

      deallocateStrings(speechParameters);
      speechParameters = NULL;
    }

    unloadDriverObject(&speechObject);
  } else {
    logMessage(LOG_ERR, "%s: %s", gettext("speech driver not loadable"), code);
  }

  speech = &noSpeech;
  return 0;
}

static int
activateSpeechDriver (int verify) {
  static const char *const autodetectableDrivers[] = {
    NULL
  };

  const DriverActivationData data = {
    .driverType = "speech",
    .requestedDrivers = (const char *const *)speechDrivers,
    .autodetectableDrivers = autodetectableDrivers,
    .getDefaultDriver = getDefaultSpeechDriver,
    .haveDriver = haveSpeechDriver,
    .initializeDriver = initializeSpeechDriver
  };

  return activateDriver(&data, verify);
}

static void
deactivateSpeechDriver (void) {
  if (speechDriver) {
    destructSpeechDriver();

    speech = &noSpeech;
    speechDriver = NULL;
  }

  unloadDriverObject(&speechObject);

  if (speechParameters) {
    deallocateStrings(speechParameters);
    speechParameters = NULL;
  }
}

static int
startSpeechDriver (void) {
  if (!activateSpeechDriver(0)) return 0;
  applySpeechPreferences();

  if (!opt_quiet) {
    char banner[0X100];
    makeProgramBanner(banner, sizeof(banner));
    sayString(&spk, banner, 1);
  }

  return 1;
}

static int trySpeechDriver (void);

static void
retrySpeechDriver (void *data UNUSED) {
  if (!speechDriver) trySpeechDriver();
}

static int
trySpeechDriver (void) {
  if (startSpeechDriver()) return 1;
  asyncRelativeAlarm(5000, retrySpeechDriver, NULL);
  return 0;
}

static void
stopSpeechDriver (void) {
  speech->mute(&spk);
  deactivateSpeechDriver();
}

void
restartSpeechDriver (void) {
  stopSpeechDriver();
  logMessage(LOG_INFO, gettext("reinitializing speech driver"));
  trySpeechDriver();
}

static void
exitSpeechDriver (void) {
  stopSpeechDriver();
}
#endif /* ENABLE_SPEECH_SUPPORT */

static int
initializeScreenDriver (const char *code, int verify) {
  if ((screen = loadScreenDriver(code, &screenObject, opt_driversDirectory))) {
    screenParameters = getParameters(getScreenParameters(screen),
                                     getScreenDriverDefinition(screen)->code,
                                     opt_screenParameters);
    if (screenParameters) {
      int constructed = verify;

      if (!constructed) {
        logMessage(LOG_DEBUG, "initializing screen driver: %s",
                   getScreenDriverDefinition(screen)->code);

        if (constructScreenDriver(screenParameters)) {
          constructed = 1;
          screenDriver = screen;
        }
      }

      if (constructed) {
        logMessage(LOG_INFO, "%s: %s [%s]",
                   gettext("Screen Driver"),
                   getScreenDriverDefinition(screen)->code,
                   getScreenDriverDefinition(screen)->name);
        identifyScreenDriver(screen, 0);
        logParameters(getScreenParameters(screen),
                      screenParameters,
                      gettext("Screen Parameter"));

        return 1;
      }

      deallocateStrings(screenParameters);
      screenParameters = NULL;
    }

    unloadDriverObject(&screenObject);
  } else {
    logMessage(LOG_ERR, "%s: %s", gettext("screen driver not loadable"), code);
  }

  screen = &noScreen;
  return 0;
}

static int
activateScreenDriver (int verify) {
  static const char *const autodetectableDrivers[] = {
    NULL
  };

  const DriverActivationData data = {
    .driverType = "screen",
    .requestedDrivers = (const char *const *)screenDrivers,
    .autodetectableDrivers = autodetectableDrivers,
    .getDefaultDriver = getDefaultScreenDriver,
    .haveDriver = haveScreenDriver,
    .initializeDriver = initializeScreenDriver
  };

  return activateDriver(&data, verify);
}

static void
deactivateScreenDriver (void) {
  if (screenDriver) {
    destructScreenDriver();

    screen = &noScreen;
    screenDriver = NULL;
  }

  unloadDriverObject(&screenObject);

  if (screenParameters) {
    deallocateStrings(screenParameters);
    screenParameters = NULL;
  }
}

static int
startScreenDriver (void) {
  if (!activateScreenDriver(0)) return 0;
  return 1;
}

static int tryScreenDriver (void);

static void
retryScreenDriver (void *data UNUSED) {
  if (!screenDriver) tryScreenDriver();
}

static int
tryScreenDriver (void) {
  if (startScreenDriver()) return 1;
  asyncRelativeAlarm(5000, retryScreenDriver, NULL);
  initializeScreen();
  return 0;
}

static void
stopScreenDriver (void) {
  deactivateScreenDriver();
}

static void
exitScreens (void) {
  stopScreenDriver();
  destructSpecialScreens();
}

static void
exitTunes (void) {
  closeTuneDevice(1);
}

static void
exitPidFile (void) {
#if defined(GRUB_RUNTIME)

#else /* remove pid file */
  unlink(opt_pidFile);
#endif /* remove pid file */
}

static int
makePidFile (ProcessIdentifier pid) {
  return createPidFile(opt_pidFile, pid);
}

static int tryPidFile (void);

static void
retryPidFile (void *data UNUSED) {
  tryPidFile();
}

static int
tryPidFile (void) {
  if (makePidFile(0)) {
    onProgramExit(exitPidFile);
  } else if (errno == EEXIST) {
    return 0;
  } else {
    asyncRelativeAlarm(5000, retryPidFile, NULL);
  }

  return 1;
}

#if defined(__MINGW32__)
static void
background (void) {
  char *variableName;

  {
    const char *strings[] = {programName, "_DAEMON"};
    variableName = joinStrings(strings, ARRAY_COUNT(strings));
  }

  {
    int i;
    for (i=0; variableName[i]; i+=1) {
      char c = variableName[i];

      if (c == '_') continue;
      if (isdigit(c) && (i > 0)) continue;

      if (isalpha(c)) {
        if (islower(c)) variableName[i] = toupper(c);
        continue;
      }

      variableName[i] = '_';
    }
  }

  if (!getenv(variableName)) {
    LPTSTR commandLine = GetCommandLine();
    STARTUPINFO startupInfo;
    PROCESS_INFORMATION processInfo;
    
    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    if (!SetEnvironmentVariable(variableName, "")) {
      logWindowsSystemError("SetEnvironmentVariable");
      exit(PROG_EXIT_FATAL);
    }

    if (!CreateProcess(NULL, commandLine, NULL, NULL, TRUE,
                       CREATE_NEW_PROCESS_GROUP | CREATE_SUSPENDED,
                       NULL, NULL, &startupInfo, &processInfo)) {
      logWindowsSystemError("CreateProcess");
      exit(PROG_EXIT_FATAL);
    }

    {
      int created = makePidFile(processInfo.dwProcessId);
      int resumed = ResumeThread(processInfo.hThread) != -1;

      if (!created) {
        if (errno == EEXIST) {
          ExitProcess(PROG_EXIT_FATAL);
        }
      }

      if (!resumed) {
        logWindowsSystemError("ResumeThread");
        ExitProcess(PROG_EXIT_FATAL);
      }
    }

    ExitProcess(PROG_EXIT_SUCCESS);
  }

  free(variableName);
}

#elif defined(__MSDOS__)
static void
background (void) {
  msdosBackground();
}

#elif defined(GRUB_RUNTIME)
static void
background (void) {
}

#else /* Unix */
static void
background (void) {
  int fds[2];

  if (pipe(fds) == -1) {
    logSystemError("pipe");
    exit(PROG_EXIT_FATAL);
  }

  fflush(stdout);
  fflush(stderr);

  {
    pid_t child = fork();

    if (child == -1) {
      logSystemError("fork");
      exit(PROG_EXIT_FATAL);
    }

    if (child) {
      ProgramExitStatus exitStatus = PROG_EXIT_SUCCESS;

      if (close(fds[0]) == -1) logSystemError("close");

      if (!makePidFile(child)) {
        if (errno == EEXIST) {
          exitStatus = PROG_EXIT_SEMANTIC;
        }
      }

      if (close(fds[1]) == -1) logSystemError("close");
      _exit(exitStatus);
    }
  }

  if (close(fds[1]) == -1) logSystemError("close");

  {
    unsigned char buffer[1];

    if (read(fds[0], buffer, sizeof(buffer)) == -1) logSystemError("read");
    if (close(fds[0]) == -1) logSystemError("close");
  }

  if (setsid() == -1) {                        
    logSystemError("setsid");
    exit(PROG_EXIT_FATAL);
  }
}
#endif /* background() */

static int
validateInterval (int *value, const char *string) {
  if (!*string) return 1;

  {
    static const int minimum = 1;
    int ok = validateInteger(value, string, &minimum, NULL);
    if (ok) *value *= 10;
    return ok;
  }
}

static void
processLogOperand (const char *operand) {
  char **strings = splitString(operand, ',', NULL);

  if (strings) {
    char **string = strings;

    while (*string) {
      int level = LOG_NOTICE;

      if (isLogLevel(&level, *string)) {
        systemLogLevel = level;
      } else if (!enableLogCategory(*string)) {
        logMessage(LOG_ERR, "%s: %s", gettext("unknown log level or category"), *string);
      }

      string += 1;
    }

    deallocateStrings(strings);
  }
}

ProgramExitStatus
brlttyStart (int argc, char *argv[]) {
  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .doBootParameters = &opt_bootParameters,
      .doEnvironmentVariables = &opt_environmentVariables,
      .configurationFile = &opt_configurationFile,
      .applicationName = "brltty"
    };
    ProgramExitStatus exitStatus = processOptions(&descriptor, &argc, &argv);
    if (exitStatus == PROG_EXIT_FORCE) return PROG_EXIT_FORCE;
  }

  {
    char **const paths[] = {
      &opt_driversDirectory,
      &opt_writableDirectory,
      &opt_tablesDirectory,
      &opt_pidFile,
      NULL
    };
    fixInstallPaths(paths);
  }

  if (argc) {
    logMessage(LOG_ERR, "%s: %s", gettext("excess argument"), argv[0]);
  }

  if (opt_cancelExecution) {
    ProgramExitStatus exitStatus;

    if (!*opt_pidFile) {
      exitStatus = PROG_EXIT_SEMANTIC;
      logMessage(LOG_ERR, "%s", gettext("pid file not specified"));
    } else if (cancelProgram(opt_pidFile)) {
      exitStatus = PROG_EXIT_FORCE;
    } else {
      exitStatus = PROG_EXIT_FATAL;
    }

    return exitStatus;
  }

  if (!validateInterval(&updateInterval, opt_updateInterval)) {
    logMessage(LOG_ERR, "%s: %s", gettext("invalid update interval"), opt_updateInterval);
  }

  if (!validateInterval(&messageDelay, opt_messageDelay)) {
    logMessage(LOG_ERR, "%s: %s", gettext("invalid message delay"), opt_messageDelay);
  }

  /* Set logging levels. */
  processLogOperand(opt_logLevel);

  {
    unsigned char level;

    if (opt_standardError) {
      level = systemLogLevel;
      closeSystemLog();
    } else {
      level = LOG_NOTICE;
      if (opt_version || opt_verify) level += 1;
      if (opt_quiet) level -= 1;
    }

    stderrLogLevel = level;
  }

  if (*opt_logFile) {
    openLogFile(opt_logFile);
    closeSystemLog();
  }

  {
    const char *oldPrefix = setLogPrefix(NULL);
    char banner[0X100];

    makeProgramBanner(banner, sizeof(banner));
    logMessage(LOG_NOTICE, "%s [%s]", banner, PACKAGE_URL);
    setLogPrefix(oldPrefix);
  }

  if (opt_version) {
    logMessage(LOG_INFO, "%s", PACKAGE_COPYRIGHT);
    identifyScreenDrivers(1);

#ifdef ENABLE_API
    api_identify(1);
#endif /* ENABLE_API */

    identifyBrailleDrivers(1);

#ifdef ENABLE_SPEECH_SUPPORT
    identifySpeechDrivers(1);
#endif /* ENABLE_SPEECH_SUPPORT */

    return PROG_EXIT_FORCE;
  }

#ifdef __MINGW32__
  {
    int stop = 0;

    if (opt_removeService) {
      removeService(SERVICE_NAME);
      stop = 1;
    }

    if (opt_installService) {
      installService(SERVICE_NAME, SERVICE_DESCRIPTION);
      stop = 1;
    }

    if (stop) return PROG_EXIT_FORCE;
  }
#endif /* __MINGW32__ */

  if (opt_verify) opt_noDaemon = 1;
  if (!opt_noDaemon
#ifdef __MINGW32__
      && !isWindowsService
#endif
     ) {
    background();
  }
  if (!tryPidFile()) return PROG_EXIT_SEMANTIC;

  if (!opt_noDaemon) {
    fflush(stdout);
    fflush(stderr);
    stderrLogLevel = 0;

#if defined(GRUB_RUNTIME)

#else /* redirect stdio streams to /dev/null */
    {
      const char *nullDevice = "/dev/null";

      if (!freopen(nullDevice, "r", stdin)) {
        logSystemError("freopen[stdin]");
      }

      if (!freopen(nullDevice, "a", stdout)) {
        logSystemError("freopen[stdout]");
      }

      if (!opt_standardError) {
        if (!freopen(nullDevice, "a", stderr)) {
          logSystemError("freopen[stderr]");
        }
      }
    }
#endif /* redirect stdio streams to /dev/null */

#ifdef __MINGW32__
    {
      HANDLE h = CreateFile("NUL", GENERIC_READ|GENERIC_WRITE,
                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, 0, NULL);

      if (!h) {
        logWindowsSystemError("CreateFile[NUL]");
      } else {
        SetStdHandle(STD_INPUT_HANDLE, h);
        SetStdHandle(STD_OUTPUT_HANDLE, h);

        if (!opt_standardError) {
          SetStdHandle(STD_ERROR_HANDLE, h);
        }
      }
    }
#endif /* __MINGW32__ */
  }

  /*
   * From this point, all IO functions as printf, puts, perror, etc. can't be
   * used anymore since we are a daemon.  The logMessage() facility should 
   * be used instead.
   */

  onProgramExit(exitScreens);
  constructSpecialScreens();
  enableBrailleHelpPage(); /* ensure that it's first */

  onProgramExit(exitTunes);
  suppressTuneDeviceOpenErrors();

  {
    char *directory;

    if ((directory = getWorkingDirectory())) {
      logMessage(LOG_INFO, "%s: %s", gettext("Working Directory"), directory);
      free(directory);
    } else {
      logMessage(LOG_ERR, "%s: %s", gettext("cannot determine working directory"), strerror(errno));
    }
  }

  logMessage(LOG_INFO, "%s: %s", gettext("Writable Directory"), opt_writableDirectory);
  writableDirectory = opt_writableDirectory;

  logMessage(LOG_INFO, "%s: %s", gettext("Configuration File"), opt_configurationFile);
  logMessage(LOG_INFO, "%s: %s", gettext("Preferences File"), opt_preferencesFile);
  loadPreferences();

  logMessage(LOG_INFO, "%s: %s", gettext("Drivers Directory"), opt_driversDirectory);

  logMessage(LOG_INFO, "%s: %s", gettext("Tables Directory"), opt_tablesDirectory);
  setGlobalDataVariable("tablesDirectory", opt_tablesDirectory);

  /* handle text table option */
  if (*opt_textTable) {
    if (strcmp(opt_textTable, "auto") == 0) {
      char *name = selectTextTable(opt_tablesDirectory);
      opt_textTable = "";

      if (name) {
        if (replaceTextTable(opt_tablesDirectory, name)) {
          opt_textTable = name;
        } else {
          free(name);
        }
      }
    } else {
      if (!replaceTextTable(opt_tablesDirectory, opt_textTable)) opt_textTable = "";
    }
  }

  if (!*opt_textTable) opt_textTable = TEXT_TABLE;
  logMessage(LOG_INFO, "%s: %s", gettext("Text Table"), opt_textTable);

  /* handle attributes table option */
  if (*opt_attributesTable)
    if (!replaceAttributesTable(opt_tablesDirectory, opt_attributesTable))
      opt_attributesTable = "";
  if (!*opt_attributesTable) opt_attributesTable = ATTRIBUTES_TABLE;
  logMessage(LOG_INFO, "%s: %s", gettext("Attributes Table"), opt_attributesTable);

#ifdef ENABLE_CONTRACTED_BRAILLE
  /* handle contraction table option */
  onProgramExit(exitContractionTable);
  if (*opt_contractionTable) loadContractionTable(opt_contractionTable);
  logMessage(LOG_INFO, "%s: %s", gettext("Contraction Table"),
             *opt_contractionTable? opt_contractionTable: gettext("none"));
#endif /* ENABLE_CONTRACTED_BRAILLE */

  /* handle key table option */
  onProgramExit(exitKeyTable);
  replaceKeyboardKeyTable(opt_keyTable);
  logMessage(LOG_INFO, "%s: %s", gettext("Key Table"),
             *opt_keyTable? opt_keyTable: gettext("none"));

  if (parseKeyboardProperties(&keyboardProperties, opt_keyboardProperties))
    if (keyboardKeyTable)
      scheduleKeyboardMonitor(0);

  /* initialize screen driver */
  screenDrivers = splitString(opt_screenDriver? opt_screenDriver: "", ',', NULL);
  if (opt_verify) {
    if (activateScreenDriver(1)) deactivateScreenDriver();
  } else {
    tryScreenDriver();
  }
  
#ifdef ENABLE_API
  apiStarted = 0;
  if (!opt_noApi) {
    api_identify(0);
    apiParameters = getParameters(api_parameters,
                                  NULL,
                                  opt_apiParameters);
    logParameters(api_parameters, apiParameters,
                  gettext("API Parameter"));
    if (!opt_verify) {
      if (api_start(&brl, apiParameters)) {
        onProgramExit(exitApi);
        apiStarted = 1;
      }
    }
  }
#endif /* ENABLE_API */

  /* The device(s) the braille display might be connected to. */
  if (!*opt_brailleDevice) {
    logMessage(LOG_ERR, gettext("braille device not specified"));
    return PROG_EXIT_SYNTAX;
  }
  brailleDevices = splitString(opt_brailleDevice, ',', NULL);

  /* Activate the braille display. */
  brailleDrivers = splitString(opt_brailleDriver? opt_brailleDriver: "", ',', NULL);
  brailleConstructed = 0;
  if (opt_verify) {
    if (activateBrailleDriver(1)) deactivateBrailleDriver();
  } else {
    onProgramExit(exitBrailleDriver);
    tryBrailleDriver();
  }

#ifdef ENABLE_SPEECH_SUPPORT
  /* Activate the speech synthesizer. */
  speechDrivers = splitString(opt_speechDriver? opt_speechDriver: "", ',', NULL);
  if (opt_verify) {
    if (activateSpeechDriver(1)) deactivateSpeechDriver();
  } else {
    onProgramExit(exitSpeechDriver);
    trySpeechDriver();
  }

  /* Create the file system object for speech input. */
  logMessage(LOG_INFO, "%s: %s", gettext("Speech Input"),
             *opt_speechInput? opt_speechInput: gettext("none"));
  if (!opt_verify) {
    if (*opt_speechInput) enableSpeechInput(opt_speechInput);
  }
#endif /* ENABLE_SPEECH_SUPPORT */

  return opt_verify? PROG_EXIT_FORCE: PROG_EXIT_SUCCESS;
}
