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

#include <stdio.h>
#include <jni.h>

#include "embed.h"

static jobject jProgramArguments = NULL;
static const char **cProgramArguments = NULL;

JNIEXPORT jint JNICALL
Java_brltty_construct (JNIEnv *env, jobject object, jobject argumentArray) {
  jint argumentCount = (*env)->GetArrayLength(env, argumentArray);

  jProgramArguments = argumentArray;
  cProgramArguments = malloc((argumentCount + 2) * sizeof(*cProgramArguments));

  cProgramArguments[0] = PACKAGE_NAME;
  cProgramArguments[argumentCount+1] = NULL;

  {
    unsigned int i;

    for (i=0; i<argumentCount; i+=1) {
      jstring jArgument = (*env)->GetObjectArrayElement(env, argumentArray, i);
      jboolean isCopy;
      cProgramArguments[i+1] = (*env)->GetStringUTFChars(env, jArgument, &isCopy);
    }
  }

  return brlttyConstruct(argumentCount+1, cProgramArguments);
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

printf("length=%d\n", (*env)->GetArrayLength(env, jProgramArguments)); fflush(stdout);
      while (cProgramArguments[++i]) {
printf("arg=%d\n", i); fflush(stdout);
        jstring jArgument = (*env)->GetObjectArrayElement(env, jProgramArguments, i-1);
printf("releasing\n"); fflush(stdout);
        (*env)->ReleaseStringUTFChars(env, jArgument, cProgramArguments[i]);
printf("released\n"); fflush(stdout);
      }

      free(cProgramArguments);
      cProgramArguments = NULL;
    }

    jProgramArguments = NULL;
  }
}
