/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

/* config.h - Configurable definitions
 *
 * Edit as necessary for your system.
 */

/* ------------------------ OVERALL CONFIGURATIONS ------------------------ */

#define CONFIG_FILE "/etc/brltty.conf"

/* Delay times, measured in milliseconds.
 * Note that I found a large error in timing - 40 is nearer 50 ms.
 */
#define DELAY_TIME 40		/* sleep time per cycle - overall speed */
#define KEYDEL 40		/* sleep time when awaiting a keypress */
#define DISPDEL 10000		/* time duration to display messages */


/* Character cursor initialisation: */
#define BIG_CSRCHAR 0xff	/* block cursor */
#define SMALL_CSRCHAR 0xc0	/* underline cursor */

/* Dot super-imposition for attribute underlining */
#define ATTR1CHAR 0xC0
#define ATTR2CHAR 0x80

typedef enum {
   sbwAll,
   sbwEndOfLine,
   sbwRestOfLine
} SkipBlankWindowsMode;

typedef enum {
   tdSpeaker,
   tdSoundCard,
   tdSequencer,
   tdAdLib
} TuneDevice;

/* Initialisation of BRLTTY environment settings: */
#define INIT_CSRVIS 1		/* 1 for cursor display, 0 for no cursor */
#define INIT_CSRTRK 1		/* 1 for cursor tracking on, 0 for off */
#define INIT_CSRHIDE 0		/* 1 for hiding cursor, 0 for showing it */
#define INIT_CSRBLINK 0		/* 1 for cursor blink on, 0 for off */
#define INIT_CAPBLINK 0		/* 1 for capital blink on, 0 for off */
#define INIT_ATTRVIS 0          /* 1 for attribute underlining */
#define INIT_ATTRBLINK 1        /* 1 for attr underlining that blinks */
#define INIT_CSRSIZE 0		/* 1 for block, 0 for underline */
#define INIT_SIXDOTS 0		/* 1 for six-dot mode, 0 for eight-dot */
#define INIT_SLIDEWIN 0		/* 1 for sliding window on, 0 for off */
#define INIT_SOUND 1		/* 1 for sound, 0 for silence */
#define INIT_TUNEDEV tdSpeaker
#define INIT_SKPIDLNS 0		/* 1 = skip all identical lines after first */
#define INIT_SKPBLNKWINS 0       /* 1 = skip blank windows */
#define INIT_SKPBLNKWINSMODE sbwEndOfLine
#define INIT_WINOVLP 0

/* These control the speed of any blinking cursor or capital letters.
 * The numbers refer to cycles of the main program loop.
 */
#define INIT_CSR_ON_CNT 10	/* for blinking cursor */
#define INIT_CSR_OFF_CNT 10
#define INIT_CAP_ON_CNT 4	/* for blinking caps */
#define INIT_CAP_OFF_CNT 2
#define INIT_ATTR_ON_CNT 4      /* for attribute underlining */
#define INIT_ATTR_OFF_CNT 12

/* Define this to allow "offright" position: i.e. positions such that part
   of the braille display goes beyond the right edge of the screen .*/
#define ALLOW_OFFRIGHT_POSITIONS
