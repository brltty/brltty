/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

#include <stdlib.h>
#include <unistd.h>

#include "misc.h"
#include "tones.h"
#include "adlib.h"

static int cardOpen = 0;
static unsigned int channelNumber = 0;

static int openAdLib (void) {
   if (!cardOpen) {
      int ready = 0;
      if (AL_enablePorts()) {
         if (AL_testCard()) {
	    ready = 1;
	 }
      }
      if (!ready) {
         return 0;
      }
      cardOpen = 1;
   }
   return cardOpen;
}

static int generateAdLib (int frequency, int duration) {
   if (frequency)
      AL_playTone(channelNumber, frequency, duration, 100);
   else
      shortdelay(duration);
   return 1;
}

static void closeAdLib (void) {
   if (cardOpen) {
      AL_disablePorts();
      cardOpen = 0;
   }
}

static ToneGenerator toneGenerator = {
   openAdLib,
   generateAdLib,
   closeAdLib
};

ToneGenerator *toneAdLib (void) {
   return &toneGenerator;
}
