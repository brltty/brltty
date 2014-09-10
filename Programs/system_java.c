/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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
#include "system_java.h"

static JavaVM *javaVirtualMachine = NULL;

JNIEXPORT jint
JNI_OnLoad (JavaVM *vm, void *reserved) {
  javaVirtualMachine = vm;
  return JAVA_JNI_VERSION;
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
    jint result = (*vm)->GetEnv(vm, (void **)&env, JAVA_JNI_VERSION);

    if (result != JNI_OK) {
      if (result == JNI_EDETACHED) {
        JavaVMAttachArgs args = {
          .version = JAVA_JNI_VERSION,
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

static jobject javaClassLoaderInstance = NULL;
static jclass javaClassLoaderClass = NULL;
static jmethodID loadClassMethod = 0;

int
setJavaClassLoader (JNIEnv *env, jobject instance) {
  if (instance) {
    javaClassLoaderInstance = (*env)->NewGlobalRef(env, instance);

    if (javaClassLoaderInstance) {
      jclass class = (*env)->GetObjectClass(env, instance);

      if (class) {
        javaClassLoaderClass = (*env)->NewGlobalRef(env, class);

        (*env)->DeleteLocalRef(env, class);
        class = NULL;

        if (javaClassLoaderClass) {
          jmethodID method = (*env)->GetMethodID(env, javaClassLoaderClass, "loadClass",
                                                 JAVA_SIG_METHOD(JAVA_SIG_OBJECT(java/lang/Class),
                                                                 JAVA_SIG_OBJECT(java/lang/String) // className
                                                                ));

          if (method) {
            loadClassMethod = method;
            return 1;
          }

          (*env)->DeleteGlobalRef(env, javaClassLoaderClass);
        }
      }

      (*env)->DeleteGlobalRef(env, javaClassLoaderInstance);
    }
  }

  javaClassLoaderInstance = NULL;
  javaClassLoaderClass = NULL;
  loadClassMethod = 0;
  return 0;
}

static jclass
loadJavaClass (JNIEnv *env, const char *path) {
  size_t size = strlen(path) + 1;
  char cName[size];

  jclass class = NULL;
  jobject jName;

  {
    const char *p = path;
    char *n = cName;
    char c;

    do {
      c = *p++;
      if (c == '/') c = '.';
      *n++ = c;
    } while (c);
  }

  if ((jName = (*env)->NewStringUTF(env, cName))) {
    jclass result = (*env)->CallObjectMethod(env, javaClassLoaderInstance, loadClassMethod, jName);

    if (clearJavaException(env, 1)) {
      (*env)->DeleteLocalRef(env, result);
    } else {
      class = result;
    }

    (*env)->DeleteLocalRef(env, jName);
  } else {
    logMallocError();
  }

  return class;
}

int
findJavaClass (JNIEnv *env, jclass *class, const char *path) {
  if (*class) return 1;

  {
    jclass localReference = loadClassMethod?
                              loadJavaClass(env, path):
                              (*env)->FindClass(env, path);

    if (localReference) {
      jclass globalReference = (*env)->NewGlobalRef(env, localReference);

      (*env)->DeleteLocalRef(env, localReference);
      localReference = NULL;

      if (globalReference) {
        logMessage(LOG_DEBUG, "java class found: %s", path);
        *class = globalReference;
        return 1;
      } else {
        logMallocError();
        clearJavaException(env, 0);
      }
    } else {
      logMessage(LOG_ERR, "java class not found: %s", path);
      clearJavaException(env, 1);
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

    logMessage(LOG_DEBUG, "java instance method found: %s: %s", name, signature);
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

    logMessage(LOG_DEBUG, "java static method found: %s: %s", name, signature);
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

    logMessage(LOG_DEBUG, "java instance field found: %s: %s", name, signature);
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

    logMessage(LOG_DEBUG, "java static field found: %s: %s", name, signature);
  }

  return 1;
}

char *
getJavaLocaleName (void) {
  char *name = NULL;
  JNIEnv *env;

  if ((env = getJavaNativeInterface())) {
    jclass Locale_class = NULL;

    if (findJavaClass(env, &Locale_class, "java/util/Locale")) {
      jmethodID Locale_getDefault = 0;

      if (findJavaStaticMethod(env, &Locale_getDefault, Locale_class, "getDefault",
                               JAVA_SIG_METHOD(JAVA_SIG_OBJECT(java/util/Locale),
                                              ))) {
        jobject locale = (*env)->CallStaticObjectMethod(env, Locale_class, Locale_getDefault);

        if (!clearJavaException(env, 1)) {
          jmethodID Locale_toString = 0;

          if (findJavaInstanceMethod(env, &Locale_toString, Locale_class, "toString",
                                     JAVA_SIG_METHOD(JAVA_SIG_OBJECT(java/lang/String),
                                                    ))) {
            jstring jName = (*env)->CallObjectMethod(env, locale, Locale_toString);

            if (!clearJavaException(env, 1)) {
              jboolean isCopy;
              const char *cName = (*env)->GetStringUTFChars(env, jName, &isCopy);

              if (!(name = strdup(cName))) {
                logMallocError();
              }

              (*env)->ReleaseStringUTFChars(env, jName, cName);
              (*env)->DeleteLocalRef(env, jName);
            }
          }

          (*env)->DeleteLocalRef(env, locale);
        }
      }
    }
  }

  return name;
}

void
initializeSystemObject (void) {
}
