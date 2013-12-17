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

#ifndef BRLTTY_INCLUDED_SPK_THREAD
#define BRLTTY_INCLUDED_SPK_THREAD

#include "spk.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef ENABLE_SPEECH_SUPPORT
typedef struct SpeechThreadObjectStruct SpeechThreadObject;

extern SpeechThreadObject *newSpeechThreadObject (SpeechSynthesizer *spk, char **parameters);
extern void destroySpeechThreadObject (SpeechThreadObject *obj);

extern int speechFunction_sayText (
  SpeechThreadObject *obj,
  const char *text,
  size_t length,
  size_t count,
  const unsigned char *attributes
);

extern int speechFunction_muteSpeech (
  SpeechThreadObject *obj
);

extern int speechFunction_doTrack (
  SpeechThreadObject *obj
);

extern int speechFunction_getTrack (
  SpeechThreadObject *obj
);

extern int speechFunction_isSpeaking (
  SpeechThreadObject *obj
);

extern int speechFunction_setVolume (
  SpeechThreadObject *obj,
  unsigned char setting
);

extern int speechFunction_setRate (
  SpeechThreadObject *obj,
  unsigned char setting
);

extern int speechFunction_setPitch (
  SpeechThreadObject *obj,
  unsigned char setting
);

extern int speechFunction_setPunctuation (
  SpeechThreadObject *obj,
  SpeechPunctuation setting
);
#endif /* ENABLE_SPEECH_SUPPORT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SPK_THREAD */
