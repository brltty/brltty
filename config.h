/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Version 1.0, 26 July 1996
 *
 * Copyright (C) 1995, 1996 by Nikhil Nair and others.  All rights reserved.
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nikhil Nair <nn201@cus.cam.ac.uk>.
 */

/* config.h - Configurable definitions
 * N. Nair, 25 January 1996
 *
 * Edit as necessary for your system.
 */

/* ------------------------ OVERALL CONFIGURATIONS ------------------------ */

#ifdef BRLTTY_C			/* for brltty.c */
#undef BRLTTY_C			/* only do once */

/* Delay times, measured in milliseconds.
 * Note that I found a large error in timing - 40 is nearer 50 ms.
 */
#define DELAY_TIME 40		/* sleep time per cycle - overall speed */
#define KEYDEL 40		/* sleep time when awaiting a keypress */
#define DISPDEL 2000		/* time duration to display messages */

/* Character cursor initialisation: */
#define BIG_CSRCHAR 0xff	/* block cursor */
#define SMALL_CSRCHAR 0xc0	/* underline cursor */

/* Initialisation of BRLTTY environment settings: */
#define INIT_CSRVIS 1		/* 1 for cursor display, 0 for no cursor */
#define INIT_CSRTRK 1		/* 1 for cursor tracking on, 0 for off */
#define INIT_CSRBLINK 0		/* 1 for cursor blink on, 0 for off */
#define INIT_CAPBLINK 0		/* 1 for capital blink on, 0 for off */
#define INIT_CSRSIZE 0		/* 1 for block, 0 for underline */
#define INIT_SIXDOTS 0		/* 1 for six-dot mode, 0 for eight-dot */
#define INIT_SLIDEWIN 0		/* 1 for sliding window on, 0 for off */
#define INIT_BEEPSON 1		/* 1 for beeps, 0 for no beeps */

/* Filenames of translation tables and configuration file: */
#define TEXTTRN_NAME "us.tbl"	/* filename of text trans. table */
#define ATTRIBTRN_NAME "attrib.tbl"	/* filename of attrib. table */
#define CONFFILE_NAME "brlttyconf.dat"	/* filename of binary config. file */

/* These control the speed of any blinking cursor or capital letters.
 * The numbers refer to cycles of the main program loop.
 */
#define INIT_CSR_ON_CNT 4	/* for blinking cursor */
#define INIT_CSR_OFF_CNT 4
#define INIT_CAP_ON_CNT 4	/* for blinking caps */
#define INIT_CAP_OFF_CNT 2

/* These control the performance of cursor routing.  The optimal settings
 * will depend heavily on system load, etc.  See the documentation for
 * further details.
 * NOTE: if you try to route the cursor to an invalid place, BRLTTY won't
 * give up until the timeout has elapsed!
 */
#define CSRJMP_NICENESS 10	/* niceness of cursor routing subprocess */
#define CSRJMP_TIMEOUT 4000	/* cursor routing timeout in milliseconds */
#define CSRJMP_LOOP_DELAY 0	/* delay to use in csrjmp_sub() loops (ms) */

/* These are for inskey(): */
#define CONSOLE "/dev/console"	/* name of device for simulating keystrokes */
#define UP_CSR "\033[A"		/* vt100 up-cursor */
#define DN_CSR "\033[B"		/* vt100 down-cursor */
#define LT_CSR "\033[D"		/* vt100 left-cursor */
#define RT_CSR "\033[C"		/* vt100 right-cursor */

/* for keyboard key mapping (see KEYMAP) ... */
#define KEY_RETURN "\r"
#endif /* BRLTTY_C */

/* ---------------- SETTINGS FOR THE SCREEN-READING LIBRARY ---------------- */

#ifdef SCR_C			/* for scr.c */
#undef SCR_C			/* only do once */
#define FRZFILE "/tmp/vcsa.frz"	/* filename for frozen screen image */
#define HLPFILE "brlttyhelp.scr"	/* filename of help screen */
#endif /* SCR_C */

/* ------------------------------------------------------------------------- */
