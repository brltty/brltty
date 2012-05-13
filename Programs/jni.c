/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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
#include <dlfcn.h>
#include <stdio.h>

#include "embed.h"
#include "log.h"

#define FUNCTION_POINTER(name) static FUNCTION_TYPE(name) *name##_p = NULL;
FUNCTION_POINTER(brlttyConstruct);
FUNCTION_POINTER(brlttyUpdate);
FUNCTION_POINTER(brlttyDestruct);

typedef struct {
  const char *symbolName;
  void *pointerAddress;
} SymbolEntry;

#define BEGIN_SYMBOL_TABLE static const SymbolEntry symbolTable[] = {
#define END_SYMBOL_TABLE {.symbolName=NULL} };
#define SYMBOL_ENTRY(symbol) {.symbolName=#symbol, .pointerAddress=&symbol##_p}

BEGIN_SYMBOL_TABLE
  SYMBOL_ENTRY(brlttyConstruct),
  SYMBOL_ENTRY(brlttyUpdate),
  SYMBOL_ENTRY(brlttyDestruct),
END_SYMBOL_TABLE

static void *coreHandle = NULL;

static jobject jArgumentArray = NULL;
static const char **cArgumentArray = NULL;
static int cArgumentCount;

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
//          logMallocError_p();
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
//    logMallocError_p();
    }
  } else {
//  logMallocError_p();
  }

  return 0;
}

static int
loadCoreLibrary (void) {
  if ((coreHandle = dlopen("libbrltty.so", RTLD_NOW | RTLD_GLOBAL))) {
    const SymbolEntry *symbol = symbolTable;

    while (symbol->symbolName) {
      void **pointer = symbol->pointerAddress;

      if (!(*pointer = dlsym(coreHandle, symbol->symbolName))) {
        return 0;
      }

      symbol += 1;
    }

    return 1;
  }

  return 0;
}

JNIEXPORT jint JNICALL
Java_brltty_construct (JNIEnv *env, jobject object, jobjectArray arguments) {
  if (prepareProgramArguments(env, arguments)) {
    if (loadCoreLibrary()) {
      return brlttyConstruct_p(cArgumentCount, (char **)cArgumentArray);
    }
  }

  return PROG_EXIT_FATAL;
}

JNIEXPORT jboolean JNICALL
Java_brltty_update (JNIEnv *env, jobject object) {
  return brlttyUpdate_p()? JNI_TRUE: JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_brltty_destruct (JNIEnv *env, jobject object) {
  brlttyDestruct_p();

  if (coreHandle) {
    dlclose(coreHandle);
    coreHandle = NULL;
  }

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
