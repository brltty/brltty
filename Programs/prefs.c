/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "prefs.h"
#include "statdefs.h"
#include "defaults.h"
#include "log.h"
#include "file.h"
#include "parse.h"

#define PREFS_MAGIC_NUMBER 0x4005

Preferences prefs;                /* environment (i.e. global) parameters */
static unsigned char statusFieldsSet;

typedef struct {
  const char *const *table;
  unsigned char count;
} PreferenceStringTable;

#define PREFERENCE_STRING_TABLE(name, ...) \
static const char *const preferenceStringArray_##name[] = {__VA_ARGS__}; \
static const PreferenceStringTable preferenceStringTable_##name = { \
  .table = preferenceStringArray_##name, \
  .count = ARRAY_COUNT(preferenceStringArray_##name) \
};

PREFERENCE_STRING_TABLE(boolean,
  "no", "yes"
)

PREFERENCE_STRING_TABLE(textStyle,
  "8dot", "contracted", "6dot"
)

PREFERENCE_STRING_TABLE(capitalizationMode,
  "none", "sign", "dot7"
)

PREFERENCE_STRING_TABLE(skipBlankWindowsMode,
  "all", "end", "rest"
)

PREFERENCE_STRING_TABLE(cursorStyle,
  "underline", "block", "dot7", "dot8"
)

PREFERENCE_STRING_TABLE(brailleFirmness,
  "minimum", "low", "medium", "high", "maximum"
)

PREFERENCE_STRING_TABLE(brailleSensitivity,
  "minimum", "low", "medium", "high", "maximum"
)

PREFERENCE_STRING_TABLE(tuneDevice,
  "beeper", "pcm", "midi", "fm"
)

PREFERENCE_STRING_TABLE(sayLineMode,
  "immediate", "enqueue"
)

PREFERENCE_STRING_TABLE(uppercaseIndicator,
  "none", "cap", "higher"
)

PREFERENCE_STRING_TABLE(speechPunctuation,
  "none", "some", "all"
)

PREFERENCE_STRING_TABLE(statusPosition,
  "none", "left", "right"
)

PREFERENCE_STRING_TABLE(statusSeparator,
  "none", "space", "block", "status", "text"
)

PREFERENCE_STRING_TABLE(statusField,
  "end",
  "wxy", "wx", "wy",
  "cxy", "cx", "cy",
  "cwx", "cwy",
  "sn",
  "dots", "letter",
  "time",
  "wxya", "cxya",
  "generic"
)

typedef struct {
  const char *name;
  unsigned char *encountered;
  const PreferenceStringTable *settingNames;
  unsigned char defaultValue;
  unsigned char settingCount;
  unsigned char *setting;
} PreferenceEntry;

