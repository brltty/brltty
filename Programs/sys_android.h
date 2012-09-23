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

#ifndef BRLTTY_INCLUDED_SYS_ANDROID
#define BRLTTY_INCLUDED_SYS_ANDROID

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define ANDROID_JNI_VERSION JNI_VERSION_1_6

extern JavaVM *getJavaInvocationInterface (void);
extern JNIEnv *getJavaNativeInterface (void);
extern void clearJavaException (JNIEnv *env);

extern int findJavaClass (JNIEnv *env, jclass *class, const char *path);
extern int findJavaConstructor (JNIEnv *env, jmethodID *constructor, jclass class, const char *signature);
extern int findJavaMethod (JNIEnv *env, jmethodID *method, jclass class, const char *name, const char *signature);
extern int findJavaField (JNIEnv *env, jfieldID *field, jclass class, const char *name, const char *signature);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SYS_ANDROID */
