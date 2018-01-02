/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>

#include "log.h"

#include "spk_driver.h"
#include "system_java.h"

struct SpeechDataStruct {
  JNIEnv *env;
  jclass driverClass;
  jmethodID startMethod;
  jmethodID stopMethod;
  jmethodID sayMethod;
  jmethodID muteMethod;
  jmethodID setVolumeMethod;
  jmethodID setRateMethod;
  jmethodID setPitchMethod;
};

static int
findDriverClass (volatile SpeechSynthesizer *spk) {
  return findJavaClass(spk->driver.data->env, &spk->driver.data->driverClass, "org/a11y/brltty/android/SpeechDriver");
}

static void
releaseDriverClass (volatile SpeechSynthesizer *spk) {
  jclass class = spk->driver.data->driverClass;

  if (class) {
    JNIEnv *env = spk->driver.data->env;

    (*env)->DeleteGlobalRef(env, class);
  }
}

static int
findDriverMethod (volatile SpeechSynthesizer *spk, jmethodID *method, const char *name, const char *signature) {
  if (findDriverClass(spk)) {
    if (findJavaStaticMethod(spk->driver.data->env, method, spk->driver.data->driverClass, name, signature)) {
      return 1;
    }
  }

  return 0;
}

static void
spk_say (volatile SpeechSynthesizer *spk, const unsigned char *buffer, size_t length, size_t count, const unsigned char *attributes) {
  if (findDriverMethod(spk, &spk->driver.data->sayMethod, "say",
                       JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                       JAVA_SIG_OBJECT(java/lang/String) // text
                                      ))) {
    jstring string = (*spk->driver.data->env)->NewStringUTF(spk->driver.data->env, (const char *)buffer);
    if (string) {
      jboolean result = (*spk->driver.data->env)->CallStaticBooleanMethod(spk->driver.data->env, spk->driver.data->driverClass, spk->driver.data->sayMethod, string);

      (*spk->driver.data->env)->DeleteLocalRef(spk->driver.data->env, string);
      string = NULL;

      if (!clearJavaException(spk->driver.data->env, 1)) {
        if (result == JNI_TRUE) {
        }
      }
    } else {
      logMallocError();
      clearJavaException(spk->driver.data->env, 0);
    }
  }
}

static void
spk_mute (volatile SpeechSynthesizer *spk) {
  if (findDriverMethod(spk, &spk->driver.data->muteMethod, "mute",
                       JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                      ))) {
    jboolean result = (*spk->driver.data->env)->CallStaticBooleanMethod(spk->driver.data->env, spk->driver.data->driverClass, spk->driver.data->muteMethod);

    if (!clearJavaException(spk->driver.data->env, 1)) {
      if (result == JNI_TRUE) {
      }
    }
  }
}

static void
spk_setVolume (volatile SpeechSynthesizer *spk, unsigned char setting) {
  if (findDriverMethod(spk, &spk->driver.data->setVolumeMethod, "setVolume",
                       JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                       JAVA_SIG_FLOAT // volume
                                      ))) {
    jboolean result = (*spk->driver.data->env)->CallStaticBooleanMethod(spk->driver.data->env, spk->driver.data->driverClass, spk->driver.data->setVolumeMethod, getFloatSpeechVolume(setting));

    if (!clearJavaException(spk->driver.data->env, 1)) {
      if (result == JNI_TRUE) {
      }
    }
  }
}

static void
spk_setRate (volatile SpeechSynthesizer *spk, unsigned char setting) {
  if (findDriverMethod(spk, &spk->driver.data->setRateMethod, "setRate",
                       JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                       JAVA_SIG_FLOAT // rate
                                      ))) {
    jboolean result = (*spk->driver.data->env)->CallStaticBooleanMethod(spk->driver.data->env, spk->driver.data->driverClass, spk->driver.data->setRateMethod, getFloatSpeechRate(setting));

    if (!clearJavaException(spk->driver.data->env, 1)) {
      if (result == JNI_TRUE) {
      }
    }
  }
}

static void
spk_setPitch (volatile SpeechSynthesizer *spk, unsigned char setting) {
  if (findDriverMethod(spk, &spk->driver.data->setPitchMethod, "setPitch",
                       JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                       JAVA_SIG_FLOAT // pitch
                                      ))) {
    jboolean result = (*spk->driver.data->env)->CallStaticBooleanMethod(spk->driver.data->env, spk->driver.data->driverClass, spk->driver.data->setPitchMethod, getFloatSpeechPitch(setting));

    if (!clearJavaException(spk->driver.data->env, 1)) {
      if (result == JNI_TRUE) {
      }
    }
  }
}

static int
spk_construct (volatile SpeechSynthesizer *spk, char **parameters) {
  if ((spk->driver.data = malloc(sizeof(*spk->driver.data)))) {
    memset(spk->driver.data, 0, sizeof(*spk->driver.data));

    spk->driver.data->env = getJavaNativeInterface();
    spk->driver.data->driverClass = NULL;
    spk->driver.data->startMethod = 0;
    spk->driver.data->stopMethod = 0;
    spk->driver.data->sayMethod = 0;
    spk->driver.data->muteMethod = 0;
    spk->driver.data->setVolumeMethod = 0;
    spk->driver.data->setRateMethod = 0;
    spk->driver.data->setPitchMethod = 0;

    spk->setVolume = spk_setVolume;
    spk->setRate = spk_setRate;
    spk->setPitch = spk_setPitch;

    if (spk->driver.data->env) {
      if (findDriverMethod(spk, &spk->driver.data->startMethod, "start",
                           JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                          ))) {
        jboolean result = (*spk->driver.data->env)->CallStaticBooleanMethod(spk->driver.data->env, spk->driver.data->driverClass, spk->driver.data->startMethod);

        if (!clearJavaException(spk->driver.data->env, 1)) {
          if (result == JNI_TRUE) {
            return 1;
          }
        }
      }
    }

    releaseDriverClass(spk);
    free(spk->driver.data);
  } else {
    logMallocError();
  }

  return 0;
}

static void
spk_destruct (volatile SpeechSynthesizer *spk) {
  if (findDriverMethod(spk, &spk->driver.data->stopMethod, "stop",
                       JAVA_SIG_METHOD(JAVA_SIG_VOID,
                                      ))) {
    (*spk->driver.data->env)->CallStaticVoidMethod(spk->driver.data->env, spk->driver.data->driverClass, spk->driver.data->stopMethod);
    clearJavaException(spk->driver.data->env, 1);
  }

  releaseDriverClass(spk);
  free(spk->driver.data);
  spk->driver.data = NULL;
}
