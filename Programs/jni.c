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

#include "embed.h"
#include "log.h"

static jobject jProgramArguments = NULL;
static const char **cProgramArguments = NULL;

JNIEXPORT jint JNICALL
Java_brltty_construct (JNIEnv *env, jobject object, jobjectArray arguments) {
  jsize count = (*env)->GetArrayLength(env, arguments);

  if ((jProgramArguments = (*env)->NewGlobalRef(env, arguments))) {
    if ((cProgramArguments = malloc((count + 2) * sizeof(*cProgramArguments)))) {
      cProgramArguments[0] = PACKAGE_NAME;
      cProgramArguments[count+1] = NULL;

      {
        unsigned int i;

        for (i=1; i<=count; i+=1) cProgramArguments[i] = NULL;

        for (i=1; i<=count; i+=1) {
          jstring jArgument = (*env)->GetObjectArrayElement(env, arguments, i-1);
          jboolean isCopy;
          const char *cArgument = (*env)->GetStringUTFChars(env, jArgument, &isCopy);

          (*env)->DeleteLocalRef(env, jArgument);
          jArgument = NULL;

          if (!cArgument) {
            logMallocError();
            break;
          }

          cProgramArguments[i] = cArgument;
        }

        if (i > count) return brlttyConstruct(count+1, (char **)cProgramArguments);
      }
    } else {
      logMallocError();
    }
  } else {
    logMallocError();
  }

  return PROG_EXIT_FATAL;
}

JNIEXPORT jboolean JNICALL
Java_brltty_update (JNIEnv *env, jobject object) {
  return brlttyUpdate()? JNI_TRUE: JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_brltty_destruct (JNIEnv *env, jobject object) {
  brlttyDestruct();

  if (jProgramArguments) {
    if (cProgramArguments) {
      unsigned int i = 0;

      while (cProgramArguments[++i]) {
        jstring jArgument = (*env)->GetObjectArrayElement(env, jProgramArguments, i-1);
        (*env)->ReleaseStringUTFChars(env, jArgument, cProgramArguments[i]);
        (*env)->DeleteLocalRef(env, jArgument);
        jArgument = NULL;
      }

      free(cProgramArguments);
      cProgramArguments = NULL;
    }

    (*env)->DeleteGlobalRef(env, jProgramArguments);
    jProgramArguments = NULL;
  }
}
