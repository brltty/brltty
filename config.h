/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
 *
 * Web Page: http://www.cam.org/~nico/brltty
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
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
#define DISPDEL 2000		/* time duration to display messages */


#ifdef BRLTTY_C			/* for brltty.c */
#undef BRLTTY_C			/* only do once */

/* Character cursor initialisation: */
#define BIG_CSRCHAR 0xff	/* block cursor */
#define SMALL_CSRCHAR 0xc0	/* underline cursor */

/* Dot super-imposition for attribute underlining */
#define ATTR1CHAR 0xC0
#define ATTR2CHAR 0x80

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
#define INIT_BEEPSON 1		/* 1 for beeps, 0 for no beeps */
#define INIT_SKPIDLNS 0		/* 1 = skip all identical lines after first */
#define INIT_SKPBLNKEOL 0       /* 1 = when remaining of line is blank, skip
				   to the next. */

/* These control the speed of any blinking cursor or capital letters.
 * The numbers refer to cycles of the main program loop.
 */
#define INIT_CSR_ON_CNT 10	/* for blinking cursor */
#define INIT_CSR_OFF_CNT 10
#define INIT_CAP_ON_CNT 4	/* for blinking caps */
#define INIT_CAP_OFF_CNT 2
#define INIT_ATTR_ON_CNT 4      /* for attribute underlining */
#define INIT_ATTR_OFF_CNT 12

/* These control the performance of cursor routing.  The optimal settings
 * will depend heavily on system load, etc.  See the documentation for
 * further details.
 * NOTE: if you try to route the cursor to an invalid place, BRLTTY won't
 * give up until the timeout has elapsed!
 */
#define CSRJMP_NICENESS 10	/* niceness of cursor routing subprocess */
#define CSRJMP_TIMEOUT 2000	/* cursor routing idle timeout in ms */
#define CSRJMP_LOOP_DELAY 0	/* delay to use in csrjmp_sub() loops (ms) */
#define CSRJMP_SETTLE_DELAY 400	/* delay to use in csrjmp_sub() loops (ms) */

#endif /* BRLTTY_C */


/* These are for inskey(): */
#define CONSOLE "/dev/tty0"	/* name of device for simulating keystrokes */
#define UP_CSR "\033[A"		/* vt100 up-cursor */
#define DN_CSR "\033[B"		/* vt100 down-cursor */
#define LT_CSR "\033[D"		/* vt100 left-cursor */
#define RT_CSR "\033[C"		/* vt100 right-cursor */

/* for keyboard key mapping (see KEYMAP) ... */
#define KEY_RETURN "\r"

/* ---------------- SETTINGS FOR THE SCREEN-READING LIBRARY ---------------- */

#ifdef SCR_C			/* for scr.c */
#undef SCR_C			/* only do once */
#endif /* SCR_C */

/* ------------------------------------------------------------------------- */

/* misc for every files */
#define NBR_SCR 16		/* actual number of separate screens */

/* see misc.h to disable use of syslog */
