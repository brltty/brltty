/*
 * BRLTTY - A background process providing access to the Linux console (when in
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>

#include "brltty.h"
#include "misc.h"
#include "notes.h"
#include "adlib.h"

static int cardOpen = 0;
static unsigned int channelNumber = 0;

static int openFm (int errorLevel) {
   if (!cardOpen) {
      int ready = 0;
      if (AL_enablePorts(errorLevel)) {
         if (AL_testCard(errorLevel)) {
	    ready = 1;
	 }
      }
      if (!ready) {
         LogPrint(LOG_DEBUG, "Cannot open FM.");
         return 0;
      }
      LogPrint(LOG_DEBUG, "FM opened.");
      cardOpen = 1;
   }
   return cardOpen;
}

static int playFm (int note, int duration) {
   LogPrint(LOG_DEBUG, "Tone: msec=%d note=%d",
	    duration, note);
   if (note)
      AL_playTone(channelNumber, (int)noteFrequencies[note], duration, prefs.fmVolume);
   else
      accurateDelay(duration);
   return 1;
}

static int flushFm (void) {
   return cardOpen;
}

static void closeFm (void) {
   if (cardOpen) {
      AL_disablePorts();
      LogPrint(LOG_DEBUG, "FM closed.");
      cardOpen = 0;
   }
}

const NoteGenerator fmNoteGenerator = {
   openFm,
   playFm,
   flushFm,
   closeFm
};
