/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

#ifndef _DEFAULTS_H
#define _DEFAULTS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* config.h - Configurable definitions
 *
 * Edit as necessary for your system.
 */

/* Delay times, measured in milliseconds.
 * Note that I found a large error in timing - 40 is nearer 50 ms.
 */
#define DEFAULT_REFRESH_INTERVAL 40	/* default sleep time per cycle - overall speed */
#define DEFAULT_MESSAGE_DELAY 4000 /* Default time duration to display messages.
                            * Under 5 seconds (init's SIGTERM-SIGKILL delay
                            * during shutdown) is good as that allows "exiting" 
                            * to be replaced by "terminated" on the display.
                            */

/* Initialisation of BRLTTY environment settings: */

#define DEFAULT_SHOW_CURSOR 1		/* 1 for cursor display, 0 for no cursor */
#define DEFAULT_CURSOR_STYLE 0		/* 1 for block, 0 for underline */
#define DEFAULT_TRACK_CURSOR 1		/* 1 for cursor tracking on, 0 for off */
#define DEFAULT_HIDE_CURSOR 0		/* 1 for hiding cursor, 0 for showing it */
#define DEFAULT_BLINKING_CURSOR 0		/* 1 for cursor blink on, 0 for off */
#define DEFAULT_CURSOR_VISIBLE_PERIOD 10	/* for blinking cursor */
#define DEFAULT_CURSOR_INVISIBLE_PERIOD 10

#define DEFAULT_SHOW_ATTRIBUTES 0          /* 1 for attribute underlining */
#define DEFAULT_BLINKING_ATTRIBUTES 1        /* 1 for attr underlining that blinks */
#define DEFAULT_ATTRIBUTES_VISIBLE_PERIOD 4      /* for attribute underlining */
#define DEFAULT_ATTRIBUTES_INVISIBLE_PERIOD 12

#define DEFAULT_BLINKING_CAPITALS 0		/* 1 for capital blink on, 0 for off */
#define DEFAULT_CAPITALS_VISIBLE_PERIOD 4	/* for blinking caps */
#define DEFAULT_CAPITALS_INVISIBLE_PERIOD 2

#define DEFAULT_WINDOW_FOLLOWS_POINTER 0		/* 1 for capital blink on, 0 for off */
#define DEFAULT_POINTER_FOLLOWS_WINDOW 0		/* 1 for capital blink on, 0 for off */

#define DEFAULT_TEXT_STYLE 0		/* 1 for six-dot mode, 0 for eight-dot */
#define DEFAULT_META_MODE 0		/* 1 for six-dot mode, 0 for eight-dot */

#define DEFAULT_WINDOW_OVERLAP 0
#define DEFAULT_SLIDING_WINDOW 0		/* 1 for sliding window on, 0 for off */
#define DEFAULT_EAGER_SLIDING_WINDOW 0

#define DEFAULT_SKIP_IDENTICAL_LINES 0		/* 1 = skip all identical lines after first */
#define DEFAULT_SKIP_BLANK_WINDOWS 0       /* 1 = skip blank windows */
#define DEFAULT_BLANK_WINDOWS_SKIP_MODE sbwEndOfLine

#define DEFAULT_ALERT_MESSAGES 0		/* 1 for on, 0 for off */
#define DEFAULT_ALERT_DOTS 0		/* 1 for on, 0 for off */
#define DEFAULT_ALERT_TUNES 1		/* 1 for on, 0 for off */
#define DEFAULT_PCM_VOLUME 70		/* 0 to 100 (percent) */
#define DEFAULT_MIDI_VOLUME 70		/* 0 to 100 (percent) */
#define DEFAULT_MIDI_INSTRUMENT 0	/* 0 to 127 */
#define DEFAULT_FM_VOLUME 70		/* 0 to 100 (percent) */

#define DEFAULT_SAY_LINE_ACTION slaMute		/* 0 to 100 (percent) */
#define DEFAULT_AUTOSPEAK 0		/* 0 to 100 (percent) */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _DEFAULTS_H */
