/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team, All rights reserved.
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

/* prototype for inskey functions */

void inskey (unsigned char *string);

/* For the command to switch to another virtual terminal */
void switchvt(int n);


/* These are for inskey(): */
#define CONSOLE "/dev/tty0"	/* name of device for simulating keystrokes */
#define UP_CSR "\033[A"		/* vt100 up-cursor */
#define DN_CSR "\033[B"		/* vt100 down-cursor */
#define LT_CSR "\033[D"		/* vt100 left-cursor */
#define RT_CSR "\033[C"		/* vt100 right-cursor */

/* for keyboard key mapping (see KEYMAP) ... */
#define KEY_RETURN "\r"

