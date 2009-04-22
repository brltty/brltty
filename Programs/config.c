/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
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
#include <sys/stat.h>
#include <limits.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */

#ifdef ENABLE_PREFERENCES_MENU
#ifdef ENABLE_TABLE_SELECTION
#if defined(HAVE_GLOB_H)
#include <glob.h>
#elif defined(__MINGW32__)
#include <io.h>
#else /* glob: paradigm-specific global definitions */
#warning file globbing support not available on this platform
#endif /* glob: paradigm-specific global definitions */
#endif /* ENABLE_TABLE_SELECTION */
#endif /* ENABLE_PREFERENCES_MENU */

#include "cmd.h"
#include "brl.h"
#include "spk.h"
#include "scr.h"
#include "ttb.h"
#include "atb.h"
#include "ctb.h"
#include "ktb.h"
#include "tunes.h"
#include "message.h"
#include "misc.h"
#include "system.h"
#include "async.h"
#include "program.h"
#include "options.h"
#include "brltty.h"
#include "defaults.h"
#include "io_serial.h"

#ifdef ENABLE_USB_SUPPORT
#include "io_usb.h"
#endif /* ENABLE_USB_SUPPORT */

#ifdef ENABLE_BLUETOOTH_SUPPORT
#include "io_bluetooth.h"
#endif /* ENABLE_BLUETOOTH_SUPPORT */

#ifdef __MINGW32__
#include "sys_windows.h"

static int opt_installService;
static int opt_removeService;

#define SERVICE_NAME "BrlAPI"
#define SERVICE_DESCRIPTION "Braille API (BrlAPI)"

static const char *const optionStrings_InstallService[] = {
  SERVICE_NAME,
  NULL
};

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
static int opt_bootParameters = 1;
static int opt_environmentVariables;
static char *opt_updateInterval;
static char *opt_messageDelay;

static char *opt_configurationFile;
static char *opt_pidFile;
static char *opt_writableDirectory;
static char *opt_dataDirectory;
static char *opt_libraryDirectory;

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
static char *preferencesFile = NULL;

static char *opt_tablesDirectory;
static char *opt_textTable;
static char *opt_attributesTable;

#ifdef ENABLE_CONTRACTED_BRAILLE
static char *opt_contractionTable;
ContractionTable *contractionTable = NULL;
#endif /* ENABLE_CONTRACTED_BRAILLE */

static char *opt_keyTable;
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
static char *opt_speechFifo;
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
    .word = "data-directory",
    .flags = OPT_Hidden | OPT_Config | OPT_Environ,
    .argument = strtext("directory"),
    .setting.string = &opt_dataDirectory,
    .defaultSetting = DATA_DIRECTORY,
    .description = strtext("Path to directory for driver help and configuration files.")
  },

  { .letter = 'L',
    .word = "library-directory",
    .flags = OPT_Hidden | OPT_Config | OPT_Environ,
    .argument = strtext("directory"),
    .setting.string = &opt_libraryDirectory,
    .defaultSetting = LIBRARY_DIRECTORY,
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
    .defaultSetting = DATA_DIRECTORY,
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

  { .letter = 'F',
    .word = "speech-fifo",
    .flags = OPT_Hidden | OPT_Config | OPT_Environ,
    .argument = strtext("file"),
    .setting.string = &opt_speechFifo,
    .description = strtext("Path to speech pass-through FIFO.")
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
    .flags = OPT_Hidden,
    .argument = strtext("level"),
    .setting.string = &opt_logLevel,
    .description = strtext("Diagnostic logging level: %s, or one of {%s}"),
    .strings = optionStrings_LogLevel
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

static int
testTextTable (char *table) {
  int exists = 0;
  char *file;

  if ((file = ensureTextTableExtension(table))) {
    char *path;

    LogPrint(LOG_DEBUG, "checking for text table: %s", file);

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

      if ((newTable = compileTextTable(path))) {
        TextTable *oldTable = textTable;
        textTable = newTable;
        destroyTextTable(oldTable);
        ok = 1;
      }

      free(path);
    }

    free(file);
  }

  if (!ok) LogPrint(LOG_ERR, "%s: %s", gettext("cannot load text table"), name);
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

      if ((newTable = compileAttributesTable(path))) {
        AttributesTable *oldTable = attributesTable;
        attributesTable = newTable;
        destroyAttributesTable(oldTable);
        ok = 1;
      }

      free(path);
    }

    free(file);
  }

  if (!ok) LogPrint(LOG_ERR, "%s: %s", gettext("cannot load attributes table"), name);
  return ok;
}

