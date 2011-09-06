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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "prefs.h"
#include "statdefs.h"
#include "defaults.h"
#include "system.h"
#include "log.h"
#include "file.h"
#include "parse.h"

#define PREFS_TEXTUAL
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

PREFERENCE_STRING_TABLE(skipBlankWindowsMode,
  "all", "end", "rest"
)

PREFERENCE_STRING_TABLE(cursorStyle,
  "underline", "block"
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
  unsigned char settingCount;
  unsigned char *setting;
} PreferenceEntry;

static const PreferenceEntry preferenceTable[] = {
  { .name = "text-style",
    .settingNames = &preferenceStringTable_textStyle,
    .setting = &prefs.textStyle
  }
  ,
  { .name = "expand-current-word",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.expandCurrentWord
  }
  ,
  { .name = "skip-identical-lines",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.skipIdenticalLines
  }
  ,
  { .name = "skip-blank-windows",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.skipBlankWindows
  }
  ,
  { .name = "skip-blank-windows-mode",
    .settingNames = &preferenceStringTable_skipBlankWindowsMode,
    .setting = &prefs.skipBlankWindowsMode
  }
  ,
  { .name = "sliding-window",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.slidingWindow
  }
  ,
  { .name = "eager-sliding-window",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.eagerSlidingWindow
  }
  ,
  { .name = "window-overlap",
    .setting = &prefs.windowOverlap
  }
  ,
  { .name = "autorepeat",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.autorepeat
  }
  ,
  { .name = "autorepeat-panning",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.autorepeatPanning
  }
  ,
  { .name = "autorepeat-delay",
    .setting = &prefs.autorepeatDelay
  }
  ,
  { .name = "autorepeat-interval",
    .setting = &prefs.autorepeatInterval
  }
  ,
  { .name = "show-cursor",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.showCursor
  }
  ,
  { .name = "cursor-style",
    .settingNames = &preferenceStringTable_cursorStyle,
    .setting = &prefs.cursorStyle
  }
  ,
  { .name = "blinking-cursor",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.blinkingCursor
  }
  ,
  { .name = "cursor-visible-time",
    .setting = &prefs.cursorVisibleTime
  }
  ,
  { .name = "cursor-invisible-time",
    .setting = &prefs.cursorInvisibleTime
  }
  ,
  { .name = "show-attributes",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.showAttributes
  }
  ,
  { .name = "blinking-attributes",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.blinkingAttributes
  }
  ,
  { .name = "attributes-visible-time",
    .setting = &prefs.attributesVisibleTime
  }
  ,
  { .name = "attributes-invisible-time",
    .setting = &prefs.attributesInvisibleTime
  }
  ,
  { .name = "blinking-capitals",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.blinkingCapitals
  }
  ,
  { .name = "capitals-visible-time",
    .setting = &prefs.capitalsVisibleTime
  }
  ,
  { .name = "capitals-invisible-time",
    .setting = &prefs.capitalsInvisibleTime
  }
  ,
  { .name = "braille-firmness",
    .settingNames = &preferenceStringTable_brailleFirmness,
    .setting = &prefs.brailleFirmness
  }
  ,
  { .name = "braille-sensitivity",
    .settingNames = &preferenceStringTable_brailleSensitivity,
    .setting = &prefs.brailleSensitivity
  }
  ,
  { .name = "window-follows-pointer",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.windowFollowsPointer
  }
  ,
  { .name = "highlight-window",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.highlightWindow
  }
  ,
  { .name = "alert-tunes",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.alertTunes
  }
  ,
  { .name = "tune-device",
    .settingNames = &preferenceStringTable_tuneDevice,
    .setting = &prefs.tuneDevice
  }
  ,
  { .name = "pcm-volume",
    .setting = &prefs.pcmVolume
  }
  ,
  { .name = "midi-volume",
    .setting = &prefs.midiVolume
  }
  ,
  { .name = "midi-instrument",
    .setting = &prefs.midiInstrument
  }
  ,
  { .name = "fm-volume",
    .setting = &prefs.fmVolume
  }
  ,
  { .name = "alert-dots",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.alertDots
  }
  ,
  { .name = "alert-messages",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.alertMessages
  }
  ,
  { .name = "say-line-mode",
    .settingNames = &preferenceStringTable_sayLineMode,
    .setting = &prefs.sayLineMode
  }
  ,
  { .name = "autospeak",
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.autospeak
  }
  ,
  { .name = "speech-volume",
    .setting = &prefs.speechVolume
  }
  ,
  { .name = "speech-rate",
    .setting = &prefs.speechRate
  }
  ,
  { .name = "speech-pitch",
    .setting = &prefs.speechPitch
  }
  ,
  { .name = "speech-punctuation",
    .settingNames = &preferenceStringTable_speechPunctuation,
    .setting = &prefs.speechPunctuation
  }
  ,
  { .name = "status-position",
    .settingNames = &preferenceStringTable_statusPosition,
    .setting = &prefs.statusPosition
  }
  ,
  { .name = "status-count",
    .setting = &prefs.statusCount
  }
  ,
  { .name = "status-separator",
    .settingNames = &preferenceStringTable_statusSeparator,
    .setting = &prefs.statusSeparator
  }
  ,
  { .name = "status-fields",
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
resetStatusPreferences (void) {
  prefs.statusPosition = spNone;
  prefs.statusCount = 0;
  prefs.statusSeparator = ssNone;

  memset(prefs.statusFields, sfEnd, sizeof(prefs.statusFields));
  statusFieldsSet = 0;
}

void
resetPreferences (void) {
  memset(&prefs, 0, sizeof(prefs));

  prefs.magic[0] = PREFS_MAGIC_NUMBER & 0XFF;
  prefs.magic[1] = PREFS_MAGIC_NUMBER >> 8;
  prefs.version = 6;

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
  prefs.expandCurrentWord = DEFAULT_EXPAND_CURRENT_WORD;
  prefs.brailleFirmness = DEFAULT_BRAILLE_FIRMNESS;
  prefs.brailleSensitivity = DEFAULT_BRAILLE_SENSITIVITY;

  prefs.windowOverlap = DEFAULT_WINDOW_OVERLAP;
  prefs.slidingWindow = DEFAULT_SLIDING_WINDOW;
  prefs.eagerSlidingWindow = DEFAULT_EAGER_SLIDING_WINDOW;

  prefs.skipIdenticalLines = DEFAULT_SKIP_IDENTICAL_LINES;
  prefs.skipBlankWindows = DEFAULT_SKIP_BLANK_WINDOWS;
  prefs.skipBlankWindowsMode = DEFAULT_SKIP_BLANK_WINDOWS_MODE;

  prefs.alertMessages = DEFAULT_ALERT_MESSAGES;
  prefs.alertDots = DEFAULT_ALERT_DOTS;
  prefs.alertTunes = DEFAULT_ALERT_TUNES;

  prefs.tuneDevice = DEFAULT_TUNE_DEVICE;
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
  resetStatusPreferences();
}

char *
makePreferencesFilePath (const char *name) {
  if (!name) name = PREFERENCES_FILE;
  return makePath(STATE_DIRECTORY, name);
}

#ifdef PREFS_TEXTUAL
static int
sortPreferences (const void *element1, const void *element2) {
  const PreferenceEntry *const *pref1 = element1;
  const PreferenceEntry *const *pref2 = element2;
  return strcmp((*pref1)->name, (*pref2)->name);
}

static int
searchPreference (const void *target, const void *element) {
  const char *name = target;
  const PreferenceEntry *const *pref = element;
  return strcmp(name, (*pref)->name);
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

    qsort(sortedEntries, preferenceCount, sizeof(*sortedEntries), sortPreferences);
  }

  {
    const PreferenceEntry *const *pref = bsearch(name, sortedEntries, preferenceCount, sizeof(*sortedEntries), searchPreference);
    return pref? *pref: NULL;
  }
}

static int
getPreferenceSetting (const char *delimiters, unsigned char *setting, const PreferenceStringTable *names) {
  const char *operand = strtok(NULL, delimiters);

  if (operand) {
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

    logMessage(LOG_WARNING, "invalid preference setting: %s", operand);
  }

  return 0;
}

static int
processPreferenceLine (char *line, void *data) {
  static const char delimiters[] = " \t";
  const char *name = strtok(line, delimiters);

  if (name) {
    const PreferenceEntry *pref = findPreferenceEntry(name);

    if (pref) {
      if (pref->encountered) *pref->encountered = 1;

      if (pref->settingCount) {
        unsigned char count = pref->settingCount;
        unsigned char *setting = pref->setting;

        while (count) {
          if (!getPreferenceSetting(delimiters, setting, pref->settingNames)) {
            *setting = 0;
            break;
          }

          setting += 1;
          count -= 1;
        }
      } else if (!getPreferenceSetting(delimiters, pref->setting, pref->settingNames)) {
        logMessage(LOG_WARNING, "missing preference setting: %s", name);
      }
    } else {
      logMessage(LOG_WARNING, "unknown preference: %s", name);
    }
  }

  return 1;
}
#endif /* PREFS_TEXTUAL */

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
#ifdef PREFS_TEXTUAL
      rewind(file);
      resetPreferences();

      if (processLines(file, processPreferenceLine, NULL)) {
        ok = 1;
      }
#else /* PREFS_TEXTUAL */
      logMessage(LOG_ERR, "%s: %s", gettext("invalid preferences file"), path);
#endif /* PREFS_TEXTUAL */
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
        resetStatusPreferences();
        setStatusFields(fields);
      } else {
        statusFieldsSet = 1;
      }

      if (prefs.version == 5) {
        prefs.version++;
        prefs.expandCurrentWord = DEFAULT_EXPAND_CURRENT_WORD;
      }
    }

    fclose(file);
  }

  return ok;
}

#ifdef PREFS_TEXTUAL
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
#endif /* PREFS_TEXTUAL */

int
savePreferencesFile (const char *path) {
  int ok = 0;
  FILE *file = openDataFile(path, "w+b", 0);

  if (file) {
#ifdef PREFS_TEXTUAL
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
#else /* PREFS_TEXTUAL */
    size_t length = fwrite(&prefs, 1, sizeof(prefs), file);
    if (length == sizeof(prefs)) ok = 1;
#endif /* PREFS_TEXTUAL */

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
