/*
 * BrlTty - A daemon providing access to the Linux console (when in text
 *          mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BrlTty Team. All rights reserved.
 *
 * BrlTty comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* scr.h - C header file for the screen reading library
 */

#ifndef _SCR_H
#define _SCR_H


/* mode argument for getscr () */
#define SCR_TEXT 0		/* get screen text */
#define SCR_ATTRIB 1		/* get screen attributes */

/* disp argument for selectdisp () */
#define LIVE_SCRN 0		/* read the physical screen */
#define FROZ_SCRN 1		/* read frozen screen image */
#define HELP_SCRN 2		/* read help screen */

typedef struct
  {
    short rows, cols;		/* screen dimentions */
    short posx, posy;		/* cursor position */
    short no;			/* screen number */
  }
scrstat;

typedef struct
  {
    short left, top;		/* coordinates of corner, counting from 0 */
    short width, height;	/* dimensions */
  }
winpos;

/* Functions provided by this library */

int initscr (void);			/* initialise screen reading functions */
void getstat (scrstat *);		/* get screen status */
unsigned char *getscr (winpos, unsigned char *, short);
		/* Read a rectangle from the screen - text or attributes: */
void closescr (void);		/* close screen reading */
int selectdisp (int);		/* select display page */

/* An extra `thread' for the cursor routing subprocess.
 * This is needed because the forked subprocess shares the parent's
 * filedescriptors.  A getscr equivalent is not needed, and so not provided.
 */
int initscr_phys (void);
void getstat_phys (scrstat *);
void closescr_phys (void);

/* Manipulation of the help screen filename, help number, etc. */
int inithlpscr (char *helpfile);	/* open help screen file */
void sethlpscr (short);			/* set screen number (initial default 0) */
short numhlpscr (void);			/* get number of help screens */

#endif /* !_SCR_H */
