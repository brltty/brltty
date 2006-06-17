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
#include <math.h>

#include "Programs/misc.h"

//#define SPK_HAVE_EXPRESS
//#define SPK_HAVE_TRACK
#define SPK_HAVE_RATE
#define SPK_HAVE_VOLUME
#include "Programs/spk_driver.h"

#include <libspeechd.h>

static SPDConnection *connection = NULL;

static int
spk_open (char **parameters) {
  if ((connection = spd_open("brltty", "driver", NULL, SPD_MODE_THREADED))) {
    return 1;
  } else {
    LogPrint(LOG_ERR, "speech dispatcher open failure");
  }

  return 0;
}

static void
spk_close (void) {
  if (connection) {
    spd_close(connection);
    connection = NULL;
  }
}

static void
spk_say (const unsigned char *buffer, int length) {
  spd_sayf(connection, SPD_TEXT, "%.*s", length, buffer);
}

static void
spk_mute (void) {
  spd_cancel(connection);
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

static signed int
scaleSetting (float setting, float minimum, float maximum) {
  if (setting < minimum) {
    setting = minimum;
  } else if (setting > maximum) {
    setting = maximum;
  }
  return (signed int)((setting - minimum) * 200.0 / (maximum - minimum)) - 100;
}

static void
spk_rate (float setting) {
  signed int value;
  static float minimum;
  static float maximum;
  static char initialized = 0;

  if (!initialized) {
    maximum = logf(3.0);
    minimum = -maximum;
    initialized = 1;
  }

  value = scaleSetting(logf(setting), minimum, maximum);
  spd_set_voice_rate(connection, value);
  LogPrint(LOG_DEBUG, "set rate: %g -> %d", setting, value);
}

static void
spk_volume (float setting) {
  signed int value = scaleSetting(setting, 0.0, 2.0);
  spd_set_volume(connection, value);
  LogPrint(LOG_DEBUG, "set volume: %g -> %d", setting, value);
}
