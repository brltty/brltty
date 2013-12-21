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
#include "system_java.h"

static JNIEnv *env = NULL;
static jclass speechDriverClass = NULL;

static int
findSpeechDriverClass (void) {
  return findJavaClass(env, &speechDriverClass, "org/a11y/brltty/android/SpeechDriver");
}

static void
spk_say (SpeechSynthesizer *spk, const unsigned char *buffer, size_t length, size_t count, const unsigned char *attributes) {
  if (findSpeechDriverClass()) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, speechDriverClass, "say",
                             JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                             JAVA_SIG_OBJECT(java/lang/String) // text
                                            ))) {
      jstring string = (*env)->NewStringUTF(env, (const char *)buffer);
      if (string) {
        jboolean result = (*env)->CallStaticBooleanMethod(env, speechDriverClass, method, string);

        (*env)->DeleteLocalRef(env, string);
        string = NULL;

        if (!clearJavaException(env, 1)) {
          if (result == JNI_TRUE) {
          }
        }
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
                             JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                            ))) {
      jboolean result = (*env)->CallStaticBooleanMethod(env, speechDriverClass, method);

      if (!clearJavaException(env, 1)) {
        if (result == JNI_TRUE) {
        }
      }
    }
  }
}

static void
spk_setVolume (SpeechSynthesizer *spk, unsigned char setting) {
  if (findSpeechDriverClass()) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, speechDriverClass, "setVolume",
                             JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                             JAVA_SIG_FLOAT // volume
                                            ))) {
      jboolean result = (*env)->CallStaticBooleanMethod(env, speechDriverClass, method, getFloatSpeechVolume(setting));

      if (!clearJavaException(env, 1)) {
        if (result == JNI_TRUE) {
        }
      }
    }
  }
}

static void
spk_setRate (SpeechSynthesizer *spk, unsigned char setting) {
  if (findSpeechDriverClass()) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, speechDriverClass, "setRate",
                             JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                             JAVA_SIG_FLOAT // rate
                                            ))) {
      jboolean result = (*env)->CallStaticBooleanMethod(env, speechDriverClass, method, getFloatSpeechRate(setting));

      if (!clearJavaException(env, 1)) {
        if (result == JNI_TRUE) {
        }
      }
    }
  }
}

static void
spk_setPitch (SpeechSynthesizer *spk, unsigned char setting) {
  if (findSpeechDriverClass()) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, speechDriverClass, "setPitch",
                             JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                             JAVA_SIG_FLOAT // pitch
                                            ))) {
      jboolean result = (*env)->CallStaticBooleanMethod(env, speechDriverClass, method, getFloatSpeechPitch(setting));

      if (!clearJavaException(env, 1)) {
        if (result == JNI_TRUE) {
        }
      }
    }
  }
}

static int
spk_construct (SpeechSynthesizer *spk, char **parameters) {
  env = getJavaNativeInterface();

  spk->setVolume = spk_setVolume;;
  spk->setRate = spk_setRate;;
  spk->setPitch = spk_setPitch;;

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
