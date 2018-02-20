/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2018 by
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 *   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <jni.h>

#define BRLAPI_NO_DEPRECATED
#define BRLAPI_NO_SINGLE_SESSION
#include "brlapi.h"

typedef enum {
  ERR_NULL_POINTER,
  ERR_NO_MEMORY,
  ERR_INDEX_BOUNDS,
} ErrorCode;

/* TODO: threads */
static JNIEnv *env;

static void
throwException(JNIEnv *jenv, int code, const char *msg) {
  jclass excep;
  const char *exception;

  switch (code) {
    case ERR_NULL_POINTER:  exception = "java/lang/NullPointerException";      break;
    case ERR_NO_MEMORY: exception = "java/lang/OutOfMemoryError";          break;
    case ERR_INDEX_BOUNDS:    exception = "java/lang/IndexOutOfBoundsException"; break;
    default:           exception = "java/lang/UnknownError";              break;
  }

  (*jenv)->ExceptionClear(jenv);
  excep = (*jenv)->FindClass(jenv, exception);
  if (excep)
    (*jenv)->ThrowNew(jenv, excep, msg);
  else
    fprintf(stderr,"couldn't find exception %s !\n",exception);
}

static void
throwError(JNIEnv *jenv, const char *msg) {
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

    if (!(jcexcep = (*jenv)->FindClass(jenv, "org/a11y/BrlAPI/Error"))) {
      throwException(jenv, ERR_NULL_POINTER, "ThrowBrlapiErrorFindClass");
      return;
    }
    if (!(jinit = (*jenv)->GetMethodID(jenv, jcexcep, "<init>", "(IIILjava/lang/String;)V"))) {
      throwException(jenv, ERR_NULL_POINTER, "ThrowBrlapiErrorGetMethodID");
      return;
    }
    if (brlapi_errfun)
      errfun = (*jenv)->NewStringUTF(jenv, brlapi_errfun);
    if (!(jexcep = (*jenv)->NewObject(jenv, jcexcep, jinit, brlapi_errno, brlapi_libcerrno, brlapi_gaierrno, errfun))) {
      throwException(jenv, ERR_NULL_POINTER, "ThrowBrlapiErrorNewObject");
      return;
    }
    (*jenv)->ExceptionClear(jenv);
    (*jenv)->Throw(jenv, jexcep);
  }
}

static void BRLAPI_STDCALL exceptionHandler(brlapi_handle_t *handle, int err, brlapi_packetType_t type, const void *buf, size_t size) {
  jarray jbuf;
  jclass jcexcep;
  jmethodID jinit;
  jthrowable jexcep;

  if (!(jbuf = (*env)->NewByteArray(env, size))) {
    throwException(env, ERR_NO_MEMORY, __func__);
    return;
  }
  (*env)->SetByteArrayRegion(env, jbuf, 0, size, (jbyte *) buf);

  if (!(jcexcep = (*env)->FindClass(env, "org/a11y/BrlAPI/Exception"))) {
    throwException(env, ERR_NULL_POINTER, "exceptionHandlerFindClass");
    return;
  }
  if (!(jinit = (*env)->GetMethodID(env, jcexcep, "<init>", "(JII[B)V"))) {
    throwException(env, ERR_NULL_POINTER, "exceptionHandlerGetMethodID");
    return;
  }
  if (!(jexcep = (*env)->NewObject(env, jcexcep, jinit, (jlong)(intptr_t) handle, err, type, jbuf))) {
    throwException(env, ERR_NULL_POINTER, "exceptionHandlerNewObject");
    return;
  }
  (*env)->ExceptionClear(env);
  (*env)->Throw(env, jexcep);
  return;
}

static int gotVersionNumbers = 0;
static int majorVersion, minorVersion, revision;

static void
getVersionNumbers (void) {
  if (!gotVersionNumbers) {
    brlapi_getLibraryVersion(&majorVersion, &minorVersion, &revision);
    gotVersionNumbers = 1;
  }
}

#define JAVA_METHOD(object,name,type) \
  JNIEXPORT type JNICALL Java_ ## object ## _ ## name (JNIEnv *jenv

