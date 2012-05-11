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

JNIEXPORT jint JNICALL
Java_org_a11y_BRLTTY_Native_construct (JNIEnv *env, jobject arguments) {
  jint length = (*env)->GetArrayLength(env, arguments);
  printf("length=%d\n", length);
  return 2;
}

JNIEXPORT jboolean JNICALL
Java_org_a11y_BRLTTY_Native_update (JNIEnv *env, jobject this) {
  return brlttyUpdate()? JNI_TRUE: JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_org_a11y_BRLTTY_Native_destruct (JNIEnv *env, jobject this) {
  brlttyDestruct();
}