static const PreferenceEntry preferenceTable[] = {
  { .name = "save-on-exit",
    .defaultValue = DEFAULT_SAVE_ON_EXIT,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.saveOnExit
  },

  { .name = "show-all-items",
    .defaultValue = DEFAULT_SHOW_ALL_ITEMS,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.showAllItems
  },

  { .name = "show-submenu-sizes",
    .defaultValue = DEFAULT_SHOW_SUBMENU_SIZES,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.showSubmenuSizes
  },

  { .name = "text-style",
    .defaultValue = DEFAULT_TEXT_STYLE,
    .settingNames = &preferenceStringTable_textStyle,
    .setting = &prefs.textStyle
  },

  { .name = "expand-current-word",
    .defaultValue = DEFAULT_EXPAND_CURRENT_WORD,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.expandCurrentWord
  },

  { .name = "capitalization-mode",
    .defaultValue = DEFAULT_CAPITALIZATION_MODE,
    .settingNames = &preferenceStringTable_capitalizationMode,
    .setting = &prefs.capitalizationMode
  },

  { .name = "braille-firmness",
    .defaultValue = DEFAULT_BRAILLE_FIRMNESS,
    .settingNames = &preferenceStringTable_brailleFirmness,
    .setting = &prefs.brailleFirmness
  },

  { .name = "show-cursor",
    .defaultValue = DEFAULT_SHOW_CURSOR,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.showCursor
  },

  { .name = "cursor-style",
    .defaultValue = DEFAULT_CURSOR_STYLE,
    .settingNames = &preferenceStringTable_cursorStyle,
    .setting = &prefs.cursorStyle
  },

  { .name = "blinking-cursor",
    .defaultValue = DEFAULT_BLINKING_CURSOR,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.blinkingCursor
  },

  { .name = "cursor-visible-time",
    .defaultValue = DEFAULT_CURSOR_VISIBLE_TIME,
    .setting = &prefs.cursorVisibleTime
  },

  { .name = "cursor-invisible-time",
    .defaultValue = DEFAULT_CURSOR_INVISIBLE_TIME,
    .setting = &prefs.cursorInvisibleTime
  },

  { .name = "show-attributes",
    .defaultValue = DEFAULT_SHOW_ATTRIBUTES,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.showAttributes
  },

  { .name = "blinking-attributes",
    .defaultValue = DEFAULT_BLINKING_ATTRIBUTES,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.blinkingAttributes
  },

  { .name = "attributes-visible-time",
    .defaultValue = DEFAULT_ATTRIBUTES_VISIBLE_TIME,
    .setting = &prefs.attributesVisibleTime
  },

  { .name = "attributes-invisible-time",
    .defaultValue = DEFAULT_ATTRIBUTES_INVISIBLE_TIME,
    .setting = &prefs.attributesInvisibleTime
  },

  { .name = "blinking-capitals",
    .defaultValue = DEFAULT_BLINKING_CAPITALS,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.blinkingCapitals
  },

  { .name = "capitals-visible-time",
    .defaultValue = DEFAULT_CAPITALS_VISIBLE_TIME,
    .setting = &prefs.capitalsVisibleTime
  },

  { .name = "capitals-invisible-time",
    .defaultValue = DEFAULT_CAPITALS_INVISIBLE_TIME,
    .setting = &prefs.capitalsInvisibleTime
  },

  { .name = "skip-identical-lines",
    .defaultValue = DEFAULT_SKIP_IDENTICAL_LINES,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.skipIdenticalLines
  },

  { .name = "skip-blank-windows",
    .defaultValue = DEFAULT_SKIP_BLANK_WINDOWS,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.skipBlankWindows
  },

  { .name = "skip-blank-windows-mode",
    .defaultValue = DEFAULT_SKIP_BLANK_WINDOWS_MODE,
    .settingNames = &preferenceStringTable_skipBlankWindowsMode,
    .setting = &prefs.skipBlankWindowsMode
  },

  { .name = "sliding-window",
    .defaultValue = DEFAULT_SLIDING_WINDOW,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.slidingWindow
  },

  { .name = "eager-sliding-window",
    .defaultValue = DEFAULT_EAGER_SLIDING_WINDOW,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.eagerSlidingWindow
  },

  { .name = "window-overlap",
    .defaultValue = DEFAULT_WINDOW_OVERLAP,
    .setting = &prefs.windowOverlap
  },

  { .name = "window-follows-pointer",
    .defaultValue = DEFAULT_WINDOW_FOLLOWS_POINTER,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.windowFollowsPointer
  },

  { .name = "highlight-window",
    .defaultValue = DEFAULT_HIGHLIGHT_WINDOW,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.highlightWindow
  },

  { .name = "autorepeat",
    .defaultValue = DEFAULT_AUTOREPEAT,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.autorepeat
  },

  { .name = "autorepeat-panning",
    .defaultValue = DEFAULT_AUTOREPEAT_PANNING,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.autorepeatPanning
  },

  { .name = "autorepeat-delay",
    .defaultValue = DEFAULT_AUTOREPEAT_DELAY,
    .setting = &prefs.autorepeatDelay
  },

  { .name = "autorepeat-interval",
    .defaultValue = DEFAULT_AUTOREPEAT_INTERVAL,
    .setting = &prefs.autorepeatInterval
  },

  { .name = "braille-sensitivity",
    .defaultValue = DEFAULT_BRAILLE_SENSITIVITY,
    .settingNames = &preferenceStringTable_brailleSensitivity,
    .setting = &prefs.brailleSensitivity
  },

  { .name = "alert-tunes",
    .defaultValue = DEFAULT_ALERT_TUNES,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.alertTunes
  },

  { .name = "tune-device",
    .defaultValue = DEFAULT_TUNE_DEVICE,
    .settingNames = &preferenceStringTable_tuneDevice,
    .setting = &prefs.tuneDevice
  },

  { .name = "pcm-volume",
    .defaultValue = DEFAULT_PCM_VOLUME,
    .setting = &prefs.pcmVolume
  },

  { .name = "midi-volume",
    .defaultValue = DEFAULT_MIDI_VOLUME,
    .setting = &prefs.midiVolume
  },

  { .name = "midi-instrument",
    .defaultValue = DEFAULT_MIDI_INSTRUMENT,
    .setting = &prefs.midiInstrument
  },

  { .name = "fm-volume",
    .defaultValue = DEFAULT_FM_VOLUME,
    .setting = &prefs.fmVolume
  },

  { .name = "alert-dots",
    .defaultValue = DEFAULT_ALERT_DOTS,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.alertDots
  },

  { .name = "alert-messages",
    .defaultValue = DEFAULT_ALERT_MESSAGES,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.alertMessages
  },

  { .name = "speech-volume",
    .defaultValue = DEFAULT_SPEECH_VOLUME,
    .setting = &prefs.speechVolume
  },

  { .name = "speech-rate",
    .defaultValue = DEFAULT_SPEECH_RATE,
    .setting = &prefs.speechRate
  },

  { .name = "speech-pitch",
    .defaultValue = DEFAULT_SPEECH_PITCH,
    .setting = &prefs.speechPitch
  },

  { .name = "speech-punctuation",
    .defaultValue = DEFAULT_SPEECH_PUNCTUATION,
    .settingNames = &preferenceStringTable_speechPunctuation,
    .setting = &prefs.speechPunctuation
  },

  { .name = "uppercase-indicator",
    .defaultValue = DEFAULT_UPPERCASE_INDICATOR,
    .settingNames = &preferenceStringTable_uppercaseIndicator,
    .setting = &prefs.uppercaseIndicator
  },

  { .name = "say-line-mode",
    .defaultValue = DEFAULT_SAY_LINE_MODE,
    .settingNames = &preferenceStringTable_sayLineMode,
    .setting = &prefs.sayLineMode
  },

  { .name = "autospeak",
    .defaultValue = DEFAULT_AUTOSPEAK,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.autospeak
  },

  { .name = "autospeak-current-character",
    .defaultValue = DEFAULT_AUTOSPEAK_CURRENT_CHARACTER,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.autospeakCurrentCharacter
  },

  { .name = "autospeak-inserted-characters",
    .defaultValue = DEFAULT_AUTOSPEAK_INSERTED_CHARACTERS,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.autospeakInsertedCharacters
  },

  { .name = "autospeak-deleted-characters",
    .defaultValue = DEFAULT_AUTOSPEAK_DELETED_CHARACTERS,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.autospeakDeletedCharacters
  },

  { .name = "autospeak-replaced-characters",
    .defaultValue = DEFAULT_AUTOSPEAK_REPLACED_CHARACTERS,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.autospeakReplacedCharacters
  },

  { .name = "autospeak-completed-words",
    .defaultValue = DEFAULT_AUTOSPEAK_COMPLETED_WORDS,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.autospeakCompletedWords
  },

  { .name = "autospeak-white-space",
    .defaultValue = DEFAULT_AUTOSPEAK_WHITE_SPACE,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.autospeakWhiteSpace
  },

  { .name = "show-speech-cursor",
    .defaultValue = DEFAULT_SHOW_SPEECH_CURSOR,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.showSpeechCursor
  },

  { .name = "speech-cursor-style",
    .defaultValue = DEFAULT_SPEECH_CURSOR_STYLE,
    .settingNames = &preferenceStringTable_cursorStyle,
    .setting = &prefs.speechCursorStyle
  },

  { .name = "blinking-speech-cursor",
    .defaultValue = DEFAULT_BLINKING_SPEECH_CURSOR,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.blinkingSpeechCursor
  },

  { .name = "speech-cursor-visible-time",
    .defaultValue = DEFAULT_SPEECH_CURSOR_VISIBLE_TIME,
    .setting = &prefs.speechCursorVisibleTime
  },

  { .name = "speech-cursor-invisible-time",
    .defaultValue = DEFAULT_SPEECH_CURSOR_INVISIBLE_TIME,
    .setting = &prefs.speechCursorInvisibleTime
  },

  { .name = "status-position",
    .defaultValue = DEFAULT_STATUS_POSITION,
    .settingNames = &preferenceStringTable_statusPosition,
    .setting = &prefs.statusPosition
  },

  { .name = "status-count",
    .defaultValue = DEFAULT_STATUS_COUNT,
    .setting = &prefs.statusCount
  },

  { .name = "status-separator",
    .defaultValue = DEFAULT_STATUS_SEPARATOR,
    .settingNames = &preferenceStringTable_statusSeparator,
    .setting = &prefs.statusSeparator
  },

  { .name = "status-fields",
    .defaultValue = sfEnd,
    .encountered = &statusFieldsSet,
    .settingNames = &preferenceStringTable_statusField,
    .settingCount = ARRAY_COUNT(prefs.statusFields),
    .setting = prefs.statusFields
  }
};
static const unsigned char preferenceCount = ARRAY_COUNT(preferenceTable);

