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

Preferences prefs;                /* environment (i.e. global) parameters */
static unsigned char statusFieldsSet;

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
  prefs.blankWindowsSkipMode = DEFAULT_BLANK_WINDOWS_SKIP_MODE;

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

int
loadPreferencesFile (const char *preferencesFile) {
  int ok = 0;
  FILE *file = openDataFile(preferencesFile, "rb", 1);

  if (file) {
    Preferences newPreferences;
    size_t length = fread(&newPreferences, 1, sizeof(newPreferences), file);

    if (ferror(file)) {
      logMessage(LOG_ERR, "%s: %s: %s",
                 gettext("cannot read preferences file"), preferencesFile, strerror(errno));
    } else if ((length < 40) ||
               (newPreferences.magic[0] != (PREFS_MAGIC_NUMBER & 0XFF)) ||
               (newPreferences.magic[1] != (PREFS_MAGIC_NUMBER >> 8))) {
      logMessage(LOG_ERR, "%s: %s", gettext("invalid preferences file"), preferencesFile);
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
