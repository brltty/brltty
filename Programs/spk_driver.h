/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#include "spk.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef SPKPARMS
static const char *const spk_parameters[] = {SPKPARMS, NULL};
#else /* SPKPARMS */
#define spk_parameters NULL
#endif /* SPKPARMS */

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

#ifdef SPK_HAVE_VOLUME
static void spk_setVolume (SpeechSynthesizer *spk, unsigned char setting);
#else /* SPK_HAVE_VOLUME */
#define spk_setVolume NULL
#endif /* SPK_HAVE_VOLUME */

#ifdef SPK_HAVE_RATE
static void spk_setRate (SpeechSynthesizer *spk, unsigned char setting);
#else /* SPK_HAVE_RATE */
#define spk_setRate NULL
#endif /* SPK_HAVE_RATE */

#ifdef SPK_HAVE_PITCH
static void spk_setPitch (SpeechSynthesizer *spk, unsigned char setting);
#else /* SPK_HAVE_PITCH */
#define spk_setPitch NULL
#endif /* SPK_HAVE_PITCH */

#ifdef SPK_HAVE_PUNCTUATION
static void spk_setPunctuation (SpeechSynthesizer *spk, SpeechPunctuation setting);
#else /* SPK_HAVE_PUNCTUATION */
#define spk_setPunctuation NULL
#endif /* SPK_HAVE_PUNCTUATION */

#ifndef SPKSYMBOL
#define SPKSYMBOL CONCATENATE(spk_driver_,DRIVER_CODE)
#endif /* SPKSYMBOL */

#ifndef SPKCONST
#define SPKCONST const
#endif /* SPKCONST */

extern SPKCONST SpeechDriver SPKSYMBOL;
SPKCONST SpeechDriver SPKSYMBOL = {
  DRIVER_DEFINITION_INITIALIZER,

  spk_parameters,

  spk_construct,
  spk_destruct,

  spk_say,
  spk_mute,

  spk_doTrack,
  spk_getTrack,
  spk_isSpeaking,

  spk_setVolume,
  spk_setRate,
  spk_setPitch,
  spk_setPunctuation
};

DRIVER_VERSION_DECLARATION(spk);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SPK_DRIVER */