void
setStatusFields (const unsigned char *fields) {
  if (!statusFieldsSet) {
    if (fields) {
      unsigned int index = 0;

      while (index < (ARRAY_COUNT(prefs.statusFields) - 1)) {
        unsigned char field = fields[index];
        if (field == sfEnd) break;
        prefs.statusFields[index++] = field;
      }

      statusFieldsSet = 1;
    }
  }
}

static void
resetPreference (const PreferenceEntry *pref) {
  if (pref->settingCount) {
    memset(pref->setting, pref->defaultValue, pref->settingCount);
  } else {
    *pref->setting = pref->defaultValue;
  }

  if (pref->encountered) *pref->encountered = 0;
}

void
resetPreferences (void) {
  memset(&prefs, 0, sizeof(prefs));

  prefs.magic[0] = PREFS_MAGIC_NUMBER & 0XFF;
  prefs.magic[1] = PREFS_MAGIC_NUMBER >> 8;
  prefs.version = 6;

  {
    const PreferenceEntry *pref = preferenceTable;
    const PreferenceEntry *end = pref + preferenceCount;

    while (pref < end) resetPreference(pref++);
  }
}

char *
makePreferencesFilePath (const char *name) {
  if (!name) name = PREFERENCES_FILE;
  return makePath(STATE_DIRECTORY, name);
}

