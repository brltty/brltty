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

/* prototype for inskey functions */

void inskey (const unsigned char *string);

/* For the command to switch to another virtual terminal */
int switchvt(int n);


/* These are for inskey(): */
#define CONSOLE "/dev/tty0"	/* name of device for simulating keystrokes */
#define INS_RETURN "\r"
#define INS_TAB "\t"
#define INS_BACKSPACE "\177"
#define INS_ESCAPE "\033"
#define INS_CURSOR_LEFT "\033[D"		/* vt100 left-cursor */
#define INS_CURSOR_RIGHT "\033[C"		/* vt100 right-cursor */
#define INS_CURSOR_UP "\033[A"		/* vt100 up-cursor */
#define INS_CURSOR_DOWN "\033[B"		/* vt100 down-cursor */
#define INS_PAGE_UP "\033[5~"		/* vt100 right-cursor */
#define INS_PAGE_DOWN "\033[6~"		/* vt100 right-cursor */
#define INS_HOME "\033[1~"		/* vt100 right-cursor */
#define INS_END "\033[4~"		/* vt100 right-cursor */
#define INS_INSERT "\033[2~"		/* vt100 right-cursor */
#define INS_DELETE "\033[3~"		/* vt100 right-cursor */
#define INS_F1 "\033OP"
#define INS_F2 "\033OQ"
#define INS_F3 "\033OR"
#define INS_F4 "\033OS"
#define INS_F5 "\033[15~"
#define INS_F6 "\033[17~"
#define INS_F7 "\033[18~"
#define INS_F8 "\033[19~"
#define INS_F9 "\033[20~"
#define INS_F10 "\033[21~"
#define INS_F11 "\033[23~"
#define INS_F12 "\033[24~"
#define INS_F13 "\033[25~"
#define INS_F14 "\033[26~"
#define INS_F15 "\033[28~"
#define INS_F16 "\033[29~"
#define INS_F17 "\033[31~"
#define INS_F18 "\033[32~"
#define INS_F19 "\033[33~"
#define INS_F20 "\033[34~"

