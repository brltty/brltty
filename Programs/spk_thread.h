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
typedef struct SpeechDriverThreadStruct SpeechDriverThread;

extern SpeechDriverThread *newSpeechDriverThread (
  SpeechSynthesizer *spk,
  char **parameters
);

extern void destroySpeechDriverThread (
  SpeechDriverThread *sdt
);

extern int speechFunction_sayText (
  SpeechDriverThread *sdt,
  const char *text, size_t length,
  size_t count, const unsigned char *attributes
);

extern int speechFunction_muteSpeech (
  SpeechDriverThread *sdt
);

extern int speechFunction_doTrack (
  SpeechDriverThread *sdt
);

extern int speechFunction_getTrack (
  SpeechDriverThread *sdt
);

extern int speechFunction_isSpeaking (
  SpeechDriverThread *sdt
);

extern int speechFunction_setVolume (
  SpeechDriverThread *sdt,
  unsigned char setting
);

extern int speechFunction_setRate (
  SpeechDriverThread *sdt,
  unsigned char setting
);

extern int speechFunction_setPitch (
  SpeechDriverThread *sdt,
  unsigned char setting
);

extern int speechFunction_setPunctuation (
  SpeechDriverThread *sdt,
  SpeechPunctuation setting
);

extern int driverMessage_speechLocation (
  SpeechDriverThread *sdt,
  int index
);

extern int driverMessage_speechEnd (
  SpeechDriverThread *sdt
);
#endif /* ENABLE_SPEECH_SUPPORT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SPK_THREAD */
