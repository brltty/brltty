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

/* scr.h - Header file for the screen reading library
 * N. Nair, 23 July 1995
 */

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

/* Functions provided by this library */
int initscr (void);		/* initialise screen reading functions */
scrstat getstat (void);		/* get screen status */
/* Read a rectangle from the screen - text or attributes: */
unsigned char *getscr (short, short, short, short, unsigned char *, short);
void closescr (void);		/* close screen reading */
int selectdisp (int);		/* select display page */

/* To set help screen index number related to terminal submodel, provided for
 * use in brl.c 
 */
void sethlpscr (short);

/* index value type (also used in txt2scrn.c) */
typedef unsigned long INDEX;
