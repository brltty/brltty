/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

#ifndef _BRLTTY_H
#define _BRLTTY_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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
  unsigned char spare3;
  unsigned char blinkingCapitals;
  unsigned char spare4;
  unsigned char blinkingAttributes;
  unsigned char spare5;
  unsigned char cursorStyle;
  unsigned char sayLineMode;
  unsigned char cursorVisiblePeriod;
  unsigned char autospeak;
  unsigned char cursorInvisiblePeriod;
  unsigned char pcmVolume;
  unsigned char capitalsVisiblePeriod;
  unsigned char midiVolume;
  unsigned char capitalsInvisiblePeriod;
  unsigned char fmVolume;
  unsigned char attributesVisiblePeriod;
  unsigned char pointerFollowsWindow;
  unsigned char attributesInvisiblePeriod;
  unsigned char windowFollowsPointer;
  unsigned char textStyle;
  unsigned char metaMode;
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
} __attribute__((packed)) Preferences;
extern Preferences prefs;		/* current preferences settings */

extern BrailleDisplay brl;			/* braille driver reference */
extern short fwinshift;			/* Full window horizontal distance */
extern short hwinshift;			/* Half window horizontal distance */
extern short vwinshift;			/* Window vertical distance */

extern int refreshInterval;
extern int messageDelay;

extern void startup (int argc, char *argv[]);
extern int loadPreferences (int change);
extern int savePreferences (void);
extern void updatePreferences (void);

extern void initializeBraille (void);
extern void startBrailleDriver (void);
extern void stopBrailleDriver (void);
extern int getBrailleCommand (DriverCommandContext cmds);
extern void clearStatusCells (void);
extern void setStatusText (const char *text);

extern void initializeSpeech (void);
extern void startSpeechDriver (void);
extern void stopSpeechDriver (void);

extern void api_identify (void);
extern void api_open (BrailleDisplay *brl, char **parameters);
extern void api_close (BrailleDisplay *brl);
extern void api_link (void);
extern void api_unlink (void);
extern const char *const api_parameters[];

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BRLTTY_H */
