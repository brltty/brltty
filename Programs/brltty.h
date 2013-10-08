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
#include "timing.h"
#include "cmd.h"
#include "brl.h"
#include "spk.h"
#include "scrdefs.h"
#include "ses.h"
#include "ctb.h"
#include "ktb.h"
#include "menu.h"
#include "prefs.h"
#include "tunes.h"

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

extern void resetUpdateAlarm (void);
extern void suspendUpdates (void);
extern void resumeUpdates (void);

extern void updateSessionAttributes (void);
extern SessionEntry *ses;

typedef int (*IsSameCharacter) (
  const ScreenCharacter *character1,
  const ScreenCharacter *character2
);

extern int isSameText (
  const ScreenCharacter *character1,
  const ScreenCharacter *character2
);

extern int isSameAttributes (
  const ScreenCharacter *character1,
  const ScreenCharacter *character2
);

extern int isSameRow (
  const ScreenCharacter *characters1,
  const ScreenCharacter *characters2,
  int count,
  IsSameCharacter isSameCharacter
);

extern int toggleFlag (
  int *bits, int bit, int command,
  const TuneDefinition *offTune, const TuneDefinition *onTune
);

extern int toggleSetting (
  unsigned char *setting, int command,
  const TuneDefinition *offTune, const TuneDefinition *onTune
);

extern int toggleModeSetting (unsigned char *setting, int command);
extern int toggleFeatureSetting (unsigned char *setting, int command);

extern unsigned char infoMode;

extern int writeBrailleCharacters (const char *mode, const wchar_t *characters, size_t length);
extern int writeBrailleText (const char *mode, const char *text);
extern int showBrailleText (const char *mode, const char *text, int minimumDelay);

extern char *opt_tablesDirectory;
extern char *opt_textTable;
extern char *opt_attributesTable;
extern char *opt_contractionTable;

extern int opt_releaseDevice;

extern int updateInterval;
extern int messageDelay;

extern int restartRequired;
extern int isOffline;
extern int isSuspended;
extern int inputModifiers;

extern void resetBrailleState (void);

extern void placeRightEdge (int column);
extern void placeWindowRight (void);
extern void placeWindowHorizontally (int x);

extern int moveWindowLeft (unsigned int amount);
extern int moveWindowRight (unsigned int amount);

extern int shiftWindowLeft (unsigned int amount);
extern int shiftWindowRight (unsigned int amount);

extern void slideWindowVertically (int y);

extern int showCursor (void);
extern int trackCursor (int place);

extern int isTextOffset (int *arg, int end, int relaxed);

typedef struct {
  TimeComponents time;
  const char *meridian;
} TimeFormattingData;

extern void getTimeFormattingData (TimeFormattingData *fmt);
extern size_t formatBrailleTime (char *buffer, size_t size, const TimeFormattingData *fmt);

extern size_t formatCharacterDescription (char *buffer, size_t size, int column, int row);

#ifdef ENABLE_CONTRACTED_BRAILLE
extern int isContracted;
extern int contractedLength;

extern int isContracting (void);
extern int getContractedLength (unsigned int outputLimit);
#endif /* ENABLE_CONTRACTED_BRAILLE */

FUNCTION_DECLARE(changeLogLevel, int, (const char *operand));
FUNCTION_DECLARE(changeLogCategories, int, (const char *operand));

FUNCTION_DECLARE(changeTextTable, int, (const char *name));
FUNCTION_DECLARE(changeAttributesTable, int, (const char *name));

extern ContractionTable *contractionTable;
FUNCTION_DECLARE(changeContractionTable, int, (const char *name));

extern KeyTable *keyboardKeyTable;
FUNCTION_DECLARE(changeKeyboardKeyTable, int, (const char *name));

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

FUNCTION_DECLARE(restartBrailleDriver, void, (void));
FUNCTION_DECLARE(changeBrailleDriver, int, (const char *driver));
FUNCTION_DECLARE(changeBrailleDevice, int, (const char *device));

extern int constructBrailleDriver (void);
extern void destructBrailleDriver (void);

extern void reconfigureWindow (void);
extern int haveStatusCells (void);

typedef enum {
  SCT_WORD,
  SCT_NONWORD,
  SCT_SPACE
} ScreenCharacterType;

extern ScreenCharacterType getScreenCharacterType (const ScreenCharacter *character);
extern int findFirstNonSpaceCharacter (const ScreenCharacter *characters, int count);
extern int findLastNonSpaceCharacter (const ScreenCharacter *characters, int count);
extern int isAllSpaceCharacters (const ScreenCharacter *characters, int count);

#ifdef ENABLE_SPEECH_SUPPORT
extern SpeechSynthesizer spk;
extern int opt_quietIfNoBraille;

extern int speechTracking;
extern int speechScreen;
extern int speechLine;
extern int speechIndex;

extern int autospeak (void);
extern void sayScreenCharacters (const ScreenCharacter *characters, size_t count, int immediate);
extern void speakCharacters (const ScreenCharacter *characters, size_t count, int spell);
extern void trackSpeech (int index);

FUNCTION_DECLARE(restartSpeechDriver, void, (void));
FUNCTION_DECLARE(changeSpeechDriver, int, (const char *driver));

extern int constructSpeechDriver (void);
extern void destructSpeechDriver (void);
#endif /* ENABLE_SPEECH_SUPPORT */

#ifdef __MINGW32__
extern int isWindowsService;
#endif /* __MINGW32__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BRLTTY */
