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
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
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
 */

#include "spk.h"

/* Routines provided by this speech driver. */
static int spk_construct (SpeechSynthesizer *spk, char **parameters);
static void spk_destruct (SpeechSynthesizer *spk);
static void spk_say (SpeechSynthesizer *spk, const unsigned char *text, size_t length, size_t count, const unsigned char *attributes);
static void spk_mute (SpeechSynthesizer *spk);

#ifdef SPK_HAVE_TRACK
  static void spk_doTrack (SpeechSynthesizer *spk);
  static int spk_getTrack (SpeechSynthesizer *spk);
  static int spk_isSpeaking (SpeechSynthesizer *spk);
#else
  static void spk_doTrack (SpeechSynthesizer *spk) { }
  static int spk_getTrack (SpeechSynthesizer *spk) { return 0; }
  static int spk_isSpeaking (SpeechSynthesizer *spk) { return 0; }
#endif /* SPK_HAVE_TRACK */

#ifdef SPK_HAVE_RATE
  static void spk_rate (SpeechSynthesizer *spk, unsigned char setting);
#endif /* SPK_HAVE_RATE */

#ifdef SPK_HAVE_VOLUME
  static void spk_volume (SpeechSynthesizer *spk, unsigned char setting);
#endif /* SPK_HAVE_VOLUME */

#ifdef SPK_HAVE_PITCH
  static void spk_pitch (SpeechSynthesizer *spk, unsigned char setting);
#endif /* SPK_HAVE_PITCH */

#ifdef SPK_HAVE_PUNCTUATION
  static void spk_punctuation (SpeechSynthesizer *spk, SpeechPunctuation setting);
#endif /* SPK_HAVE_PUNCTUATION */

#ifdef SPKPARMS
  static const char *const spk_parameters[] = {SPKPARMS, NULL};
#endif /* SPKPARMS */

#ifndef SPKSYMBOL
#  define SPKSYMBOL CONCATENATE(spk_driver_,DRIVER_CODE)
#endif /* SPKSYMBOL */

#ifndef SPKCONST
#  define SPKCONST const
#endif /* SPKCONST */

extern SPKCONST SpeechDriver SPKSYMBOL;
SPKCONST SpeechDriver SPKSYMBOL = {
  DRIVER_DEFINITION_INITIALIZER,

#ifdef SPKPARMS
  spk_parameters,
#else /* SPKPARMS */
  NULL,
#endif /* SPKPARMS */

  spk_construct,
  spk_destruct,
  spk_say,
  spk_mute,

  spk_doTrack,
  spk_getTrack,
  spk_isSpeaking,

#ifdef SPK_HAVE_RATE
  spk_rate,
#else /* SPK_HAVE_RATE */
  NULL,
#endif /* SPK_HAVE_RATE */

#ifdef SPK_HAVE_VOLUME
  spk_volume,
#else /* SPK_HAVE_VOLUME */
  NULL,
#endif /* SPK_HAVE_VOLUME */

#ifdef SPK_HAVE_PITCH
  spk_pitch,
#else /* SPK_HAVE_PITCH */
  NULL,
#endif /* SPK_HAVE_PITCH */

#ifdef SPK_HAVE_PUNCTUATION
  spk_punctuation
#else /* SPK_HAVE_PUNCTUATION */
  NULL
#endif /* SPK_HAVE_PUNCTUATION */
};

DRIVER_VERSION_DECLARATION(spk);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SPK_DRIVER */
