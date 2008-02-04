/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#include <stdio.h>

#include "misc.h"

//#define SPK_HAVE_TRACK
//#define SPK_HAVE_RATE
//#define SPK_HAVE_VOLUME
#include "spk_driver.h"

static int
spk_construct (SpeechSynthesizer *spk, char **parameters) {
  return 0;
}

static void
spk_destruct (SpeechSynthesizer *spk) {
}

static void
spk_say (SpeechSynthesizer *spk, const unsigned char *buffer, int length, size_t count, const unsigned char *attributes) {
}

static void
spk_mute (SpeechSynthesizer *spk) {
}

#ifdef SPK_HAVE_TRACK
static void
spk_doTrack (SpeechSynthesizer *spk) {
}

static int
spk_getTrack (SpeechSynthesizer *spk) {
  return 0;
}

static int
spk_isSpeaking (SpeechSynthesizer *spk) {
  return 0;
}
#endif /* SPK_HAVE_TRACK */

#ifdef SPK_HAVE_RATE
static void
spk_rate (SpeechSynthesizer *spk, float setting) {
}
#endif /* SPK_HAVE_RATE */

#ifdef SPK_HAVE_VOLUME
static void
spk_volume (SpeechSynthesizer *spk, float setting) {
}
#endif /* SPK_HAVE_VOLUME */
