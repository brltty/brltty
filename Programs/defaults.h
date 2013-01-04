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

#ifndef BRLTTY_INCLUDED_DEFAULTS
#define BRLTTY_INCLUDED_DEFAULTS

#include "ctbdefs.h"
#include "brldefs.h"
#include "spkdefs.h"
#include "tunedefs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Edit as necessary for your system. */

/* Delay times, measured in milliseconds. */
#define DEFAULT_UPDATE_INTERVAL 40	/* sleep time per cycle - overall speed */

#define DEFAULT_MESSAGE_DELAY 4000 /* message hold time */
/* Under 5 seconds (init's SIGTERM-SIGKILL delay during shutdown)
 * is good as that allows "exiting" to be replaced by "terminated"
 * on the display.
 */

#define DEFAULT_TRACK_CURSOR 1		/* 1 for on, 0 for off */
#define DEFAULT_HIDE_CURSOR 0		/* 1 for yes, 0 for no */

#define DEFAULT_SAVE_ON_EXIT 0
#define DEFAULT_SHOW_SUBMENU_SIZES 0
#define DEFAULT_SHOW_ADVANCED_SUBMENUS 0
#define DEFAULT_SHOW_ALL_ITEMS 0

#define DEFAULT_TEXT_STYLE tsComputerBraille8
#define DEFAULT_EXPAND_CURRENT_WORD 1
#define DEFAULT_CAPITALIZATION_MODE CTB_CAP_SIGN
#define DEFAULT_BRAILLE_FIRMNESS BRL_FIRMNESS_MEDIUM

#define DEFAULT_SHOW_CURSOR 1		/* 1 for yes, 0 for no */
#define DEFAULT_CURSOR_STYLE csUnderline
#define DEFAULT_BLINKING_CURSOR 0		/* 1 for on, 0 for off */
#define DEFAULT_CURSOR_VISIBLE_TIME 40	/* for blinking cursor */
#define DEFAULT_CURSOR_INVISIBLE_TIME 40

#define DEFAULT_SHOW_ATTRIBUTES 0          /* 1 for on, 0 for off */
#define DEFAULT_BLINKING_ATTRIBUTES 1        /* 1 for on, 0 for off */
#define DEFAULT_ATTRIBUTES_VISIBLE_TIME 20      /* for attribute underlining */
#define DEFAULT_ATTRIBUTES_INVISIBLE_TIME 60

#define DEFAULT_BLINKING_CAPITALS 0		/* 1 for on, 0 for off */
#define DEFAULT_CAPITALS_VISIBLE_TIME 60	/* for blinking caps */
#define DEFAULT_CAPITALS_INVISIBLE_TIME 20

#define DEFAULT_SKIP_IDENTICAL_LINES 0		/* 1 = skip all identical lines after first */
#define DEFAULT_SKIP_BLANK_WINDOWS 0       /* 1 = skip blank windows */
#define DEFAULT_SKIP_BLANK_WINDOWS_MODE sbwEndOfLine
#define DEFAULT_SLIDING_WINDOW 0		/* 1 for on, 0 for off */
#define DEFAULT_EAGER_SLIDING_WINDOW 0
#define DEFAULT_WINDOW_OVERLAP 0

#define DEFAULT_WINDOW_FOLLOWS_POINTER 0		/* 1 for on, 0 for off */
#define DEFAULT_HIGHLIGHT_WINDOW 0		/* 1 for on, 0 for off */

#define DEFAULT_AUTOREPEAT 1		/* 1 for on, 0 for off */
#define DEFAULT_AUTOREPEAT_PANNING 0	/* 1 for on, 0 for off */
#define DEFAULT_AUTOREPEAT_DELAY 50	/* hundredths of a second */
#define DEFAULT_AUTOREPEAT_INTERVAL 10	/* hundredths of a second */

#define DEFAULT_BRAILLE_SENSITIVITY BRL_SENSITIVITY_MEDIUM

#define DEFAULT_ALERT_TUNES 1		/* 1 for on, 0 for off */
#define DEFAULT_TUNE_DEVICE tdBeeper
#define DEFAULT_PCM_VOLUME 70		/* 0 to 100 (percent) */
#define DEFAULT_MIDI_VOLUME 70		/* 0 to 100 (percent) */
#define DEFAULT_MIDI_INSTRUMENT 0	/* 0 to 127 */
#define DEFAULT_FM_VOLUME 70		/* 0 to 100 (percent) */

#define DEFAULT_ALERT_DOTS 0		/* 1 for on, 0 for off */
#define DEFAULT_ALERT_MESSAGES 0		/* 1 for on, 0 for off */

#define DEFAULT_SPEECH_VOLUME SPK_VOLUME_DEFAULT
#define DEFAULT_SPEECH_RATE SPK_RATE_DEFAULT
#define DEFAULT_SPEECH_PITCH SPK_PITCH_DEFAULT
#define DEFAULT_SPEECH_PUNCTUATION SPK_PUNCTUATION_SOME

#define DEFAULT_UPPERCASE_INDICATOR ucNone
#define DEFAULT_WHITESPACE_INDICATOR wsNone
#define DEFAULT_SAY_LINE_MODE sayImmediate

#define DEFAULT_AUTOSPEAK 0		/* 1 for on, 0 for off */
#define DEFAULT_AUTOSPEAK_SELECTED_LINE 1
#define DEFAULT_AUTOSPEAK_SELECTED_CHARACTER 1
#define DEFAULT_AUTOSPEAK_INSERTED_CHARACTERS 1
#define DEFAULT_AUTOSPEAK_DELETED_CHARACTERS 1
#define DEFAULT_AUTOSPEAK_REPLACED_CHARACTERS 1
#define DEFAULT_AUTOSPEAK_COMPLETED_WORDS 1

#define DEFAULT_SHOW_SPEECH_CURSOR 0
#define DEFAULT_SPEECH_CURSOR_STYLE csLowerRightDot
#define DEFAULT_BLINKING_SPEECH_CURSOR 0
#define DEFAULT_SPEECH_CURSOR_VISIBLE_TIME 50
#define DEFAULT_SPEECH_CURSOR_INVISIBLE_TIME 30

#define DEFAULT_TIME_FORMAT tf24Hour
#define DEFAULT_TIME_SEPARATOR tsColon
#define DEFAULT_SHOW_SECONDS 1
#define DEFAULT_DATE_POSITION dpNone
#define DEFAULT_DATE_FORMAT dfYearMonthDay
#define DEFAULT_DATE_SEPARATOR dsDash

#define DEFAULT_STATUS_POSITION spNone
#define DEFAULT_STATUS_COUNT 0
#define DEFAULT_STATUS_SEPARATOR ssNone

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_DEFAULTS */