#define JAVA_INSTANCE_METHOD(object,name,type,...) \
  JAVA_METHOD(object,name,type), jobject jobj, ## __VA_ARGS__)

#define JAVA_STATIC_METHOD(object,name,type,...) \
  JAVA_METHOD(object,name,type), jclass jcls, ## __VA_ARGS__)

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, getMajorVersion, jint
) {
  getVersionNumbers();
  return majorVersion;
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, getMinorVersion, jint
) {
  getVersionNumbers();
  return minorVersion;
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, getRevision, jint
) {
  getVersionNumbers();
  return revision;
}

#define GET_CLASS(jenv, class, obj, ret) \
  jclass class; \
  do { \
    if (!((class) = (*(jenv))->GetObjectClass((jenv), (obj)))) { \
      throwException((jenv), ERR_NULL_POINTER, #obj " -> " #class); \
      return ret; \
    } \
  } while (0)

#define GET_FIELD(jenv, field, class, name, signature, ret) \
  jfieldID field; \
  do { \
    if (!(field = (*(jenv))->GetFieldID((jenv), (class), (name), (signature)))) {\
      throwException((jenv), ERR_NULL_POINTER, #class "." name); \
      return ret; \
    } \
  } while (0)

#define GET_HANDLE(jenv, jobj, ret) \
  brlapi_handle_t *handle; \
  jfieldID handleID; \
  do { \
    GET_CLASS(jenv, jcls, jobj, ret); \
    GET_FIELD(jenv, id, jcls, "handle", "J", ret); \
    handleID = id; \
    handle = (void*) (intptr_t) (*jenv)->GetLongField(jenv, jobj, handleID); \
    if (!handle) { \
      throwException((jenv), ERR_NULL_POINTER, "connection has been closed"); \
      return ret; \
    } \
  } while (0)

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, openConnection, jint,
  jobject JclientSettings , jobject JusedSettings
) {
  brlapi_connectionSettings_t clientSettings,  usedSettings,
            *PclientSettings, *PusedSettings;
  int result;
  jstring auth = NULL, host = NULL;
  const char *str;
  brlapi_handle_t *handle;

  GET_CLASS(jenv, jcls, jobj, -1);
  GET_FIELD(jenv, handleID, jcls, "handle", "J", -1);
  handle = malloc(brlapi_getHandleSize());
  if (!handle) {
    throwException(jenv, ERR_NO_MEMORY, __func__);
    return -1;
  }

  (*jenv)->SetLongField(jenv, jobj, handleID, (jlong) (intptr_t) handle);

  env = jenv;

  if (JclientSettings) {
    GET_CLASS(jenv, jcclientSettings, JclientSettings, -1);
    GET_FIELD(jenv, clientAuthID, jcclientSettings, "auth", "Ljava/lang/String;", -1);
    GET_FIELD(jenv, clientHostID, jcclientSettings, "host", "Ljava/lang/String;", -1);

    PclientSettings = &clientSettings;
    if ((auth = (*jenv)->GetObjectField(jenv, JclientSettings, clientAuthID))) {
      if (!(clientSettings.auth = (char *)(*jenv)->GetStringUTFChars(jenv, auth, NULL))) {
	throwException(jenv, ERR_NO_MEMORY, __func__);
	return -1;
      }
    } else clientSettings.auth = NULL;
    if ((host = (*jenv)->GetObjectField(jenv, JclientSettings, clientHostID))) {
      if (!(clientSettings.host = (char *)(*jenv)->GetStringUTFChars(jenv, host, NULL))) {
	throwException(jenv, ERR_NO_MEMORY, __func__);
	return -1;
      }
    } else clientSettings.host = NULL;
  } else PclientSettings = NULL;

  if (JusedSettings)
    PusedSettings = &usedSettings;
  else
    PusedSettings = NULL;

  if ((result = brlapi__openConnection(handle, PclientSettings, PusedSettings)) < 0) {
    throwError(jenv, __func__);
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
    GET_FIELD(jenv, usedAuthID, jcusedSettings, "auth", "Ljava/lang/String;", -1);
    GET_FIELD(jenv, usedHostID, jcusedSettings, "host", "Ljava/lang/String;", -1);

    auth = (*jenv)->NewStringUTF(jenv, usedSettings.auth);
    if (!auth) {
      throwException(jenv, ERR_NO_MEMORY, __func__);
      return -1;
    }
    str = (*jenv)->GetStringUTFChars(jenv, auth, NULL);
    (*jenv)->SetObjectField(jenv, JusedSettings, usedAuthID, auth);
    (*jenv)->ReleaseStringUTFChars(jenv, auth, str);

    host = (*jenv)->NewStringUTF(jenv, usedSettings.host);
    if (!host) {
      throwException(jenv, ERR_NO_MEMORY, __func__);
      return -1;
    }
    str = (*jenv)->GetStringUTFChars(jenv, host, NULL);
    (*jenv)->SetObjectField(jenv, JusedSettings, usedHostID, host);
    (*jenv)->ReleaseStringUTFChars(jenv, host, str);
  }

  return (jint) result;
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, closeConnection, void
) {
  env = jenv;
  GET_HANDLE(jenv, jobj, );

  brlapi__closeConnection(handle);
  free((void*) (intptr_t) handle);
  (*jenv)->SetLongField(jenv, jobj, handleID, (jlong) (intptr_t) NULL);
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, getDriverName, jstring
) {
  char name[32];
  GET_HANDLE(jenv, jobj, NULL);

  env = jenv;

  if (brlapi__getDriverName(handle, name, sizeof(name)) < 0) {
    throwError(jenv, __func__);
    return NULL;
  }

  name[sizeof(name)-1] = 0;
  return (*jenv)->NewStringUTF(jenv, name);
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, getModelIdentifier, jstring
) {
  char identifier[32];
  GET_HANDLE(jenv, jobj, NULL);

  env = jenv;

  if (brlapi__getModelIdentifier(handle, identifier, sizeof(identifier)) < 0) {
    throwError(jenv, __func__);
    return NULL;
  }

  identifier[sizeof(identifier)-1] = 0;
  return (*jenv)->NewStringUTF(jenv, identifier);
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, getDisplaySize, jobject
) {
  unsigned int x, y;
  jclass jcsize;
  jmethodID jinit;
  jobject jsize;
  GET_HANDLE(jenv, jobj, NULL);

  env = jenv;

  if (brlapi__getDisplaySize(handle, &x, &y) < 0) {
    throwError(jenv, __func__);
    return NULL;
  }

  if (!(jcsize = (*jenv)->FindClass(jenv, "org/a11y/BrlAPI/DisplaySize"))) {
    throwException(jenv, ERR_NULL_POINTER, __func__);
    return NULL;
  }
  if (!(jinit = (*jenv)->GetMethodID(jenv, jcsize, "<init>", "(II)V"))) {
    throwException(jenv, ERR_NULL_POINTER, __func__);
    return NULL;
  }
  if (!(jsize = (*jenv)->NewObject(jenv, jcsize, jinit, x, y))) {
    throwException(jenv, ERR_NULL_POINTER, __func__);
    return NULL;
  }

  return jsize;
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, enterTtyMode, jint,
  jint jtty, jstring jdriver
) {
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
      throwException(jenv, ERR_NO_MEMORY, __func__);
      return -1;
    }

  result = brlapi__enterTtyMode(handle, tty,driver);
  if (result < 0) {
    throwError(jenv, __func__);
    return -1;
  }

  return (jint) result;
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, enterTtyModeWithPath, void,
  jintArray jttys, jstring jdriver
) {
  jint *ttys ;
  char *driver;
  int result;
  GET_HANDLE(jenv, jobj, );
  
  env = jenv;

  if (!jttys) {
    throwException(jenv, ERR_NULL_POINTER, __func__);
    return;
  }
  if (!(ttys = (*jenv)->GetIntArrayElements(jenv, jttys, NULL))) {
    throwException(jenv, ERR_NO_MEMORY, __func__);
    return;
  }

  if (!jdriver) {
    driver = NULL;
  } else if (!(driver = (char *)(*jenv)->GetStringUTFChars(jenv, jdriver, NULL))) {
    throwException(jenv, ERR_NO_MEMORY, __func__);
    return;
  }

  result = brlapi__enterTtyModeWithPath(handle, ttys,(*jenv)->GetArrayLength(jenv,jttys),driver);
  (*jenv)->ReleaseIntArrayElements(jenv, jttys, ttys, JNI_ABORT);
  if (result < 0) {
    throwError(jenv, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, leaveTtyMode, void
) {
  env = jenv;
  GET_HANDLE(jenv, jobj, );

  if (brlapi__leaveTtyMode(handle) < 0) {
    throwError(jenv, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, setFocus, void,
  jint jarg1
) {
  int arg1 ;
  GET_HANDLE(jenv, jobj, );
  
  env = jenv;

  arg1 = (int)jarg1; 
  if (brlapi__setFocus(handle, arg1) < 0) {
    throwError(jenv, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, writeText, void,
  jint jarg1, jstring jarg2
) {
  brlapi_writeArguments_t s = BRLAPI_WRITEARGUMENTS_INITIALIZER;
  int result;
  GET_HANDLE(jenv, jobj, );
  
  env = jenv;

  s.cursor = (int)jarg1; 

  if (jarg2) {
    s.regionBegin = 1;
    s.regionSize = (*jenv)->GetStringLength(jenv, jarg2);

    if (!(s.text = (char *)(*jenv)->GetStringUTFChars(jenv, jarg2, NULL))) {
      throwException(jenv, ERR_NO_MEMORY, __func__);
      return;
    }
    s.charset = "UTF-8";
  }

  result = brlapi__write(handle, &s);
  if (jarg2)
    (*jenv)->ReleaseStringUTFChars(jenv, jarg2, s.text); 

  if (result < 0) {
    throwError(jenv, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, writeDots, void,
  jbyteArray jarg1
) {
  jbyte *arg1;
  int result;
  GET_HANDLE(jenv, jobj, );
  
  env = jenv;

  if (!jarg1) {
    throwException(jenv, ERR_NULL_POINTER, __func__);
    return;
  }
  arg1 = (*jenv)->GetByteArrayElements(jenv, jarg1, NULL);
  if (!arg1) {
    throwException(jenv, ERR_NO_MEMORY, __func__);
    return;
  }

  result = brlapi__writeDots(handle, (const unsigned char *)arg1);
  (*jenv)->ReleaseByteArrayElements(jenv, jarg1, arg1, JNI_ABORT); 
  
  if (result < 0) {
    throwError(jenv, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, write, void,
  jobject jarguments
) {
  brlapi_writeArguments_t arguments = BRLAPI_WRITEARGUMENTS_INITIALIZER;
  int result;
  jstring text, andMask, orMask;
  GET_HANDLE(jenv, jobj, );

  env = jenv;

  if (!jarguments) {
    throwException(jenv, ERR_NULL_POINTER, __func__);
    return;
  }

  GET_CLASS(jenv, jcwriteArguments, jarguments,);

  GET_FIELD(jenv, displayNumberID,jcwriteArguments, "displayNumber", "I",);
  GET_FIELD(jenv, regionBeginID,  jcwriteArguments, "regionBegin",   "I",);
  GET_FIELD(jenv, regionSizeID,   jcwriteArguments, "regionSize",    "I",);
  GET_FIELD(jenv, textID,         jcwriteArguments, "text",          "Ljava/lang/String;",);
  GET_FIELD(jenv, andMaskID,      jcwriteArguments, "andMask",       "[B",);
  GET_FIELD(jenv, orMaskID,       jcwriteArguments, "orMask",        "[B",);
  GET_FIELD(jenv, cursorID,       jcwriteArguments, "cursor",        "I",);

  arguments.displayNumber = (*jenv)->GetIntField(jenv, jarguments, displayNumberID);
  arguments.regionBegin   = (*jenv)->GetIntField(jenv, jarguments, regionBeginID);
  arguments.regionSize    = (*jenv)->GetIntField(jenv, jarguments, regionSizeID);
  if ((text  = (*jenv)->GetObjectField(jenv, jarguments, textID)))
    arguments.text   = (char *)(*jenv)->GetStringUTFChars(jenv, text, NULL);
  else 
    arguments.text  = NULL;
  if ((andMask = (*jenv)->GetObjectField(jenv, jarguments, andMaskID)))
    arguments.andMask  = (unsigned char *)(*jenv)->GetByteArrayElements(jenv, andMask, NULL);
  else
    arguments.andMask = NULL;
  if ((orMask  = (*jenv)->GetObjectField(jenv, jarguments, orMaskID)))
    arguments.orMask   = (unsigned char *)(*jenv)->GetByteArrayElements(jenv, orMask, NULL);
  else
    arguments.orMask  = NULL;
  arguments.cursor     = (*jenv)->GetIntField(jenv, jarguments, cursorID);
  arguments.charset = "UTF-8";

  result = brlapi__write(handle, &arguments);

  if (text)
    (*jenv)->ReleaseStringUTFChars(jenv, text, arguments.text); 
  if (andMask)
    (*jenv)->ReleaseByteArrayElements(jenv, andMask, (jbyte*) arguments.andMask, JNI_ABORT); 
  if (orMask)
    (*jenv)->ReleaseByteArrayElements(jenv, orMask,  (jbyte*) arguments.orMask,  JNI_ABORT); 

  if (result < 0) {
    throwError(jenv, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, readKey, jlong,
  jboolean jblock
) {
  brlapi_keyCode_t code;
  int result;
  GET_HANDLE(jenv, jobj, -1);

  env = jenv;

  result = brlapi__readKey(handle, (int) jblock, &code);

  if (result < 0) {
    throwError(jenv, __func__);
    return -1;
  }

  if (!result) return (jlong)(-1);
  return (jlong)code;
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, readKeyWithTimeout, jlong,
  jint timeout_ms
) {
  brlapi_keyCode_t code;
  int result;
  GET_HANDLE(jenv, jobj, -1);

  env = jenv;

  result = brlapi__readKeyWithTimeout(handle, timeout_ms, &code);

  if (result < 0) {
    throwError(jenv, __func__);
    return -1;
  }

  if (!result) return (jlong)(-1);
  return (jlong)code;
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, ignoreKeys, void,
  jlong jrange, jlongArray js
) {
  jlong *s;
  unsigned int n;
  int result;
  GET_HANDLE(jenv, jobj, );

  env = jenv;

  if (!js) {
    throwException(jenv, ERR_NULL_POINTER, __func__);
    return;
  }

  n = (unsigned int) (*jenv)->GetArrayLength(jenv, js);
  s = (*jenv)->GetLongArrayElements(jenv, js, NULL);

  // XXX jlong != brlapi_keyCode_t probably
  result = brlapi__ignoreKeys(handle, jrange, (const brlapi_keyCode_t *)s, n);
  (*jenv)->ReleaseLongArrayElements(jenv, js, s, JNI_ABORT);
  
  if (result < 0) {
    throwError(jenv, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, acceptKeys, void,
  jlong jrange, jlongArray js
) {
  jlong *s;
  unsigned int n;
  int result;
  GET_HANDLE(jenv, jobj, );

  env = jenv;

  if (!js) {
    throwException(jenv, ERR_NULL_POINTER, __func__);
    return;
  }

  n = (unsigned int) (*jenv)->GetArrayLength(jenv, js);
  s = (*jenv)->GetLongArrayElements(jenv, js, NULL);

  // XXX jlong != brlapi_keyCode_t probably
  result = brlapi__acceptKeys(handle, jrange, (const brlapi_keyCode_t *)s, n);
  (*jenv)->ReleaseLongArrayElements(jenv, js, s, JNI_ABORT);

  if (result < 0) {
    throwError(jenv, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, ignoreAllKeys, void
) {
  GET_HANDLE(jenv, jobj, );

  if (brlapi__ignoreAllKeys(handle) < 0)
    throwError(jenv, __func__);
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, acceptAllKeys, void
) {
  GET_HANDLE(jenv, jobj, );

  if (brlapi__acceptAllKeys(handle) < 0)
    throwError(jenv, __func__);
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, ignoreKeyRanges, void,
  jobjectArray js
) {
  unsigned int n;
  GET_HANDLE(jenv, jobj, );

  env = jenv;

  if (!js) {
    throwException(jenv, ERR_NULL_POINTER, __func__);
    return;
  }

  n = (unsigned int) (*jenv)->GetArrayLength(jenv, js);

  {
    unsigned int i;
    brlapi_range_t s[n];

    for (i=0; i<n; i++) {
      jlongArray jl = (*jenv)->GetObjectArrayElement(jenv, js, i);
      jlong *l = (*jenv)->GetLongArrayElements(jenv, jl, NULL);
      s[i].first = l[0];
      s[i].last = l[1];
      (*jenv)->ReleaseLongArrayElements(jenv, jl, l, JNI_ABORT);
    }
    if (brlapi__ignoreKeyRanges(handle, s, n)) {
      throwError(jenv, __func__);
      return;
    }
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, acceptKeyRanges, void,
  jobjectArray js
) {
  unsigned int n;
  GET_HANDLE(jenv, jobj, );

  env = jenv;

  if (!js) {
    throwException(jenv, ERR_NULL_POINTER, __func__);
    return;
  }

  n = (unsigned int) (*jenv)->GetArrayLength(jenv, js);

  {
    unsigned int i;
    brlapi_range_t s[n];

    for (i=0; i<n; i++) {
      jlongArray jl = (*jenv)->GetObjectArrayElement(jenv, js, i);
      jlong *l = (*jenv)->GetLongArrayElements(jenv, jl, NULL);
      s[i].first = l[0];
      s[i].last = l[1];
      (*jenv)->ReleaseLongArrayElements(jenv, jl, l, JNI_ABORT);
    }
    if (brlapi__acceptKeyRanges(handle, s, n)) {
      throwError(jenv, __func__);
      return;
    }
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, enterRawMode, void,
  jstring jdriver
) {
  env = jenv;
  char *driver;
  int res;
  GET_HANDLE(jenv, jobj, );

  if (!jdriver) {
    driver = NULL;
  } else if (!(driver = (char *)(*jenv)->GetStringUTFChars(jenv, jdriver, NULL))) {
    throwException(jenv, ERR_NULL_POINTER, __func__);
    return;
  }
  res = brlapi__enterRawMode(handle, driver);
  if (jdriver) (*jenv)->ReleaseStringUTFChars(jenv, jdriver, driver);
  if (res < 0) {
    throwError(jenv, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, leaveRawMode, void
) {
  env = jenv;
  GET_HANDLE(jenv, jobj, );

  if (brlapi__leaveRawMode(handle) < 0) {
    throwError(jenv, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, sendRaw, jint,
  jbyteArray jbuf
) {
  jbyte *buf;
  unsigned int n;
  int result;
  GET_HANDLE(jenv, jobj, -1);

  env = jenv;

  if (!jbuf) {
    throwException(jenv, ERR_NULL_POINTER, __func__);
    return -1;
  }

  n = (unsigned int) (*jenv)->GetArrayLength(jenv, jbuf);
  buf = (*jenv)->GetByteArrayElements(jenv, jbuf, NULL);

  result = brlapi__sendRaw(handle, (const unsigned char *)buf, n);
  (*jenv)->ReleaseByteArrayElements(jenv, jbuf, buf, JNI_ABORT);

  if (result < 0) {
    throwError(jenv, __func__);
    return -1;
  }

  return (jint) result;
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, recvRaw, jint,
  jbyteArray jbuf
) {
  jbyte *buf;
  unsigned int n;
  int result;
  GET_HANDLE(jenv, jobj, -1);

  env = jenv;

  if (!jbuf) {
    throwException(jenv, ERR_NULL_POINTER, __func__);
    return -1;
  }

  n = (unsigned int) (*jenv)->GetArrayLength(jenv, jbuf);
  buf = (*jenv)->GetByteArrayElements(jenv, jbuf, NULL);

  result = brlapi__recvRaw(handle, (unsigned char *)buf, n);

  if (result < 0) {
    (*jenv)->ReleaseByteArrayElements(jenv, jbuf, buf, JNI_ABORT);
    throwError(jenv, __func__);
    return -1;
  }

  (*jenv)->ReleaseByteArrayElements(jenv, jbuf, buf, 0);
  return (jint) result;
}

JAVA_STATIC_METHOD(
  org_a11y_BrlAPI_Native, getPacketTypeName,
  jstring, jlong jtype
) {
  const char *type;

  env = jenv;

  if (!(type = brlapi_getPacketTypeName((brlapi_packetType_t) jtype))) {
    throwError(jenv, __func__);
    return NULL;
  }

  return (*jenv)->NewStringUTF(jenv, type);
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Error, toString, jstring
) {
  jstring jerrfun;
  brlapi_error_t error;
  const char *res;

  env = jenv;

  GET_CLASS(jenv, jcerr, jobj, NULL);
  GET_FIELD(jenv, brlerrnoID,  jcerr, "brlerrno",  "I", NULL);
  GET_FIELD(jenv, libcerrnoID, jcerr, "libcerrno", "I", NULL);
  GET_FIELD(jenv, gaierrnoID,  jcerr, "gaierrno",  "I", NULL);
  GET_FIELD(jenv, errfunID,    jcerr, "errfun",    "Ljava/lang/String;", NULL);

  error.brlerrno  = (*jenv)->GetIntField(jenv, jobj, brlerrnoID);
  error.libcerrno = (*jenv)->GetIntField(jenv, jobj, libcerrnoID);
  error.gaierrno  = (*jenv)->GetIntField(jenv, jobj, gaierrnoID);
  if (!(jerrfun = (*jenv)->GetObjectField(jenv, jobj, errfunID))) {
    error.errfun = NULL;
  } else if (!(error.errfun = (char *)(*jenv)->GetStringUTFChars(jenv, jerrfun, NULL))) {
    throwException(jenv, ERR_NO_MEMORY, __func__);
    return NULL;
  }
  res = brlapi_strerror(&error);
  if (jerrfun)
    (*jenv)->ReleaseStringUTFChars(jenv, jerrfun, error.errfun);
  return (*jenv)->NewStringUTF(jenv, res);
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Exception, toString, jstring
) {
  jarray jbuf;
  brlapi_handle_t *handle;
  long type;
  jbyte *buf;
  int size;
  char errmsg[256];

  env = jenv;

  if (!jobj) {
    throwException(jenv, ERR_NULL_POINTER, __func__);
    return NULL;
  }
  GET_CLASS(jenv, jcerr, jobj, NULL);
  GET_FIELD(jenv, handleID, jcerr, "handle", "I", NULL);
  GET_FIELD(jenv, errnoID,  jcerr, "errno",  "I", NULL);
  GET_FIELD(jenv, typeID,   jcerr, "type",   "I", NULL);
  GET_FIELD(jenv, bufID,    jcerr, "buf",    "I", NULL);

  handle = (void*)(intptr_t)(*jenv)->GetLongField(jenv, jobj, handleID);
  type  = (*jenv)->GetIntField(jenv, jobj, typeID);
  if (!(jbuf  = (*jenv)->GetObjectField(jenv, jobj, typeID))) {
    throwException(jenv, ERR_NULL_POINTER, __func__);
    return NULL;
  }
  size  = (*jenv)->GetArrayLength(jenv, jbuf);
  buf = (*jenv)->GetByteArrayElements(jenv, jbuf, NULL);

  brlapi__strexception(handle, errmsg, sizeof(errmsg), errno, type, buf, size); 

  return (*jenv)->NewStringUTF(jenv, errmsg);
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Key, expandKeyCode, void,
  jlong jkey
) {
  brlapi_keyCode_t key = jkey;
  brlapi_expandedKeyCode_t ekc;

  GET_CLASS(jenv, jckey, jobj, );
  GET_FIELD(jenv, typeID,     jckey, "typeValue",     "I", );
  GET_FIELD(jenv, commandID,  jckey, "commandValue",  "I", );
  GET_FIELD(jenv, argumentID, jckey, "argumentValue", "I", );
  GET_FIELD(jenv, flagsID,    jckey, "flagsValue",    "I", );

  brlapi_expandKeyCode(key, &ekc);
  (*jenv)->SetIntField(jenv, jobj, typeID,     ekc.type);
  (*jenv)->SetIntField(jenv, jobj, commandID,  ekc.command);
  (*jenv)->SetIntField(jenv, jobj, argumentID, ekc.argument);
  (*jenv)->SetIntField(jenv, jobj, flagsID,    ekc.flags);
}
