/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2026 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 */

/* MacOS/speech.m - macOS speech driver, backed by AVSpeechSynthesizer. */

#include "prologue.h"

#include <stdio.h>
#include <string.h>

#include "log.h"

typedef enum {
  PARM_VOICE,
} DriverParameter;
#define SPKPARMS "voice"

#include "spk_driver.h"

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

static AVSpeechSynthesizer *synthesizer = nil;
static AVSpeechSynthesisVoice *currentVoice = nil;

static float currentVolume = 1.0f;
static float currentPitch  = 1.0f;
static float currentRate   = 0.0f; /* initialised in spk_construct */

static AVSpeechSynthesisVoice *
resolveVoice (const char *spec) {
  if (!spec || !*spec) return nil;

  NSString *s = [NSString stringWithUTF8String:spec];
  if (!s) return nil;

  AVSpeechSynthesisVoice *voice = [AVSpeechSynthesisVoice voiceWithIdentifier:s];
  if (voice) return voice;

  voice = [AVSpeechSynthesisVoice voiceWithLanguage:s];
  if (voice) return voice;

  for (AVSpeechSynthesisVoice *candidate in [AVSpeechSynthesisVoice speechVoices]) {
    if ([[candidate name] caseInsensitiveCompare:s] == NSOrderedSame) {
      return candidate;
    }
  }

  logMessage(LOG_WARNING, "MacOS: voice not found: %s", spec);
  return nil;
}

static void
spk_say (SpeechSynthesizer *spk, const unsigned char *buffer, size_t length, size_t count, const unsigned char *attributes) {
  if (!synthesizer || length == 0) return;

  @autoreleasepool {
    NSString *text = [[[NSString alloc] initWithBytes:buffer
                                               length:length
                                             encoding:NSUTF8StringEncoding] autorelease];
    if (!text) {
      text = [[[NSString alloc] initWithBytes:buffer
                                       length:length
                                     encoding:NSISOLatin1StringEncoding] autorelease];
    }
    if (!text || [text length] == 0) return;

    AVSpeechUtterance *utterance = [[[AVSpeechUtterance alloc] initWithString:text] autorelease];
    utterance.rate = currentRate;
    utterance.volume = currentVolume;
    utterance.pitchMultiplier = currentPitch;
    if (currentVoice) utterance.voice = currentVoice;

    [synthesizer speakUtterance:utterance];
  }
}

static void
spk_mute (SpeechSynthesizer *spk) {
  if (!synthesizer) return;
  [synthesizer stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
}

static void
spk_setVolume (SpeechSynthesizer *spk, unsigned char setting) {
  /* Brltty volume: 0..20, 10 = normal. AVSpeech: 0.0..1.0, default 1.0. */
  currentVolume = (float)getIntegerSpeechVolume(setting, 100) / 100.0f;
  if (currentVolume < 0.0f) currentVolume = 0.0f;
  if (currentVolume > 1.0f) currentVolume = 1.0f;
}

static void
spk_setRate (SpeechSynthesizer *spk, unsigned char setting) {
  /* getFloatSpeechRate: ~0.33 .. 3.0, 1.0 = normal.
   * Map onto AVSpeechUtterance{Min,Default,Max}SpeechRate. */
  float factor = getFloatSpeechRate(setting);
  float rate;

  if (factor <= 1.0f) {
    float low = AVSpeechUtteranceMinimumSpeechRate;
    float mid = AVSpeechUtteranceDefaultSpeechRate;
    rate = low + (mid - low) * factor;
  } else {
    float mid  = AVSpeechUtteranceDefaultSpeechRate;
    float high = AVSpeechUtteranceMaximumSpeechRate;
    float t = (factor - 1.0f) / 2.0f; /* 1.0..3.0 -> 0..1 */
    if (t > 1.0f) t = 1.0f;
    rate = mid + (high - mid) * t;
  }

  if (rate < AVSpeechUtteranceMinimumSpeechRate) rate = AVSpeechUtteranceMinimumSpeechRate;
  if (rate > AVSpeechUtteranceMaximumSpeechRate) rate = AVSpeechUtteranceMaximumSpeechRate;
  currentRate = rate;
}

static void
spk_setPitch (SpeechSynthesizer *spk, unsigned char setting) {
  /* getFloatSpeechPitch is centered at 1.0; AVSpeech pitchMultiplier range 0.5..2.0. */
  float pitch = getFloatSpeechPitch(setting);
  if (pitch < 0.5f) pitch = 0.5f;
  if (pitch > 2.0f) pitch = 2.0f;
  currentPitch = pitch;
}

static int
spk_construct (SpeechSynthesizer *spk, char **parameters) {
  @autoreleasepool {
    currentRate = AVSpeechUtteranceDefaultSpeechRate;
    synthesizer = [[AVSpeechSynthesizer alloc] init];
    if (!synthesizer) {
      logMessage(LOG_ERR, "MacOS: could not create AVSpeechSynthesizer");
      return 0;
    }

    const char *voiceSpec = parameters[PARM_VOICE];
    AVSpeechSynthesisVoice *voice = resolveVoice(voiceSpec);
    if (voice) {
      currentVoice = [voice retain];
    } else {
      AVSpeechSynthesisVoice *fallback =
        [AVSpeechSynthesisVoice voiceWithLanguage:[AVSpeechSynthesisVoice currentLanguageCode]];
      if (fallback) currentVoice = [fallback retain];
    }

    spk->setVolume = spk_setVolume;
    spk->setRate   = spk_setRate;
    spk->setPitch  = spk_setPitch;

    const char *voiceId = currentVoice ? [[currentVoice identifier] UTF8String] : NULL;
    logMessage(LOG_INFO, "MacOS: AVSpeechSynthesizer ready (voice: %s)",
               voiceId ? voiceId : "default");
  }

  return 1;
}

static void
spk_destruct (SpeechSynthesizer *spk) {
  if (synthesizer) {
    [synthesizer stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
    [synthesizer release];
    synthesizer = nil;
  }

  if (currentVoice) {
    [currentVoice release];
    currentVoice = nil;
  }
}
