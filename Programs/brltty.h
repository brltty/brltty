/*
 * BRLTTY - A background process providing access to the console screen (when in
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

#ifndef BRLTTY_INCLUDED_BRLTTY
#define BRLTTY_INCLUDED_BRLTTY

#include "prologue.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "cmd.h"
#include "brl.h"

typedef enum {
  sbwAll,
  sbwEndOfLine,
  sbwRestOfLine
} SkipBlankWindowsMode;

typedef enum {
  sayImmediate,
  sayEnqueue
} SayMode;

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
  unsigned char spare2;
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
  unsigned char pointerFollowsWindow;
  unsigned char attributesInvisibleTime;
  unsigned char windowFollowsPointer;
  unsigned char textStyle;
  unsigned char spare1;
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
  unsigned char statusStyle;
  unsigned char windowOverlap;
  unsigned char speechRate;
  unsigned char speechVolume;
  unsigned char brailleFirmness;
}
#ifdef HAVE_ATTRIBTE_PACKED
  __attribute__((packed))
#endif /* HAVE_ATTRIBTE_PACKED */
  Preferences;
extern Preferences prefs;		/* current preferences settings */
#define PREFERENCES_TIME(time) ((time) * 10)

extern char *opt_pcmDevice;
extern char *opt_midiDevice;

extern unsigned char cursorDots (void);

extern BrailleDisplay brl;			/* braille driver reference */
extern short fwinshift;			/* Full window horizontal distance */
extern short hwinshift;			/* Half window horizontal distance */
extern short vwinshift;			/* Window vertical distance */

extern int updateInterval;
extern int messageDelay;

extern void testProgramTermination (void);
extern void startup (int argc, char *argv[]);
extern int loadPreferences (int change);
extern int savePreferences (void);
extern void updatePreferences (void);

extern void restartBrailleDriver (void);
extern void restartSpeechDriver (void);

extern int readCommand (BRL_DriverCommandContext context);
extern void resetAutorepeat (void);
extern void handleAutorepeat (int *command, RepeatState *state);

extern void api_identify (void);
extern int api_open (BrailleDisplay *brl, char **parameters);
extern void api_close (BrailleDisplay *brl);
extern void api_link (void);
extern void api_unlink (void);
extern void api_flush (BrailleDisplay *brl, BRL_DriverCommandContext caller);
extern const char *const api_parameters[];
extern int apiOpened;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BRLTTY */
