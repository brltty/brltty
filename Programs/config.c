/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2011 by The BRLTTY Developers.
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
#include <locale.h>
#include <signal.h>
#include <fcntl.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */

#if defined(HAVE_GLOB_H)
#include <glob.h>
#elif defined(__MINGW32__)
#include <io.h>
#else /* glob: paradigm-specific global definitions */
#warning file globbing support not available on this platform
#endif /* glob: paradigm-specific global definitions */

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
#include "misc.h"
#include "system.h"
#include "async.h"
#include "program.h"
#include "options.h"
#include "brltty.h"
#include "prefs.h"
#include "menu.h"
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

static char *opt_tablesDirectory;
static char *opt_textTable;
static char *opt_attributesTable;

#ifdef ENABLE_CONTRACTED_BRAILLE
static char *opt_contractionTable;
ContractionTable *contractionTable = NULL;
#endif /* ENABLE_CONTRACTED_BRAILLE */

static char *opt_keyTable;
static KeyTable *keyboardKeyTable = NULL;

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

  { .letter = 'P',
    .word = "pid-file",
    .flags = OPT_Hidden,
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

static const char *
getTextTableLocale (void) {
#if defined(__MINGW32__)
  return win_getLocale();
#else /* unix */
  return setlocale(LC_CTYPE, NULL);
#endif /* text table locale */
}

static int
testTextTable (char *table) {
  int exists = 0;
  char *file;

  if ((file = ensureTextTableExtension(table))) {
    char *path;

    logMessage(LOG_DEBUG, "checking for text table: %s", file);

    if ((path = makePath(opt_tablesDirectory, file))) {
      if (testPath(path)) exists = 1;

      free(path);
    }

    free(file);
  }

  return exists;
}

static int
replaceTextTable (const char *name) {
  int ok = 0;
  char *file;

  if ((file = ensureTextTableExtension(name))) {
    char *path;

    if ((path = makePath(opt_tablesDirectory, file))) {
      TextTable *newTable;

      logMessage(LOG_DEBUG, "compiling text table: %s", path);
      if ((newTable = compileTextTable(path))) {
        TextTable *oldTable = textTable;
        textTable = newTable;
        destroyTextTable(oldTable);
        ok = 1;
      } else {
        logMessage(LOG_ERR, "%s: %s", gettext("cannot compile text table"), path);
      }

      free(path);
    }

    free(file);
  }

  if (!ok) logMessage(LOG_ERR, "%s: %s", gettext("cannot load text table"), name);
  return ok;
}

static int
replaceAttributesTable (const char *name) {
  int ok = 0;
  char *file;

  if ((file = ensureAttributesTableExtension(name))) {
    char *path;

    if ((path = makePath(opt_tablesDirectory, file))) {
      AttributesTable *newTable;

      logMessage(LOG_DEBUG, "compiling attributes table: %s", path);
      if ((newTable = compileAttributesTable(path))) {
        AttributesTable *oldTable = attributesTable;
        attributesTable = newTable;
        destroyAttributesTable(oldTable);
        ok = 1;
      } else {
        logMessage(LOG_ERR, "%s: %s", gettext("cannot compile attributes table"), path);
      }

      free(path);
    }

    free(file);
  }

  if (!ok) logMessage(LOG_ERR, "%s: %s", gettext("cannot load attributes table"), name);
  return ok;
}

int
readCommand (KeyTableCommandContext context) {
  int command = readBrailleCommand(&brl, context);
  if (command != EOF) {
    logCommand(command);
    if (BRL_DELAYED_COMMAND(command)) command = BRL_CMD_NOOP;
    command &= BRL_MSK_CMD;
  }
  return command;
}

#ifdef ENABLE_CONTRACTED_BRAILLE
static void
exitContractionTable (void) {
  if (contractionTable) {
    destroyContractionTable(contractionTable);
    contractionTable = NULL;
  }
}

static int
loadContractionTable (const char *name) {
  ContractionTable *table = NULL;

  if (*name) {
    char *file;

    if ((file = ensureContractionTableExtension(name))) {
      char *path;

      if ((path = makePath(opt_tablesDirectory, file))) {
        logMessage(LOG_DEBUG, "compiling contraction table: %s", path);

        if (!(table = compileContractionTable(path))) {
          logMessage(LOG_ERR, "%s: %s", gettext("cannot compile contraction table"), path);
        }

        free(path);
      }

      free(file);
    }

    if (!table) return 0;
  }

  if (contractionTable) destroyContractionTable(contractionTable);
  contractionTable = table;
  return 1;
}
#endif /* ENABLE_CONTRACTED_BRAILLE */

static void
exitKeyTable (void) {
  if (keyboardKeyTable) {
    destroyKeyTable(keyboardKeyTable);
    keyboardKeyTable = NULL;
  }
}

static int
loadKeyTable (const char *name) {
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

  if (keyboardKeyTable) destroyKeyTable(keyboardKeyTable);
  keyboardKeyTable = table;
  return 1;
}

static KeyTableState
handleKeyboardKeyEvent (unsigned char set, unsigned char key, int press) {
  return processKeyEvent(keyboardKeyTable, getCurrentCommandContext(), set, key, press);
}

static void scheduleKeyboardMonitor (int interval);

static void
tryKeyboardMonitor (void *data) {
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
windowConfigurationChanged (int infoLevel, int rows, int columns) {
  textStart = 0;
  textCount = columns;
  statusStart = 0;
  statusCount = 0;

  if (!(textMaximized || haveStatusCells())) {
    int separatorWidth = (prefs.statusSeparator == ssNone)? 0: 1;
    int statusWidth = prefs.statusCount;

    if (!statusWidth) statusWidth = getStatusFieldsLength(prefs.statusFields);
    statusWidth = MAX(MIN(statusWidth, brl.textColumns-1-separatorWidth), 0);

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
  windowConfigurationChanged(LOG_INFO, brl.textRows, brl.textColumns);
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
  if (speech->rate) setSpeechRate(&spk, prefs.speechRate, 0);
  if (speech->volume) setSpeechVolume(&spk, prefs.speechVolume, 0);
  if (speech->pitch) setSpeechPitch(&spk, prefs.speechPitch, 0);
  if (speech->punctuation) setSpeechPunctuation(&spk, prefs.speechPunctuation, 0);
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

static void
resetStatusFields (void) {
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

static char *
makePreferencesFilePath (void) {
  return makePath(STATE_DIRECTORY, opt_preferencesFile);
}

int
loadPreferences (void) {
  int ok = 0;

  {
    char *path = makePreferencesFilePath();

    if (path) {
      if (testPath(path)) {
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
  char *path = makePreferencesFilePath();

  if (path) {
    FILE *file = openDataFile(path, "w+b", 0);

    if (file) {
      size_t length = fwrite(&prefs, 1, sizeof(prefs), file);

      if (length == sizeof(prefs)) {
        ok = 1;
        oldPreferencesEnabled = 0;
      } else {
        if (!ferror(file)) errno = EIO;
        logMessage(LOG_ERR, "%s: %s: %s",
                   gettext("cannot write to preferences file"), path, strerror(errno));
      }

      fclose(file);
    }

    free(path);
  }

  if (!ok) message(NULL, gettext("not saved"), 0);
  return ok;
}

static int
testBrailleFirmness (void) {
  return brl.setFirmness != NULL;
}

static int
changedBrailleFirmness (unsigned char setting) {
  return setBrailleFirmness(&brl, setting);
}

static int
testBrailleSensitivity (void) {
  return brl.setSensitivity != NULL;
}

static int
changedBrailleSensitivity (unsigned char setting) {
  return setBrailleSensitivity(&brl, setting);
}

static int
testStatusPosition (void) {
  return !haveStatusCells();
}

static int
changedStatusPosition (unsigned char setting) {
  reconfigureWindow();
  return 1;
}

static int
testStatusCount (void) {
  return testStatusPosition() && (prefs.statusPosition != spNone);
}

static int
changedStatusCount (unsigned char setting) {
  reconfigureWindow();
  return 1;
}

static int
testStatusSeparator (void) {
  return testStatusCount();
}

static int
changedStatusSeparator (unsigned char setting) {
  reconfigureWindow();
  return 1;
}

static int
testStatusField (unsigned char index) {
  return (haveStatusCells() || (prefs.statusPosition != spNone)) &&
         ((index == 0) || (prefs.statusFields[index-1] != sfEnd));
}

static int
changedStatusField (unsigned char index, unsigned char setting) {
  switch (setting) {
    case sfGeneric:
      if (index > 0) return 0;
      if (!haveStatusCells()) return 0;
      if (!braille->statusFields) return 0;
      if (*braille->statusFields != sfGeneric) return 0;

    case sfEnd:
      if (prefs.statusFields[index+1] != sfEnd) return 0;
      break;

    default:
      if ((index > 0) && (prefs.statusFields[index-1] == sfGeneric)) return 0;
      break;
  }

  reconfigureWindow();
  return 1;
}

#define STATUS_FIELD_HANDLERS(n) \
  static int testStatusField##n (void) { return testStatusField(n-1); } \
  static int changedStatusField##n (unsigned char setting) { return changedStatusField(n-1, setting); }
STATUS_FIELD_HANDLERS(1)
STATUS_FIELD_HANDLERS(2)
STATUS_FIELD_HANDLERS(3)
STATUS_FIELD_HANDLERS(4)
STATUS_FIELD_HANDLERS(5)
STATUS_FIELD_HANDLERS(6)
STATUS_FIELD_HANDLERS(7)
STATUS_FIELD_HANDLERS(8)
STATUS_FIELD_HANDLERS(9)
#undef STATUS_FIELD_HANDLERS

#ifdef ENABLE_SPEECH_SUPPORT
static int
testSpeechRate (void) {
  return speech->rate != NULL;
}

static int
changedSpeechRate (unsigned char setting) {
  setSpeechRate(&spk, setting, 1);
  return 1;
}

static int
testSpeechVolume (void) {
  return speech->volume != NULL;
}

static int
changedSpeechVolume (unsigned char setting) {
  setSpeechVolume(&spk, setting, 1);
  return 1;
}

static int
testSpeechPitch (void) {
  return speech->pitch != NULL;
}

static int
changedSpeechPitch (unsigned char setting) {
  setSpeechPitch(&spk, setting, 1);
  return 1;
}

static int
testSpeechPunctuation (void) {
  return speech->punctuation != NULL;
}

static int
changedSpeechPunctuation (unsigned char setting) {
  setSpeechPunctuation(&spk, setting, 1);
  return 1;
}
#endif /* ENABLE_SPEECH_SUPPORT */

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

typedef struct {
  const char *directory;
  const char *extension;
  const char *pattern;
  char *initial;
  char *current;
  unsigned none:1;

#if defined(HAVE_GLOB_H)
  glob_t glob;
#elif defined(__MINGW32__)
  char **names;
  int offset;
#endif /* glob: paradigm-specific field declarations */

  const char **paths;
  int count;
  unsigned char setting;
  const char *pathsArea[3];

  MenuItem *menuItem;
} GlobData;

static GlobData glob_textTable;
static GlobData glob_attributesTable;

static int
qsortCompare_fileNames (const void *element1, const void *element2) {
  const char *const *name1 = element1;
  const char *const *name2 = element2;
  return strcmp(*name1, *name2);
}

static void
globPrepare (
  GlobData *data, MenuItem *menuItem,
  const char *directory, const char *extension,
  const char *initial, int none
) {
  memset(data, 0, sizeof(*data));

  data->directory = directory;
  data->extension = extension;
  data->none = !!none;
  data->menuItem = menuItem;

  {
    const char *strings[] = {"*", extension};
    data->pattern = joinStrings(strings, ARRAY_COUNT(strings));
  }

  data->initial = *initial? ensureExtension(initial, extension): strdup("");
  data->current = strdup(data->initial);
}

static void
globBegin (GlobData *data) {
  int index;

  data->paths = data->pathsArea;
  data->count = ARRAY_COUNT(data->pathsArea) - 1;
  data->paths[data->count] = NULL;
  index = data->count;

  {
#ifdef HAVE_FCHDIR
    int originalDirectory = open(".", O_RDONLY);
    if (originalDirectory != -1)
#else /* HAVE_FCHDIR */
    char *originalDirectory = getWorkingDirectory();
    if (originalDirectory)
#endif /* HAVE_FCHDIR */
    {
      if (chdir(data->directory) != -1) {
#if defined(HAVE_GLOB_H)
        memset(&data->glob, 0, sizeof(data->glob));
        data->glob.gl_offs = data->count;

        if (glob(data->pattern, GLOB_DOOFFS, NULL, &data->glob) == 0) {
          data->paths = (const char **)data->glob.gl_pathv;
          /* The behaviour of gl_pathc is inconsistent. Some implementations
           * include the leading NULL pointers and some don't. Let's just
           * figure it out the hard way by finding the trailing NULL.
           */
          while (data->paths[data->count]) data->count += 1;
        }
#elif defined(__MINGW32__)
        struct _finddata_t findData;
        long findHandle = _findfirst(data->pattern, &findData);
        int allocated = data->count | 0XF;

        data->offset = data->count;
        data->names = malloc(allocated * sizeof(*data->names));

        if (findHandle != -1) {
          do {
            if (data->count >= allocated) {
              allocated = allocated * 2;
              data->names = realloc(data->names, allocated * sizeof(*data->names));
            }

            data->names[data->count++] = strdup(findData.name);
          } while (_findnext(findHandle, &findData) == 0);

          _findclose(findHandle);
        }

        data->names = realloc(data->names, data->count * sizeof(*data->names));
        data->paths = data->names;
#endif /* glob: paradigm-specific field initialization */

#ifdef HAVE_FCHDIR
        if (fchdir(originalDirectory) == -1) logSystemError("fchdir");
#else /* HAVE_FCHDIR */
        if (chdir(originalDirectory) == -1) logSystemError("chdir");
#endif /* HAVE_FCHDIR */
      } else {
        logMessage(LOG_ERR, "%s: %s: %s",
                   gettext("cannot set working directory"), data->directory, strerror(errno));
      }

#ifdef HAVE_FCHDIR
      close(originalDirectory);
#else /* HAVE_FCHDIR */
      free(originalDirectory);
#endif /* HAVE_FCHDIR */
    } else {
#ifdef HAVE_FCHDIR
      logMessage(LOG_ERR, "%s: %s",
                 gettext("cannot open working directory"), strerror(errno));
#else /* HAVE_FCHDIR */
      logMessage(LOG_ERR, "%s", gettext("cannot determine working directory"));
#endif /* HAVE_FCHDIR */
    }
  }

  qsort(&data->paths[index], data->count-index, sizeof(*data->paths), qsortCompare_fileNames);
  if (data->none) data->paths[--index] = "";
  data->paths[--index] = data->initial;
  data->paths += index;
  data->count -= index;
  data->setting = 0;

  for (index=1; index<data->count; index+=1) {
    if (strcmp(data->paths[index], data->initial) == 0) {
      data->paths += 1;
      data->count -= 1;
      break;
    }
  }

  for (index=0; index<data->count; index+=1) {
    if (strcmp(data->paths[index], data->current) == 0) {
      data->setting = index;
      break;
    }
  }

  setMenuItemStrings(data->menuItem, data->paths, data->count);
}

static void
globEnd (GlobData *data) {
#if defined(HAVE_GLOB_H)
  if (data->glob.gl_pathc) {
    int i;
    for (i=0; i<data->glob.gl_offs; i+=1) data->glob.gl_pathv[i] = NULL;
    globfree(&data->glob);
  }
#elif defined(__MINGW32__)
  if (data->names) {
    int i;
    for (i=data->offset; i<data->count; i++) free(data->names[i]);
    free(data->names);
  }
#endif /* glob: paradigm-specific memory deallocation */
}

static const char *
globChanged (GlobData *data) {
  char *path = strdup(data->paths[data->setting]);
  if (path) {
    free(data->current);
    return data->current = path;
  } else {
    logSystemError("strdup");
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
testContractedBraille (void) {
  return prefs.textStyle == tsContractedBraille;
}

static int
changedContractionTable (unsigned char setting) {
  return loadContractionTable(globChanged(&glob_contractionTable));
}
#endif /* ENABLE_CONTRACTED_BRAILLE */

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
  reconfigureWindow();
  return 1;
}

static int
testAutorepeat (void) {
  return prefs.autorepeat;
}

static int
changedAutorepeat (unsigned char setting) {
  if (setting) resetAutorepeat();
  return 1;
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

static MenuItem *
newGlobMenuItem (Menu *menu, GlobData *data, const char *label) {
  static MenuItemString strings[] = {
    NULL
  };

  return newEnumeratedMenuItem(menu, &data->setting, label, strings);
}

static MenuItem *
newStatusFieldMenuItem (
  Menu *menu, unsigned char number, const char *label,
  MenuItemTester *test, MenuItemChanged *changed
) {
  static MenuItemString strings[] = {
    strtext("End"),
    strtext("Window Coordinates (2 cells)"),
    strtext("Window Column (1 cell)"),
    strtext("Window Row (1 cell)"),
    strtext("Cursor Coordinates (2 cells)"),
    strtext("Cursor Column (1 cell)"),
    strtext("Cursor Row (1 cell)"),
    strtext("Cursor and Window Column (2 cells)"),
    strtext("Cursor and Window Row (2 cells)"),
    strtext("Screen Number (1 cell)"),
    strtext("State Dots (1 cell)"),
    strtext("State Letter (1 cell)"),
    strtext("Time (2 cells)"),
    strtext("Alphabetic Window Coordinates (1 cell)"),
    strtext("Alphabetic Cursor Coordinates (1 cell)"),
    strtext("Generic")
  };

  MenuItem *item = newEnumeratedMenuItem(menu, &prefs.statusFields[number-1], label, strings);

  if (item) {
    setMenuItemTester(item, test);
    setMenuItemChanged(item, changed);
  }

  return item;
}

static MenuItem *
newTimeMenuItem (Menu *menu, unsigned char *setting, const char *label) {
  return newNumericMenuItem(menu, setting, label, 1, 100, updateInterval/10);
}

#if defined(ENABLE_PCM_SUPPORT) || defined(ENABLE_MIDI_SUPPORT) || defined(ENABLE_FM_SUPPORT)
static MenuItem *
newVolumeMenuItem (Menu *menu, unsigned char *setting, const char *label) {
  return newNumericMenuItem(menu, setting, label, 0, 100, 5);
}
#endif /* defined(ENABLE_PCM_SUPPORT) || defined(ENABLE_MIDI_SUPPORT) || defined(ENABLE_FM_SUPPORT) */

static unsigned char saveOnExit = 0;                /* 1 == save preferences on exit */

static Menu *
makePreferencesMenu (void) {
  Menu *menu = newMenu();
  if (!menu) goto noMenu;

#define ITEM(new) MenuItem *item = (new); if (!item) goto noItem
#define TEST(property) setMenuItemTester(item, test##property)
#define CHANGED(setting) setMenuItemChanged(item, changed##setting)

  {
    ITEM(newBooleanMenuItem(menu, &saveOnExit, strtext("Save on Exit")));
  }

  {
    static MenuItemString strings[] = {
      strtext("8-Dot Computer Braille"),
      strtext("Contracted Braille"),
      strtext("6-Dot Computer Braille")
    };

    ITEM(newEnumeratedMenuItem(menu, &prefs.textStyle, strtext("Text Style"), strings));
  }

#ifdef ENABLE_CONTRACTED_BRAILLE
  {
    ITEM(newBooleanMenuItem(menu, &prefs.expandCurrentWord, strtext("Expand Current Word")));
    TEST(ContractedBraille);
  }
#endif /* ENABLE_CONTRACTED_BRAILLE */

  {
    ITEM(newBooleanMenuItem(menu, &prefs.skipIdenticalLines, strtext("Skip Identical Lines")));
  }

  {
    ITEM(newBooleanMenuItem(menu, &prefs.skipBlankWindows, strtext("Skip Blank Windows")));
  }

  {
    static MenuItemString strings[] = {
      strtext("All"),
      strtext("End of Line"),
      strtext("Rest of Line")
    };

    ITEM(newEnumeratedMenuItem(menu, &prefs.blankWindowsSkipMode, strtext("Which Blank Windows"), strings));
    TEST(SkipBlankWindows);
  }

  {
    ITEM(newBooleanMenuItem(menu, &prefs.slidingWindow, strtext("Sliding Window")));
  }

  {
    ITEM(newBooleanMenuItem(menu, &prefs.eagerSlidingWindow, strtext("Eager Sliding Window")));
    TEST(SlidingWindow);
  }

  {
    ITEM(newNumericMenuItem(menu, &prefs.windowOverlap, strtext("Window Overlap"), 0, 20, 1));
    CHANGED(WindowOverlap);
  }

  {
    ITEM(newBooleanMenuItem(menu, &prefs.autorepeat, strtext("Autorepeat")));
    CHANGED(Autorepeat);
  }

  {
    ITEM(newBooleanMenuItem(menu, &prefs.autorepeatPanning, strtext("Autorepeat Panning")));
    TEST(Autorepeat);
  }

  {
    ITEM(newTimeMenuItem(menu, &prefs.autorepeatDelay, strtext("Autorepeat Delay")));
    TEST(Autorepeat);
  }

  {
    ITEM(newTimeMenuItem(menu, &prefs.autorepeatInterval, strtext("Autorepeat Interval")));
    TEST(Autorepeat);
  }

  {
    ITEM(newBooleanMenuItem(menu, &prefs.showCursor, strtext("Show Cursor")));
  }

  {
    static MenuItemString strings[] = {
      strtext("Underline"),
      strtext("Block")
    };

    ITEM(newEnumeratedMenuItem(menu, &prefs.cursorStyle, strtext("Cursor Style"), strings));
    TEST(ShowCursor);
  }

  {
    ITEM(newBooleanMenuItem(menu, &prefs.blinkingCursor, strtext("Blinking Cursor")));
    TEST(ShowCursor);
  }

  {
    ITEM(newTimeMenuItem(menu, &prefs.cursorVisibleTime, strtext("Cursor Visible Time")));
    TEST(BlinkingCursor);
  }

  {
    ITEM(newTimeMenuItem(menu, &prefs.cursorInvisibleTime, strtext("Cursor Invisible Time")));
    TEST(BlinkingCursor);
  }

  {
    ITEM(newBooleanMenuItem(menu, &prefs.showAttributes, strtext("Show Attributes")));
  }

  {
    ITEM(newBooleanMenuItem(menu, &prefs.blinkingAttributes, strtext("Blinking Attributes")));
    TEST(ShowAttributes);
  }

  {
    ITEM(newTimeMenuItem(menu, &prefs.attributesVisibleTime, strtext("Attributes Visible Time")));
    TEST(BlinkingAttributes);
  }

  {
    ITEM(newTimeMenuItem(menu, &prefs.attributesInvisibleTime, strtext("Attributes Invisible Time")));
    TEST(BlinkingAttributes);
  }

  {
    ITEM(newBooleanMenuItem(menu, &prefs.blinkingCapitals, strtext("Blinking Capitals")));
  }

  {
    ITEM(newTimeMenuItem(menu, &prefs.capitalsVisibleTime, strtext("Capitals Visible Time")));
    TEST(BlinkingCapitals);
  }

  {
    ITEM(newTimeMenuItem(menu, &prefs.capitalsInvisibleTime, strtext("Capitals Invisible Time")));
    TEST(BlinkingCapitals);
  }

  {
    static MenuItemString strings[] = {
      strtext("Minimum"),
      strtext("Low"),
      strtext("Medium"),
      strtext("High"),
      strtext("Maximum")
    };

    ITEM(newEnumeratedMenuItem(menu, &prefs.brailleFirmness, strtext("Braille Firmness"), strings));
    TEST(BrailleFirmness);
    CHANGED(BrailleFirmness);
  }

  {
    static MenuItemString strings[] = {
      strtext("Minimum"),
      strtext("Low"),
      strtext("Medium"),
      strtext("High"),
      strtext("Maximum")
    };

    ITEM(newEnumeratedMenuItem(menu, &prefs.brailleSensitivity, strtext("Braille Sensitivity"), strings));
    TEST(BrailleSensitivity);
    CHANGED(BrailleSensitivity);
  }

#ifdef HAVE_LIBGPM
  {
    ITEM(newBooleanMenuItem(menu, &prefs.windowFollowsPointer, strtext("Window Follows Pointer")));
  }
#endif /* HAVE_LIBGPM */

  {
    ITEM(newBooleanMenuItem(menu, &prefs.highlightWindow, strtext("Highlight Window")));
  }

  {
    ITEM(newBooleanMenuItem(menu, &prefs.alertTunes, strtext("Alert Tunes")));
  }

  {
    static MenuItemString strings[] = {
      "Beeper"
        " ("
#ifdef ENABLE_BEEPER_SUPPORT
        strtext("console tone generator")
#else /* ENABLE_BEEPER_SUPPORT */
        strtext("unsupported")
#endif /* ENABLE_BEEPER_SUPPORT */
        ")",

      "PCM"
        " ("
#ifdef ENABLE_PCM_SUPPORT
        strtext("soundcard digital audio")
#else /* ENABLE_PCM_SUPPORT */
        strtext("unsupported")
#endif /* ENABLE_PCM_SUPPORT */
        ")",

      "MIDI"
        " ("
#ifdef ENABLE_MIDI_SUPPORT
        strtext("Musical Instrument Digital Interface")
#else /* ENABLE_MIDI_SUPPORT */
        strtext("unsupported")
#endif /* ENABLE_MIDI_SUPPORT */
        ")",

      "FM"
        " ("
#ifdef ENABLE_FM_SUPPORT
        strtext("soundcard synthesizer")
#else /* ENABLE_FM_SUPPORT */
        strtext("unsupported")
#endif /* ENABLE_FM_SUPPORT */
        ")"
    };

    ITEM(newEnumeratedMenuItem(menu, &prefs.tuneDevice, strtext("Tune Device"), strings));
    TEST(Tunes);
    CHANGED(TuneDevice);
  }

#ifdef ENABLE_PCM_SUPPORT
  {
    ITEM(newVolumeMenuItem(menu, &prefs.pcmVolume, strtext("PCM Volume")));
    TEST(TunesPcm);
  }
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
  {
    ITEM(newVolumeMenuItem(menu, &prefs.midiVolume, strtext("MIDI Volume")));
    TEST(TunesMidi);
  }

  {
    ITEM(newStringsMenuItem(menu, &prefs.midiInstrument, strtext("MIDI Instrument"), midiInstrumentTable, midiInstrumentCount));
    TEST(TunesMidi);
  }
#endif /* ENABLE_MIDI_SUPPORT */

#ifdef ENABLE_FM_SUPPORT
  {
    ITEM(newVolumeMenuItem(menu, &prefs.fmVolume, strtext("FM Volume")));
    TEST(TunesFm);
  }
#endif /* ENABLE_FM_SUPPORT */

  {
    ITEM(newBooleanMenuItem(menu, &prefs.alertDots, strtext("Alert Dots")));
  }

  {
    ITEM(newBooleanMenuItem(menu, &prefs.alertMessages, strtext("Alert Messages")));
  }

#ifdef ENABLE_SPEECH_SUPPORT
  {
    static MenuItemString strings[] = {
      strtext("Immediate"),
      strtext("Enqueue")
    };

    ITEM(newEnumeratedMenuItem(menu, &prefs.sayLineMode, strtext("Say-Line Mode"), strings));
  }

  {
    ITEM(newBooleanMenuItem(menu, &prefs.autospeak, strtext("Autospeak")));
  }

  {
    ITEM(newNumericMenuItem(menu, &prefs.speechRate, strtext("Speech Rate"), 0, SPK_RATE_MAXIMUM, 1));
    TEST(SpeechRate);
    CHANGED(SpeechRate);
  }

  {
    ITEM(newNumericMenuItem(menu, &prefs.speechVolume, strtext("Speech Volume"), 0, SPK_VOLUME_MAXIMUM, 1));
    TEST(SpeechVolume);
    CHANGED(SpeechVolume);
  }

  {
    ITEM(newNumericMenuItem(menu, &prefs.speechPitch, strtext("Speech Pitch"), 0, SPK_PITCH_MAXIMUM, 1));
    TEST(SpeechPitch);
    CHANGED(SpeechPitch);
  }

  {
    static MenuItemString strings[] = {
      strtext("None"),
      strtext("Some"),
      strtext("All")
    };

    ITEM(newEnumeratedMenuItem(menu, &prefs.speechPunctuation, strtext("Speech Punctuation"), strings));
    TEST(SpeechPunctuation);
    CHANGED(SpeechPunctuation);
  }
#endif /* ENABLE_SPEECH_SUPPORT */

  {
    static MenuItemString strings[] = {
      strtext("None"),
      strtext("Left"),
      strtext("Right")
    };

    ITEM(newEnumeratedMenuItem(menu, &prefs.statusPosition, strtext("Status Position"), strings));
    TEST(StatusPosition);
    CHANGED(StatusPosition);
  }

  {
    ITEM(newNumericMenuItem(menu, &prefs.statusCount, strtext("Status Count"), 0, MAX((int)brl.textColumns/2-1, 0), 1));
    TEST(StatusCount);
    CHANGED(StatusCount);
  }

  {
    static MenuItemString strings[] = {
      strtext("None"),
      strtext("Space"),
      strtext("Block"),
      strtext("Status Side"),
      strtext("Text Side")
    };

    ITEM(newEnumeratedMenuItem(menu, &prefs.statusSeparator, strtext("Status Separator"), strings));
    TEST(StatusSeparator);
    CHANGED(StatusSeparator);
  }

  {
#define STATUS_FIELD_ITEM(number) { ITEM(newStatusFieldMenuItem(menu, number, strtext("Status Field ") #number, testStatusField##number, changedStatusField##number)); }
    STATUS_FIELD_ITEM(1);
    STATUS_FIELD_ITEM(2);
    STATUS_FIELD_ITEM(3);
    STATUS_FIELD_ITEM(4);
    STATUS_FIELD_ITEM(5);
    STATUS_FIELD_ITEM(6);
    STATUS_FIELD_ITEM(7);
    STATUS_FIELD_ITEM(8);
    STATUS_FIELD_ITEM(9);
#undef STATUS_FIELD_ITEM
  }

  {
    ITEM(newGlobMenuItem(menu, &glob_textTable, strtext("Text Table")));
    CHANGED(TextTable);
    globPrepare(&glob_textTable, item, opt_tablesDirectory, TEXT_TABLE_EXTENSION, opt_textTable, 0);
  }

  {
    ITEM(newGlobMenuItem(menu, &glob_attributesTable, strtext("Attributes Table")));
    CHANGED(AttributesTable);
    globPrepare(&glob_attributesTable, item, opt_tablesDirectory, ATTRIBUTES_TABLE_EXTENSION, opt_attributesTable, 0);
  }

#ifdef ENABLE_CONTRACTED_BRAILLE
  {
    ITEM(newGlobMenuItem(menu, &glob_contractionTable, strtext("Contraction Table")));
    CHANGED(ContractionTable);
    globPrepare(&glob_contractionTable, item, opt_tablesDirectory, CONTRACTION_TABLE_EXTENSION, opt_contractionTable, 1);
  }
#endif /* ENABLE_CONTRACTED_BRAILLE */

#undef ITEM
#undef TEST
#undef CHANGED

  return menu;

noItem:
  deallocateMenu(menu);
noMenu:
  return NULL;
}

int
updatePreferences (void) {
  const char *mode = "prf";
  static Menu *menu = NULL;
  int ok = 1;

  if (!menu) menu = makePreferencesMenu();

  globBegin(&glob_textTable);
  globBegin(&glob_attributesTable);
#ifdef ENABLE_CONTRACTED_BRAILLE
  globBegin(&glob_contractionTable);
#endif /* ENABLE_CONTRACTED_BRAILLE */

  if (setStatusText(&brl, mode) &&
      message(mode, gettext("Preferences Menu"), 0)) {
    unsigned int lineIndent = 0;                                /* braille window pos in buffer */
    int indexChanged = 1;
    int settingChanged = 0;                        /* 1 when item's value has changed */

    Preferences oldPreferences = prefs;        /* backup preferences */
    int command = EOF;                                /* readbrl() value */

    if (prefs.autorepeat) resetAutorepeat();

    while (ok) {
      MenuItem *item = getCurrentMenuItem(menu);
      const char *value = getMenuItemValue(item);

      testProgramTermination();
      closeTuneDevice(0);

      {
        const char *label = getMenuItemLabel(item);
        const char *delimiter = ": ";
        unsigned int settingIndent = strlen(label) + strlen(delimiter);
        unsigned int valueLength = strlen(value);
        unsigned int lineLength = settingIndent + valueLength;
        char line[lineLength + 1];
        unsigned int textLength = textCount * brl.textRows;

        /* First we draw the current menu item in the buffer */
        snprintf(line,  sizeof(line), "%s%s%s", label, delimiter, value);

#ifdef ENABLE_SPEECH_SUPPORT
        if (prefs.autospeak) {
          if (indexChanged) {
            sayString(&spk, line, 1);
          } else if (settingChanged) {
            sayString(&spk, value, 1);
          }
        }
#endif /* ENABLE_SPEECH_SUPPORT */

        /* Next we deal with the braille window position in the buffer.
         * This is intended for small displays and/or long item descriptions 
         */
        if (settingChanged) {
          settingChanged = 0;
          /* make sure the updated value is visible */
          if (((lineLength - lineIndent) > textLength) &&
              (lineIndent < settingIndent))
            lineIndent = settingIndent;
        }
        indexChanged = 0;

        /* Then draw the braille window */
        if (!showBrailleText(mode, &line[lineIndent], updateInterval)) ok = 0;
        if (!ok) break;

        /* Now process any user interaction */
        command = readBrailleCommand(&brl, KTB_CTX_MENU);
        handleAutorepeat(&command, NULL);
        if (command != EOF) {
          switch (command) {
            case BRL_CMD_NOOP:
              continue;

            case BRL_CMD_HELP:
              /* This is quick and dirty... Something more intelligent 
               * and friendly needs to be done here...
               */
              message(mode,
                  "Press UP and DOWN to select an item, "
                  "HOME to toggle the setting. "
                  "Routing keys are available too! "
                  "Press PREFS again to quit.", MSG_WAITKEY|MSG_NODELAY);
              break;

            case BRL_BLK_PASSKEY+BRL_KEY_HOME:
            case BRL_CMD_PREFLOAD:
              prefs = oldPreferences;
              applyAllPreferences();
              message(mode, gettext("changes discarded"), 0);
              break;
            case BRL_BLK_PASSKEY+BRL_KEY_ENTER:
            case BRL_CMD_PREFSAVE:
              saveOnExit = 1;
              goto exitMenu;

            case BRL_CMD_TOP:
            case BRL_CMD_TOP_LEFT:
            case BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP:
            case BRL_CMD_MENU_FIRST_ITEM:
              if (setMenuFirstItem(menu)) {
                lineIndent = 0;
                indexChanged = 1;
              } else {
                playTune(&tune_command_rejected);
              }
              break;
            case BRL_CMD_BOT:
            case BRL_CMD_BOT_LEFT:
            case BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN:
            case BRL_CMD_MENU_LAST_ITEM:
              if (setMenuLastItem(menu)) {
                lineIndent = 0;
                indexChanged = 1;
              } else {
                playTune(&tune_command_rejected);
              }
              break;

            case BRL_CMD_LNUP:
            case BRL_CMD_PRDIFLN:
            case BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP:
            case BRL_CMD_MENU_PREV_ITEM:
              if (setMenuPreviousItem(menu)) {
                lineIndent = 0;
                indexChanged = 1;
              } else {
                playTune(&tune_command_rejected);
              }
              break;
            case BRL_CMD_LNDN:
            case BRL_CMD_NXDIFLN:
            case BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN:
            case BRL_CMD_MENU_NEXT_ITEM:
              if (setMenuNextItem(menu)) {
                lineIndent = 0;
                indexChanged = 1;
              } else {
                playTune(&tune_command_rejected);
              }
              break;

            case BRL_CMD_FWINLT:
              if (lineIndent > 0) {
                lineIndent -= MIN(textLength, lineIndent);
              } else {
                playTune(&tune_bounce);
              }
              break;
            case BRL_CMD_FWINRT:
              if ((lineLength - lineIndent) > textLength) {
                lineIndent += textLength;
              } else {
                playTune(&tune_bounce);
              }
              break;

            case BRL_CMD_WINUP:
            case BRL_CMD_CHRLT:
            case BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT:
            case BRL_CMD_BACK:
            case BRL_CMD_MENU_PREV_SETTING:
              if (changeMenuItemPrevious(item)) {
                settingChanged = 1;
              } else {
                playTune(&tune_command_rejected);
              }
              break;

            case BRL_CMD_WINDN:
            case BRL_CMD_CHRRT:
            case BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT:
            case BRL_CMD_HOME:
            case BRL_CMD_RETURN:
            case BRL_CMD_MENU_NEXT_SETTING:
              if (changeMenuItemNext(item)) {
                settingChanged = 1;
              } else {
                playTune(&tune_command_rejected);
              }
              break;

#ifdef ENABLE_SPEECH_SUPPORT
            case BRL_CMD_SAY_LINE:
              sayCharacters(&spk, line, lineLength, 1);
              break;
            case BRL_CMD_MUTE:
              speech->mute(&spk);
              break;
#endif /* ENABLE_SPEECH_SUPPORT */

            default:
              if ((command >= BRL_BLK_ROUTE) &&
                  (command < (BRL_BLK_ROUTE + brl.textColumns))) {
                int key = command - BRL_BLK_ROUTE;

                if ((key >= textStart) && (key < (textStart + textCount))) {
                  if (changeMenuItemScaled(item, key-textStart, textCount)) {
                    settingChanged = 1;
                  } else {
                    playTune(&tune_command_rejected);
                  }
                } else if ((key >= statusStart) && (key < (statusStart + statusCount))) {
                  switch (key - statusStart) {
                    default:
                      playTune(&tune_command_rejected);
                      break;
                  }
                } else {
                  playTune(&tune_command_rejected);
                }

                break;
              }

              /* For any other keystroke, we exit */
              playTune(&tune_command_rejected);
            case BRL_BLK_PASSKEY+BRL_KEY_ESCAPE:
            case BRL_BLK_PASSKEY+BRL_KEY_END:
            case BRL_CMD_PREFMENU:
              goto exitMenu;

            case BRL_CMD_RESTARTBRL:
              ok = 0;
              break;
          }
        }
      }
    }
  }

exitMenu:
  if (saveOnExit) {
    if (savePreferences()) {
      playTune(&tune_command_done);
    }
  }

  globEnd(&glob_textTable);
  globEnd(&glob_attributesTable);
#ifdef ENABLE_CONTRACTED_BRAILLE
  globEnd(&glob_contractionTable);
#endif /* ENABLE_CONTRACTED_BRAILLE */

  return ok;
}

static int
handleWcharHelpLine (const wchar_t *line, void *data) {
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
      logMessage(LOG_INFO, "%s: %s", gettext("Key Bindings"),
                 brl.keyBindings? brl.keyBindings: gettext("none"));
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
              logMessage(LOG_INFO, "%s: %s", gettext("Key Table"), path);

              if (constructHelpScreen()) {
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

            if (constructHelpScreen())
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
  destructHelpScreen();

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
          "al", "bm", "eu", "fs", "ht", "hm", "mt", "pg", "pm", "sk", "vo",
          NULL
        };
        autodetectableDrivers = usbDrivers;
      } else if (isBluetoothDevice(&dev)) {
        static const char *bluetoothDrivers[] = {
          "ht", "al", "bm",
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

    resetStatusFields();
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
retryBrailleDriver (void *data) {
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
retrySpeechDriver (void *data) {
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
retryScreenDriver (void *data) {
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
exitScreen (void) {
  stopScreenDriver();
  destructSpecialScreens();
}

static void
exitTunes (void) {
  closeTuneDevice(1);
}

static void
exitPidFile (void) {
  unlink(opt_pidFile);
}

static int tryPidFile (ProcessIdentifier pid);

static void
retryPidFile (void *data) {
  tryPidFile(0);
}

static int
tryPidFile (ProcessIdentifier pid) {
  int created = createPidFile(opt_pidFile, pid);

  if (!pid) {
    if (created) {
      atexit(exitPidFile);
    } else if (errno == EEXIST) {
      exit(12);
    } else {
      asyncRelativeAlarm(5000, retryPidFile, NULL);
    }
  }

  return created;
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
      exit(11);
    }

    if (!CreateProcess(NULL, commandLine, NULL, NULL, TRUE,
                       CREATE_NEW_PROCESS_GROUP | CREATE_SUSPENDED,
                       NULL, NULL, &startupInfo, &processInfo)) {
      logWindowsSystemError("CreateProcess");
      exit(10);
    }

    {
      int created = tryPidFile(processInfo.dwProcessId);
      int resumed = ResumeThread(processInfo.hThread) != -1;

      if (!created) {
        if (errno == EEXIST) {
          ExitProcess(12);
        }
      }

      if (!resumed) {
        logWindowsSystemError("ResumeThread");
        ExitProcess(13);
      }
    }

    ExitProcess(0);
  }

  free(variableName);
}

#elif defined(__MSDOS__)

#else /* Unix */
static void
background (void) {
  int fds[2];

  if (pipe(fds) == -1) {
    logSystemError("pipe");
    exit(11);
  }

  fflush(stdout);
  fflush(stderr);

  {
    pid_t child = fork();

    if (child == -1) {
      logSystemError("fork");
      exit(10);
    }

    if (child) {
      int returnCode = 0;

      if (close(fds[0]) == -1) logSystemError("close");

      if (!tryPidFile(child)) {
        if (errno == EEXIST) {
          returnCode = 12;
        }
      }

      if (close(fds[1]) == -1) logSystemError("close");
      _exit(returnCode);
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
    exit(13);
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

void
startup (int argc, char *argv[]) {
  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .doBootParameters = &opt_bootParameters,
      .doEnvironmentVariables = &opt_environmentVariables,
      .configurationFile = &opt_configurationFile,
      .applicationName = "brltty"
    };
    processOptions(&descriptor, &argc, &argv);
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

  if (!validateInterval(&updateInterval, opt_updateInterval)) {
    logMessage(LOG_ERR, "%s: %s", gettext("invalid update interval"), opt_updateInterval);
  }

  if (!validateInterval(&messageDelay, opt_messageDelay)) {
    logMessage(LOG_ERR, "%s: %s", gettext("invalid message delay"), opt_messageDelay);
  }

  /* Set logging levels. */
  {
    int level = LOG_NOTICE;

    if (*opt_logLevel) {
      static const char *const words[] = {
        "emergency", "alert", "critical", "error",
        "warning", "notice", "information", "debug"
      };
      static unsigned int count = ARRAY_COUNT(words);

      {
        int length = strlen(opt_logLevel);
        int index;
        for (index=0; index<count; ++index) {
          const char *word = words[index];
          if (strncasecmp(opt_logLevel, word, length) == 0) {
            level = index;
            goto setLevel;
          }
        }
      }

      {
        int value;
        if (isInteger(&value, opt_logLevel) && (value >= 0) && (value < count)) {
          level = value;
          goto setLevel;
        }
      }

      logMessage(LOG_ERR, "%s: %s", gettext("invalid log level"), opt_logLevel);
    }

  setLevel:
    setLogLevel(level);

    if (opt_standardError) {
      closeSystemLog();
    } else {
      level = LOG_NOTICE;
      if (opt_version || opt_verify) level += 1;
      if (opt_quiet) level -= 1;
    }

    setPrintLevel(level);
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

    exit(0);
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

    if (stop) exit(0);
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
  tryPidFile(0);

  if (!opt_noDaemon) {
    setPrintOff();

    {
      const char *nullDevice = "/dev/null";

      if (!freopen(nullDevice, "r", stdin)) {
        logSystemError("freopen[stdin]");
      }

      if (!freopen(nullDevice, "a", stdout)) {
        logSystemError("freopen[stdout]");
      }

      if (opt_standardError) {
        fflush(stderr);
      } else {
        if (!freopen(nullDevice, "a", stderr)) {
          logSystemError("freopen[stderr]");
        }
      }
    }

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

        if (opt_standardError) {
          fflush(stderr);
        } else {
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

  atexit(exitTunes);
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
      const char *locale = getTextTableLocale();
      opt_textTable = "";

      if (locale) {
        char name[strlen(locale) + 1];

        {
          size_t length = strcspn(locale, ".@");
          strncpy(name, locale, length);
          name[length] = 0;
        }

        if (strcmp(name, "C") == 0) {
          name[0] = 0;
        } else if (!testTextTable(name)) {
          char *delimiter = strchr(name, '_');

          if (delimiter) {
            *delimiter = 0;
            if (!testTextTable(name)) name[0] = 0;
          }
        }

        if (name[0])
          if (replaceTextTable(name))
            opt_textTable = strdupWrapper(name);
      }
    } else {
      if (!replaceTextTable(opt_textTable)) opt_textTable = "";
    }
  }

  if (!*opt_textTable) opt_textTable = TEXT_TABLE;
  logMessage(LOG_INFO, "%s: %s", gettext("Text Table"), opt_textTable);

  /* handle attributes table option */
  if (*opt_attributesTable)
    if (!replaceAttributesTable(opt_attributesTable))
      opt_attributesTable = "";
  if (!*opt_attributesTable) opt_attributesTable = ATTRIBUTES_TABLE;
  logMessage(LOG_INFO, "%s: %s", gettext("Attributes Table"), opt_attributesTable);

#ifdef ENABLE_CONTRACTED_BRAILLE
  /* handle contraction table option */
  atexit(exitContractionTable);
  if (*opt_contractionTable) loadContractionTable(opt_contractionTable);
  logMessage(LOG_INFO, "%s: %s", gettext("Contraction Table"),
             *opt_contractionTable? opt_contractionTable: gettext("none"));
#endif /* ENABLE_CONTRACTED_BRAILLE */

  /* handle key table option */
  atexit(exitKeyTable);
  if (*opt_keyTable) loadKeyTable(opt_keyTable);
  logMessage(LOG_INFO, "%s: %s", gettext("Key Table"),
             *opt_keyTable? opt_keyTable: gettext("none"));

  if (parseKeyboardProperties(&keyboardProperties, opt_keyboardProperties))
    if (keyboardKeyTable)
      scheduleKeyboardMonitor(0);

  /* initialize screen driver */
  atexit(exitScreen);
  constructSpecialScreens();
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
        atexit(exitApi);
        apiStarted = 1;
      }
    }
  }
#endif /* ENABLE_API */

  /* The device(s) the braille display might be connected to. */
  if (!*opt_brailleDevice) {
    logMessage(LOG_ERR, gettext("braille device not specified"));
    exit(4);
  }
  brailleDevices = splitString(opt_brailleDevice, ',', NULL);

  /* Activate the braille display. */
  brailleDrivers = splitString(opt_brailleDriver? opt_brailleDriver: "", ',', NULL);
  brailleConstructed = 0;
  if (opt_verify) {
    if (activateBrailleDriver(1)) deactivateBrailleDriver();
  } else {
    atexit(exitBrailleDriver);
    tryBrailleDriver();
  }

#ifdef ENABLE_SPEECH_SUPPORT
  /* Activate the speech synthesizer. */
  speechDrivers = splitString(opt_speechDriver? opt_speechDriver: "", ',', NULL);
  if (opt_verify) {
    if (activateSpeechDriver(1)) deactivateSpeechDriver();
  } else {
    atexit(exitSpeechDriver);
    trySpeechDriver();
  }

  /* Create the file system object for speech input. */
  logMessage(LOG_INFO, "%s: %s", gettext("Speech Input"),
             *opt_speechInput? opt_speechInput: gettext("none"));
  if (!opt_verify) {
    if (*opt_speechInput) enableSpeechInput(opt_speechInput);
  }
#endif /* ENABLE_SPEECH_SUPPORT */

  if (opt_verify) exit(0);
}
