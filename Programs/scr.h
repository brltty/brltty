/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
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

#include "driver.h"

#define SCR_ATTR_FG_BLUE   0X01
#define SCR_ATTR_FG_GREEN  0X02
#define SCR_ATTR_FG_RED    0X04
#define SCR_ATTR_FG_BRIGHT 0X08
#define SCR_ATTR_BG_BLUE   0X10
#define SCR_ATTR_BG_GREEN  0X20
#define SCR_ATTR_BG_RED    0X40
#define SCR_ATTR_BLINK     0X80

typedef struct {
  wchar_t text;
  unsigned char attributes;
} ScreenCharacter;

typedef struct {
  short rows, cols;	/* screen dimensions */
  short posx, posy;	/* cursor position */
  int number;		      /* screen number */
  unsigned cursor:1;
  const char *unreadable;
} ScreenDescription;

typedef struct {
  short left, top;	/* top-left corner (offset from 0) */
  short width, height;	/* dimensions */
} ScreenBox;

extern int validateScreenBox (const ScreenBox *box, int columns, int rows);
extern void setScreenMessage (const ScreenBox *box, ScreenCharacter *buffer, const char *message);

extern void clearScreenCharacters (ScreenCharacter *characters, size_t count);
extern void setScreenCharacterText (ScreenCharacter *characters, char text, size_t count);
extern void copyScreenCharacterText (ScreenCharacter *characters, const char *text, size_t count);
extern void setScreenCharacterAttributes (ScreenCharacter *characters, unsigned char attributes, size_t count);

#define SCR_KEY_SHIFT     0X40000000
#define SCR_KEY_UPPER     0X20000000
#define SCR_KEY_CONTROL   0X10000000
#define SCR_KEY_ALT_LEFT  0X08000000
#define SCR_KEY_ALT_RIGHT 0X04000000
#define SCR_KEY_CHAR_MASK 0X00FFFFFF

#define SCR_KEY_UNICODE_ROW 0XF800

typedef enum {
  SCR_KEY_ENTER = SCR_KEY_UNICODE_ROW,
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
extern void constructSpecialScreens (void);
extern void destructSpecialScreens (void);

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
extern int readScreen (short left, short top, short width, short height, ScreenCharacter *buffer);
extern int readScreenText (short left, short top, short width, short height, wchar_t *buffer);
extern int insertScreenKey (ScreenKey key);
extern int routeCursor (int column, int row, int screen);
extern int highlightScreenRegion (int left, int right, int top, int bottom);
extern int unhighlightScreenRegion (void);
extern int getScreenPointer (int *column, int *row);
extern int selectVirtualTerminal (int vt);
extern int switchVirtualTerminal (int vt);
extern int currentVirtualTerminal (void);
extern int userVirtualTerminal (int number);
extern int executeScreenCommand (int);

/* Routines which apply to the routing screen.
 * An extra `thread' for the cursor routing subprocess.
 * This is needed because the forked subprocess shares its parent's
 * file descriptors.  A readScreen equivalent is not needed.
 */
extern int constructRoutingScreen (void);
extern void destructRoutingScreen (void);

/* Routines which apply to the help screen. */
extern int constructHelpScreen (const char *);
extern void destructHelpScreen (void);
extern void setHelpPageNumber (short);
extern short getHelpPageNumber (void);
extern short getHelpPageCount (void);

typedef struct ScreenDriverStruct ScreenDriver;
extern const char *const *getScreenParameters (const ScreenDriver *driver);
extern const DriverDefinition *getScreenDriverDefinition (const ScreenDriver *driver);

extern int haveScreenDriver (const char *code);
extern const char *getDefaultScreenDriver (void);
extern const ScreenDriver *loadScreenDriver (const char *code, void **driverObject, const char *driverDirectory);
extern void initializeScreen (void);
extern int constructScreenDriver (char **parameters);
extern void destructScreenDriver (void);
extern void identifyScreenDriver (const ScreenDriver *driver, int full);
extern void identifyScreenDrivers (int full);
extern const ScreenDriver *screen;
extern const ScreenDriver noScreen;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SCR */
