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
void switchvt(int n);


/* These are for inskey(): */
#define CONSOLE "/dev/tty0"	/* name of device for simulating keystrokes */
#define UP_CSR "\033[A"		/* vt100 up-cursor */
#define DN_CSR "\033[B"		/* vt100 down-cursor */
#define LT_CSR "\033[D"		/* vt100 left-cursor */
#define RT_CSR "\033[C"		/* vt100 right-cursor */
#define UP_PAGE "\033[5~"		/* vt100 right-cursor */
#define DN_PAGE "\033[6~"		/* vt100 right-cursor */
#define KEY_HOME "\033[1~"		/* vt100 right-cursor */
#define KEY_END "\033[4~"		/* vt100 right-cursor */
#define KEY_INSERT "\033[2~"		/* vt100 right-cursor */
#define KEY_DELETE "\033[3~"		/* vt100 right-cursor */
#define KEY_RETURN "\r"
#define KEY_TAB "\t"
#define KEY_BACKSPACE "\177"
#define KEY_ESCAPE "\033"
#define KEY_F1 "\033OP"
#define KEY_F2 "\033OQ"
#define KEY_F3 "\033OR"
#define KEY_F4 "\033OS"
#define KEY_F5 "\033[15~"
#define KEY_F6 "\033[17~"
#define KEY_F7 "\033[18~"
#define KEY_F8 "\033[19~"
#define KEY_F9 "\033[20~"
#define KEY_F10 "\033[21~"
#define KEY_F11 "\033[23~"
#define KEY_F12 "\033[24~"
#define KEY_F13 "\033[25~"
#define KEY_F14 "\033[26~"
#define KEY_F15 "\033[28~"
#define KEY_F16 "\033[29~"
#define KEY_F17 "\033[31~"
#define KEY_F18 "\033[32~"
#define KEY_F19 "\033[33~"
#define KEY_F20 "\033[34~"

