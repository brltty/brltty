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
#include <string.h>

#include "misc.h"
#include "message.h"
#include "brl.h"
#include "config.h"
#include "common.h"
#include "tunes.h"
#include "tones.h"

static ToneDefinition tones_detected[] = {
   {  330,  60},
   {  440, 100},
   {    0,   0}
};
TuneDefinition tune_detected = {
   NULL, 0X0000, tones_detected
};

static ToneDefinition tones_braille_off[] = {
   {  330,  60},
   {  220,  60},
   {    0,   0}
};
TuneDefinition tune_braille_off = {
   NULL, 0X0000, tones_braille_off
};

static ToneDefinition tones_link[] = {
   {  850,   7},
   {  793,   7},
   {  661,  12},
   {    0,   0}
};
TuneDefinition tune_link = {
   NULL, 0X0000, tones_link
};

static ToneDefinition tones_unlink[] = {
   {  743,   7},
   {  793,   7},
   {  991,  20},
   {    0,   0}
};
TuneDefinition tune_unlink = {
   NULL, 0X0000, tones_unlink
};

static ToneDefinition tones_wrap_down[] = {
   { 1200,   6},
   {  600,   6},
   {  300,   6},
   {  150,  10},
   {    0,   0}
};
TuneDefinition tune_wrap_down = {
   NULL, 0X1455, tones_wrap_down
};

static ToneDefinition tones_wrap_up[] = {
   {  150,   6},
   {  300,   6},
   {  600,   6},
   { 1200,  10},
   {    0,   0}
};
TuneDefinition tune_wrap_up = {
   NULL, 0X14AA, tones_wrap_up
};

static ToneDefinition tones_bounce[] = {
   { 2400,   6},
   { 1200,   6},
   {  600,   6},
   {  300,   6},
   {  150,  10},
   {    0,   0}
};
TuneDefinition tune_bounce = {
   NULL, 0X30FF, tones_bounce
};

static ToneDefinition tones_freeze[] = {
   {  238,   5},
   {  247,   5},
   {  258,   5},
   {  270,   5},
   {  283,   5},
   {  297,   5},
   {  313,   5},
   {  330,   5},
   {  350,   5},
   {  371,   5},
   {  396,   5},
   {  425,   5},
   {  457,   5},
   {  495,   5},
   {  540,   5},
   {  595,   5},
   {  661,   5},
   {  743,   5},
   {  850,   5},
   {  991,   5},
   { 1190,   5},
   { 1487,   5},
   { 1983,   5},
   {    0,   0}
};
TuneDefinition tune_freeze = {
   "Frozen", 0X0000, tones_freeze
};

static ToneDefinition tones_unfreeze[] = {
   { 1487,   5},
   { 1190,   5},
   {  991,   5},
   {  850,   5},
   {  743,   5},
   {  661,   5},
   {  595,   5},
   {  540,   5},
   {  495,   5},
   {  457,   5},
   {  425,   5},
   {  396,   5},
   {  371,   5},
   {  350,   5},
   {  330,   5},
   {  313,   5},
   {  297,   5},
   {  283,   5},
   {  270,   5},
   {  258,   5},
   {  247,   5},
   {  238,   5},
   {    0,   0}
};
TuneDefinition tune_unfreeze = {
   "Unfrozen", 0X0000, tones_unfreeze
};

static ToneDefinition tones_cut_begin[] = {
   {  595,  40},
   { 1190,  20},
   {    0,   0}
};
TuneDefinition tune_cut_begin = {
   NULL, 0X0000, tones_cut_begin
};

static ToneDefinition tones_cut_end[] = {
   { 1190,  50},
   {  595,  30},
   {    0,   0}
};
TuneDefinition tune_cut_end = {
   NULL, 0X0000, tones_cut_end
};

static ToneDefinition tones_toggle_on[] = {
   {  595,  30},
   {    0,  30},
   {  793,  30},
   {    0,  30},
   { 1190,  40},
   {    0,   0}
};
TuneDefinition tune_toggle_on = {
   NULL, 0X0000, tones_toggle_on
};

static ToneDefinition tones_toggle_off[] = {
   { 1190,  30},
   {    0,  30},
   {  793,  30},
   {    0,  30},
   {  540,  30},
   {    0,   0}
};
TuneDefinition tune_toggle_off = {
   NULL, 0X0000, tones_toggle_off
};

static ToneDefinition tones_done[] = {
   {  595,  40},
   {    0,  30},
   {  595,  40},
   {    0,  40},
   {  595, 140},
   {    0,  20},
   {  793,  50},
   {    0,   0}
};
TuneDefinition tune_done = {
   "Done", 0X0000, tones_done
};

static ToneDefinition tones_skip_first[] = {
   {    0,  40},
   {  297,   4},
   {  396,   6},
   {  595,   8},
   {    0,  25},
   {    0,   0}
};
TuneDefinition tune_skip_first = {
   NULL, 0X20C3, tones_skip_first
};

static ToneDefinition tones_skip[] = {
   {  595,  10},
   {    0,  18},
   {    0,   0}
};
TuneDefinition tune_skip = {
   NULL, 0X0000, tones_skip
};

static ToneDefinition tones_skip_more[] = {
   {  566,  20},
   {    0,   1},
   {    0,   0}
};
TuneDefinition tune_skip_more = {
   NULL, 0X0000, tones_skip_more
};

static ToneGenerator *toneGenerator = NULL;
void setTuneDevice (unsigned char device) {
   if (toneGenerator)
      toneGenerator->close();
   switch (device) {
      case tdSpeaker:
         toneGenerator = toneSpeaker();
	 break;
      case tdSoundCard:
         toneGenerator = toneSoundCard();
	 break;
      case tdSequencer:
         toneGenerator = toneSequencer();
	 break;
      case tdAdLib:
         toneGenerator = toneAdLib();
	 break;
   }
}

void closeTuneDevice (void) {
   toneGenerator->close();
}
 
void playTune (TuneDefinition *tune) {
   int tunePlayed = 0;
   if (env.sound) {
      if (toneGenerator->open()) {
         ToneDefinition *tone = tune->tones;
	 tunePlayed = 1;
	 while (tone->duration) {
	    if (!toneGenerator->generate(tone->frequency, tone->duration)) {
	       tunePlayed = 0;
	       break;
	    }
	    ++tone;
	 }
      // toneGenerator->close();
      }
   }
   if (!tunePlayed) {
      if (tune->tactile) {
	 unsigned char dots = tune->tactile & 0XFF;
	 unsigned char duration = tune->tactile >> 8;
	 memset(statcells, dots, sizeof(statcells));
	 memset(brl.disp, dots, brl.x*brl.y);
	 braille->setstatus(statcells);
	 braille->write(&brl);
	 shortdelay(duration);
      }
      if (tune->message) {
	 message(tune->message, 0);
      }
   }
}
