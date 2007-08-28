/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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

#include "brltty.h"
#include "misc.h"
#include "notes.h"
#include "adlib.h"

static int cardOpen = 0;
static unsigned int channelNumber = 0;

static int fmConstruct (int errorLevel) {
   if (!cardOpen) {
      int ready = 0;
      if (AL_enablePorts(errorLevel)) {
         if (AL_testCard(errorLevel)) {
	    ready = 1;
	 }
      }
      if (!ready) {
         LogPrint(LOG_DEBUG, "cannot open FM.");
         return 0;
      }
      LogPrint(LOG_DEBUG, "FM opened.");
      cardOpen = 1;
   }
   return cardOpen;
}

static int fmPlay (int note, int duration) {
   LogPrint(LOG_DEBUG, "tone: msec=%d note=%d",
	    duration, note);
   if (note)
      AL_playTone(channelNumber, (int)noteFrequencies[note], duration, prefs.fmVolume);
   else
      accurateDelay(duration);
   return 1;
}

static int fmFlush (void) {
   return cardOpen;
}

static void fmDestruct (void) {
   if (cardOpen) {
      AL_disablePorts();
      LogPrint(LOG_DEBUG, "FM closed.");
      cardOpen = 0;
   }
}

const NoteGenerator fmNoteGenerator = {
   fmConstruct,
   fmPlay,
   fmFlush,
   fmDestruct
};
