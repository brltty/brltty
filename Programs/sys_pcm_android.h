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

#include "log.h"
#include "sys_android.h"

struct PcmDeviceStruct {
  JNIEnv *env;
  jobject device;
};

static jclass pcmDeviceClass = NULL;

static int
findPcmDeviceClass (JNIEnv *env) {
  return findJavaClass(env, &pcmDeviceClass, "org/a11y/brltty/android/PcmDevice");
}

PcmDevice *
openPcmDevice (int errorLevel, const char *device) {
  PcmDevice *pcm = malloc(sizeof(*pcm));

  if (pcm) {
    memset(pcm, 0, sizeof(*pcm));
    pcm->env = getJavaNativeInterface();
    pcm->device = NULL;

    if (findPcmDeviceClass(pcm->env)) {
      static jmethodID constructor = 0;

      if (findJavaConstructor(pcm->env, &constructor, pcmDeviceClass,
                              JAVA_SIG_CONSTRUCTOR())) {
        jobject localReference = (*pcm->env)->NewObject(pcm->env, pcmDeviceClass, constructor);

        if (!clearJavaException(pcm->env, 1)) {
          jobject globalReference = (*pcm->env)->NewGlobalRef(pcm->env, localReference);

          (*pcm->env)->DeleteLocalRef(pcm->env, localReference);
          localReference = NULL;

          if (globalReference) {
            pcm->device = globalReference;
            return pcm;
          } else {
            logMallocError();
            clearJavaException(pcm->env, 0);
          }
        }
      }
    }

    free(pcm);
  } else {
    logMallocError();
  }

  return NULL;
}

void
closePcmDevice (PcmDevice *pcm) {
  if (pcm) {
    if (pcm->device) {
      if (findPcmDeviceClass(pcm->env)) {
        static jmethodID method = 0;

        if (findJavaInstanceMethod(pcm->env, &method, pcmDeviceClass, "close",
                                   JAVA_SIG_METHOD(JAVA_SIG_VOID,
                                                  ))) {
          (*pcm->env)->CallVoidMethod(pcm->env, pcm->device, method);
          clearJavaException(pcm->env, 1);
        }
      }

      (*pcm->env)->DeleteGlobalRef(pcm->env, pcm->device);
    }

    free(pcm);
  }
}

int
writePcmData (PcmDevice *pcm, const unsigned char *buffer, int count) {
  if (findPcmDeviceClass(pcm->env)) {
    static jmethodID method = 0;

    if (findJavaInstanceMethod(pcm->env, &method, pcmDeviceClass, "write",
                               JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                               JAVA_SIG_ARRAY(JAVA_SIG_SHORT) // samples
                                              ))) {
      jint size = count / 2;
      jshortArray jSamples = (*pcm->env)->NewShortArray(pcm->env, size);
      if (jSamples) {
        jshort cSamples[size];
        jboolean result;

        {
          const unsigned char *source = buffer;
          jshort *target = cSamples;
          jshort *end = target + size;

          while (target < end) {
            *target++ = (source[0] << 8) | source[1];
            source += 2;
          }
        }

        (*pcm->env)->SetShortArrayRegion(pcm->env, jSamples, 0, size, cSamples);
        result = (*pcm->env)->CallBooleanMethod(pcm->env, pcm->device, method, jSamples);
        (*pcm->env)->DeleteLocalRef(pcm->env, jSamples);

        if (!clearJavaException(pcm->env, 1)) {
          if (result == JNI_TRUE) {
            return 1;
          }
        }
      } else {
        logMallocError();
        clearJavaException(pcm->env, 0);
      }
    }
  }

  return 0;
}

int
getPcmBlockSize (PcmDevice *pcm) {
  if (findPcmDeviceClass(pcm->env)) {
    static jmethodID method = 0;

    if (findJavaInstanceMethod(pcm->env, &method, pcmDeviceClass, "getBufferSize",
                               JAVA_SIG_METHOD(JAVA_SIG_INT,
                                              ))) {
      jint result = (*pcm->env)->CallIntMethod(pcm->env, pcm->device, method);

      if (!clearJavaException(pcm->env, 1)) return result;
    }
  }

  return 0X100;
}

int
getPcmSampleRate (PcmDevice *pcm) {
  if (findPcmDeviceClass(pcm->env)) {
    static jmethodID method = 0;

    if (findJavaInstanceMethod(pcm->env, &method, pcmDeviceClass, "getSampleRate",
                               JAVA_SIG_METHOD(JAVA_SIG_INT,
                                              ))) {
      jint result = (*pcm->env)->CallIntMethod(pcm->env, pcm->device, method);

      if (!clearJavaException(pcm->env, 1)) return result;
    }
  }

  return 8000;
}

int
setPcmSampleRate (PcmDevice *pcm, int rate) {
  return getPcmSampleRate(pcm);
}

int
getPcmChannelCount (PcmDevice *pcm) {
  if (findPcmDeviceClass(pcm->env)) {
    static jmethodID method = 0;

    if (findJavaInstanceMethod(pcm->env, &method, pcmDeviceClass, "getChannelCount",
                               JAVA_SIG_METHOD(JAVA_SIG_INT,
                                              ))) {
      jint result = (*pcm->env)->CallIntMethod(pcm->env, pcm->device, method);

      if (!clearJavaException(pcm->env, 1)) return result;
    }
  }

  return 1;
}

int
setPcmChannelCount (PcmDevice *pcm, int channels) {
  return getPcmChannelCount(pcm);
}

PcmAmplitudeFormat
getPcmAmplitudeFormat (PcmDevice *pcm) {
  return PCM_FMT_S16B;
}

PcmAmplitudeFormat
setPcmAmplitudeFormat (PcmDevice *pcm, PcmAmplitudeFormat format) {
  return getPcmAmplitudeFormat(pcm);
}

void
forcePcmOutput (PcmDevice *pcm) {
}

void
awaitPcmOutput (PcmDevice *pcm) {
}

void
cancelPcmOutput (PcmDevice *pcm) {
}
