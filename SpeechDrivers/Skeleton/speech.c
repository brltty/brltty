/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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

#include "Programs/misc.h"

//#define SPK_HAVE_EXPRESS
//#define SPK_HAVE_TRACK
//#define SPK_HAVE_RATE
//#define SPK_HAVE_VOLUME
#include "Programs/spk_driver.h"

static int
spk_open (char **parameters) {
  return 0;
}

static void
spk_close (void) {
}

static void
spk_say (const unsigned char *buffer, int length) {
}

static void
spk_mute (void) {
}

#ifdef SPK_HAVE_EXPRESS
static void
spk_express (const unsigned char *buffer, int length) {
}
#endif /* SPK_HAVE_EXPRESS */

#ifdef SPK_HAVE_TRACK
static void
spk_doTrack (void) {
}

static int
spk_getTrack (void) {
  return 0;
}

static int
spk_isSpeaking (void) {
  return 0;
}
#endif /* SPK_HAVE_TRACK */

#ifdef SPK_HAVE_RATE
static void
spk_rate (float setting) {
}
#endif /* SPK_HAVE_RATE */

#ifdef SPK_HAVE_VOLUME
static void
spk_volume (float setting) {
}
#endif /* SPK_HAVE_VOLUME */