static int
comparePreferenceNames (const char *name1, const char *name2) {
  return strcmp(name1, name2);
}

static int
sortPreferencesByName (const void *element1, const void *element2) {
  const PreferenceEntry *const *pref1 = element1;
  const PreferenceEntry *const *pref2 = element2;
  return comparePreferenceNames((*pref1)->name, (*pref2)->name);
}

static int
searchPreferenceByName (const void *target, const void *element) {
  const char *name = target;
  const PreferenceEntry *const *pref = element;
  return comparePreferenceNames(name, (*pref)->name);
}

static const PreferenceEntry *
findPreferenceEntry (const char *name) {
  static const PreferenceEntry **sortedEntries = NULL;

  if (!sortedEntries) {
    if (!(sortedEntries = malloc(preferenceCount * sizeof(*sortedEntries)))) {
      logMallocError();
      return NULL;
    }

    {
      unsigned int index;

      for (index=0; index<preferenceCount; index+=1) {
        sortedEntries[index] = &preferenceTable[index];
      }
    }

    qsort(sortedEntries, preferenceCount, sizeof(*sortedEntries), sortPreferencesByName);
  }

  {
    const PreferenceEntry *const *pref = bsearch(name, sortedEntries, preferenceCount, sizeof(*sortedEntries), searchPreferenceByName);
    return pref? *pref: NULL;
  }
}

static int
getPreferenceSetting (
  const char *name, const char *operand,
  unsigned char *setting, const PreferenceStringTable *names
) {
  int value;

  if (isInteger(&value, operand)) {
    if ((value >= 0) && (value <= 0XFF)) {
      *setting = value;
      return 1;
    }
  } else {
    for (value=0; value<names->count; value+=1) {
      if (strcmp(operand, names->table[value]) == 0) {
        *setting = value;
        return 1;
      }
    }
  }

  logMessage(LOG_WARNING, "invalid preference setting: %s %s", name, operand);
  return 0;
}

static int
processPreferenceLine (char *line, void *data) {
  static const char delimiters[] = " \t";
  const char *name = strtok(line, delimiters);

  if (name) {
    const PreferenceEntry *pref = findPreferenceEntry(name);

    if (pref) {
      const char *operand;

      if (pref->encountered) *pref->encountered = 1;

      if (pref->settingCount) {
        unsigned char count = pref->settingCount;
        unsigned char *setting = pref->setting;

        while (count) {
          if ((operand = strtok(NULL, delimiters))) {
            if (getPreferenceSetting(name, operand, setting, pref->settingNames)) {
              setting += 1;
              count -= 1;
              continue;
            }
          }

          *setting = 0;
          break;
        }
      } else if (!(operand = strtok(NULL, delimiters))) {
        logMessage(LOG_WARNING, "missing preference setting: %s", name);
      } else if (!getPreferenceSetting(name, operand, pref->setting, pref->settingNames)) {
      }
    } else {
      logMessage(LOG_WARNING, "unknown preference: %s", name);
    }
  }

  return 1;
}

