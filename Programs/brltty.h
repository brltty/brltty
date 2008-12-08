/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "cmd.h"
#include "brl.h"
#include "spk.h"
#include "ctb.h"
#include "ktb.h"

typedef enum {
  sbwAll,
  sbwEndOfLine,
  sbwRestOfLine
} SkipBlankWindowsMode;

typedef enum {
  sayImmediate,
  sayEnqueue
} SayMode;

typedef enum {
  spNone,
  spLeft,
  spRight
} StatusPosition;

typedef enum {
  ssNone,
  ssSpace,
  ssBlock,
  ssStatusSide,
  ssTextSide
} StatusSeparator;

/*
 * Structure definition for preferences (settings which are saveable).
 * PREFS_MAGIC_NUMBER has to be bumped whenever the definition of
 * Preferences is modified otherwise this structure could be
 * filled with incompatible data from disk.
 */
#define PREFS_MAGIC_NUMBER 0x4005

typedef struct {
  unsigned char magic[2];
  unsigned char showCursor;
  unsigned char version;
  unsigned char showAttributes;
  unsigned char brailleSensitivity;
  unsigned char blinkingCursor;
  unsigned char autorepeat;
  unsigned char blinkingCapitals;
  unsigned char autorepeatDelay;
  unsigned char blinkingAttributes;
  unsigned char autorepeatInterval;
  unsigned char cursorStyle;
  unsigned char sayLineMode;
  unsigned char cursorVisibleTime;
  unsigned char autospeak;
  unsigned char cursorInvisibleTime;
  unsigned char pcmVolume;
  unsigned char capitalsVisibleTime;
  unsigned char midiVolume;
  unsigned char capitalsInvisibleTime;
  unsigned char fmVolume;
  unsigned char attributesVisibleTime;
  unsigned char highlightWindow;
  unsigned char attributesInvisibleTime;
  unsigned char windowFollowsPointer;
  unsigned char textStyle;
  unsigned char autorepeatPanning;
  unsigned char slidingWindow;
  unsigned char eagerSlidingWindow;
  unsigned char alertTunes;
  unsigned char tuneDevice;
  unsigned char skipIdenticalLines;
  unsigned char alertMessages;
  unsigned char blankWindowsSkipMode;
  unsigned char alertDots;
  unsigned char skipBlankWindows;
  unsigned char midiInstrument;
  unsigned char spare1;
  unsigned char windowOverlap;
  unsigned char speechRate;
  unsigned char speechVolume;
  unsigned char brailleFirmness;
  unsigned char speechPunctuation;
  unsigned char speechPitch;
  unsigned char statusPosition;
  unsigned char statusCount;
  unsigned char statusSeparator;
  unsigned char statusFields[10];
} PACKED Preferences;

extern Preferences prefs;		/* current preferences settings */
#define PREFERENCES_TIME(time) ((time) * 10)

extern unsigned int getStatusFieldsLength (const unsigned char *fields);

extern int writeBrailleCharacters (const char *mode, const wchar_t *characters, size_t length);
extern int writeBrailleBytes (const char *mode, const char *bytes, size_t length);
extern int writeBrailleString (const char *mode, const char *string);
extern int showBrailleString (const char *mode, const char *string, unsigned int duration);

extern int opt_releaseDevice;
extern char *opt_pcmDevice;
extern char *opt_midiDevice;

extern int updateInterval;
extern int messageDelay;
extern ContractionTable *contractionTable;
extern KeyTable *keyTable;

extern void testProgramTermination (void);
extern void startup (int argc, char *argv[]);
extern int loadPreferences (void);
extern int savePreferences (void);
extern int updatePreferences (void);

extern unsigned char cursorDots (void);

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

extern int readCommand (BRL_DriverCommandContext context);
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
extern void api_flush (BrailleDisplay *brl, BRL_DriverCommandContext caller);
extern const char *const api_parameters[];
extern int apiStarted;

#ifdef __MINGW32__
extern int isWindowsService;
#endif /* __MINGW32__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BRLTTY */
