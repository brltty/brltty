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

#include "prefs.h"
#include "prefs_internal.h"
#include "statdefs.h"
#include "defaults.h"

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

PREFERENCE_STRING_TABLE(speechPunctuation,
  "none", "some", "all"
)

PREFERENCE_STRING_TABLE(uppercaseIndicator,
  "none", "cap", "higher"
)

PREFERENCE_STRING_TABLE(whitespaceIndicator,
  "none", "space"
)

PREFERENCE_STRING_TABLE(sayLineMode,
  "immediate", "enqueue"
)

PREFERENCE_STRING_TABLE(timeFormat,
  "24hour", "12hour"
)

PREFERENCE_STRING_TABLE(timeSeparator,
  "colon", "dot"
)

PREFERENCE_STRING_TABLE(datePosition,
  "no", "before", "after"
)

PREFERENCE_STRING_TABLE(dateFormat,
  "ymd", "mdy", "dmy"
)

PREFERENCE_STRING_TABLE(dateSeparator,
  "dash", "slash", "dot"
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

const PreferenceEntry preferenceTable[] = {
  { .name = "save-on-exit",
    .defaultValue = DEFAULT_SAVE_ON_EXIT,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.saveOnExit
  },

  { .name = "show-submenu-sizes",
    .defaultValue = DEFAULT_SHOW_SUBMENU_SIZES,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.showSubmenuSizes
  },

  { .name = "show-advanced-submenus",
    .defaultValue = DEFAULT_SHOW_ADVANCED_SUBMENUS,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.showAdvancedSubmenus
  },

  { .name = "show-all-items",
    .defaultValue = DEFAULT_SHOW_ALL_ITEMS,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.showAllItems
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

  { .name = "whitespace-indicator",
    .defaultValue = DEFAULT_WHITESPACE_INDICATOR,
    .settingNames = &preferenceStringTable_whitespaceIndicator,
    .setting = &prefs.whitespaceIndicator
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

  { .name = "autospeak-selected-line",
    .defaultValue = DEFAULT_AUTOSPEAK_SELECTED_LINE,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.autospeakSelectedLine
  },

  { .name = "autospeak-selected-character",
    .defaultValue = DEFAULT_AUTOSPEAK_SELECTED_CHARACTER,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.autospeakSelectedCharacter
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

  { .name = "time-format",
    .defaultValue = DEFAULT_TIME_FORMAT,
    .settingNames = &preferenceStringTable_timeFormat,
    .setting = &prefs.timeFormat
  },

  { .name = "time-separator",
    .defaultValue = DEFAULT_TIME_SEPARATOR,
    .settingNames = &preferenceStringTable_timeSeparator,
    .setting = &prefs.timeSeparator
  },

  { .name = "show-seconds",
    .defaultValue = DEFAULT_SHOW_SECONDS,
    .settingNames = &preferenceStringTable_boolean,
    .setting = &prefs.showSeconds
  },

  { .name = "date-position",
    .defaultValue = DEFAULT_DATE_POSITION,
    .settingNames = &preferenceStringTable_datePosition,
    .setting = &prefs.datePosition
  },

  { .name = "date-format",
    .defaultValue = DEFAULT_DATE_FORMAT,
    .settingNames = &preferenceStringTable_dateFormat,
    .setting = &prefs.dateFormat
  },

  { .name = "date-separator",
    .defaultValue = DEFAULT_DATE_SEPARATOR,
    .settingNames = &preferenceStringTable_dateSeparator,
    .setting = &prefs.dateSeparator
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

const unsigned char preferenceCount = ARRAY_COUNT(preferenceTable);
