/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

#ifndef _COMMON_H
#define _COMMON_H

#include "brl.h"

typedef enum {
  sbwAll,
  sbwEndOfLine,
  sbwRestOfLine
} SkipBlankWindowsMode;

typedef enum {
  tdSpeaker,
  tdSoundCard,
  tdSequencer,
  tdAdLib
} TuneDevice;

/*
 * Structure definition for preferences (settings which are saveable).
 * PREFS_MAGICNUM has to be bumped whenever the definition of
 * Preferences is modified otherwise this structure could be
 * filled with incompatible data from disk.
 */
#define PREFS_MAGICNUM 0x4005

typedef struct {
  unsigned char magicnum[2];
  unsigned char csrvis;
  unsigned char spare1;
  unsigned char attrvis;
  unsigned char spare2;
  unsigned char csrblink;
  unsigned char spare3;
  unsigned char capblink;
  unsigned char spare4;
  unsigned char attrblink;
  unsigned char spare5;
  unsigned char csrsize;
  unsigned char spare6;
  unsigned char csroncnt;
  unsigned char spare7;
  unsigned char csroffcnt;
  unsigned char spare8;
  unsigned char caponcnt;
  unsigned char spare9;
  unsigned char capoffcnt;
  unsigned char spare10;
  unsigned char attroncnt;
  unsigned char spare11;
  unsigned char attroffcnt;
  unsigned char spare12;
  unsigned char sixdots;
  unsigned char metamode;
  unsigned char slidewin;
  unsigned char eager_slidewin;
  unsigned char tunes;
  unsigned char tunedev;
  unsigned char skpidlns;
  unsigned char spare15;
  unsigned char skpblnkwinsmode;
  unsigned char dots;
  unsigned char skpblnkwins;
  unsigned char midiinstr;
  unsigned char stcellstyle;
  unsigned char winovlp;
} __attribute__((packed)) Preferences;
extern Preferences prefs;		/* current preferences settings */

extern brldim brl;			/* braille driver reference */
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
extern void clearStatusCells (void);
extern void setStatusText (const unsigned char *text);

extern void initializeSpeech (void);
extern void startSpeechDriver (void);
extern void stopSpeechDriver (void);

#endif /* !defined(_COMMON_H) */
