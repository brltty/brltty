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

#ifndef BRLTTY_INCLUDED_BRLTTY
#define BRLTTY_INCLUDED_BRLTTY

#include "prologue.h"

#include "program.h"
#include "cmd.h"
#include "brl.h"
#include "spk.h"
#include "ctb.h"
#include "ktb.h"
#include "scr.h"
#include "ses.h"
#include "menu.h"
#include "prefs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern ScreenDescription scr;
#define SCR_COORDINATE_OK(coordinate,limit) (((coordinate) >= 0) && ((coordinate) < (limit)))
#define SCR_COLUMN_OK(column) SCR_COORDINATE_OK((column), scr.cols)
#define SCR_ROW_OK(row) SCR_COORDINATE_OK((row), scr.rows)
#define SCR_COORDINATES_OK(column,row) (SCR_COLUMN_OK((column)) && SCR_ROW_OK((row)))
#define SCR_CURSOR_OK() SCR_COORDINATES_OK(scr.posx, scr.posy)
#define SCR_COLUMN_NUMBER(column) (SCR_COLUMN_OK((column))? (column)+1: 0)
#define SCR_ROW_NUMBER(row) (SCR_ROW_OK((row))? (row)+1: 0)

extern SessionEntry *ses;
extern unsigned int updateIntervals;
extern unsigned char infoMode;

extern int writeBrailleCharacters (const char *mode, const wchar_t *characters, size_t length);
extern int writeBrailleText (const char *mode, const char *text);
extern int showBrailleText (const char *mode, const char *text, int minimumDelay);

extern char *opt_tablesDirectory;
extern char *opt_textTable;
extern char *opt_attributesTable;
extern char *opt_contractionTable;

extern int opt_releaseDevice;
extern char *opt_pcmDevice;
extern char *opt_midiDevice;

extern int updateInterval;
extern int messageDelay;

extern ContractionTable *contractionTable;
extern int loadContractionTable (const char *name);

extern KeyTable *keyboardKeyTable;

extern ProgramExitStatus brlttyStart (int argc, char *argv[]);

extern void setPreferences (const Preferences *newPreferences);
extern int loadPreferences (void);
extern int savePreferences (void);

extern unsigned char getCursorDots (void);

extern BrailleDisplay brl;			/* braille driver reference */
extern unsigned int textStart;
extern unsigned int textCount;
extern unsigned char textMaximized;
extern unsigned int statusStart;
extern unsigned int statusCount;
extern unsigned int fullWindowShift;			/* Full window horizontal distance */
extern unsigned int halfWindowShift;			/* Half window horizontal distance */
extern unsigned int verticalWindowShift;			/* Window vertical distance */

extern void restartBrailleDriver (void);
extern int constructBrailleDriver (void);
extern void destructBrailleDriver (void);

extern void reconfigureWindow (void);
extern int haveStatusCells (void);

#ifdef ENABLE_SPEECH_SUPPORT
extern SpeechSynthesizer spk;

extern void restartSpeechDriver (void);
extern int constructSpeechDriver (void);
extern void destructSpeechDriver (void);
#endif /* ENABLE_SPEECH_SUPPORT */

extern void resetAutorepeat (void);
extern void handleAutorepeat (int *command, RepeatState *state);

extern void highlightWindow (void);

extern void api_identify (int full);
extern int api_start (BrailleDisplay *brl, char **parameters);
extern void api_stop (BrailleDisplay *brl);
extern void api_link (BrailleDisplay *brl);
extern void api_unlink (BrailleDisplay *brl);
extern void api_suspend (BrailleDisplay *brl);
extern int api_resume (BrailleDisplay *brl);
extern int api_claimDriver (BrailleDisplay *brl);
extern void api_releaseDriver (BrailleDisplay *brl);
extern int api_flush (BrailleDisplay *brl);
extern int api_handleCommand (int command);
extern int api_handleKeyEvent (unsigned char set, unsigned char key, int press);
extern const char *const api_parameters[];
extern int apiStarted;

#ifdef __MINGW32__
extern int isWindowsService;
#endif /* __MINGW32__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BRLTTY */