int
readCommand (BRL_DriverCommandContext context) {
  int command = readBrailleCommand(&brl, context);
  if (command != EOF) {
    LogPrint(LOG_DEBUG, "command: %06X", command);
    if (IS_DELAYED_COMMAND(command)) command = BRL_CMD_NOOP;
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
        LogPrint(LOG_DEBUG, "compiling contraction table: %s", path);

        if (!(table = compileContractionTable(path))) {
          LogPrint(LOG_ERR, "%s: %s", gettext("cannot compile contraction table"), path);
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
  if (keyTable) {
    destroyKeyTable(keyTable);
    keyTable = NULL;
  }
}

static int
loadKeyTable (const char *name) {
  KeyTable *table = NULL;

  if (*name) {
    char *file;

    if ((file = ensureKeyTableExtension(name))) {
      char *path;

      if ((path = makePath(opt_tablesDirectory, file))) {
        LogPrint(LOG_DEBUG, "compiling key table: %s", path);

        if (!(table = compileKeyTable(path))) {
          LogPrint(LOG_ERR, "%s: %s", gettext("cannot compile key table"), path);
        }

        free(path);
      }

      free(file);
    }

    if (!table) return 0;
  }

  if (keyTable) destroyKeyTable(keyTable);
  keyTable = table;
  return 1;
}

static PressedKeysState
handleKeyEvent (const KeyCodeSet *modifiers, KeyCode code, int press) {
  {
    static int lastCommand = EOF;
    int command = getKeyCommand(keyTable, modifiers, code);
    int bound = command != EOF;

    if (!press || !bound) {
      if (lastCommand != EOF) {
        lastCommand = EOF;
        enqueueCommand(BRL_CMD_NOOP);
      }
    } else if (command != lastCommand) {
      lastCommand = command;
      enqueueCommand(command | BRL_FLG_REPEAT_INITIAL | BRL_FLG_REPEAT_DELAY);
    }

    if (bound) return PKS_YES;
  }

  {
    KeyCodeSet keys = *modifiers;
    addKeyCode(&keys, code);
    if (isKeyModifiers(keyTable, &keys)) return PKS_MAYBE;
  }

  return PKS_NO;
}

static void scheduleKeyboardMonitor (int interval);

static void
tryKeyboardMonitor (void *data) {
  LogPrint(LOG_DEBUG, "starting keyboard monitor");
  if (!startKeyboardMonitor(&keyboardProperties, handleKeyEvent)) {
    LogPrint(LOG_DEBUG, "keyboard monitor failed");
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
  LogPrint(LOG_DEBUG, "regions: text=%u.%u status=%u.%u",
           textStart, textCount, statusStart, statusCount);

  fullWindowShift = MAX(textCount-prefs.windowOverlap, 1);
  halfWindowShift = textCount / 2;
  verticalWindowShift = (rows > 1)? rows: 5;
  LogPrint(LOG_DEBUG, "shifts: full=%u half=%u vertical=%u",
           fullWindowShift, halfWindowShift, verticalWindowShift);
}

void
reconfigureWindow (void) {
  windowConfigurationChanged(LOG_INFO, brl.textRows, brl.textColumns);
}

static void
applyBraillePreferences (void) {
  if (braille->firmness) braille->firmness(&brl, prefs.brailleFirmness);
  if (braille->sensitivity) braille->sensitivity(&brl, prefs.brailleSensitivity);
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
applyPreferences (void) {
  reconfigureWindow();
  setTuneDevice(prefs.tuneDevice);
  applyBraillePreferences();

#ifdef ENABLE_SPEECH_SUPPORT
  applySpeechPreferences();
#endif /* ENABLE_SPEECH_SUPPORT */
}

static void
resetStatusFields (const unsigned char *fields) {
  unsigned int count = brl.statusColumns * brl.statusRows;

  prefs.statusPosition = spNone;
  prefs.statusCount = 0;
  prefs.statusSeparator = ssNone;
  memset(prefs.statusFields, sfEnd, sizeof(prefs.statusFields));

  if (!count && (brl.textColumns > 40)) {
    count = (brl.textColumns % 20) * brl.textRows;

    if (count) {
      prefs.statusPosition = spRight;

      if (count > 1) {
        count -= 1;
        prefs.statusSeparator = ssStatusSide;
      }
    }
  }

  if (!fields) fields = braille->statusFields;
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

  if (fields) {
    unsigned int index = 0;

    while (index < (ARRAY_COUNT(prefs.statusFields) - 1)) {
      unsigned char field = fields[index];
      if (field == sfEnd) break;
      prefs.statusFields[index++] = field;
    }
  }
}

static void
resetPreferences (void) {
  memset(&prefs, 0, sizeof(prefs));

  prefs.magic[0] = PREFS_MAGIC_NUMBER & 0XFF;
  prefs.magic[1] = PREFS_MAGIC_NUMBER >> 8;
  prefs.version = 4;

  prefs.autorepeat = DEFAULT_AUTOREPEAT;
  prefs.autorepeatPanning = DEFAULT_AUTOREPEAT_PANNING;
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
  prefs.highlightWindow = DEFAULT_HIGHLIGHT_WINDOW;

  prefs.textStyle = DEFAULT_TEXT_STYLE;
  prefs.brailleFirmness = DEFAULT_BRAILLE_FIRMNESS;
  prefs.brailleSensitivity = DEFAULT_BRAILLE_SENSITIVITY;

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
  prefs.speechRate = DEFAULT_SPEECH_RATE;
  prefs.speechVolume = DEFAULT_SPEECH_VOLUME;
  prefs.speechPitch = DEFAULT_SPEECH_PITCH;
  prefs.speechPunctuation = DEFAULT_SPEECH_PUNCTUATION;

  prefs.statusPosition = DEFAULT_STATUS_POSITION;
  prefs.statusCount = DEFAULT_STATUS_COUNT;
  prefs.statusSeparator = DEFAULT_STATUS_SEPARATOR;
  resetStatusFields(NULL);
  applyPreferences();
}

int
loadPreferences (void) {
  int ok = 0;
  FILE *file = openDataFile(preferencesFile, "rb", 1);

  if (file) {
    Preferences newPreferences;
    size_t length = fread(&newPreferences, 1, sizeof(newPreferences), file);

    if (ferror(file)) {
      LogPrint(LOG_ERR, "%s: %s: %s",
               gettext("cannot read preferences file"), preferencesFile, strerror(errno));
    } else if ((length < 40) ||
               (newPreferences.magic[0] != (PREFS_MAGIC_NUMBER & 0XFF)) ||
               (newPreferences.magic[1] != (PREFS_MAGIC_NUMBER >> 8))) {
      LogPrint(LOG_ERR, "%s: %s", gettext("invalid preferences file"), preferencesFile);
    } else {
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
        prefs.speechRate = DEFAULT_SPEECH_RATE;
      }

      if (length == 41) {
        length++;
        prefs.speechVolume = DEFAULT_SPEECH_VOLUME;
      }

      if (length == 42) {
        length++;
        prefs.brailleFirmness = DEFAULT_BRAILLE_FIRMNESS;
      }

      if (length == 43) {
        length++;
        prefs.speechPunctuation = DEFAULT_SPEECH_PUNCTUATION;
      }

      if (length == 44) {
        length++;
        prefs.speechPitch = DEFAULT_SPEECH_PITCH;
      }

      if (prefs.version == 3) {
        prefs.version++;
        prefs.autorepeatPanning = DEFAULT_AUTOREPEAT_PANNING;
      }

      if (prefs.version == 4) {
        prefs.version++;
        prefs.brailleSensitivity = DEFAULT_BRAILLE_SENSITIVITY;
      }

      if (length == 45) {
        length++;
        prefs.statusPosition = DEFAULT_STATUS_POSITION;
      }

      if (length == 46) {
        length++;
        prefs.statusCount = DEFAULT_STATUS_COUNT;
      }

      if (length == 47) {
        length++;
        prefs.statusSeparator = DEFAULT_STATUS_SEPARATOR;
      }

      if (length < 58) {
        const unsigned char *fields = NULL;

        {
          static const unsigned char styleNone[] = {
            sfEnd
          };

          static const unsigned char styleAlva[] = {
            sfAlphabeticCursorCoordinates, sfAlphabeticWindowCoordinates, sfStateLetter, sfEnd
          };

          static const unsigned char styleTieman[] = {
            sfCursorAndWindowColumn, sfCursorAndWindowRow, sfStateDots, sfEnd
          };

          static const unsigned char stylePB80[] = {
            sfWindowRow, sfEnd
          };

          static const unsigned char styleConfigurable[] = {
            sfGeneric, sfEnd
          };

          static const unsigned char styleMDV[] = {
            sfWindowCoordinates, sfEnd
          };

          static const unsigned char styleVoyager[] = {
            sfWindowRow, sfCursorRow, sfCursorColumn, sfEnd
          };

          static const unsigned char styleTime[] = {
            sfTime, sfEnd
          };

          static const unsigned char *const styleTable[] = {
            styleNone, styleAlva, styleTieman, stylePB80,
            styleConfigurable, styleMDV, styleVoyager, styleTime
          };
          static const unsigned char styleCount = ARRAY_COUNT(styleTable);

          unsigned char style = ((const unsigned char *)&prefs)[38];

          if (style < styleCount) {
            fields = styleTable[style];
            if (*fields == sfEnd) fields = NULL;
          }
        }

        length = 58;
        resetStatusFields(fields);
      }

      applyPreferences();
    }

    fclose(file);
  }

  return ok;
}

static void
getPreferences (void) {
  if (!loadPreferences()) resetPreferences();
  setTuneDevice(prefs.tuneDevice);
}

#ifdef ENABLE_PREFERENCES_MENU
int 
savePreferences (void) {
  int ok = 0;
  FILE *file = openDataFile(preferencesFile, "w+b", 0);
  if (file) {
    size_t length = fwrite(&prefs, 1, sizeof(prefs), file);
    if (length == sizeof(prefs)) {
      ok = 1;
    } else {
      if (!ferror(file)) errno = EIO;
      LogPrint(LOG_ERR, "%s: %s: %s",
               gettext("cannot write to preferences file"), preferencesFile, strerror(errno));
    }
    fclose(file);
  }
  if (!ok) message(NULL, gettext("not saved"), 0);
  return ok;
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

static int
testBrailleSensitivity (void) {
  return braille->sensitivity != NULL;
}

static int
changedBrailleSensitivity (unsigned char setting) {
  setBrailleSensitivity(&brl, setting);
  return 1;
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

#ifdef ENABLE_TABLE_SELECTION
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
globPrepare (GlobData *data, const char *directory, const char *extension, const char *initial, int none) {
  memset(data, 0, sizeof(*data));

  data->directory = directory;
  data->extension = extension;
  data->none = (none != 0);

  {
    const char *components[] = {"*", extension};
    data->pattern = joinStrings(components, ARRAY_COUNT(components));
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
        if (fchdir(originalDirectory) == -1) LogError("fchdir");
#else /* HAVE_FCHDIR */
        if (chdir(originalDirectory) == -1) LogError("chdir");
#endif /* HAVE_FCHDIR */
      } else {
        LogPrint(LOG_ERR, "%s: %s: %s",
                 gettext("cannot set working directory"), data->directory, strerror(errno));
      }

#ifdef HAVE_FCHDIR
      close(originalDirectory);
#else /* HAVE_FCHDIR */
      free(originalDirectory);
#endif /* HAVE_FCHDIR */
    } else {
#ifdef HAVE_FCHDIR
      LogPrint(LOG_ERR, "%s: %s",
               gettext("cannot open working directory"), strerror(errno));
#else /* HAVE_FCHDIR */
      LogPrint(LOG_ERR, "%s", gettext("cannot determine working directory"));
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
    LogError("strdup");
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

int
updatePreferences (void) {
  static unsigned char saveOnExit = 0;                /* 1 == save preferences on exit */
  const char *mode = "prf";
  int ok = 1;

#ifdef ENABLE_TABLE_SELECTION
  globBegin(&glob_textTable);
  globBegin(&glob_attributesTable);
#ifdef ENABLE_CONTRACTED_BRAILLE
  globBegin(&glob_contractionTable);
#endif /* ENABLE_CONTRACTED_BRAILLE */
#endif /* ENABLE_TABLE_SELECTION */

  if (setStatusText(&brl, mode) &&
      message(mode, gettext("Preferences Menu"), 0)) {
    static const char *booleanValues[] = {
      strtext("No"),
      strtext("Yes")
    };

    static const char *cursorStyles[] = {
      strtext("Underline"),
      strtext("Block")
    };

    static const char *firmnessLevels[] = {
      strtext("Minimum"),
      strtext("Low"),
      strtext("Medium"),
      strtext("High"),
      strtext("Maximum")
    };

    static const char *sensitivityLevels[] = {
      strtext("Minimum"),
      strtext("Low"),
      strtext("Medium"),
      strtext("High"),
      strtext("Maximum")
    };

    static const char *skipBlankWindowsModes[] = {
      strtext("All"),
      strtext("End of Line"),
      strtext("Rest of Line")
    };

    static const char *statusPositions[] = {
      strtext("None"),
      strtext("Left"),
      strtext("Right")
    };

    static const char *statusSeparators[] = {
      strtext("None"),
      strtext("Space"),
      strtext("Block"),
      strtext("Status Side"),
      strtext("Text Side")
    };

    static const char *statusFields[] = {
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

    static const char *textStyles[] = {
      "8-dot",
      "6-dot"
    };

    static const char *tuneDevices[] = {
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

#ifdef ENABLE_SPEECH_SUPPORT
    static const char *sayModes[] = {
      strtext("Immediate"),
      strtext("Enqueue")
    };

    static const char *punctuationLevels[] = {
      strtext("None"),
      strtext("Some"),
      strtext("All")
    };
#endif /* ENABLE_SPEECH_SUPPORT */

    #define MENU_ITEM(setting, changed, test, label, values, minimum, maximum, divisor) {&setting, changed, test, label, values, minimum, maximum, divisor}
    #define NUMERIC_ITEM(setting, changed, test, label, minimum, maximum, divisor) MENU_ITEM(setting, changed, test, label, NULL, minimum, maximum, divisor)
    #define TIME_ITEM(setting, changed, test, label) NUMERIC_ITEM(setting, changed, test, label, 1, 100, updateInterval/10)
    #define VOLUME_ITEM(setting, changed, test, label) NUMERIC_ITEM(setting, changed, test, label, 0, 100, 5)
    #define TEXT_ITEM(setting, changed, test, label, names, count) MENU_ITEM(setting, changed, test, label, names, 0, count-1, 1)
    #define SYMBOLIC_ITEM(setting, changed, test, label, names) TEXT_ITEM(setting, changed, test, label, names, ARRAY_COUNT(names))
    #define BOOLEAN_ITEM(setting, changed, test, label) SYMBOLIC_ITEM(setting, changed, test, label, booleanValues)
    #define GLOB_ITEM(data, changed, test, label) TEXT_ITEM(data.setting, changed, test, label, data.paths, data.count)
    #define STATUS_FIELD_ITEM(number) SYMBOLIC_ITEM(prefs.statusFields[number-1], changedStatusField##number, testStatusField##number, strtext("Status Field ") #number, statusFields)
    MenuItem menu[] = {
      BOOLEAN_ITEM(saveOnExit, NULL, NULL, strtext("Save on Exit")),
      SYMBOLIC_ITEM(prefs.textStyle, NULL, NULL, strtext("Text Style"), textStyles),
      BOOLEAN_ITEM(prefs.skipIdenticalLines, NULL, NULL, strtext("Skip Identical Lines")),
      BOOLEAN_ITEM(prefs.skipBlankWindows, NULL, NULL, strtext("Skip Blank Windows")),
      SYMBOLIC_ITEM(prefs.blankWindowsSkipMode, NULL, testSkipBlankWindows, strtext("Which Blank Windows"), skipBlankWindowsModes),
      BOOLEAN_ITEM(prefs.slidingWindow, NULL, NULL, strtext("Sliding Window")),
      BOOLEAN_ITEM(prefs.eagerSlidingWindow, NULL, testSlidingWindow, strtext("Eager Sliding Window")),
      NUMERIC_ITEM(prefs.windowOverlap, changedWindowOverlap, NULL, strtext("Window Overlap"), 0, 20, 1),
      BOOLEAN_ITEM(prefs.autorepeat, changedAutorepeat, NULL, strtext("Autorepeat")),
      BOOLEAN_ITEM(prefs.autorepeatPanning, NULL, testAutorepeat, strtext("Autorepeat Panning")),
      TIME_ITEM(prefs.autorepeatDelay, NULL, testAutorepeat, strtext("Autorepeat Delay")),
      TIME_ITEM(prefs.autorepeatInterval, NULL, testAutorepeat, strtext("Autorepeat Interval")),
      BOOLEAN_ITEM(prefs.showCursor, NULL, NULL, strtext("Show Cursor")),
      SYMBOLIC_ITEM(prefs.cursorStyle, NULL, testShowCursor, strtext("Cursor Style"), cursorStyles),
      BOOLEAN_ITEM(prefs.blinkingCursor, NULL, testShowCursor, strtext("Blinking Cursor")),
      TIME_ITEM(prefs.cursorVisibleTime, NULL, testBlinkingCursor, strtext("Cursor Visible Time")),
      TIME_ITEM(prefs.cursorInvisibleTime, NULL, testBlinkingCursor, strtext("Cursor Invisible Time")),
      BOOLEAN_ITEM(prefs.showAttributes, NULL, NULL, strtext("Show Attributes")),
      BOOLEAN_ITEM(prefs.blinkingAttributes, NULL, testShowAttributes, strtext("Blinking Attributes")),
      TIME_ITEM(prefs.attributesVisibleTime, NULL, testBlinkingAttributes, strtext("Attributes Visible Time")),
      TIME_ITEM(prefs.attributesInvisibleTime, NULL, testBlinkingAttributes, strtext("Attributes Invisible Time")),
      BOOLEAN_ITEM(prefs.blinkingCapitals, NULL, NULL, strtext("Blinking Capitals")),
      TIME_ITEM(prefs.capitalsVisibleTime, NULL, testBlinkingCapitals, strtext("Capitals Visible Time")),
      TIME_ITEM(prefs.capitalsInvisibleTime, NULL, testBlinkingCapitals, strtext("Capitals Invisible Time")),
      SYMBOLIC_ITEM(prefs.brailleFirmness, changedBrailleFirmness, testBrailleFirmness, strtext("Braille Firmness"), firmnessLevels),
      SYMBOLIC_ITEM(prefs.brailleSensitivity, changedBrailleSensitivity, testBrailleSensitivity, strtext("Braille Sensitivity"), sensitivityLevels),
#ifdef HAVE_LIBGPM
      BOOLEAN_ITEM(prefs.windowFollowsPointer, NULL, NULL, strtext("Window Follows Pointer")),
#endif /* HAVE_LIBGPM */
      BOOLEAN_ITEM(prefs.highlightWindow, NULL, NULL, strtext("Highlight Window")),
      BOOLEAN_ITEM(prefs.alertTunes, NULL, NULL, strtext("Alert Tunes")),
      SYMBOLIC_ITEM(prefs.tuneDevice, changedTuneDevice, testTunes, strtext("Tune Device"), tuneDevices),
#ifdef ENABLE_PCM_SUPPORT
      VOLUME_ITEM(prefs.pcmVolume, NULL, testTunesPcm, strtext("PCM Volume")),
#endif /* ENABLE_PCM_SUPPORT */
#ifdef ENABLE_MIDI_SUPPORT
      VOLUME_ITEM(prefs.midiVolume, NULL, testTunesMidi, strtext("MIDI Volume")),
      TEXT_ITEM(prefs.midiInstrument, NULL, testTunesMidi, strtext("MIDI Instrument"), midiInstrumentTable, midiInstrumentCount),
#endif /* ENABLE_MIDI_SUPPORT */
#ifdef ENABLE_FM_SUPPORT
      VOLUME_ITEM(prefs.fmVolume, NULL, testTunesFm, strtext("FM Volume")),
#endif /* ENABLE_FM_SUPPORT */
      BOOLEAN_ITEM(prefs.alertDots, NULL, NULL, strtext("Alert Dots")),
      BOOLEAN_ITEM(prefs.alertMessages, NULL, NULL, strtext("Alert Messages")),
#ifdef ENABLE_SPEECH_SUPPORT
      SYMBOLIC_ITEM(prefs.sayLineMode, NULL, NULL, strtext("Say-Line Mode"), sayModes),
      BOOLEAN_ITEM(prefs.autospeak, NULL, NULL, strtext("Autospeak")),
      NUMERIC_ITEM(prefs.speechRate, changedSpeechRate, testSpeechRate, strtext("Speech Rate"), 0, SPK_RATE_MAXIMUM, 1),
      NUMERIC_ITEM(prefs.speechVolume, changedSpeechVolume, testSpeechVolume, strtext("Speech Volume"), 0, SPK_VOLUME_MAXIMUM, 1),
      NUMERIC_ITEM(prefs.speechPitch, changedSpeechPitch, testSpeechPitch, strtext("Speech Pitch"), 0, SPK_PITCH_MAXIMUM, 1),
      SYMBOLIC_ITEM(prefs.speechPunctuation, changedSpeechPunctuation, testSpeechPunctuation, strtext("Speech Punctuation"), punctuationLevels),
#endif /* ENABLE_SPEECH_SUPPORT */
      SYMBOLIC_ITEM(prefs.statusPosition, changedStatusPosition, testStatusPosition, strtext("Status Position"), statusPositions),
      NUMERIC_ITEM(prefs.statusCount, changedStatusCount, testStatusCount, strtext("Status Count"), 0, MAX((int)brl.textColumns/2-1, 0), 1),
      SYMBOLIC_ITEM(prefs.statusSeparator, changedStatusSeparator, testStatusSeparator, strtext("Status Separator"), statusSeparators),
      STATUS_FIELD_ITEM(1),
      STATUS_FIELD_ITEM(2),
      STATUS_FIELD_ITEM(3),
      STATUS_FIELD_ITEM(4),
      STATUS_FIELD_ITEM(5),
      STATUS_FIELD_ITEM(6),
      STATUS_FIELD_ITEM(7),
      STATUS_FIELD_ITEM(8),
      STATUS_FIELD_ITEM(9),
#ifdef ENABLE_TABLE_SELECTION
      GLOB_ITEM(glob_textTable, changedTextTable, NULL, strtext("Text Table")),
      GLOB_ITEM(glob_attributesTable, changedAttributesTable, NULL, strtext("Attributes Table")),
#ifdef ENABLE_CONTRACTED_BRAILLE
      GLOB_ITEM(glob_contractionTable, changedContractionTable, NULL, strtext("Contraction Table"))
#endif /* ENABLE_CONTRACTED_BRAILLE */
#endif /* ENABLE_TABLE_SELECTION */
    };
    int menuSize = ARRAY_COUNT(menu);
    static int menuIndex = 0;                        /* current menu item */

    unsigned int lineIndent = 0;                                /* braille window pos in buffer */
    int indexChanged = 1;
    int settingChanged = 0;                        /* 1 when item's value has changed */

    Preferences oldPreferences = prefs;        /* backup preferences */
    int command = EOF;                                /* readbrl() value */

    if (prefs.autorepeat) resetAutorepeat();

    while (ok) {
      MenuItem *item = &menu[menuIndex];
      char valueBuffer[0X10];
      const char *value;

      testProgramTermination();
      closeTuneDevice(0);

      if (item->names) {
        if (!*(value = item->names[*item->setting - item->minimum])) value = strtext("<off>");
        value = gettext(value);
      } else {
        snprintf(valueBuffer, sizeof(valueBuffer), "%d", *item->setting);
        value = valueBuffer;
      }

      {
        const char *label = gettext(item->label);
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
        if (!writeBrailleBytes(mode, &line[lineIndent], MAX(0, lineLength-lineIndent))) ok = 0;
        drainBrailleOutput(&brl, updateInterval);
        if (!ok) break;

        /* Now process any user interaction */
        command = readBrailleCommand(&brl, BRL_CTX_PREFS);
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
              applyPreferences();
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
              menuIndex = lineIndent = 0;
              indexChanged = 1;
              break;
            case BRL_CMD_BOT:
            case BRL_CMD_BOT_LEFT:
            case BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN:
            case BRL_CMD_MENU_LAST_ITEM:
              menuIndex = menuSize - 1;
              lineIndent = 0;
              indexChanged = 1;
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
              indexChanged = 1;
              break;
            case BRL_CMD_LNDN:
            case BRL_CMD_NXDIFLN:
            case BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN:
            case BRL_CMD_MENU_NEXT_ITEM:
              do {
                if (++menuIndex == menuSize) menuIndex = 0;
              } while (menu[menuIndex].test && !menu[menuIndex].test());
              lineIndent = 0;
              indexChanged = 1;
              break;

            case BRL_CMD_FWINLT:
              if (lineIndent > 0)
                lineIndent -= MIN(textLength, lineIndent);
              else
                playTune(&tune_bounce);
              break;
            case BRL_CMD_FWINRT:
              if ((lineLength - lineIndent) > textLength) {
                lineIndent += textLength;
              } else {
                playTune(&tune_bounce);
              }
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
                  unsigned char oldSetting = *item->setting;
                  key -= textStart;

                  if (item->names) {
                    *item->setting = key % (item->maximum + 1);
                  } else {
                    *item->setting = rescaleInteger(key, textCount-1, item->maximum-item->minimum) + item->minimum;
                  }

                  if (item->changed && !item->changed(*item->setting)) {
                    *item->setting = oldSetting;
                    playTune(&tune_command_rejected);
                  } else if (*item->setting != oldSetting) {
                    settingChanged = 1;
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

#ifdef ENABLE_TABLE_SELECTION
  globEnd(&glob_textTable);
  globEnd(&glob_attributesTable);
#ifdef ENABLE_CONTRACTED_BRAILLE
  globEnd(&glob_contractionTable);
#endif /* ENABLE_CONTRACTED_BRAILLE */
#endif /* ENABLE_TABLE_SELECTION */

  return ok;
}
#endif /* ENABLE_PREFERENCES_MENU */

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
    LogPrint(LOG_DEBUG, "performing %s driver autodetection", data->driverType);
  } else {
    LogPrint(LOG_DEBUG, "no autodetectable %s drivers", data->driverType);
  }

  if (!*driver) {
    static const char *const fallbackDrivers[] = {"no", NULL};
    driver = fallbackDrivers;
    autodetect = 0;
  }

  while (*driver) {
    if (!autodetect || data->haveDriver(*driver)) {
      LogPrint(LOG_DEBUG, "checking for %s driver: %s", data->driverType, *driver);
      if (data->initializeDriver(*driver, verify)) return 1;
    }

    ++driver;
  }

  LogPrint(LOG_DEBUG, "%s driver not found", data->driverType);
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
  brl.dataDirectory = opt_dataDirectory;
}

int
constructBrailleDriver (void) {
  initializeBraille();

  if (braille->construct(&brl, brailleParameters, brailleDevice)) {
    if (ensureBrailleBuffer(&brl, LOG_INFO)) {
      brailleConstructed = 1;
      return 1;
    }

    braille->destruct(&brl);
  } else {
    LogPrint(LOG_DEBUG, "%s: %s -> %s",
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
}

static int
initializeBrailleDriver (const char *code, int verify) {
  if ((braille = loadBrailleDriver(code, &brailleObject, opt_libraryDirectory))) {
    brailleParameters = getParameters(braille->parameters,
                                      braille->definition.code,
                                      opt_brailleParameters);
    if (brailleParameters) {
      int constructed = verify;

      if (!constructed) {
        LogPrint(LOG_DEBUG, "initializing braille driver: %s -> %s",
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
        LogPrint(LOG_INFO, "%s: %s [%s]",
                 gettext("Braille Driver"), braille->definition.code, braille->definition.name);
        identifyBrailleDriver(braille, 0);
        logParameters(braille->parameters, brailleParameters,
                      gettext("Braille Parameter"));
        LogPrint(LOG_INFO, "%s: %s", gettext("Braille Device"), brailleDevice);

        /* Initialize the braille driver's help screen. */
        LogPrint(LOG_INFO, "%s: %s", gettext("Help File"),
                 braille->helpFile? braille->helpFile: gettext("none"));
        {
          char *path = makePath(opt_dataDirectory, braille->helpFile);
          if (path) {
            if (verify || constructHelpScreen(path)) {
              LogPrint(LOG_INFO, "%s: %s[%d]", gettext("Help Page"), path, getHelpPageNumber());
            } else {
              LogPrint(LOG_WARNING, "%s: %s", gettext("cannot open help file"), path);
            }
            free(path);
          }
        }

        {
          const char *part1 = CONFIGURATION_DIRECTORY "/brltty-";
          const char *part2 = braille->definition.code;
          const char *part3 = ".prefs";
          char *path = mallocWrapper(strlen(part1) + strlen(part2) + strlen(part3) + 1);
          sprintf(path, "%s%s%s", part1, part2, part3);
          preferencesFile = path;
          fixInstallPath(&preferencesFile);
          if (path != preferencesFile) free(path);
        }
        LogPrint(LOG_INFO, "%s: %s", gettext("Preferences File"), preferencesFile);

        return 1;
      }

      deallocateStrings(brailleParameters);
      brailleParameters = NULL;
    }

    unloadDriverObject(&brailleObject);
  } else {
    LogPrint(LOG_ERR, "%s: %s", gettext("braille driver not loadable"), code);
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
    LogPrint(LOG_DEBUG, "checking braille device: %s", brailleDevice);

    {
      const char *type;
      const char *dev = brailleDevice;

      if (isSerialDevice(&dev)) {
        static const char *const serialDrivers[] = {
          "md", "pm", "ts", "ht", "bn", "al", "bm", "pg", "sk",
          NULL
        };
        autodetectableDrivers = serialDrivers;
        type = "serial";
      } else

#ifdef ENABLE_USB_SUPPORT
      if (isUsbDevice(&dev)) {
        static const char *const usbDrivers[] = {
          "al", "bm", "eu", "fs", "ht", "hm", "pg", "pm", "sk", "vo",
          NULL
        };
        autodetectableDrivers = usbDrivers;
        type = "USB";
      } else
#endif /* ENABLE_USB_SUPPORT */

#ifdef ENABLE_BLUETOOTH_SUPPORT
      if (isBluetoothDevice(&dev)) {
        static const char *bluetoothDrivers[] = {
          "ht", "al", "bm",
          NULL
        };
        autodetectableDrivers = bluetoothDrivers;
        type = "bluetooth";
      } else
#endif /* ENABLE_BLUETOOTH_SUPPORT */

      {
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
  destructHelpScreen();

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

  if (preferencesFile) {
    free(preferencesFile);
    preferencesFile = NULL;
  }
}

static int
startBrailleDriver (void) {
#ifdef ENABLE_BLUETOOTH_SUPPORT
  btForgetConnectErrors();
#endif /* ENABLE_BLUETOOTH_SUPPORT */

  if (activateBrailleDriver(0)) {
    getPreferences();
    applyBraillePreferences();
    setHelpPageNumber(brl.helpPage);
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
  LogPrint(LOG_INFO, gettext("reinitializing braille driver"));
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
    LogPrint(LOG_DEBUG, "speech driver initialization failed: %s",
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
  if ((speech = loadSpeechDriver(code, &speechObject, opt_libraryDirectory))) {
    speechParameters = getParameters(speech->parameters,
                                     speech->definition.code,
                                     opt_speechParameters);
    if (speechParameters) {
      int constructed = verify;

      if (!constructed) {
        LogPrint(LOG_DEBUG, "initializing speech driver: %s",
                 speech->definition.code);

        if (constructSpeechDriver()) {
          constructed = 1;
          speechDriver = speech;
        }
      }

      if (constructed) {
        LogPrint(LOG_INFO, "%s: %s [%s]",
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
    LogPrint(LOG_ERR, "%s: %s", gettext("speech driver not loadable"), code);
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
  LogPrint(LOG_INFO, gettext("reinitializing speech driver"));
  trySpeechDriver();
}

static void
exitSpeechDriver (void) {
  stopSpeechDriver();
}
#endif /* ENABLE_SPEECH_SUPPORT */

static int
initializeScreenDriver (const char *code, int verify) {
  if ((screen = loadScreenDriver(code, &screenObject, opt_libraryDirectory))) {
    screenParameters = getParameters(getScreenParameters(screen),
                                     getScreenDriverDefinition(screen)->code,
                                     opt_screenParameters);
    if (screenParameters) {
      int constructed = verify;

      if (!constructed) {
        LogPrint(LOG_DEBUG, "initializing screen driver: %s",
                 getScreenDriverDefinition(screen)->code);

        if (constructScreenDriver(screenParameters)) {
          constructed = 1;
          screenDriver = screen;
        }
      }

      if (constructed) {
        LogPrint(LOG_INFO, "%s: %s [%s]",
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
    LogPrint(LOG_ERR, "%s: %s", gettext("screen driver not loadable"), code);
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

static void createPidFile (void);

static void
retryPidFile (void *data) {
  createPidFile();
}

static void
createPidFile (void) {
  FILE *stream = fopen(opt_pidFile, "w");
  if (stream) {
    long int pid = getpid();
    fprintf(stream, "%ld\n", pid);
    fclose(stream);
    atexit(exitPidFile);
  } else {
    LogPrint(LOG_WARNING, "%s: %s: %s",
             gettext("cannot open process identifier file"),
             opt_pidFile, strerror(errno));
    asyncRelativeAlarm(5000, retryPidFile, NULL);
  }
}

#if defined(__MINGW32__)
static void
background (void) {
  char *variableName;

  {
    const char *components[] = {programName, "_DAEMON"};
    variableName = joinStrings(components, ARRAY_COUNT(components));
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
      LogWindowsError("SetEnvironmentVariable");
      exit(11);
    }

    if (!CreateProcess(NULL, commandLine, NULL, NULL, TRUE,
                       CREATE_NEW_PROCESS_GROUP,
                       NULL, NULL, &startupInfo, &processInfo)) {
      LogWindowsError("CreateProcess");
      exit(10);
    }

    ExitProcess(0);
  }

  free(variableName);
}

#elif defined(__MSDOS__)

#else /* Unix */
static void
background (void) {
  fflush(stdout);
  fflush(stderr);

  {
    pid_t child = fork();

    if (child == -1) {
      LogError("fork");
      exit(10);
    }

    if (child) _exit(0);
  }

  if (setsid() == -1) {                        
    LogError("setsid");
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
      &opt_libraryDirectory,
      &opt_writableDirectory,
      &opt_dataDirectory,
      &opt_tablesDirectory,
      NULL
    };
    fixInstallPaths(paths);
  }

  if (argc) {
    LogPrint(LOG_ERR, "%s: %s", gettext("excess argument"), argv[0]);
  }

  if (!validateInterval(&updateInterval, opt_updateInterval)) {
    LogPrint(LOG_ERR, "%s: %s", gettext("invalid update interval"), opt_updateInterval);
  }

  if (!validateInterval(&messageDelay, opt_messageDelay)) {
    LogPrint(LOG_ERR, "%s: %s", gettext("invalid message delay"), opt_messageDelay);
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

      LogPrint(LOG_ERR, "%s: %s", gettext("invalid log level"), opt_logLevel);
    }
  setLevel:

    setLogLevel(level);
    setPrintLevel((opt_version || opt_verify)?
                    (opt_quiet? LOG_NOTICE: LOG_INFO):
                    (opt_quiet? LOG_WARNING: LOG_NOTICE));
    if (opt_standardError) LogClose();
  }

  {
    const char *prefix = setPrintPrefix(NULL);
    char banner[0X100];
    makeProgramBanner(banner, sizeof(banner));
    LogPrint(LOG_NOTICE, "%s [%s]", banner, PACKAGE_URL);
    setPrintPrefix(prefix);
  }

  if (opt_version) {
    LogPrint(LOG_INFO, "%s", PACKAGE_COPYRIGHT);
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
  ) background();

  if (!opt_standardError) {
    LogClose();
    LogOpen(1);
  }

  if (!opt_noDaemon) {
    setPrintOff();

    {
      const char *nullDevice = "/dev/null";

      freopen(nullDevice, "r", stdin);
      freopen(nullDevice, "a", stdout);

      if (opt_standardError) {
        fflush(stderr);
      } else {
        freopen(nullDevice, "a", stderr);
      }
    }

#ifdef __MINGW32__
    {
      HANDLE h = CreateFile("NUL", GENERIC_READ|GENERIC_WRITE,
                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, 0, NULL);

      if (!h) {
        LogWindowsError("CreateFile[NUL]");
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
   * used anymore since we are a daemon.  The LogPrint facility should 
   * be used instead.
   */

  atexit(exitTunes);
  suppressTuneDeviceOpenErrors();

  /* Create the process identifier file. */
  if (*opt_pidFile) createPidFile();

  {
    const char *directories[] = {opt_dataDirectory, "/", NULL};
    const char **directory = directories;

    while (*directory) {
      if (setWorkingDirectory(*directory)) break;                /* * change to directory containing data files  */
      ++directory;
    }
  }

  {
    char *directory;
    if ((directory = getWorkingDirectory())) {
      LogPrint(LOG_INFO, "%s: %s", gettext("Working Directory"), directory);
      free(directory);
    } else {
      LogPrint(LOG_ERR, "%s: %s", gettext("cannot determine working directory"), strerror(errno));
    }
  }

  LogPrint(LOG_INFO, "%s: %s", gettext("Writable Directory"), opt_writableDirectory);
  writableDirectory = opt_writableDirectory;

  LogPrint(LOG_INFO, "%s: %s", gettext("Configuration File"), opt_configurationFile);
  LogPrint(LOG_INFO, "%s: %s", gettext("Data Directory"), opt_dataDirectory);
  LogPrint(LOG_INFO, "%s: %s", gettext("Library Directory"), opt_libraryDirectory);
  LogPrint(LOG_INFO, "%s: %s", gettext("Tables Directory"), opt_tablesDirectory);

  /* handle text table option */
  if (*opt_textTable) {
    if (strcmp(opt_textTable, "auto") == 0) {
      const char *locale = setlocale(LC_CTYPE, NULL);
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
  LogPrint(LOG_INFO, "%s: %s", gettext("Text Table"), opt_textTable);

#ifdef ENABLE_PREFERENCES_MENU
#ifdef ENABLE_TABLE_SELECTION
  globPrepare(&glob_textTable, opt_tablesDirectory, TEXT_TABLE_EXTENSION, opt_textTable, 0);
#endif /* ENABLE_TABLE_SELECTION */
#endif /* ENABLE_PREFERENCES_MENU */

  /* handle attributes table option */
  if (*opt_attributesTable)
    if (!replaceAttributesTable(opt_attributesTable))
      opt_attributesTable = "";
  if (!*opt_attributesTable) opt_attributesTable = ATTRIBUTES_TABLE;
  LogPrint(LOG_INFO, "%s: %s", gettext("Attributes Table"), opt_attributesTable);

#ifdef ENABLE_PREFERENCES_MENU
#ifdef ENABLE_TABLE_SELECTION
  globPrepare(&glob_attributesTable, opt_tablesDirectory, ATTRIBUTES_TABLE_EXTENSION, opt_attributesTable, 0);
#endif /* ENABLE_TABLE_SELECTION */
#endif /* ENABLE_PREFERENCES_MENU */

#ifdef ENABLE_CONTRACTED_BRAILLE
  /* handle contraction table option */
  atexit(exitContractionTable);
  if (*opt_contractionTable) loadContractionTable(opt_contractionTable);
  LogPrint(LOG_INFO, "%s: %s", gettext("Contraction Table"),
           *opt_contractionTable? opt_contractionTable: gettext("none"));

#ifdef ENABLE_PREFERENCES_MENU
#ifdef ENABLE_TABLE_SELECTION
  globPrepare(&glob_contractionTable, opt_tablesDirectory, CONTRACTION_TABLE_EXTENSION, opt_contractionTable, 1);
#endif /* ENABLE_TABLE_SELECTION */
#endif /* ENABLE_PREFERENCES_MENU */
#endif /* ENABLE_CONTRACTED_BRAILLE */

  /* handle key table option */
  atexit(exitKeyTable);
  if (*opt_keyTable) loadKeyTable(opt_keyTable);
  LogPrint(LOG_INFO, "%s: %s", gettext("Key Table"),
           *opt_keyTable? opt_keyTable: gettext("none"));

  if (parseKeyboardProperties(&keyboardProperties, opt_keyboardProperties))
    if (keyTable)
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
    LogPrint(LOG_ERR, gettext("braille device not specified"));
    exit(4);
  }
  brailleDevices = splitString(opt_brailleDevice, ',', NULL);

  /* Activate the braille display. */
  brailleDrivers = splitString(opt_brailleDriver? opt_brailleDriver: "", ',', NULL);
  brailleConstructed = 0;
  if (opt_verify) {
    if (activateBrailleDriver(1)) deactivateBrailleDriver();
  } else {
    resetPreferences();
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

  /* Create the speech pass-through FIFO. */
  LogPrint(LOG_INFO, "%s: %s", gettext("Speech FIFO"),
           *opt_speechFifo? opt_speechFifo: gettext("none"));
  if (!opt_verify) {
    if (*opt_speechFifo) openSpeechFifo(opt_dataDirectory, opt_speechFifo);
  }
#endif /* ENABLE_SPEECH_SUPPORT */

  if (opt_verify) exit(0);
}
