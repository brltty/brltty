/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 * Nicolas Pitre <nico@cam.org>
 * Stephane Doyon <doyons@jsp.umontreal.ca>
 *
 * Version 1.0.2, 17 September 1996
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

/* scr.h - C header file for the screen reading library
 * $Id: scr.h,v 1.3 1996/09/24 01:04:27 nn201 Exp $
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
  }
scrstat;

typedef struct
  {
    short left, top;		/* coordinates of corner, counting from 0 */
    short width, height;	/* dimensions */
  }
winpos;

/* Functions provided by this library */
int initscr (void);		/* initialise screen reading functions */
scrstat getstat (void);		/* get screen status */
/* Read a rectangle from the screen - text or attributes: */
unsigned char *getscr (winpos, unsigned char *, short);
void closescr (void);		/* close screen reading */
int selectdisp (int);		/* select display page */

/* An extra `thread' for the cursor routing subprocess.
 * This is needed because the forked subprocess shares the parent's
 * filedescriptors.  A getscr equivalent is not needed, and so not provided.
 */
int initscr_phys (void);
scrstat getstat_phys (void);
void closescr_phys (void);

/* Manipulation of the help screen number, for use in brl.c: */
void sethlpscr (short);		/* set screen number (initial default 0) */
short numhlpscr (void);		/* get number of help screens */

#endif /* !_SCR_H */
