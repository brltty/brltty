/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

#ifndef _SPK_DRIVER_H
#define _SPK_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* this header file is used to create the driver structure
 * for a dynamically loadable speech driver.
 * SPKNAME must be defined - see spkconf.h
 */

#include "spk.h"

/* Routines provided by this speech driver. */
static void spk_identify (void);
static void spk_open (char **parameters);
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

#ifdef SPKPARMS
  static const char *const spk_parameters[] = {SPKPARMS, NULL};
#endif /* SPKPARMS */

#ifndef SPKSYMBOL
#  define SPKSYMBOL CONCATENATE(spk_driver_,SPKDRIVER)
#endif /* SPKSYMBOL */
#ifndef SPKCONST
#  define SPKCONST const
#endif /* SPKCONST */
extern SPKCONST SpeechDriver SPKSYMBOL;
SPKCONST SpeechDriver SPKSYMBOL = {
  SPKNAME,
  STRINGIFY(SPKDRIVER),

  #ifdef SPKPARMS
    spk_parameters,
  #else
    NULL,
  #endif

  spk_identify,
  spk_open,
  spk_say,
  spk_mute,
  spk_close,

#ifdef SPK_HAVE_EXPRESS
  spk_express,
#else /* SPK_HAVE_EXPRESS */
  NULL,
#endif /* SPK_HAVE_EXPRESS */

  spk_doTrack,
  spk_getTrack,
  spk_isSpeaking
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SPK_DRIVER_H */
