/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

#ifndef _CONFIG_H
#define _CONFIG_H

/* config.h - Configurable definitions
 *
 * Edit as necessary for your system.
 */

#include "common.h"
#include "brl.h"

#define CONFIG_FILE "/etc/brltty.conf"

/* Delay times, measured in milliseconds.
 * Note that I found a large error in timing - 40 is nearer 50 ms.
 */
#define REFRESH_INTERVAL 40	/* default sleep time per cycle - overall speed */
#define MESSAGE_DELAY 4000	/* Default time duration to display messages.
                            * Under 5 seconds (init's SIGTERM-SIGKILL delay
                            * during shutdown) is good as that allows "exiting" 
                            * to be replaced by "terminated" on the display.
                            */

/* Character cursor initialisation: */
#define BIG_CSRCHAR (B1 | B2 | B3 | B4 | B5 | B6 | B7 | B8) /* block cursor */
#define SMALL_CSRCHAR (B7 | B8) /* underline cursor */

/* Dot super-imposition for attribute underlining */
#define ATTR1CHAR (B7 | B8)
#define ATTR2CHAR (B8)

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
#define INIT_METAMODE 0		/* 1 for six-dot mode, 0 for eight-dot */
#define INIT_SLIDEWIN 0		/* 1 for sliding window on, 0 for off */
#define INIT_EAGER_SLIDEWIN 0
#define INIT_TUNES 1		/* 1 for on, 0 for off */
#define INIT_DOTS 0		/* 1 for on, 0 for off */
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

#endif /* !defined(_CONFIG_H) */
