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

#include "prologue.h"

#include "misc.h"
#include "system.h"
#include "notes.h"

static int beeperOpened = 0;

static int openBeeper (int errorLevel) {
   if (!beeperOpened) {
      if (!canBeep()) {
         LogPrint(LOG_DEBUG, "cannot open beeper.");
         return 0;
      }
      beeperOpened = 1;
      LogPrint(LOG_DEBUG, "beeper opened.");
   }
   return 1;
}

static int playBeeper (int note, int duration) {
   if (beeperOpened) {
      LogPrint(LOG_DEBUG, "tone: msec=%d note=%d",
               duration, note);

      if (!note) {
         accurateDelay(duration);
	 return 1;
      }

      if (asynchronousBeep((int)noteFrequencies[note], duration*4)) {
         accurateDelay(duration);
	 stopBeep();
         return 1;
      }

      if (startBeep((int)noteFrequencies[note])) {
         accurateDelay(duration);
	 stopBeep();
         return 1;
      }

      if (synchronousBeep((int)noteFrequencies[note], duration)) return 1;
   }
   return 0;
}

static int flushBeeper (void) {
   return beeperOpened;
}

static void closeBeeper (void) {
   if (beeperOpened) {
      beeperOpened = 0;
      endBeep();
      LogPrint(LOG_DEBUG, "beeper closed.");
   }
}

const NoteGenerator beeperNoteGenerator = {
   openBeeper,
   playBeeper,
   flushBeeper,
   closeBeeper
};
