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

/* scr.h - C header file for the screen reading library
 */

#ifndef _SCR_H
#define _SCR_H


/* mode argument for getScreenContent() */
typedef enum {
  SCR_TEXT,		/* get screen text */
  SCR_ATTRIB		/* get screen attributes */
} ScreenMode;

/* disp argument for selectDisplay() */
#define LIVE_SCRN 0		/* read the physical screen */
#define FROZ_SCRN 1		/* read frozen screen image */
#define HELP_SCRN 2		/* read help screen */

typedef struct {
  short rows, cols;	/* screen dimentions */
  short posx, posy;	/* cursor position */
  short no;		      /* screen number */
} ScreenStatus;

typedef struct {
  short left, top;	/* top-left corner (offset from 0) */
  short width, height;	/* dimensions */
} ScreenBox;

/* Functions provided by this library */

char **getScreenParameters (void);			/* initialise screen reading functions */
int initializeScreen (char **parameters);			/* initialise screen reading functions */
void getScreenStatus (ScreenStatus *);		/* get screen status */
unsigned char *getScreenContent (ScreenBox, unsigned char *, ScreenMode);
		/* Read a rectangle from the screen - text or attributes: */
void closeScreen (void);		/* close screen reading */
int selectDisplay (int);		/* select display page */

typedef enum {
  KEY_RETURN = 0X100,
  KEY_TAB,
  KEY_BACKSPACE,
  KEY_ESCAPE,
  KEY_CURSOR_LEFT,
  KEY_CURSOR_RIGHT,
  KEY_CURSOR_UP,
  KEY_CURSOR_DOWN,
  KEY_PAGE_UP,
  KEY_PAGE_DOWN,
  KEY_HOME,
  KEY_END,
  KEY_INSERT,
  KEY_DELETE,
  KEY_FUNCTION
} ConsoleKey;
int insertKey (unsigned short);
int insertString (const unsigned char *);

int selectVirtualTerminal (int);
int switchVirtualTerminal (int);

/* An extra `thread' for the cursor routing subprocess.
 * This is needed because the forked subprocess shares the parent's
 * filedescriptors.  A getScreenContent equivalent is not needed, and so not provided.
 */
int initializeRoutingScreen (void);
void getRoutingScreenStatus (ScreenStatus *);
void closeRoutingScreen (void);

/* Manipulation of the help screen filename, help number, etc. */
int initializeHelpScreen (char *helpfile);	/* open help screen file */
void setHelpScreenNumber (short);			/* set screen number (initial default 0) */
short getHelpScreenCount (void);			/* get number of help screens */

#endif /* !_SCR_H */
