/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#ifndef BRLTTY_INCLUDED_SCR
#define BRLTTY_INCLUDED_SCR

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

#define SCR_KEY_MOD_META 0X100
typedef enum {
  SCR_KEY_ENTER = 0X200,
  SCR_KEY_TAB,
  SCR_KEY_BACKSPACE,
  SCR_KEY_ESCAPE,
  SCR_KEY_CURSOR_LEFT,
  SCR_KEY_CURSOR_RIGHT,
  SCR_KEY_CURSOR_UP,
  SCR_KEY_CURSOR_DOWN,
  SCR_KEY_PAGE_UP,
  SCR_KEY_PAGE_DOWN,
  SCR_KEY_HOME,
  SCR_KEY_END,
  SCR_KEY_INSERT,
  SCR_KEY_DELETE,
  SCR_KEY_FUNCTION
} ScreenKey;

/* Routines which apply to all screens. */
void initializeAllScreens (void);		/* close screen reading */
void closeAllScreens (void);		/* close screen reading */
int selectDisplay (int);		/* select display page */

/* Routines which apply to the current screen. */
void describeScreen (ScreenDescription *);		/* get screen status */
unsigned char *readScreen (short, short, short, short, unsigned char *, ScreenMode);
int insertKey (ScreenKey);
int insertCharacters (const char *, int);
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
extern int openHelpScreen (const char *);
extern void closeHelpScreen (void);
extern void setHelpPageNumber (short);
extern short getHelpPageNumber (void);
extern short getHelpPageCount (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SCR */
