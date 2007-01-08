/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2007 by
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 *   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 * All rights reserved.
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 * Please see the file COPYING-API for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include <jni.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define BRLAPI_NO_DEPRECATED
#define BRLAPI_NO_SINGLE_SESSION
#include "Programs/brlapi.h"

#define ERR_NULLPTR 0
#define ERR_OUTOFMEM 1
#define ERR_INDEX 2

/* TODO: threads */
static JNIEnv *env;

static void ThrowException(JNIEnv *jenv, int code, const char *msg) {
  jclass excep;
  const char *exception;

  switch (code) {
    case ERR_NULLPTR:  exception = "java/lang/NullPointerException";      break;
    case ERR_OUTOFMEM: exception = "java/lang/OutOfMemoryError";          break;
    case ERR_INDEX:    exception = "java/lang/IndexOutOfBoundsException"; break;
    default:           exception = "java/lang/UnknownError";              break;
  }

  (*jenv)->ExceptionClear(jenv);
  excep = (*jenv)->FindClass(jenv, exception);
  if (excep)
    (*jenv)->ThrowNew(jenv, excep, msg);
  else
    fprintf(stderr,"couldn't find exception %s !\n",exception);
}

static void ThrowError(JNIEnv *jenv, const char *msg) {
  const char *error = brlapi_strerror(&brlapi_error);
  int lenmsg = strlen(msg);
  int lenerr = strlen(error);
  jclass jcexcep;
  jmethodID jinit;
  jthrowable jexcep;
  jstring errfun = NULL;

  {
    char message[lenmsg + 2 + lenerr + 1];
    snprintf(message, sizeof(message), "%s: %s", msg, error);

    if (!(jcexcep = (*jenv)->FindClass(jenv, "BrlapiError"))) {
      ThrowException(jenv, ERR_NULLPTR, "ThrowBrlapiErrorFindClass");
      return;
    }
    if (!(jinit = (*jenv)->GetMethodID(jenv, jcexcep, "<init>", "(IIILjava/lang/String;)V"))) {
      ThrowException(jenv, ERR_NULLPTR, "ThrowBrlapiErrorGetMethodID");
      return;
    }
    if (brlapi_errfun)
      errfun = (*jenv)->NewStringUTF(jenv, brlapi_errfun);
    if (!(jexcep = (*jenv)->NewObject(jenv, jcexcep, jinit, brlapi_errno, brlapi_libcerrno, brlapi_gaierrno, errfun))) {
      ThrowException(jenv, ERR_NULLPTR, "ThrowBrlapiErrorNewObject");
      return;
    }
    (*jenv)->ExceptionClear(jenv);
    (*jenv)->Throw(jenv, jexcep);
  }
}

static void exceptionHandler(brlapi_handle_t *handle, int err, brlapi_type_t type, const void *buf, size_t size) {
  jarray jbuf;
  jclass jcexcep;
  jmethodID jinit;
  jthrowable jexcep;

  if (!(jbuf = (*env)->NewByteArray(env, size))) {
    ThrowException(env, ERR_OUTOFMEM, __func__);
    return;
  }
  (*env)->SetByteArrayRegion(env, jbuf, 0, size, (jbyte *) buf);

  if (!(jcexcep = (*env)->FindClass(env, "BrlapiException"))) {
    ThrowException(env, ERR_NULLPTR, "exceptionHandlerFindClass");
    return;
  }
  if (!(jinit = (*env)->GetMethodID(env, jcexcep, "<init>", "(JII[B)V"))) {
    ThrowException(env, ERR_NULLPTR, "exceptionHandlerGetMethodID");
    return;
  }
  if (!(jexcep = (*env)->NewObject(env, jcexcep, jinit, (jlong)(intptr_t) handle, err, type, jbuf))) {
    ThrowException(env, ERR_NULLPTR, "exceptionHandlerNewObject");
    return;
  }
  (*env)->ExceptionClear(env);
  (*env)->Throw(env, jexcep);
  return;
}

