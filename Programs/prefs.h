/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2011 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_PREFS
#define BRLTTY_INCLUDED_PREFS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  tsComputerBraille8,
  tsContractedBraille,
  tsComputerBraille6
} TextStyles;

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
 */
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
  unsigned char expandCurrentWord;
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

extern void resetPreferences (void);
extern void setStatusFields (const unsigned char *fields);

extern char *makePreferencesFilePath (const char *name);
extern int loadPreferencesFile (const char *path);
extern int savePreferencesFile (const char *path);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PREFS */
