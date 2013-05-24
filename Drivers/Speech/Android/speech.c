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

#include "prologue.h"

#include <stdio.h>

#include "log.h"

#include "spk_driver.h"
#include "sys_android.h"

static JNIEnv *env = NULL;
static jclass speechDriverClass = NULL;

static int
findSpeechDriverClass (void) {
  return findJavaClass(env, &speechDriverClass, "org/a11y/brltty/android/SpeechDriver");
}

static int
spk_construct (SpeechSynthesizer *spk, char **parameters) {
  env = getJavaNativeInterface();

  if (findSpeechDriverClass()) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, speechDriverClass, "start",
                             JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                            ))) {
      jboolean result = (*env)->CallStaticBooleanMethod(env, speechDriverClass, method);

      if (!clearJavaException(env, 1)) {
        if (result == JNI_TRUE) {
          return 1;
        }
      }
    }
  }

  return 0;
}

static void
spk_destruct (SpeechSynthesizer *spk) {
  if (findSpeechDriverClass()) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, speechDriverClass, "stop",
                             JAVA_SIG_METHOD(JAVA_SIG_VOID,
                                            ))) {
      (*env)->CallStaticVoidMethod(env, speechDriverClass, method);
      clearJavaException(env, 1);
    }
  }
}

static void
spk_say (SpeechSynthesizer *spk, const unsigned char *buffer, size_t length, size_t count, const unsigned char *attributes) {
  if (findSpeechDriverClass()) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, speechDriverClass, "say",
                             JAVA_SIG_METHOD(JAVA_SIG_VOID,
                                             JAVA_SIG_OBJECT(java/lang/String) // text
                                            ))) {
      jstring string = (*env)->NewStringUTF(env, (const char *)buffer);
      if (string) {
        (*env)->CallStaticVoidMethod(env, speechDriverClass, method, string);
        clearJavaException(env, 1);
        (*env)->DeleteLocalRef(env, string);
      } else {
        logMallocError();
        clearJavaException(env, 0);
      }
    }
  }
}

static void
spk_mute (SpeechSynthesizer *spk) {
  if (findSpeechDriverClass()) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, speechDriverClass, "mute",
                             JAVA_SIG_METHOD(JAVA_SIG_VOID,
                                            ))) {
      (*env)->CallStaticVoidMethod(env, speechDriverClass, method);
      clearJavaException(env, 1);
    }
  }
}

#ifdef SPK_HAVE_TRACK
static void
spk_doTrack (SpeechSynthesizer *spk) {
}

static int
spk_getTrack (SpeechSynthesizer *spk) {
  return 0;
}

static int
spk_isSpeaking (SpeechSynthesizer *spk) {
  return 0;
}
#endif /* SPK_HAVE_TRACK */

#ifdef SPK_HAVE_VOLUME
static void
spk_setVolume (SpeechSynthesizer *spk, unsigned char setting) {
}
#endif /* SPK_HAVE_VOLUME */

#ifdef SPK_HAVE_RATE
static void
spk_setRate (SpeechSynthesizer *spk, unsigned char setting) {
}
#endif /* SPK_HAVE_RATE */

#ifdef SPK_HAVE_PITCH
static void
spk_setPitch (SpeechSynthesizer *spk, unsigned char setting) {
}
#endif /* SPK_HAVE_PITCH */

#ifdef SPK_HAVE_PUNCTUATION
static void
spk_setPunctuation (SpeechSynthesizer *spk, SpeechPunctuation setting) {
}
#endif /* SPK_HAVE_PUNCTUATION */
