/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#include "prologue.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */

#ifdef ENABLE_PREFERENCES_MENU
#ifdef ENABLE_TABLE_SELECTION
#ifdef HAVE_GLOB_H
#include <glob.h>
#endif /* HAVE_GLOB_H */
#endif /* ENABLE_TABLE_SELECTION */
#endif /* ENABLE_PREFERENCES_MENU */

#include "cmd.h"
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
#include "io_serial.h"

#ifdef ENABLE_USB_SUPPORT
#include "io_usb.h"
#endif /* ENABLE_USB_SUPPORT */

#ifdef ENABLE_BLUETOOTH_SUPPORT
#include "io_bluetooth.h"
#endif /* ENABLE_BLUETOOTH_SUPPORT */

char COPYRIGHT[] = "Copyright (C) 1995-2005 by The BRLTTY Team - all rights reserved.";

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
static char *opt_dataDirectory;
static char *opt_libraryDirectory;

static char *opt_brailleDevice;
static char **brailleDevices;
static const char *brailleDevice = NULL;

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
static char *opt_contractionsDirectory;
static char *opt_contractionTable;
#endif /* ENABLE_CONTRACTED_BRAILLE */

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
static char *opt_screenParameters;
static char **screenParameters = NULL;

#ifdef ENABLE_PCM_SUPPORT
char *opt_pcmDevice;
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
char *opt_midiDevice;
#endif /* ENABLE_MIDI_SUPPORT */

BEGIN_OPTION_TABLE
  {"attributes-table", "file", 'a', 0, OPT_Config | OPT_Environ,
   &opt_attributesTable, NULL,
   "Path to attributes translation table file."},

  {"braille-driver", "driver", 'b', 1, OPT_Config | OPT_Environ,
   &opt_brailleDriver, NULL,
   "Braille driver: one of {" BRAILLE_DRIVER_CODES "}"},

#ifdef ENABLE_CONTRACTED_BRAILLE
  {"contraction-table", "file", 'c', 0, OPT_Config | OPT_Environ,
   &opt_contractionTable, NULL,
   "Path to contraction table file."},
#endif /* ENABLE_CONTRACTED_BRAILLE */

  {"braille-device", "device", 'd', 2, OPT_Config | OPT_Environ,
   &opt_brailleDevice, BRAILLE_DEVICE,
   "Path to device for accessing braille display."},

  {"standard-error", NULL, 'e', 0, 0,
   &opt_standardError, NULL,
   "Log to standard error rather than to syslog."},

  {"configuration-file", "file", 'f', 0, OPT_Environ,
   &opt_configurationFile, CONFIGURATION_DIRECTORY "/" CONFIGURATION_FILE,
   "Path to default parameters file."},

  {"log-level", "level", 'l', 0, 0,
   &opt_logLevel, NULL,
   "Diagnostic logging level: 0-7 [5], or one of {emergency alert critical error warning [notice] information debug}"},

#ifdef ENABLE_MIDI_SUPPORT
  {"midi-device", "device", 'm', 0, OPT_Config | OPT_Environ,
   &opt_midiDevice, NULL,
   "Device specifier for the Musical Instrument Digital Interface."},
#endif /* ENABLE_MIDI_SUPPORT */

  {"no-daemon", NULL, 'n', 0, 0,
   &opt_noDaemon, NULL,
   "Remain a foreground process."},

#ifdef ENABLE_PCM_SUPPORT
  {"pcm-device", "device", 'p', 0, OPT_Config | OPT_Environ,
   &opt_pcmDevice, NULL,
   "Device specifier for soundcard digital audio."},
#endif /* ENABLE_PCM_SUPPORT */

  {"quiet", NULL, 'q', 0, 0,
   &opt_quiet, NULL,
   "Suppress start-up messages."},

#ifdef ENABLE_SPEECH_SUPPORT
  {"speech-driver", "driver", 's', 0, OPT_Config | OPT_Environ,
   &opt_speechDriver, NULL,
   "Speech driver: one of {" SPEECH_DRIVER_CODES "}"},
#endif /* ENABLE_SPEECH_SUPPORT */

  {"text-table", "file", 't', 3, OPT_Config | OPT_Environ,
   &opt_textTable, NULL,
   "Path to text translation table file."},

  {"verify", NULL, 'v', 0, 0,
   &opt_verify, NULL,
   "Print start-up messages and exit."},

  {"screen-driver", "driver", 'x', 0, OPT_Config | OPT_Environ,
   &opt_screenDriver, SCREEN_DRIVER,
   "Screen driver: one of {" SCREEN_DRIVER_CODES "}"},

#ifdef ENABLE_API
  {"api-parameters", "arg,...", 'A', 0, OPT_Extend | OPT_Config | OPT_Environ,
   &opt_apiParameters, API_PARAMETERS,
   "Parameters for the application programming interface."},
