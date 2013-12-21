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

#ifndef BRLTTY_INCLUDED_SPK
#define BRLTTY_INCLUDED_SPK

#include "spkdefs.h"
#include "driver.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct SpeechSynthesizerStruct SpeechSynthesizer;
typedef struct SpeechDataStruct SpeechData;

typedef void SpeechVolumeSetter (SpeechSynthesizer *spk, unsigned char setting);
typedef void SpeechRateSetter (SpeechSynthesizer *spk, unsigned char setting);
typedef void SpeechPitchSetter (SpeechSynthesizer *spk, unsigned char setting);
typedef void SpeechPunctuationSetter (SpeechSynthesizer *spk, SpeechPunctuation setting);

struct SpeechSynthesizerStruct {
  SpeechData *data;

  SpeechVolumeSetter *setVolume;
  SpeechRateSetter *setRate;
  SpeechPitchSetter *setPitch;
  SpeechPunctuationSetter *setPunctuation;
};

extern void initializeSpeechSynthesizer (SpeechSynthesizer *spk);

extern int startSpeechDriverThread (SpeechSynthesizer *spk, char **parameters);
extern void stopSpeechDriverThread (void);

extern int tellSpeechFinished (void);
extern void setSpeechFinished (void);

extern int tellSpeechIndex (int index);
extern void setSpeechIndex (int index);

extern int muteSpeech (const char *reason);

extern int sayUtf8Characters (
  const char *text, const unsigned char *attributes,
  size_t length, size_t count,
  int immediate
);

extern void sayString (const char *string, int immediate);

extern int canSetSpeechVolume (void);
extern int setSpeechVolume (int setting, int say);
extern unsigned int getIntegerSpeechVolume (unsigned char setting, unsigned int normal);
#ifndef NO_FLOAT
extern float getFloatSpeechVolume (unsigned char setting);
#endif /* NO_FLOAT */

extern int canSetSpeechRate (void);
extern int setSpeechRate (int setting, int say);
extern unsigned int getIntegerSpeechRate (unsigned char setting, unsigned int normal);
#ifndef NO_FLOAT
extern float getFloatSpeechRate (unsigned char setting);
#endif /* NO_FLOAT */

extern int canSetSpeechPitch (void);
extern int setSpeechPitch (int setting, int say);
extern unsigned int getIntegerSpeechPitch (unsigned char setting, unsigned int normal);
#ifndef NO_FLOAT
extern float getFloatSpeechPitch (unsigned char setting);
#endif /* NO_FLOAT */

extern int canSetSpeechPunctuation (void);
extern int setSpeechPunctuation (SpeechPunctuation setting, int say);

typedef struct {
  DRIVER_DEFINITION_DECLARATION;

  const char *const *parameters;

  int (*construct) (SpeechSynthesizer *spk, char **parameters);
  void (*destruct) (SpeechSynthesizer *spk);

  void (*say) (SpeechSynthesizer *spk, const unsigned char *text, size_t length, size_t count, const unsigned char *attributes);
  void (*mute) (SpeechSynthesizer *spk);
} SpeechDriver;

extern int haveSpeechDriver (const char *code);
extern const char *getDefaultSpeechDriver (void);
extern const SpeechDriver *loadSpeechDriver (const char *code, void **driverObject, const char *driverDirectory);
extern void identifySpeechDriver (const SpeechDriver *driver, int full);
extern void identifySpeechDrivers (int full);
extern const SpeechDriver *speech;
extern const SpeechDriver noSpeech;

extern int speechTracking;
extern int speechScreen;
extern int speechLine;
extern int speechIndex;
#define SPK_INDEX_NONE -1

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SPK */
