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
#include "tones.h"
#include "adlib.h"

static int cardOpen = 0;
static unsigned int channelNumber = 0;

static int openAdlib (void) {
   if (!cardOpen) {
      int ready = 0;
      if (AL_enablePorts()) {
         if (AL_testCard()) {
	    ready = 1;
	 }
      }
      if (!ready) {
         LogPrint(LOG_WARNING, "Cannot open Adlib.");
         return 0;
      }
      LogPrint(LOG_DEBUG, "Adlib opened.");
      cardOpen = 1;
   }
   return cardOpen;
}

static int generateAdlib (int note, int duration) {
   LogPrint(LOG_DEBUG, "Tone: msec=%d note=%d",
	    duration, note);
   if (note)
      AL_playTone(channelNumber, (int)noteFrequencies[note], duration, 100);
   else
      shortdelay(duration);
   return 1;
}

static int flushAdlib (void) {
   return cardOpen;
}

static void closeAdlib (void) {
   if (cardOpen) {
      AL_disablePorts();
      LogPrint(LOG_DEBUG, "Adlib closed.");
      cardOpen = 0;
   }
}

const ToneGenerator adlibToneGenerator = {
   openAdlib,
   generateAdlib,
   flushAdlib,
   closeAdlib
};