#define GET_CLASS(jenv, class, obj, ret) \
  if (!((class) = (*(jenv))->GetObjectClass((jenv), (obj)))) { \
    ThrowException((jenv), ERR_NULLPTR, #obj " -> " #class); \
    return ret; \
  }
#define GET_ID(jenv, id, class, field, sig, ret) \
  if (!((id) = (*(jenv))->GetFieldID((jenv), (class), (field), (sig)))) {\
    ThrowException((jenv), ERR_NULLPTR, #class "." field); \
    return ret; \
  }
#define GET_HANDLE(jenv, jobj, ret) \
  brlapi_handle_t *handle; \
  jclass jcls; \
  jfieldID handleID; \
  GET_CLASS(jenv, jcls, jobj, ret); \
  GET_ID(jenv, handleID, jcls, "handle", "J", ret); \
  handle = (void*) (intptr_t) (*jenv)->GetLongField(jenv, jcls, handleID); \
  if (!handle) { \
    ThrowException((jenv), ERR_NULLPTR, "connection has been closed"); \
    return ret; \
  }

JNIEXPORT jint JNICALL Java_BrlapiNative_openConnection(JNIEnv *jenv, jobject jobj, jobject JclientSettings , jobject JusedSettings) {
  jclass jcclientSettings, jcusedSettings;
  jfieldID clientAuthID = NULL, clientHostID = NULL, usedAuthID, usedHostID;
  brlapi_connectionSettings_t clientSettings,  usedSettings,
            *PclientSettings, *PusedSettings;
  int result;
  jstring auth = NULL, host = NULL;
  const char *str;
  jfieldID handleID;
  brlapi_handle_t *handle;
  jclass jcls;

  GET_CLASS(jenv, jcls, jobj, -1);
  GET_ID(jenv, handleID, jcls, "handle", "J", -1);
  handle = malloc(brlapi_getHandleSize());
  if (!handle) {
    ThrowException(jenv, ERR_OUTOFMEM, __func__);
    return -1;
  }
  (*jenv)->SetLongField(jenv, jcls, handleID, (jlong) (intptr_t) handle);

  env = jenv;

  if (JclientSettings) {
    GET_CLASS(jenv, jcclientSettings, JclientSettings, -1);
    GET_ID(jenv, clientAuthID, jcclientSettings, "auth", "Ljava/lang/String;", -1);
    GET_ID(jenv, clientHostID, jcclientSettings, "host", "Ljava/lang/String;", -1);

    PclientSettings = &clientSettings;
    if ((auth = (*jenv)->GetObjectField(jenv, JclientSettings, clientAuthID))) {
      if (!(clientSettings.auth = (char *)(*jenv)->GetStringUTFChars(jenv, auth, NULL))) {
	ThrowException(jenv, ERR_OUTOFMEM, __func__);
	return -1;
      }
    } else clientSettings.auth = NULL;
    if ((host = (*jenv)->GetObjectField(jenv, JclientSettings, clientHostID))) {
      if (!(clientSettings.host = (char *)(*jenv)->GetStringUTFChars(jenv, host, NULL))) {
	ThrowException(jenv, ERR_OUTOFMEM, __func__);
	return -1;
      }
    } else clientSettings.host = NULL;
  } else PclientSettings = NULL;

  if (JusedSettings)
    PusedSettings = &usedSettings;
  else
    PusedSettings = NULL;

  if ((result = brlapi__openConnection(handle, PclientSettings, PusedSettings)) < 0) {
    ThrowError(jenv, __func__);
    return -1;
  }

  (void) brlapi__setExceptionHandler(handle, exceptionHandler);

  if (JclientSettings) {
    if (clientSettings.auth)
      (*jenv)->ReleaseStringUTFChars(jenv, auth,  clientSettings.auth); 
    if (clientSettings.host)
      (*jenv)->ReleaseStringUTFChars(jenv, host, clientSettings.host); 
  }

  if (PusedSettings) {
    GET_CLASS(jenv, jcusedSettings, JusedSettings, -1);
    GET_ID(jenv, usedAuthID, jcusedSettings, "auth", "Ljava/lang/String;", -1);
    GET_ID(jenv, usedHostID, jcusedSettings, "host", "Ljava/lang/String;", -1);

    auth = (*jenv)->NewStringUTF(jenv, usedSettings.auth);
    if (!auth) {
      ThrowException(jenv, ERR_OUTOFMEM, __func__);
      return -1;
    }
    str = (*jenv)->GetStringUTFChars(jenv, auth, NULL);
    (*jenv)->SetObjectField(jenv, JusedSettings, clientAuthID, auth);
    (*jenv)->ReleaseStringUTFChars(jenv, auth, str);

    host = (*jenv)->NewStringUTF(jenv, usedSettings.host);
    if (!host) {
      ThrowException(jenv, ERR_OUTOFMEM, __func__);
      return -1;
    }
    str = (*jenv)->GetStringUTFChars(jenv, host, NULL);
    (*jenv)->SetObjectField(jenv, JusedSettings, clientHostID, host);
    (*jenv)->ReleaseStringUTFChars(jenv, host, str);
  }

  return (jint) result;
}

JNIEXPORT void JNICALL Java_BrlapiNative_closeConnection(JNIEnv *jenv, jobject jobj) {
  env = jenv;
  GET_HANDLE(jenv, jobj, );

  brlapi__closeConnection(handle);
  free((void*) (intptr_t) handle);
  (*jenv)->SetLongField(jenv, jcls, handleID, (jlong) (intptr_t) NULL);
}

JNIEXPORT jstring JNICALL Java_BrlapiNative_getDriverId(JNIEnv *jenv, jobject jobj) {
  char id[3];
  GET_HANDLE(jenv, jobj, NULL);

  env = jenv;

  if (brlapi__getDriverId(handle, id, sizeof(id)) < 0) {
    ThrowError(jenv, __func__);
    return NULL;
  }

  id[sizeof(id)-1] = 0;
  return (*jenv)->NewStringUTF(jenv, id);
}

JNIEXPORT jstring JNICALL Java_BrlapiNative_getDriverName(JNIEnv *jenv, jobject jobj) {
  char name[32];
  GET_HANDLE(jenv, jobj, NULL);

  env = jenv;

  if (brlapi__getDriverName(handle, name, sizeof(name)) < 0) {
    ThrowError(jenv, __func__);
    return NULL;
  }

  name[sizeof(name)-1] = 0;
  return (*jenv)->NewStringUTF(jenv, name);
}

JNIEXPORT jobject JNICALL Java_BrlapiNative_getDisplaySize(JNIEnv *jenv, jobject jobj) {
  unsigned int x, y;
  jclass jcsize;
  jmethodID jinit;
  jobject jsize;
  GET_HANDLE(jenv, jobj, NULL);

  env = jenv;

  if (brlapi__getDisplaySize(handle, &x, &y) < 0) {
    ThrowError(jenv, __func__);
    return NULL;
  }

  if (!(jcsize = (*jenv)->FindClass(jenv, "BrlapiSize"))) {
    ThrowException(jenv, ERR_NULLPTR, __func__);
    return NULL;
  }
  if (!(jinit = (*jenv)->GetMethodID(jenv, jcsize, "<init>", "(II)V"))) {
    ThrowException(jenv, ERR_NULLPTR, __func__);
    return NULL;
  }
  if (!(jsize = (*jenv)->NewObject(jenv, jcsize, jinit, x, y))) {
    ThrowException(jenv, ERR_NULLPTR, __func__);
    return NULL;
  }

  return jsize;
}

JNIEXPORT jint JNICALL Java_BrlapiNative_enterTtyMode(JNIEnv *jenv, jobject jobj, jint jtty, jstring jdriver) {
  int tty ;
  char *driver;
  int result;
  GET_HANDLE(jenv, jobj, -1);
  
  env = jenv;

  tty = (int)jtty; 
  if (!jdriver)
    driver = NULL;
  else
    if (!(driver = (char *)(*jenv)->GetStringUTFChars(jenv, jdriver, NULL))) {
      ThrowException(jenv, ERR_OUTOFMEM, __func__);
      return -1;
    }

  result = brlapi__enterTtyMode(handle, tty,driver);
  if (result < 0) {
    ThrowError(jenv, __func__);
    return -1;
  }

  return (jint) result;
}

JNIEXPORT void JNICALL Java_BrlapiNative_enterTtyModeWithPath(JNIEnv *jenv, jobject jobj, jintArray jttys, jstring jdriver) {
  jint *ttys ;
  char *driver;
  int result;
  GET_HANDLE(jenv, jobj, );
  
  env = jenv;

  if (!jttys) {
    ThrowException(jenv, ERR_NULLPTR, __func__);
    return;
  }
  if (!(ttys = (*jenv)->GetIntArrayElements(jenv, jttys, NULL))) {
    ThrowException(jenv, ERR_OUTOFMEM, __func__);
    return;
  }

  if (!jdriver) {
    driver = NULL;
  } else if (!(driver = (char *)(*jenv)->GetStringUTFChars(jenv, jdriver, NULL))) {
    ThrowException(jenv, ERR_OUTOFMEM, __func__);
    return;
  }

  result = brlapi__enterTtyModeWithPath(handle, ttys,(*jenv)->GetArrayLength(jenv,jttys),driver);
  (*jenv)->ReleaseIntArrayElements(jenv, jttys, ttys, JNI_ABORT);
  if (result < 0) {
    ThrowError(jenv, __func__);
    return;
  }
}

JNIEXPORT void JNICALL Java_BrlapiNative_leaveTtyMode(JNIEnv *jenv, jobject jobj) {
  env = jenv;
  GET_HANDLE(jenv, jobj, );

  if (brlapi__leaveTtyMode(handle) < 0) {
    ThrowError(jenv, __func__);
    return;
  }
}

JNIEXPORT void JNICALL Java_BrlapiNative_setFocus(JNIEnv *jenv, jobject jobj, jint jarg1) {
  int arg1 ;
  GET_HANDLE(jenv, jobj, );
  
  env = jenv;

  arg1 = (int)jarg1; 
  if (brlapi__setFocus(handle, arg1) < 0) {
    ThrowError(jenv, __func__);
    return;
  }
}

JNIEXPORT void JNICALL Java_BrlapiNative_writeTextNative(JNIEnv *jenv, jobject jobj, jint jarg1, jstring jarg2) {
  brlapi_writeStruct_t s = BRLAPI_WRITESTRUCT_INITIALIZER;
  int result;
  GET_HANDLE(jenv, jobj, );
  
  env = jenv;

  s.cursor = (int)jarg1; 

  if (jarg2) {
    s.regionBegin = 1;
    s.regionSize = (*jenv)->GetStringLength(jenv, jarg2);

    if (!(s.text = (char *)(*jenv)->GetStringUTFChars(jenv, jarg2, NULL))) {
      ThrowException(jenv, ERR_OUTOFMEM, __func__);
      return;
    }
    s.charset = "UTF-8";
  }

  result = brlapi__write(handle, &s);
  if (jarg2)
    (*jenv)->ReleaseStringUTFChars(jenv, jarg2, s.text); 

  if (result < 0) {
    ThrowError(jenv, __func__);
    return;
  }
}

JNIEXPORT void JNICALL Java_BrlapiNative_writeDots(JNIEnv *jenv, jobject jobj, jbyteArray jarg1) {
  jbyte *arg1;
  int result;
  GET_HANDLE(jenv, jobj, );
  
  env = jenv;

  if (!jarg1) {
    ThrowException(jenv, ERR_NULLPTR, __func__);
    return;
  }
  arg1 = (*jenv)->GetByteArrayElements(jenv, jarg1, NULL);
  if (!arg1) {
    ThrowException(jenv, ERR_OUTOFMEM, __func__);
    return;
  }

  result = brlapi__writeDots(handle, (const unsigned char *)arg1);
  (*jenv)->ReleaseByteArrayElements(jenv, jarg1, arg1, JNI_ABORT); 
  
  if (result < 0) {
    ThrowError(jenv, __func__);
    return;
  }
}

JNIEXPORT void JNICALL Java_BrlapiNative_write(JNIEnv *jenv, jobject jobj, jobject js) {
  brlapi_writeStruct_t s = BRLAPI_WRITESTRUCT_INITIALIZER;
  int result;
  jstring text, attrAnd, attrOr;
  jclass jcwriteStruct;
  jfieldID displayNumberID, regionBeginID, regionSizeID,
	   textID, attrAndID, attrOrID, cursorID; 
  GET_HANDLE(jenv, jobj, );

  env = jenv;

  if (!js) {
    ThrowException(jenv, ERR_NULLPTR, __func__);
    return;
  }

  GET_CLASS(jenv, jcwriteStruct, js,);

  GET_ID(jenv, displayNumberID,jcwriteStruct, "displayNumber", "I",);
  GET_ID(jenv, regionBeginID,  jcwriteStruct, "regionBegin",   "I",);
  GET_ID(jenv, regionSizeID,   jcwriteStruct, "regionSize",    "I",);
  GET_ID(jenv, textID,         jcwriteStruct, "text",          "Ljava/lang/String;",);
  GET_ID(jenv, attrAndID,      jcwriteStruct, "attrAnd",       "[B",);
  GET_ID(jenv, attrOrID,       jcwriteStruct, "attrOr",        "[B",);
  GET_ID(jenv, cursorID,       jcwriteStruct, "cursor",        "I",);

  s.displayNumber = (*jenv)->GetIntField(jenv, js, displayNumberID);
  s.regionBegin   = (*jenv)->GetIntField(jenv, js, regionBeginID);
  s.regionSize    = (*jenv)->GetIntField(jenv, js, regionSizeID);
  if ((text  = (*jenv)->GetObjectField(jenv, js, textID)))
    s.text   = (char *)(*jenv)->GetStringUTFChars(jenv, text, NULL);
  else s.text  = NULL;
  if ((attrAnd = (*jenv)->GetObjectField(jenv, js, attrAndID)))
    s.attrAnd  = (unsigned char *)(*jenv)->GetByteArrayElements(jenv, attrAnd, NULL);
  else s.attrAnd = NULL;
  if ((attrOr  = (*jenv)->GetObjectField(jenv, js, attrOrID)))
    s.attrOr   = (unsigned char *)(*jenv)->GetByteArrayElements(jenv, attrOr, NULL);
  else s.attrOr  = NULL;
  s.cursor     = (*jenv)->GetIntField(jenv, js, cursorID);
  s.charset = "UTF-8";

  result = brlapi__write(handle, &s);

  if (text)
    (*jenv)->ReleaseStringUTFChars(jenv, text, s.text); 
  if (attrAnd)
    (*jenv)->ReleaseByteArrayElements(jenv, attrAnd, (jbyte*) s.attrAnd, JNI_ABORT); 
  if (attrOr)
    (*jenv)->ReleaseByteArrayElements(jenv, attrOr,  (jbyte*) s.attrOr,  JNI_ABORT); 

  if (result < 0) {
    ThrowError(jenv, __func__);
    return;
  }
}

JNIEXPORT jlong JNICALL Java_BrlapiNative_readKey(JNIEnv *jenv, jobject jobj, jboolean jblock) {
  brlapi_keyCode_t code;
  int result;
  GET_HANDLE(jenv, jobj, -1);

  env = jenv;

  result = brlapi__readKey(handle, (int) jblock, &code);

  if (result < 0) {
    ThrowError(jenv, __func__);
    return -1;
  }

  if (!result) return (jlong)(-1);
  return (jlong)code;
}

JNIEXPORT void JNICALL Java_BrlapiNative_ignoreKeyRange(JNIEnv *jenv, jobject jobj, jlong jarg1, jlong jarg2) {
  env = jenv;
  GET_HANDLE(jenv, jobj, );

  if (brlapi__ignoreKeyRange(handle, (brlapi_keyCode_t)jarg1,(brlapi_keyCode_t)jarg2) < 0) {
    ThrowError(jenv, __func__);
    return;
  }
}

JNIEXPORT void JNICALL Java_BrlapiNative_ignoreKeySet(JNIEnv *jenv, jobject jobj, jlongArray js) {
  jlong *s;
  unsigned int n;
  int result;
  GET_HANDLE(jenv, jobj, );

  env = jenv;

  if (!js) {
    ThrowException(jenv, ERR_NULLPTR, __func__);
    return;
  }

  n = (unsigned int) (*jenv)->GetArrayLength(jenv, js);
  s = (*jenv)->GetLongArrayElements(jenv, js, NULL);

  // XXX jlong != brlapi_keyCode_t probably
  result = brlapi__ignoreKeySet(handle, (const brlapi_keyCode_t *)s, n);
  (*jenv)->ReleaseLongArrayElements(jenv, js, s, JNI_ABORT);
  
  if (result < 0) {
    ThrowError(jenv, __func__);
    return;
  }
}

JNIEXPORT void JNICALL Java_BrlapiNative_acceptKeyRange(JNIEnv *jenv, jobject jobj, jlong jarg1, jlong jarg2) {
  env = jenv;
  GET_HANDLE(jenv, jobj, );

  if (brlapi__acceptKeyRange(handle, (brlapi_keyCode_t)jarg1,(brlapi_keyCode_t)jarg2) < 0) {
    ThrowError(jenv, __func__);
    return;
  }
}

JNIEXPORT void JNICALL Java_BrlapiNative_acceptKeySet(JNIEnv *jenv, jobject jobj, jlongArray js) {
  jlong *s;
  unsigned int n;
  int result;
  GET_HANDLE(jenv, jobj, );

  env = jenv;

  if (!js) {
    ThrowException(jenv, ERR_NULLPTR, __func__);
    return;
  }

  n = (unsigned int) (*jenv)->GetArrayLength(jenv, js);
  s = (*jenv)->GetLongArrayElements(jenv, js, NULL);

  // XXX jlong != brlapi_keyCode_t probably
  result = brlapi__acceptKeySet(handle, (const brlapi_keyCode_t *)s, n);
  (*jenv)->ReleaseLongArrayElements(jenv, js, s, JNI_ABORT);

  if (result < 0) {
    ThrowError(jenv, __func__);
    return;
  }
}

JNIEXPORT void JNICALL Java_BrlapiNative_enterRawMode(JNIEnv *jenv, jobject jobj, jstring jdriver) {
  env = jenv;
  char *driver;
  int res;
  GET_HANDLE(jenv, jobj, );

  if (!jdriver) {
    driver = NULL;
  } else if (!(driver = (char *)(*jenv)->GetStringUTFChars(jenv, jdriver, NULL))) {
    ThrowException(jenv, ERR_NULLPTR, __func__);
    return;
  }
  res = brlapi__enterRawMode(handle, driver);
  if (jdriver) (*jenv)->ReleaseStringUTFChars(jenv, jdriver, driver);
  if (res < 0) {
    ThrowError(jenv, __func__);
    return;
  }
}

JNIEXPORT void JNICALL Java_BrlapiNative_leaveRawMode(JNIEnv *jenv, jobject jobj) {
  env = jenv;
  GET_HANDLE(jenv, jobj, );

  if (brlapi__leaveRawMode(handle) < 0) {
    ThrowError(jenv, __func__);
    return;
  }
}

JNIEXPORT jint JNICALL Java_BrlapiNative_sendRaw(JNIEnv *jenv, jobject jobj, jbyteArray jbuf) {
  jbyte *buf;
  unsigned int n;
  int result;
  GET_HANDLE(jenv, jobj, -1);

  env = jenv;

  if (!jbuf) {
    ThrowException(jenv, ERR_NULLPTR, __func__);
    return -1;
  }

  n = (unsigned int) (*jenv)->GetArrayLength(jenv, jbuf);
  buf = (*jenv)->GetByteArrayElements(jenv, jbuf, NULL);

  result = brlapi__sendRaw(handle, (const unsigned char *)buf, n);
  (*jenv)->ReleaseByteArrayElements(jenv, jbuf, buf, JNI_ABORT);

  if (result < 0) {
    ThrowError(jenv, __func__);
    return -1;
  }

  return (jint) result;
}

JNIEXPORT jint JNICALL Java_BrlapiNative_recvRaw(JNIEnv *jenv, jobject jobj, jbyteArray jbuf) {
  jbyte *buf;
  unsigned int n;
  int result;
  GET_HANDLE(jenv, jobj, -1);

  env = jenv;

  if (!jbuf) {
    ThrowException(jenv, ERR_NULLPTR, __func__);
    return -1;
  }

  n = (unsigned int) (*jenv)->GetArrayLength(jenv, jbuf);
  buf = (*jenv)->GetByteArrayElements(jenv, jbuf, NULL);

  result = brlapi__recvRaw(handle, (unsigned char *)buf, n);

  if (result < 0) {
    (*jenv)->ReleaseByteArrayElements(jenv, jbuf, buf, JNI_ABORT);
    ThrowError(jenv, __func__);
    return -1;
  }

  (*jenv)->ReleaseByteArrayElements(jenv, jbuf, buf, 0);
  return (jint) result;
}

JNIEXPORT jstring JNICALL Java_BrlapiNative_packetType(JNIEnv *jenv, jclass jcls, jlong jtype) {
  const char *type;

  env = jenv;

  if (!(type = brlapi_packetType((brlapi_type_t) jtype))) {
    ThrowError(jenv, __func__);
    return NULL;
  }

  return (*jenv)->NewStringUTF(jenv, type);
}

JNIEXPORT jstring JNICALL Java_BrlapiError_toString (JNIEnv *jenv, jobject jerr) {
  jclass jcerr;
  jfieldID brlerrnoID, libcerrnoID, gaierrnoID, errfunID;
  jstring jerrfun;
  brlapi_error_t error;
  const char *res;

  env = jenv;

  GET_CLASS(jenv, jcerr, jerr, NULL);
  GET_ID(jenv, brlerrnoID,  jcerr, "brlerrno",  "I", NULL);
  GET_ID(jenv, libcerrnoID, jcerr, "libcerrno", "I", NULL);
  GET_ID(jenv, gaierrnoID,  jcerr, "gaierrno",  "I", NULL);
  GET_ID(jenv, errfunID,    jcerr, "errfun",    "Ljava/lang/String;", NULL);

  error.brlerrno  = (*jenv)->GetIntField(jenv, jerr, brlerrnoID);
  error.libcerrno = (*jenv)->GetIntField(jenv, jerr, libcerrnoID);
  error.gaierrno  = (*jenv)->GetIntField(jenv, jerr, gaierrnoID);
  if (!(jerrfun = (*jenv)->GetObjectField(jenv, jerr, errfunID))) {
    error.errfun = NULL;
  } else if (!(error.errfun = (char *)(*jenv)->GetStringUTFChars(jenv, jerrfun, NULL))) {
    ThrowException(jenv, ERR_OUTOFMEM, __func__);
    return NULL;
  }
  res = brlapi_strerror(&error);
  if (jerrfun)
    (*jenv)->ReleaseStringUTFChars(jenv, jerrfun, error.errfun);
  return (*jenv)->NewStringUTF(jenv, res);
}

JNIEXPORT jstring JNICALL Java_BrlapiException_toString (JNIEnv *jenv, jobject jerr) {
  jclass jcerr;
  jfieldID handleID, errnoID, typeID, bufID;
  jarray jbuf;
  brlapi_handle_t *handle;
  int errno;
  long type;
  jbyte *buf;
  int size;
  char errmsg[256];
  brlapi_error_t error;

  env = jenv;

  if (!jerr) {
    ThrowException(jenv, ERR_NULLPTR, __func__);
    return NULL;
  }
  GET_CLASS(jenv, jcerr, jerr, NULL);
  GET_ID(jenv, handleID, jcerr, "handle", "I", NULL);
  GET_ID(jenv, errnoID,  jcerr, "errno",  "I", NULL);
  GET_ID(jenv, typeID,   jcerr, "type",   "I", NULL);
  GET_ID(jenv, bufID,    jcerr, "buf",    "I", NULL);

  handle = (void*)(intptr_t)(*jenv)->GetLongField(jenv, jerr, handleID);
  error.brlerrno = (*jenv)->GetIntField(jenv, jerr, errnoID);
  type  = (*jenv)->GetIntField(jenv, jerr, typeID);
  if (!(jbuf  = (*jenv)->GetObjectField(jenv, jerr, typeID))) {
    ThrowException(jenv, ERR_NULLPTR, __func__);
    return NULL;
  }
  size  = (*jenv)->GetArrayLength(jenv, jbuf);
  buf = (*jenv)->GetByteArrayElements(jenv, jbuf, NULL);

  brlapi__strexception(handle, errmsg, sizeof(errmsg), errno, type, buf, size); 

  return (*jenv)->NewStringUTF(jenv, errmsg);
}

JNIEXPORT void JNICALL Java_BrlapiKey_expandKeyCode (JNIEnv *jenv, jobject obj, jlong jkey) {
  jclass jckey;
  jfieldID typeID, commandID, argumentID, flagsID;
  brlapi_keyCode_t key = jkey;
  brlapi_expandedKeyCode_t ekc;

  GET_CLASS(jenv, jckey, obj, );
  GET_ID(jenv, typeID,     jckey, "type",     "I", );
  GET_ID(jenv, commandID,  jckey, "command",  "I", );
  GET_ID(jenv, argumentID, jckey, "argument", "I", );
  GET_ID(jenv, flagsID,    jckey, "flags",    "I", );

  brlapi_expandKeyCode(key, &ekc);
  (*jenv)->SetIntField(jenv, obj, typeID,     ekc.type);
  (*jenv)->SetIntField(jenv, obj, commandID,  ekc.command);
  (*jenv)->SetIntField(jenv, obj, argumentID, ekc.argument);
  (*jenv)->SetIntField(jenv, obj, flagsID,    ekc.flags);
}
