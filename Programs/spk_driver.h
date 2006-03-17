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

#ifndef BRLTTY_INCLUDED_SPK_DRIVER
#define BRLTTY_INCLUDED_SPK_DRIVER

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* this header file is used to create the driver structure
 * for a dynamically loadable speech driver.
 * The following symbols must be defined (see driver make file):
 *    SPKNAME, SPKCODE, SPKCOMMENT, SPKVERSION, SPKCOPYRIGHT
 */

#include "spk.h"

/* Routines provided by this speech driver. */
static int spk_open (char **parameters);
static void spk_say (const unsigned char *buffer, int len);
static void spk_mute (void);
static void spk_close (void);

#ifdef SPK_HAVE_EXPRESS
  static void spk_express (const unsigned char *buffer, int len);
#endif /* SPK_HAVE_EXPRESS */

#ifdef SPK_HAVE_TRACK
  static void spk_doTrack (void);
  static int spk_getTrack (void);
  static int spk_isSpeaking (void);
#else
  static void spk_doTrack (void) { }
  static int spk_getTrack () { return 0; }
  static int spk_isSpeaking () { return 0; }
#endif /* SPK_HAVE_TRACK */

#ifdef SPK_HAVE_RATE
  static void spk_rate (float setting);		/* mute speech */
#endif /* SPK_HAVE_RATE */

#ifdef SPK_HAVE_VOLUME
  static void spk_volume (float setting);		/* mute speech */
#endif /* SPK_HAVE_VOLUME */

#ifdef SPKPARMS
  static const char *const spk_parameters[] = {SPKPARMS, NULL};
#endif /* SPKPARMS */

#ifndef SPKSYMBOL
#  define SPKSYMBOL CONCATENATE(spk_driver_,SPKCODE)
#endif /* SPKSYMBOL */

#ifndef SPKCONST
#  define SPKCONST const
#endif /* SPKCONST */

extern SPKCONST SpeechDriver SPKSYMBOL;
SPKCONST SpeechDriver SPKSYMBOL = {
  STRINGIFY(SPKNAME),
  STRINGIFY(SPKCODE),
  SPKCOMMENT,
  SPKVERSION,
  SPKCOPYRIGHT,
  __DATE__,
  __TIME__,

#ifdef SPKPARMS
  spk_parameters,
#else /* SPKPARMS */
  NULL,
#endif /* SPKPARMS */

  spk_open,
  spk_close,
  spk_say,
  spk_mute,

#ifdef SPK_HAVE_EXPRESS
  spk_express,
#else /* SPK_HAVE_EXPRESS */
  NULL,
#endif /* SPK_HAVE_EXPRESS */

  spk_doTrack,
  spk_getTrack,
  spk_isSpeaking,

#ifdef SPK_HAVE_RATE
  spk_rate,
#else /* SPK_HAVE_RATE */
  NULL,
#endif /* SPK_HAVE_RATE */

#ifdef SPK_HAVE_VOLUME
  spk_volume
#else /* SPK_HAVE_VOLUME */
  NULL
#endif /* SPK_HAVE_VOLUME */
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SPK_DRIVER */
