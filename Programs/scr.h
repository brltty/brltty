/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#include "scrdefs.h"
#include "ktbdefs.h"
#include "driver.h"
#include "cmd_queue.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int validateScreenBox (const ScreenBox *box, int columns, int rows);
extern void setScreenMessage (const ScreenBox *box, ScreenCharacter *buffer, const char *message);

extern void clearScreenCharacters (ScreenCharacter *characters, size_t count);
extern void setScreenCharacterText (ScreenCharacter *characters, wchar_t text, size_t count);
extern void setScreenCharacterAttributes (ScreenCharacter *characters, unsigned char attributes, size_t count);

/* Routines which apply to all screens. */
extern void constructSpecialScreens (void);
extern void destructSpecialScreens (void);

extern int isLiveScreen (void);

extern int isHelpScreen (void);
extern int haveHelpScreen (void);
extern int activateHelpScreen (void);
extern void deactivateHelpScreen (void);

extern int isMenuScreen (void);
extern int haveMenuScreen (void);
extern int activateMenuScreen (void);
extern void deactivateMenuScreen (void);

extern int isFrozenScreen (void);
extern int haveFrozenScreen (void);
extern int activateFrozenScreen (void);
extern void deactivateFrozenScreen (void);

/* Routines which apply to the current screen. */
extern size_t formatScreenTitle (char *buffer, size_t size);
extern void describeScreen (ScreenDescription *);		/* get screen status */
extern int readScreen (short left, short top, short width, short height, ScreenCharacter *buffer);
extern int readScreenText (short left, short top, short width, short height, wchar_t *buffer);
extern int insertScreenKey (ScreenKey key);
extern int routeCursor (int column, int row, int screen);
extern int highlightScreenRegion (int left, int right, int top, int bottom);
extern int unhighlightScreenRegion (void);
extern int getScreenPointer (int *column, int *row);
extern int selectScreenVirtualTerminal (int vt);
extern int switchScreenVirtualTerminal (int vt);
extern int currentVirtualTerminal (void);
extern int userVirtualTerminal (int number);
extern CommandHandler handleScreenCommand;
extern KeyTableCommandContext getScreenCommandContext (void);

/* Routines which apply to the routing screen.
 * An extra `thread' for the cursor routing subprocess.
 * This is needed because the forked subprocess shares its parent's
 * file descriptors.  A readScreen equivalent is not needed.
 */
extern int constructRoutingScreen (void);
extern void destructRoutingScreen (void);

/* Routines which apply to the help screen. */
extern int constructHelpScreen (void);
extern void destructHelpScreen (void);
extern int addHelpPage (void);
extern unsigned int getHelpPageCount (void);
extern unsigned int getHelpPageNumber (void);
extern int setHelpPageNumber (unsigned int number);
extern int clearHelpPage (void);
extern int addHelpLine (const wchar_t *characters);
extern unsigned int getHelpLineCount (void);

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