static void
setStatusStyle (unsigned char style) {
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

  if (style < styleCount) {
    const unsigned char *fields = styleTable[style];
    if (*fields != sfEnd) setStatusFields(fields);
  }
}

int
loadPreferencesFile (const char *path) {
  int ok = 0;
  FILE *file = openDataFile(path, "rb", 1);

  if (file) {
    Preferences newPreferences;
    size_t length = fread(&newPreferences, 1, sizeof(newPreferences), file);

    if (ferror(file)) {
      logMessage(LOG_ERR, "%s: %s: %s",
                 gettext("cannot read preferences file"), path, strerror(errno));
    } else if ((length < 40) ||
               (newPreferences.magic[0] != (PREFS_MAGIC_NUMBER & 0XFF)) ||
               (newPreferences.magic[1] != (PREFS_MAGIC_NUMBER >> 8))) {
      fclose(file);

      if ((file = openDataFile(path, "r", 1))) {
        resetPreferences();
        if (processLines(file, processPreferenceLine, NULL)) ok = 1;
      }
    } else {
      prefs = newPreferences;
      ok = 1;

      {
        const PreferenceEntry *pref = preferenceTable;
        const PreferenceEntry *end = pref + preferenceCount;

        const unsigned char *from = prefs.magic;
        const unsigned char *to = from + length;

        while (pref < end) {
          unsigned char count = pref->settingCount;
          if (!count) count = 1;

          if ((pref->setting < from) || ((pref->setting + count) > to)) {
            resetPreference(pref);
          }

          pref += 1;
        }
      }

      if (length < (prefs.statusFields + sizeof(prefs.statusFields) - prefs.magic)) {
        setStatusStyle(prefs.expandCurrentWord);
      } else {
        statusFieldsSet = 1;
      }

      if (prefs.version == 0) {
        prefs.version += 1;
        prefs.pcmVolume = DEFAULT_PCM_VOLUME;
        prefs.midiVolume = DEFAULT_MIDI_VOLUME;
        prefs.fmVolume = DEFAULT_FM_VOLUME;
      }

      if (prefs.version == 1) {
        prefs.version += 1;
        prefs.sayLineMode = DEFAULT_SAY_LINE_MODE;
        prefs.autospeak = DEFAULT_AUTOSPEAK;
      }

      if (prefs.version == 2) {
        prefs.version += 1;
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

      if (prefs.version == 3) {
        prefs.version += 1;
        prefs.autorepeatPanning = DEFAULT_AUTOREPEAT_PANNING;
      }

      if (prefs.version == 4) {
        prefs.version += 1;
        prefs.brailleSensitivity = DEFAULT_BRAILLE_SENSITIVITY;
      }

      if (prefs.version == 5) {
        prefs.version += 1;
        prefs.expandCurrentWord = DEFAULT_EXPAND_CURRENT_WORD;
      }
    }

    if (file) {
      fclose(file);
      file = NULL;
    }
  }

  return ok;
}

static int
putPreferenceSetting (FILE *file, unsigned char setting, const PreferenceStringTable *names) {
  if (fputc(' ', file) == EOF) return 0;

  if (names && (setting < names->count)) {
    if (fputs(names->table[setting], file) == EOF) return 0;
  } else {
    if (fprintf(file, "%u", setting) < 0) return 0;
  }

  return 1;
}

int
savePreferencesFile (const char *path) {
  int ok = 0;
  FILE *file = openDataFile(path, "w", 0);

  if (file) {
    const PreferenceEntry *pref = preferenceTable;
    const PreferenceEntry *const end = pref + preferenceCount;

    while (pref < end) {

      if (fputs(pref->name, file) == EOF) break;

      if (pref->settingCount) {
        unsigned char count = pref->settingCount;
        unsigned char *setting = pref->setting;

        while (count-- && *setting) {
          if (!putPreferenceSetting(file, *setting++, pref->settingNames)) goto done;
        }
      } else if (!putPreferenceSetting(file, *pref->setting, pref->settingNames)) {
        break;
      }

      if (fputc('\n', file) == EOF) break;

      pref += 1;
    }
    if (pref == end) ok = 1;

  done:
    if (!ok) {
      if (!ferror(file)) errno = EIO;
      logMessage(LOG_ERR, "%s: %s: %s",
                 gettext("cannot write to preferences file"), path, strerror(errno));
    }

    fclose(file);
  }

  return ok;
}
