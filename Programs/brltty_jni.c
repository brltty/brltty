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

#include <jni.h>
#include <stdio.h>
#include <stdarg.h>
#include <dlfcn.h>

#include "embed.h"

#define SYMBOL_POINTER(name) static const SYMBOL_TYPE(name) *name##_p = NULL;
SYMBOL_POINTER(brlttyConstruct);
SYMBOL_POINTER(brlttyUpdate);
SYMBOL_POINTER(brlttyDestruct);

typedef struct {
  const char *name;
  void *pointer;
} SymbolEntry;

#define BEGIN_SYMBOL_TABLE static const SymbolEntry symbolTable[] = {
#define END_SYMBOL_TABLE {.name=NULL} };
#define SYMBOL_ENTRY(symbol) {.name=#symbol, .pointer=&symbol##_p}

BEGIN_SYMBOL_TABLE
  SYMBOL_ENTRY(brlttyConstruct),
  SYMBOL_ENTRY(brlttyUpdate),
  SYMBOL_ENTRY(brlttyDestruct),
END_SYMBOL_TABLE

static void *coreHandle = NULL;

static jobject jArgumentArray = NULL;
static const char **cArgumentArray = NULL;
static int cArgumentCount;

static void reportProblem (
  JNIEnv *env, const char *throwable,
  const char *format, ...
) PRINTF(3, 4);

static void
reportProblem (
  JNIEnv *env, const char *throwable,
  const char *format, ...
) {
  char message[0X100];

  {
    va_list arguments;

    va_start(arguments, format);
    vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
  }

  if (0) {
    FILE *stream = stderr;

    fprintf(stream, "%s\n", message);
    fflush(stream);
  }

  {
    jclass object = (*env)->FindClass(env, throwable);

    if (object) {
      (*env)->ThrowNew(env, object, message);
      (*env)->DeleteLocalRef(env, object);
    }
  }
}

static void
reportOutOfMemory (JNIEnv *env, const char *description) {
  reportProblem(env, "java/lang/OutOfMemoryError", "cannot allocate %s", description);
}

static int
prepareProgramArguments (JNIEnv *env, jstring arguments) {
  jsize count = (*env)->GetArrayLength(env, arguments);

  if ((jArgumentArray = (*env)->NewGlobalRef(env, arguments))) {
    if ((cArgumentArray = malloc((count + 2) * sizeof(*cArgumentArray)))) {
      cArgumentArray[0] = PACKAGE_NAME;
      cArgumentArray[count+1] = NULL;

      {
        unsigned int i;

        for (i=1; i<=count; i+=1) cArgumentArray[i] = NULL;

        for (i=1; i<=count; i+=1) {
          jstring jArgument = (*env)->GetObjectArrayElement(env, arguments, i-1);
          jboolean isCopy;
          const char *cArgument = (*env)->GetStringUTFChars(env, jArgument, &isCopy);

          (*env)->DeleteLocalRef(env, jArgument);
          jArgument = NULL;

          if (!cArgument) {
            reportOutOfMemory(env, "C argument string");
            break;
          }

          cArgumentArray[i] = cArgument;
        }

        if (i > count) {
          cArgumentCount = count + 1;
          return 1;
        }
      }
    } else {
      reportOutOfMemory(env, "C argument array");
    }
  } else {
    reportOutOfMemory(env, "Java arguments array global reference");
  }

  return 0;
}

static int
loadCoreLibrary (JNIEnv *env) {
  if (coreHandle) return 1;

  if ((coreHandle = dlopen("libbrltty_core.so", RTLD_NOW | RTLD_GLOBAL))) {
    const SymbolEntry *symbol = symbolTable;

    while (symbol->name) {
      const void **pointer = symbol->pointer;

      if (!(*pointer = dlsym(coreHandle, symbol->name))) goto error;
      symbol += 1;
    }

    return 1;
  }

error:
  reportProblem(env, "java/lang/UnsatisfiedLinkError", "%s", dlerror());
  return 0;
}

JNIEXPORT jint JNICALL
Java_org_a11y_brltty_core_CoreWrapper_construct (JNIEnv *env, jobject this, jobjectArray arguments) {
  if (prepareProgramArguments(env, arguments)) {
    if (loadCoreLibrary(env)) {
      return brlttyConstruct_p(cArgumentCount, (char **)cArgumentArray);
    }
  }

  return PROG_EXIT_FATAL;
}

JNIEXPORT jboolean JNICALL
Java_org_a11y_brltty_core_CoreWrapper_update (JNIEnv *env, jobject this) {
  return brlttyUpdate_p()? JNI_TRUE: JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_org_a11y_brltty_core_CoreWrapper_destruct (JNIEnv *env, jobject this) {
  brlttyDestruct_p();

/*
  {
    const SymbolEntry *symbol = symbolTable;

    while (symbol->name) {
      const void **pointer = symbol->pointer;
      *pointer = NULL;
      symbol += 1;
    }
  }

  if (coreHandle) {
    dlclose(coreHandle);
    coreHandle = NULL;
  }
*/

  if (jArgumentArray) {
    if (cArgumentArray) {
      unsigned int i = 0;

      while (cArgumentArray[++i]) {
        jstring jArgument = (*env)->GetObjectArrayElement(env, jArgumentArray, i-1);
        (*env)->ReleaseStringUTFChars(env, jArgument, cArgumentArray[i]);
        (*env)->DeleteLocalRef(env, jArgument);
        jArgument = NULL;
      }

      free(cArgumentArray);
      cArgumentArray = NULL;
    }

    (*env)->DeleteGlobalRef(env, jArgumentArray);
    jArgumentArray = NULL;
  }
}
