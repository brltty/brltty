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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "prefs.h"
#include "prefs_internal.h"
#include "statdefs.h"
#include "defaults.h"
#include "log.h"
#include "file.h"
#include "parse.h"

#define PREFS_MAGIC_NUMBER 0x4005

Preferences prefs;                /* environment (i.e. global) parameters */
unsigned char statusFieldsSet;

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
