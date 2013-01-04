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

#include <pthread.h>

#include "log.h"
#include "system.h"
#include "sys_android.h"

#include "sys_prog_none.h"

#include "sys_boot_none.h"

#include "sys_exec_none.h"

#include "sys_mount_none.h"

#ifdef ENABLE_SHARED_OBJECTS
#define SHARED_OBJECT_LOAD_FLAGS (RTLD_NOW | RTLD_GLOBAL)
#include "sys_shlib_dlfcn.h"
#endif /* ENABLE_SHARED_OBJECTS */

#include "sys_beep_none.h"

#ifdef ENABLE_PCM_SUPPORT
#include "sys_pcm_none.h"
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
#include "sys_midi_none.h"
#endif /* ENABLE_MIDI_SUPPORT */

#include "sys_ports_none.h"

static JavaVM *javaVirtualMachine = NULL;

JNIEXPORT jint
JNI_OnLoad (JavaVM *vm, void *reserved) {
  javaVirtualMachine = vm;
  return ANDROID_JNI_VERSION;
}

JNIEXPORT void
JNI_OnUnload (JavaVM *vm, void *reserved) {
  javaVirtualMachine = NULL;
}

static pthread_once_t threadDetachHandlerOnce = PTHREAD_ONCE_INIT;
static pthread_key_t threadDetachHandlerKey;

static void
threadDetachHandler (void *arg) {
  JavaVM *vm = arg;
  (*vm)->DetachCurrentThread(vm);
}

static void
createThreadDetachHandlerKey (void) {
  pthread_key_create(&threadDetachHandlerKey, threadDetachHandler);
}

JavaVM *
getJavaInvocationInterface (void) {
  return javaVirtualMachine;
}

JNIEnv *
getJavaNativeInterface (void) {
  JNIEnv *env = NULL;
  JavaVM *vm = getJavaInvocationInterface();

  if (vm) {
    jint result = (*vm)->GetEnv(vm, (void **)&env, ANDROID_JNI_VERSION);

    if (result != JNI_OK) {
      if (result == JNI_EDETACHED) {
        JavaVMAttachArgs args = {
          .version = ANDROID_JNI_VERSION,
          .name = NULL,
          .group = NULL
        };

        if ((result = (*vm)->AttachCurrentThread(vm, &env, &args)) < 0) {
        } else {
          pthread_once(&threadDetachHandlerOnce, createThreadDetachHandlerKey);
          pthread_setspecific(threadDetachHandlerKey, vm);
        }
      } else {
      }
    }
  }

  return env;
}

int
clearJavaException (JNIEnv *env, int describe) {
  int exceptionOccurred = (*env)->ExceptionCheck(env);

  if (exceptionOccurred) {
    if (describe) (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }

  return exceptionOccurred;
}

int
findJavaClass (JNIEnv *env, jclass *class, const char *path) {
  if (*class) return 1;

  {
    jclass localReference = (*env)->FindClass(env, path);

    if (localReference) {
      jclass globalReference = (*env)->NewGlobalRef(env, localReference);

      (*env)->DeleteLocalRef(env, localReference);
      localReference = NULL;

      if (globalReference) {
        *class = globalReference;
        return 1;
      } else {
        logMallocError();
        clearJavaException(env, 0);
      }
    } else {
      logMessage(LOG_ERR, "java class not found: %s", path);
      clearJavaException(env, 0);
    }
  }

  return 0;
}

int
findJavaInstanceMethod (
  JNIEnv *env, jmethodID *method,
  jclass class, const char *name, const char *signature
) {
  if (!*method) {
    if (!(*method = (*env)->GetMethodID(env, class, name, signature))) {
      logMessage(LOG_ERR, "java instance method not found: %s: %s", name, signature);
      clearJavaException(env, 0);
      return 0;
    }
  }

  return 1;
}

int
findJavaStaticMethod (
  JNIEnv *env, jmethodID *method,
  jclass class, const char *name, const char *signature
) {
  if (!*method) {
    if (!(*method = (*env)->GetStaticMethodID(env, class, name, signature))) {
      logMessage(LOG_ERR, "java static method not found: %s: %s", name, signature);
      clearJavaException(env, 0);
      return 0;
    }
  }

  return 1;
}

int
findJavaConstructor (
  JNIEnv *env, jmethodID *constructor,
  jclass class, const char *signature
) {
  return findJavaInstanceMethod(env, constructor, class, "<init>", signature);
}

int
findJavaInstanceField (
  JNIEnv *env, jfieldID *field,
  jclass class, const char *name, const char *signature
) {
  if (!*field) {
    if (!(*field = (*env)->GetFieldID(env, class, name, signature))) {
      logMessage(LOG_ERR, "java instance field not found: %s: %s", name, signature);
      clearJavaException(env, 0);
      return 0;
    }
  }

  return 1;
}

int
findJavaStaticField (
  JNIEnv *env, jfieldID *field,
  jclass class, const char *name, const char *signature
) {
  if (!*field) {
    if (!(*field = (*env)->GetStaticFieldID(env, class, name, signature))) {
      logMessage(LOG_ERR, "java static field not found: %s: %s", name, signature);
      clearJavaException(env, 0);
      return 0;
    }
  }

  return 1;
}
