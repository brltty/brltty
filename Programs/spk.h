/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_SPK
#define BRLTTY_INCLUDED_SPK

#include "spk_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void constructSpeechSynthesizer (volatile SpeechSynthesizer *spk);
extern void destructSpeechSynthesizer (volatile SpeechSynthesizer *spk);

extern int startSpeechDriverThread (char **parameters);
extern void stopSpeechDriverThread (void);

extern int muteSpeech (const char *reason);

extern int sayUtf8Characters (
  const char *text, const unsigned char *attributes,
  size_t length, size_t count,
  SayOptions options
);

extern int sayWideCharacters (
  const wchar_t *characters, const unsigned char *attributes,
  size_t count, SayOptions options
);
extern void sayString (const char *string, SayOptions);

extern int canSetSpeechVolume (void);
extern int setSpeechVolume (int setting, int say);

extern int canSetSpeechRate (void);
extern int setSpeechRate (int setting, int say);

extern int canSetSpeechPitch (void);
extern int setSpeechPitch (int setting, int say);

extern int canSetSpeechPunctuation (void);
extern int setSpeechPunctuation (SpeechPunctuation setting, int say);

extern int haveSpeechDriver (const char *code);
extern const char *getDefaultSpeechDriver (void);
extern const SpeechDriver *loadSpeechDriver (const char *code, void **driverObject, const char *driverDirectory);
extern void identifySpeechDriver (const SpeechDriver *driver, int full);
extern void identifySpeechDrivers (int full);
extern const SpeechDriver *speech;
extern const SpeechDriver noSpeech;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SPK */
