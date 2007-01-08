/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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

typedef struct {
  short rows, cols;	/* screen dimensions */
  short posx, posy;	/* cursor position */
  int number;		      /* screen number */
  const char *unreadable;
} ScreenDescription;

typedef struct {
  short left, top;	/* top-left corner (offset from 0) */
  short width, height;	/* dimensions */
} ScreenBox;

extern int validateScreenBox (const ScreenBox *box, int columns, int rows);
extern void setScreenMessage (const ScreenBox *box, unsigned char *buffer, ScreenMode mode, const char *message);

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
extern void identifyScreenDrivers (int full);
extern void initializeAllScreens (const char *identifier, const char *driverDirectory);		/* close screen reading */
extern void closeAllScreens (void);		/* close screen reading */

extern int isLiveScreen (void);

extern int isFrozenScreen (void);
extern int haveFrozenScreen (void);
extern int activateFrozenScreen (void);
extern void deactivateFrozenScreen (void);

extern int isHelpScreen (void);
extern int haveHelpScreen (void);
extern int activateHelpScreen (void);
extern void deactivateHelpScreen (void);

/* Routines which apply to the current screen. */
extern void describeScreen (ScreenDescription *);		/* get screen status */
extern int readScreen (short, short, short, short, unsigned char *, ScreenMode);
extern int insertKey (ScreenKey);
extern int insertCharacters (const char *, int);
extern int insertString (const char *);
extern int routeCursor (int, int, int);
extern int setPointer (int, int, int, int);
extern int getPointer (int *, int *);
extern int selectVirtualTerminal (int);
extern int switchVirtualTerminal (int);
extern int currentVirtualTerminal (void);
extern int userVirtualTerminal (int number);
extern int executeScreenCommand (int);

/* Routines which apply to the main screen. */
extern const char *const *getScreenParameters (void);			/* initialise screen reading functions */
extern const char *getScreenDriverCode (void);			/* initialise screen reading functions */
extern int openMainScreen (char **parameters);			/* initialise screen reading functions */

/* Routines which apply to the routing screen.
 * An extra `thread' for the cursor routing subprocess.
 * This is needed because the forked subprocess shares its parent's
 * file descriptors.  A readScreen equivalent is not needed.
 */
extern int openRoutingScreen (void);
extern void closeRoutingScreen (void);

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
