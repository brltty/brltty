/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

#ifndef _SCR_H
#define _SCR_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* scr.h - C header file for the screen reading library
 */

/* mode argument for readScreen() */
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
} ScreenDescription;

typedef struct {
  short left, top;	/* top-left corner (offset from 0) */
  short width, height;	/* dimensions */
} ScreenBox;
extern int validateScreenBox (const ScreenBox *box, int columns, int rows);

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

/* Routines which apply to all screens. */
void initializeAllScreens (void);		/* close screen reading */
void closeAllScreens (void);		/* close screen reading */
int selectDisplay (int);		/* select display page */

/* Routines which apply to the current screen. */
void describeScreen (ScreenDescription *);		/* get screen status */
unsigned char *readScreen (short, short, short, short, unsigned char *, ScreenMode);
int insertKey (unsigned short);
int insertString (const char *);
int routeCursor (int, int, int);
int setPointer (int, int);
int getPointer (int *, int *);
int selectVirtualTerminal (int);
int switchVirtualTerminal (int);
int currentVirtualTerminal (void);
int executeScreenCommand (int);

/* Routines which apply to the live screen. */
const char *const *getScreenParameters (void);			/* initialise screen reading functions */
int openLiveScreen (char **parameters);			/* initialise screen reading functions */

/* Routines which apply to the routing screen.
 * An extra `thread' for the cursor routing subprocess.
 * This is needed because the forked subprocess shares its parent's
 * file descriptors.  A readScreen equivalent is not needed.
 */
int openRoutingScreen (void);
void describeRoutingScreen (ScreenDescription *);
void closeRoutingScreen (void);

/* Routines which apply to the help screen. */
int openHelpScreen (const char *);	/* open help screen file */
void setHelpPageNumber (short);			/* set screen number (initial default 0) */
short getHelpPageNumber (void);			/* set screen number (initial default 0) */
short getHelpPageCount (void);			/* get number of help screens */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SCR_H */
