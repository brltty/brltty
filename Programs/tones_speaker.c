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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>

#include "misc.h"
#include "system.h"
#include "tones.h"

static int speakerOpened = 0;

static int openSpeaker (void) {
   if (!speakerOpened) {
      if (!canBeep()) {
         LogPrint(LOG_WARNING, "Cannot open speaker.");
         return 0;
      }
      speakerOpened = 1;
      LogPrint(LOG_DEBUG, "Speaker opened.");
   }
   return 1;
}

static int generateSpeaker (int note, int duration) {
   if (speakerOpened) {
      LogPrint(LOG_DEBUG, "Tone: msec=%d note=%d",
               duration, note);
      if (!note) {
         shortdelay(duration);
	 return 1;
      }
      if (startBeep((int)noteFrequencies[note])) {
         shortdelay(duration);
	 if (stopBeep()) {
	    return 1;
	 }
      }
   }
   return 0;
}

static int flushSpeaker (void) {
   return speakerOpened;
}

static void closeSpeaker (void) {
   if (speakerOpened) {
      speakerOpened = 0;
      LogPrint(LOG_DEBUG, "Speaker closed.");
   }
}

const ToneGenerator speakerToneGenerator = {
   openSpeaker,
   generateSpeaker,
   flushSpeaker,
   closeSpeaker
};