#endif /* ENABLE_API */

  {"braille-parameters", "arg,...", 'B', 0, OPT_Extend | OPT_Config | OPT_Environ,
   &opt_brailleParameters, BRAILLE_PARAMETERS,
   "Parameters for the braille driver."},

#ifdef ENABLE_CONTRACTED_BRAILLE
  {"contractions-directory", "directory", 'C', 0, OPT_Hidden | OPT_Config | OPT_Environ,
   &opt_contractionsDirectory, DATA_DIRECTORY,
   "Path to directory for contractions tables."},
#endif /* ENABLE_CONTRACTED_BRAILLE */

  {"data-directory", "directory", 'D', 0, OPT_Hidden | OPT_Config | OPT_Environ,
   &opt_dataDirectory, DATA_DIRECTORY,
   "Path to directory for driver help and configuration files."},

  {"environment-variables", NULL, 'E', 0, 0,
   &opt_environmentVariables, NULL,
   "Recognize environment variables."},

#ifdef ENABLE_SPEECH_SUPPORT
  {"speech-fifo", "file", 'F', 0, OPT_Config | OPT_Environ,
   &opt_speechFifo, NULL,
   "Path to speech pass-through FIFO."},
#endif /* ENABLE_SPEECH_SUPPORT */

  {"library-directory", "directory", 'L', 0, OPT_Hidden | OPT_Config | OPT_Environ,
   &opt_libraryDirectory, LIBRARY_DIRECTORY,
   "Path to directory for loading drivers."},

  {"message-delay", "csecs", 'M', 0, 0,
   &opt_messageDelay, NULL,
   "Message hold time [400]."},

#ifdef ENABLE_API
  {"no-api", NULL, 'N', 0, 0,
   &opt_noApi, NULL,
   "Disable the application programming interface."},
#endif /* ENABLE_API */

  {"pid-file", "file", 'P', 0, 0,
   &opt_pidFile, NULL,
   "Path to process identifier file."},

#ifdef ENABLE_SPEECH_SUPPORT
  {"speech-parameters", "arg,...", 'S', 0, OPT_Extend | OPT_Config | OPT_Environ,
   &opt_speechParameters, SPEECH_PARAMETERS,
   "Parameters for the speech driver."},
#endif /* ENABLE_SPEECH_SUPPORT */

  {"tables-directory", "directory", 'T', 0, OPT_Hidden | OPT_Config | OPT_Environ,
   &opt_tablesDirectory, DATA_DIRECTORY,
   "Path to directory for text and attributes tables."},

  {"update-interval", "csecs", 'U', 0, 0,
   &opt_updateInterval, NULL,
   "Braille window update interval [4]."},

  {"version", NULL, 'V', 0, 0,
   &opt_version, NULL,
   "Print the versions of the core, API, and built-in drivers, and then exit."},

  {"screen-parameters", "arg,...", 'X', 0, OPT_Extend | OPT_Config | OPT_Environ,
   &opt_screenParameters, SCREEN_PARAMETERS,
   "Parameters for the screen driver."},
END_OPTION_TABLE

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
          LogPrint(LOG_ERR, "missing %s parameter value: %s",
                   description, name);
        } else if (value == name) {
        noName:
          LogPrint(LOG_ERR, "missing %s parameter name: %s",
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
                LogPrint(LOG_ERR, "missing %s code: %s",
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
              if (strncasecmp(name, names[index], nameLength) == 0) {
                free(values[index]);
                values[index] = strdupWrapper(value);
                break;
              }
              ++index;
            }

            if (!names[index]) {
              LogPrint(LOG_ERR, "unsupported %s parameter: %s", description, name);
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
  const char *parameters
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

  parseParameters(values, names, description, qualifier, parameters);
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

static int
replaceTranslationTable (TranslationTable table, const char *file, const char *type) {
  int ok = 0;
  char *path = makePath(opt_tablesDirectory, file);
  if (path) {
    if (loadTranslationTable(path, NULL, table, 0)) {
      ok = 1;
    }
    free(path);
  }
  if (!ok) LogPrint(LOG_ERR, "cannot load %s table: %s", type, file);
  return ok;
}

static int
replaceTextTable (const char *file) {
  if (!replaceTranslationTable(textTable, file, "text")) return 0;
  makeUntextTable();
  return 1;
}

static int
replaceAttributesTable (const char *file) {
  return replaceTranslationTable(attributesTable, file, "attributes");
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
loadContractionTable (const char *file) {
  void *table = NULL;
  if (*file) {
    char *path = makePath(opt_contractionsDirectory, file);
    LogPrint(LOG_DEBUG, "compiling contraction table: %s", file);
    if (path) {
      if (!(table = compileContractionTable(path))) {
        LogPrint(LOG_ERR, "cannot compile contraction table: %s", path);
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

static void
setBraillePreferences (void) {
  if (braille->firmness) braille->firmness(&brl, prefs.brailleFirmness);
}

#ifdef ENABLE_SPEECH_SUPPORT
static void
setSpeechPreferences (void) {
  if (speech->rate) setSpeechRate(prefs.speechRate, 0);
  if (speech->volume) setSpeechVolume(prefs.speechVolume, 0);
}
#endif /* ENABLE_SPEECH_SUPPORT */

static void
dimensionsChanged (int rows, int columns) {
  fwinshift = MAX(columns-prefs.windowOverlap, 1);
  hwinshift = columns / 2;
  vwinshift = (rows > 1)? rows: 5;
  LogPrint(LOG_DEBUG, "shifts: fwin=%d hwin=%d vwin=%d",
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
  setTuneDevice(prefs.tuneDevice);
  setBraillePreferences();
#ifdef ENABLE_SPEECH_SUPPORT
  setSpeechPreferences();
#endif /* ENABLE_SPEECH_SUPPORT */
}

int
loadPreferences (int change) {
  int ok = 0;
  int fd = open(preferencesFile, O_RDONLY);
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
        LogPrint(LOG_ERR, "invalid preferences file: %s", preferencesFile);
    } else if (length == -1) {
      LogPrint(LOG_ERR, "cannot read preferences file: %s: %s",
               preferencesFile, strerror(errno));
    } else {
      long int actualSize = sizeof(newPreferences);
      LogPrint(LOG_ERR, "preferences file '%s' has incorrect size %d (should be %ld).",
               preferencesFile, length, actualSize);
    }
    close(fd);
  } else
    LogPrint((errno==ENOENT? LOG_DEBUG: LOG_ERR),
             "Cannot open preferences file: %s: %s",
             preferencesFile, strerror(errno));
  return ok;
}

static void
getPreferences (void) {
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

    prefs.statusStyle = braille->statusStyle;
  }
  setTuneDevice(prefs.tuneDevice);
}

#ifdef ENABLE_PREFERENCES_MENU
int 
savePreferences (void) {
  int ok = 0;
  int fd = open(preferencesFile, O_WRONLY | O_CREAT | O_TRUNC);
  if (fd != -1) {
#ifdef HAVE_FCHMOD
    fchmod(fd, S_IRUSR | S_IWUSR);
#else /* HAVE_FCHMOD */
    chmod(preferencesFile, S_IRUSR | S_IWUSR);
#endif /* HAVE_FCHMOD */
    if (write(fd, &prefs, sizeof(prefs)) == sizeof(prefs)) {
      ok = 1;
    } else {
      LogPrint(LOG_ERR, "cannot write to preferences file: %s: %s",
               preferencesFile, strerror(errno));
    }
    close(fd);
  } else {
    LogPrint(LOG_ERR, "cannot open preferences file: %s: %s",
             preferencesFile, strerror(errno));
  }
  if (!ok)
    message("not saved", 0);
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

#ifdef ENABLE_SPEECH_SUPPORT
static int
testSpeechRate (void) {
  return speech->rate != NULL;
}

static int
changedSpeechRate (unsigned char setting) {
  setSpeechRate(setting, 1);
  return 1;
}

static int
testSpeechVolume (void) {
  return speech->volume != NULL;
}

static int
changedSpeechVolume (unsigned char setting) {
  setSpeechVolume(setting, 1);
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
  const char *pattern;
  const char *initial;
  char *current;
  unsigned none:1;
#ifdef HAVE_GLOB_H
  glob_t glob;
#endif /* HAVE_GLOB_H */
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

  data->paths = data->pathsArea;
  data->count = (sizeof(data->pathsArea) / sizeof(data->pathsArea[0])) - 1;
  data->paths[data->count] = NULL;
  index = data->count;

#ifdef HAVE_GLOB_H
  memset(&data->glob, 0, sizeof(data->glob));
  data->glob.gl_offs = data->count;
#endif /* HAVE_GLOB_H */

  {
#ifdef HAVE_FCHDIR
    int originalDirectory = open(".", O_RDONLY);
    if (originalDirectory != -1) {
#else /* HAVE_FCHDIR */
    char *originalDirectory = getWorkingDirectory();
    if (originalDirectory) {
#endif /* HAVE_FCHDIR */
      if (chdir(data->directory) != -1) {
#ifdef HAVE_GLOB_H
        if (glob(data->pattern, GLOB_DOOFFS, NULL, &data->glob) == 0) {
          data->paths = (const char **)data->glob.gl_pathv;
          /* The behaviour of gl_pathc is inconsistent. Some implementations
           * include the leading NULL pointers and some don't. Let's just
           * figure it out the hard way by finding the trailing NULL.
           */
          while (data->paths[data->count]) ++data->count;
        }
#endif /* HAVE_GLOB_H */

#ifdef HAVE_FCHDIR
        if (fchdir(originalDirectory) == -1) {
#else /* HAVE_FCHDIR */
        if (chdir(originalDirectory) == -1) {
#endif /* HAVE_FCHDIR */
          LogError("working directory restore");
        }
      } else {
        LogPrint(LOG_ERR, "cannot set working directory: %s: %s",
                 data->directory, strerror(errno));
      }

#ifdef HAVE_FCHDIR
      close(originalDirectory);
#else /* HAVE_FCHDIR */
      free(originalDirectory);
#endif /* HAVE_FCHDIR */
    } else {
#ifdef HAVE_FCHDIR
      LogError("working directory open");
#else /* HAVE_FCHDIR */
      LogError("working directory determination");
#endif /* HAVE_FCHDIR */
    }
  }

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
#ifdef HAVE_GLOB_H
  if (data->glob.gl_pathc) {
    int index;
    for (index=0; index<data->glob.gl_offs; ++index)
      data->glob.gl_pathv[index] = NULL;
    globfree(&data->glob);
  }
#endif /* HAVE_GLOB_H */
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
       BOOLEAN_ITEM(prefs.autorepeat, changedAutorepeat, NULL, "Autorepeat"),
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
    int indexChanged = 1;
    int settingChanged = 0;                        /* 1 when item's value has changed */

    Preferences oldPreferences = prefs;        /* backup preferences */
    int command = EOF;                                /* readbrl() value */

    /* status cells */
    setStatusText(&brl, "prf");
    message("Preferences Menu", 0);

    if (prefs.autorepeat) resetAutorepeat();

    while (1) {
      MenuItem *item = &menu[menuIndex];
      char valueBuffer[0X10];
      const char *value;

      testProgramTermination();
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

#ifdef ENABLE_SPEECH_SUPPORT
        if (prefs.autospeak) {
          if (indexChanged) {
            sayString(line, 1);
          } else if (settingChanged) {
            sayString(value, 1);
          }
        }
#endif /* ENABLE_SPEECH_SUPPORT */

        /* Next we deal with the braille window position in the buffer.
         * This is intended for small displays and/or long item descriptions 
         */
        if (settingChanged) {
          settingChanged = 0;
          /* make sure the updated value is visible */
          if ((lineLength-lineIndent > brl.x*brl.y) && (lineIndent < settingIndent))
            lineIndent = settingIndent;
        }
        indexChanged = 0;

        /* Then draw the braille window */
        writeBrailleText(&brl, &line[lineIndent], MAX(0, lineLength-lineIndent));
        drainBrailleOutput(&brl, updateInterval);

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
              speech->say((unsigned char *)line, lineLength);
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
                  *item->setting = rescaleInteger(key, brl.x-1, item->maximum-item->minimum) + item->minimum;
                }
                if (item->changed && !item->changed(*item->setting)) {
                  *item->setting = oldSetting;
                  playTune(&tune_command_rejected);
                } else if (*item->setting != oldSetting) {
                  settingChanged = 1;
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
}
#endif /* ENABLE_PREFERENCES_MENU */

void
initializeBraille (void) {
  initializeBrailleDisplay(&brl);
  brl.bufferResized = &dimensionsChanged;
  brl.dataDirectory = opt_dataDirectory;
}

static int
openBrailleDriver (int verify) {
  int oneDevice = brailleDevices[0] && !brailleDevices[1];
  int oneDriver = brailleDrivers[0] && !brailleDrivers[1];
  int autodetect = oneDriver && (strcmp(brailleDrivers[0], "auto") == 0);
  const char *const *device = (const char *const *)brailleDevices;

  if (!oneDevice || !oneDriver || autodetect) verify = 0;

  while (*device) {
    const char *const *code;
    brailleDevice = *device;

    if (autodetect) {
      const char *type;
      const char *dev = brailleDevice;

      if (isSerialDevice(&dev)) {
        static const char *const serialCodes[] = {
          "md", "pm", "ts", "ht", "bn", "al", "bm",
          NULL
        };
        code = serialCodes;
        type = "serial";

#ifdef ENABLE_USB_SUPPORT
      } else if (isUsbDevice(&dev)) {
        static const char *const usbCodes[] = {
          "al", "bm", "fs", "ht", "pm", "vo",
          NULL
        };
        code = usbCodes;
        type = "USB";
#endif /* ENABLE_USB_SUPPORT */

#ifdef ENABLE_BLUETOOTH_SUPPORT
      } else if (isBluetoothDevice(&dev)) {
        static const char *bluetoothCodes[] = {
          "ht", "bm",
          NULL
        };
        code = bluetoothCodes;
        type = "bluetooth";
#endif /* ENABLE_BLUETOOTH_SUPPORT */

      } else {
        LogPrint(LOG_WARNING, "braille display autodetection not supported for device '%s'.", dev);
        goto nextDevice;
      }
      LogPrint(LOG_DEBUG, "looking for %s braille display on '%s'.", type, brailleDevice);
    } else {
      code = (const char *const *)brailleDrivers;
      LogPrint(LOG_DEBUG, "looking for braille display on '%s'.", brailleDevice);
    }

    while (*code) {
      LogPrint(LOG_DEBUG, "checking for '%s' braille display.", *code);

      if ((braille = loadBrailleDriver(*code, &brailleObject, opt_libraryDirectory))) {
        brailleParameters = processParameters(braille->parameters,
                                              "braille driver",
                                              braille->code,
                                              opt_brailleParameters);
        if (brailleParameters) {
          int opened = verify;

          if (!opened) {
            LogPrint(LOG_DEBUG, "initializing braille driver: %s -> %s",
                     braille->code, brailleDevice);

#ifdef ENABLE_API
            if (apiStarted) api_link();
#endif /* ENABLE_API */

            initializeBraille();
            if (braille->open(&brl, brailleParameters, brailleDevice)) {
              opened = 1;
              brailleDriver = braille;
            } else {
#ifdef ENABLE_API
              if (apiStarted) api_unlink();
#endif /* ENABLE_API */
            }
          }

          if (opened) {
            LogPrint(LOG_INFO, "Braille Driver: %s [%s] (compiled on %s at %s)",
                     braille->code, braille->name,
                     braille->date, braille->time);
            braille->identify();
            logParameters(braille->parameters, brailleParameters, "Braille");
            LogPrint(LOG_INFO, "Braille Device: %s", brailleDevice);

            /* Initialize the braille driver's help screen. */
            LogPrint(LOG_INFO, "Help File: %s",
                     braille->helpFile? braille->helpFile: "none");
            {
              char *path = makePath(opt_dataDirectory, braille->helpFile);
              if (path) {
                if (verify || openHelpScreen(path)) {
                  LogPrint(LOG_INFO, "Help Page: %s[%d]", path, getHelpPageNumber());
                } else {
                  LogPrint(LOG_WARNING, "cannot open help file: %s", path);
                }
                free(path);
              }
            }

            {
              const char *part1 = CONFIGURATION_DIRECTORY "/brltty-";
              const char *part2 = braille->code;
              const char *part3 = ".prefs";
              char *path = mallocWrapper(strlen(part1) + strlen(part2) + strlen(part3) + 1);
              sprintf(path, "%s%s%s", part1, part2, part3);
              preferencesFile = path;
            }
            LogPrint(LOG_INFO, "Preferences File: %s", preferencesFile);

            return 1;
          } else {
            LogPrint(LOG_DEBUG, "braille driver initialization failed: %s -> %s",
                     braille->code, brailleDevice);
          }

          deallocateStrings(brailleParameters);
          brailleParameters = NULL;
        }

        braille = &noBraille;
        if (brailleObject) {
          unloadSharedObject(brailleObject);
          brailleObject = NULL;
        }
      } else {
        LogPrint(LOG_ERR, "braille driver not loadable: %s", *code);
      }

      ++code;
    }

  nextDevice:
    ++device;
  }
  brailleDevice = NULL;

  LogPrint(LOG_DEBUG, "no braille display found.");
  return 0;
}

static void
closeBrailleDriver (void) {
  closeHelpScreen();

  if (brailleDriver) {
    drainBrailleOutput(&brl, 0);
    braille->close(&brl);

#ifdef ENABLE_API
    if (apiStarted) api_unlink();
#endif /* ENABLE_API */

    braille = &noBraille;
    brailleDevice = NULL;
    brailleDriver = NULL;
  }

  if (brailleObject) {
    unloadSharedObject(brailleObject);
    brailleObject = NULL;
  }

  if (brailleParameters) {
    deallocateStrings(brailleParameters);
    brailleParameters = NULL;
  }

  if (preferencesFile) {
    free(preferencesFile);
    preferencesFile = NULL;
  }
}

static void
startBrailleDriver (void) {
  while (1) {
    testProgramTermination();
    if (openBrailleDriver(0)) {
      if (allocateBrailleBuffer(&brl)) {
        getPreferences();
        setBraillePreferences();

        clearStatusCells(&brl);
        setHelpPageNumber(brl.helpPage);
        playTune(&tune_braille_on);
        return;
      } else {
        LogPrint(LOG_DEBUG, "braille buffer allocation failed.");
      }
      closeBrailleDriver();
    }
    approximateDelay(5000);
  }
}

static void
stopBrailleDriver (void) {
  closeBrailleDriver();

  if (brl.isCoreBuffer) {
    free(brl.buffer);
    brl.buffer = NULL;
  }

  playTune(&tune_braille_off);
}

void
restartBrailleDriver (void) {
  stopBrailleDriver();
  LogPrint(LOG_INFO, "reinitializing braille driver.");
  startBrailleDriver();
}

static void
exitBrailleDriver (void) {
  clearStatusCells(&brl);
  message("BRLTTY terminated.", MSG_NODELAY|MSG_SILENT);
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
}

static int
openSpeechDriver (int verify) {
  int oneDriver = speechDrivers[0] && !speechDrivers[1];
  int autodetect = oneDriver && (strcmp(speechDrivers[0], "auto") == 0);
  const char *const *code;

  if (!oneDriver || autodetect) verify = 0;

  if (autodetect) {
    static const char *const speechCodes[] = {
      "al", "bl", "cb",
      NULL
    };
    code = speechCodes;
  } else {
    code = (const char *const *)speechDrivers;
  }
  LogPrint(LOG_DEBUG, "looking for speech synthesizer.");

  while (*code) {
    LogPrint(LOG_DEBUG, "checking for '%s' speech synthesizer.", *code);

    if ((speech = loadSpeechDriver(*code, &speechObject, opt_libraryDirectory))) {
      speechParameters = processParameters(speech->parameters,
                                           "speech driver",
                                           speech->code,
                                           opt_speechParameters);
      if (speechParameters) {
        int opened = verify;

        if (!opened) {
          LogPrint(LOG_DEBUG, "initializing speech driver: %s",
                   speech->code);

          initializeSpeech();
          if (speech->open(speechParameters)) {
            opened = 1;
            speechDriver = speech;
          }
        }

        if (opened) {
          LogPrint(LOG_INFO, "Speech Driver: %s [%s] (compiled on %s at %s)",
                   speech->code, speech->name,
                   speech->date, speech->time);
          speech->identify();
          logParameters(speech->parameters, speechParameters, "Speech");

          return 1;
        } else {
          LogPrint(LOG_DEBUG, "speech driver initialization failed: %s",
                   speech->code);
        }

        deallocateStrings(speechParameters);
        speechParameters = NULL;
      }

      speech = &noSpeech;
      if (speechObject) {
        unloadSharedObject(speechObject);
        speechObject = NULL;
      }
    } else {
      LogPrint(LOG_ERR, "speech driver not loadable: %s", *code);
    }

    ++code;
  }

  LogPrint(LOG_DEBUG, "no speech synthesizer found.");
  return 0;
}

static void
closeSpeechDriver (void) {
  if (speechDriver) {
    speech->close();

    speech = &noSpeech;
    speechDriver = NULL;
  }

  if (speechObject) {
    unloadSharedObject(speechObject);
    speechObject = NULL;
  }

  if (speechParameters) {
    deallocateStrings(speechParameters);
    speechParameters = NULL;
  }
}

static void
startSpeechDriver (void) {
  if (openSpeechDriver(0)) {
    setSpeechPreferences();
  }
}

static void
stopSpeechDriver (void) {
  speech->mute();
  closeSpeechDriver();
}

void
restartSpeechDriver (void) {
  stopSpeechDriver();
  LogPrint(LOG_INFO, "reinitializing speech driver.");
  startSpeechDriver();
}

static void
exitSpeechDriver (void) {
  stopSpeechDriver();
}
#endif /* ENABLE_SPEECH_SUPPORT */

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

#if defined(WINDOWS)
#define BACKGROUND_EVENT_PREFIX "BRLTTY background "
static void
background (void) {
  LPTSTR cmdline = GetCommandLine();
  int len = strlen(cmdline);
  char newcmdline[len+4];
  STARTUPINFO startupinfo;
  PROCESS_INFORMATION processinfo;
  HANDLE event;
  DWORD res;
  char name[] = BACKGROUND_EVENT_PREFIX "XXXXXXXX";
  
  memset(&startupinfo, 0, sizeof(startupinfo));
  startupinfo.cb = sizeof(startupinfo);

  memcpy(newcmdline, cmdline, len);
  memcpy(newcmdline+len, " -n", 4);

  if (!CreateProcess(NULL, newcmdline, NULL, NULL, TRUE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, &startupinfo, &processinfo)) {
    LogWindowsError("CreateProcess");
    exit(10);
  }
  snprintf(name, sizeof(name), BACKGROUND_EVENT_PREFIX "%8lx", processinfo.dwProcessId);
  if (!(event = CreateEvent(NULL, TRUE, FALSE, name))) {
    LogWindowsError("Creating event for background synchronization");
    exit(11);
  }
  /* wait at most for 100ms, then check whether it's still alive */
  while ((res = WaitForSingleObject(event, 100)) != WAIT_OBJECT_0) {
    if (res == WAIT_FAILED) {
      LogWindowsError("Waiting for synchronization event");
      exit(12);
    }
    if (!GetExitCodeProcess(processinfo.hProcess, &res)) {
      LogWindowsError("Getting result code from processus");
      exit(13);
    }
    if (res != STILL_ACTIVE)
      _exit(res);
  }
  ExitProcess(0);
}
#elif defined(HAVE_SYS_WAIT_H)
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
      LogPrint(LOG_CRIT, "process creation error: %s", strerror(errno));
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
#endif /* background() */

static int
validateInterval (int *value, const char *description, const char *word) {
  if (!word || !*word) return 1;

  {
    static const int minimum = 1;
    int ok = validateInteger(value, description, word, &minimum, NULL);
    if (ok) *value *= 10;
    return ok;
  }
}

void
startup (int argc, char *argv[]) {
  int problemCount = processOptions(optionTable, optionCount,
                                    "brltty", &argc, &argv,
                                    &opt_bootParameters,
                                    &opt_environmentVariables,
                                    &opt_configurationFile,
                                    NULL);
  if (argc) {
    LogPrint(LOG_ERR, "excess parameter: %s", argv[0]);
    ++problemCount;
  }

  if (!validateInterval(&updateInterval, "update interval", opt_updateInterval)) ++problemCount;
  if (!validateInterval(&messageDelay, "message delay", opt_messageDelay)) ++problemCount;

  /* Set logging levels. */
  {
    int level = LOG_NOTICE;

    if (opt_logLevel && *opt_logLevel) {
      static const char *const words[] = {
        "emergency", "alert", "critical", "error",
        "warning", "notice", "information", "debug"
      };
      static unsigned int count = sizeof(words) / sizeof(words[0]);

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

      LogPrint(LOG_ERR, "invalid log level: %s", opt_logLevel);
      ++problemCount;
    }
  setLevel:

    setLogLevel(level);
    setPrintLevel((opt_version || opt_verify)?
                    (opt_quiet? LOG_NOTICE: LOG_INFO):
                    (opt_quiet? LOG_WARNING: LOG_NOTICE));
    if (opt_standardError) LogClose();
  }

  atexit(exitTunes);
  suppressTuneDeviceOpenErrors();

  LogPrint(LOG_NOTICE, "%s %s", PACKAGE_TITLE, PACKAGE_VERSION);
  LogPrint(LOG_INFO, "%s", COPYRIGHT);
  if (opt_version) {
#ifdef ENABLE_API
    api_identify();
#endif /* ENABLE_API */

    identifyBrailleDrivers();

#ifdef ENABLE_SPEECH_SUPPORT
    identifySpeechDrivers();
#endif /* ENABLE_SPEECH_SUPPORT */
    exit(0);
  }

#if defined(WINDOWS)
  if (!opt_noDaemon) {
    background();
  } else if (!opt_standardError) {
    LogClose();
    LogOpen(1);
  }
#elif defined(HAVE_SYS_WAIT_H)
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
#else /* background() */
  opt_noDaemon = 1;
#endif /* background() */

  /* Create the process identifier file. */
  if (opt_pidFile) {
    FILE *stream = fopen(opt_pidFile, "w");
    if (stream) {
      long pid = getpid();
      fprintf(stream, "%ld\n", pid);
      fclose(stream);
      atexit(exitPidFile);
    } else {
      LogPrint(LOG_ERR, "cannot open process identifier file: %s: %s",
               opt_pidFile, strerror(errno));
    }
  }

  {
    const char *directories[] = {DATA_DIRECTORY, "/etc", "/", NULL};
    const char **directory = directories;
    while (*directory) {
      if (chdir(*directory) != -1) break;                /* * change to directory containing data files  */
      LogPrint(LOG_WARNING, "cannot change working directory to '%s': %s",
               *directory, strerror(errno));
      ++directory;
    }
  }

  {
    char *directory;
    if ((directory = getWorkingDirectory())) {
      LogPrint(LOG_INFO, "Working Directory: %s", directory);
      free(directory);
    } else {
      LogPrint(LOG_ERR, "cannot determine working directory: %s", strerror(errno));
    }
  }

  LogPrint(LOG_INFO, "Configuration File: %s", opt_configurationFile);
  LogPrint(LOG_INFO, "Data Directory: %s", opt_dataDirectory);
  LogPrint(LOG_INFO, "Library Directory: %s", opt_libraryDirectory);
  LogPrint(LOG_INFO, "Tables Directory: %s", opt_tablesDirectory);

  if (opt_textTable) {
    fixTextTablePath(&opt_textTable);
    if (!replaceTextTable(opt_textTable)) opt_textTable = NULL;
  }
  if (!opt_textTable) {
    opt_textTable = TEXT_TABLE;
    makeUntextTable();
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
    fixAttributesTablePath(&opt_attributesTable);
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
    fixContractionTablePath(&opt_contractionTable);
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

  /* initialize screen driver */
  initializeAllScreens(opt_screenDriver, opt_libraryDirectory);
  screenParameters = processParameters(getScreenParameters(),
                                       "screen driver",
                                       getScreenDriverCode(),
                                       opt_screenParameters);
  logParameters(getScreenParameters(), screenParameters, "Screen");
  if (!opt_verify) {
    if (!openMainScreen(screenParameters)) {                                
      LogPrint(LOG_CRIT, "cannot read screen.");
      exit(7);
    }
    atexit(exitScreen);
  }
  
#if defined(HAVE_SYS_WAIT_H) || defined(WINDOWS)
  if (!(
#ifndef WINDOWS
        opt_noDaemon || 
#endif /* WINDOWS */
        opt_verify)) {
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

#ifdef WINDOWS
    {
      HANDLE h = CreateFile("NUL", GENERIC_READ|GENERIC_WRITE,
                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, 0, NULL);
      if (!h) {
        LogWindowsError("opening NUL device");
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
#endif /* WINDOWS */

#if defined(WINDOWS)
    {
      char name[] = BACKGROUND_EVENT_PREFIX "XXXXXXXX";
      HANDLE event;
      snprintf(name, sizeof(name), BACKGROUND_EVENT_PREFIX "%8lx", GetCurrentProcessId());
      if (!(event = CreateEvent(NULL, TRUE, FALSE, name))) {
        LogWindowsError("Creating event for background synchronization");
      } else {
        SetEvent(event);
      }
    }
#elif defined(HAVE_SYS_WAIT_H)
    /* tell the parent process to exit */
    if (kill(getppid(), SIGUSR1) == -1) {
      LogPrint(LOG_CRIT, "stop parent error: %s", strerror(errno));
      exit(12);
    }

    /* request a new session (job control) */
    if (setsid() == -1) {                        
      LogPrint(LOG_CRIT, "session creation error: %s", strerror(errno));
      exit(13);
    }
#endif /* detach */
  }
#endif /* HAVE_SYS_WAIT_H || WINDOWS */

  /*
   * From this point, all IO functions as printf, puts, perror, etc. can't be
   * used anymore since we are a daemon.  The LogPrint facility should 
   * be used instead.
   */

#ifdef ENABLE_API
  apiStarted = 0;
  if (!opt_noApi) {
    api_identify();
    apiParameters = processParameters(api_parameters,
                                      "application programming interface",
                                      NULL,
                                      opt_apiParameters);
    logParameters(api_parameters, apiParameters, "API");
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
    LogPrint(LOG_CRIT, "braille device not specified.");
    exit(4);
  }
  brailleDevices = splitString(opt_brailleDevice, ',', NULL);

  /* Activate the braille display. */
  brailleDrivers = splitString(opt_brailleDriver? opt_brailleDriver: "", ',', NULL);
  if (opt_verify) {
    if (openBrailleDriver(1)) closeBrailleDriver();
  } else {
    atexit(exitBrailleDriver);
    startBrailleDriver();
  }

#ifdef ENABLE_SPEECH_SUPPORT
  /* Activate the speech synthesizer. */
  speechDrivers = splitString(opt_speechDriver? opt_speechDriver: "", ',', NULL);
  if (opt_verify) {
    if (openSpeechDriver(1)) closeSpeechDriver();
  } else {
    atexit(exitSpeechDriver);
    startSpeechDriver();
  }

  /* Create the speech pass-through FIFO. */
  LogPrint(LOG_INFO, "Speech FIFO: %s",
           opt_speechFifo? opt_speechFifo: "none");
  if (!opt_verify) {
    if (opt_speechFifo) openSpeechFifo(opt_dataDirectory, opt_speechFifo);
  }
#endif /* ENABLE_SPEECH_SUPPORT */

  if (opt_verify) exit(0);

  if (!opt_quiet) {
    char buffer[18 + 1];
    snprintf(buffer, sizeof(buffer), "%s %s", PACKAGE_TITLE, PACKAGE_VERSION);
    message(buffer, 0);        /* display initialization message */
  }

  if (problemCount) {
    char buffer[0X40];
    snprintf(buffer, sizeof(buffer), "%d startup problem%s",
             problemCount,
             (problemCount==1? "": "s"));
    message(buffer, MSG_WAITKEY);
  }
}
