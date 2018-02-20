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

#include "java.h"

#define BRLAPI_NO_DEPRECATED
#define BRLAPI_NO_SINGLE_SESSION
#include "brlapi.h"
#define BRLAPI_OBJECT(name) "org/a11y/BrlAPI/" name

/* TODO: threads */
static JNIEnv *globalJavaEnvironment;
#define SET_GLOBAL_JAVA_ENVIRONMENT(env) (globalJavaEnvironment = (env))
#define GET_GLOBAL_JAVA_ENVIRONMENT(env) JNIEnv *env = globalJavaEnvironment

static void
throwException(JNIEnv *env, const char *object, const char *message) {
  (*env)->ExceptionClear(env);
  jclass class = (*env)->FindClass(env, object);

  if (class) {
    (*env)->ThrowNew(env, class, message);
  } else {
    fprintf(stderr,"couldn't find exception object: %s: %s!\n", object, message);
  }
}

static void
throwError(JNIEnv *env, const char *msg) {
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

    if (!(jcexcep = (*env)->FindClass(env, BRLAPI_OBJECT("Error")))) {
      throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, "ThrowBrlapiErrorFindClass");
      return;
    }
    if (!(jinit = (*env)->GetMethodID(env, jcexcep, JAVA_CONSTRUCTOR_NAME, "(IIILjava/lang/String;)V"))) {
      throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, "ThrowBrlapiErrorGetMethodID");
      return;
    }
    if (brlapi_errfun)
      errfun = (*env)->NewStringUTF(env, brlapi_errfun);
    if (!(jexcep = (*env)->NewObject(env, jcexcep, jinit, brlapi_errno, brlapi_libcerrno, brlapi_gaierrno, errfun))) {
      throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, "ThrowBrlapiErrorNewObject");
      return;
    }
    (*env)->ExceptionClear(env);
    (*env)->Throw(env, jexcep);
  }
}

#define GET_CLASS(env, class, obj, ret) \
  jclass class; \
  do { \
    if (!((class) = (*(env))->GetObjectClass((env), (obj)))) { \
      throwException((env), JAVA_OBJECT_NULL_POINTER_EXCEPTION, #obj " -> " #class); \
      return ret; \
    } \
  } while (0)

#define GET_FIELD(env, field, class, name, signature, ret) \
  jfieldID field; \
  do { \
    if (!(field = (*(env))->GetFieldID((env), (class), (name), (signature)))) {\
      throwException((env), JAVA_OBJECT_NULL_POINTER_EXCEPTION, #class "." name); \
      return ret; \
    } \
  } while (0)

#define GET_HANDLE(env, object, ret) \
  brlapi_handle_t *handle; \
  do { \
    GET_CLASS((env), class, (object), ret); \
    GET_FIELD((env), field, class, "handle", JAVA_SIG_LONG, ret); \
    handle = (void*) (intptr_t) GET_VALUE((env), Long, (object), field); \
    if (!handle) { \
      throwException((env), JAVA_OBJECT_NULL_POINTER_EXCEPTION, "connection has been closed"); \
      return ret; \
    } \
  } while (0)

#define SET_HANDLE(env, object, value, ret) \
  do { \
    GET_CLASS((env), class, (object), ret); \
    GET_FIELD((env), field, class, "handle", JAVA_SIG_LONG, ret); \
    SET_VALUE((env), Long, (object), field, (jlong) (intptr_t) (value)); \
  } while (0)

static void BRLAPI_STDCALL
exceptionHandler (brlapi_handle_t *handle, int err, brlapi_packetType_t type, const void *buf, size_t size) {
  GET_GLOBAL_JAVA_ENVIRONMENT(env);
  jarray jbuf;
  jclass jcexcep;
  jmethodID jinit;
  jthrowable jexcep;

  if (!(jbuf = (*env)->NewByteArray(env, size))) {
    throwException(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
    return;
  }
  (*env)->SetByteArrayRegion(env, jbuf, 0, size, (jbyte *) buf);

  if (!(jcexcep = (*env)->FindClass(env, BRLAPI_OBJECT("Exception")))) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, "exceptionHandlerFindClass");
    return;
  }
  if (!(jinit = (*env)->GetMethodID(env, jcexcep, JAVA_CONSTRUCTOR_NAME, "(JII[B)V"))) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, "exceptionHandlerGetMethodID");
    return;
  }
  if (!(jexcep = (*env)->NewObject(env, jcexcep, jinit, (jlong)(intptr_t) handle, err, type, jbuf))) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, "exceptionHandlerNewObject");
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

JAVA_STATIC_METHOD(
  org_a11y_BrlAPI_Native, getMajorVersion, jint
) {
  getVersionNumbers();
  return majorVersion;
}

JAVA_STATIC_METHOD(
  org_a11y_BrlAPI_Native, getMinorVersion, jint
) {
  getVersionNumbers();
  return minorVersion;
}

JAVA_STATIC_METHOD(
  org_a11y_BrlAPI_Native, getRevision, jint
) {
  getVersionNumbers();
  return revision;
}

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

  handle = malloc(brlapi_getHandleSize());
  if (!handle) {
    throwException(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
    return -1;
  }

  SET_HANDLE(env, this, handle, -1);

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (JclientSettings) {
    GET_CLASS(env, jcclientSettings, JclientSettings, -1);
    GET_FIELD(env, clientAuthID, jcclientSettings, "authorizationSchemes", "Ljava/lang/String;", -1);
    GET_FIELD(env, clientHostID, jcclientSettings, "serverHost", "Ljava/lang/String;", -1);

    PclientSettings = &clientSettings;
    if ((auth = GET_VALUE(env, Object, JclientSettings, clientAuthID))) {
      if (!(clientSettings.auth = (char *)(*env)->GetStringUTFChars(env, auth, NULL))) {
	throwException(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
	return -1;
      }
    } else clientSettings.auth = NULL;
    if ((host = GET_VALUE(env, Object, JclientSettings, clientHostID))) {
      if (!(clientSettings.host = (char *)(*env)->GetStringUTFChars(env, host, NULL))) {
	throwException(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
	return -1;
      }
    } else clientSettings.host = NULL;
  } else PclientSettings = NULL;

  if (JusedSettings)
    PusedSettings = &usedSettings;
  else
    PusedSettings = NULL;

  if ((result = brlapi__openConnection(handle, PclientSettings, PusedSettings)) < 0) {
    throwError(env, __func__);
    return -1;
  }

  (void) brlapi__setExceptionHandler(handle, exceptionHandler);

  if (JclientSettings) {
    if (clientSettings.auth)
      (*env)->ReleaseStringUTFChars(env, auth,  clientSettings.auth); 
    if (clientSettings.host)
      (*env)->ReleaseStringUTFChars(env, host, clientSettings.host); 
  }

  if (PusedSettings) {
    GET_CLASS(env, jcusedSettings, JusedSettings, -1);
    GET_FIELD(env, usedAuthID, jcusedSettings, "authorizationSchemes", "Ljava/lang/String;", -1);
    GET_FIELD(env, usedHostID, jcusedSettings, "serverHost", "Ljava/lang/String;", -1);

    auth = (*env)->NewStringUTF(env, usedSettings.auth);
    if (!auth) {
      throwException(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
      return -1;
    }
    str = (*env)->GetStringUTFChars(env, auth, NULL);
    SET_VALUE(env, Object, JusedSettings, usedAuthID, auth);
    (*env)->ReleaseStringUTFChars(env, auth, str);

    host = (*env)->NewStringUTF(env, usedSettings.host);
    if (!host) {
      throwException(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
      return -1;
    }
    str = (*env)->GetStringUTFChars(env, host, NULL);
    SET_VALUE(env, Object, JusedSettings, usedHostID, host);
    (*env)->ReleaseStringUTFChars(env, host, str);
  }

  return (jint) result;
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, closeConnection, void
) {
  SET_GLOBAL_JAVA_ENVIRONMENT(env);
  GET_HANDLE(env, this, );
  brlapi__closeConnection(handle);
  free(handle);
  SET_HANDLE(env, this, NULL, );
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, getDriverName, jstring
) {
  char name[32];
  GET_HANDLE(env, this, NULL);

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (brlapi__getDriverName(handle, name, sizeof(name)) < 0) {
    throwError(env, __func__);
    return NULL;
  }

  name[sizeof(name)-1] = 0;
  return (*env)->NewStringUTF(env, name);
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, getModelIdentifier, jstring
) {
  char identifier[32];
  GET_HANDLE(env, this, NULL);

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (brlapi__getModelIdentifier(handle, identifier, sizeof(identifier)) < 0) {
    throwError(env, __func__);
    return NULL;
  }

  identifier[sizeof(identifier)-1] = 0;
  return (*env)->NewStringUTF(env, identifier);
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, getDisplaySize, jobject
) {
  unsigned int x, y;
  jclass jcsize;
  jmethodID jinit;
  jobject jsize;
  GET_HANDLE(env, this, NULL);

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (brlapi__getDisplaySize(handle, &x, &y) < 0) {
    throwError(env, __func__);
    return NULL;
  }

  if (!(jcsize = (*env)->FindClass(env, BRLAPI_OBJECT("DisplaySize")))) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return NULL;
  }
  if (!(jinit = (*env)->GetMethodID(env, jcsize, JAVA_CONSTRUCTOR_NAME, "(II)V"))) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return NULL;
  }
  if (!(jsize = (*env)->NewObject(env, jcsize, jinit, x, y))) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
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
  GET_HANDLE(env, this, -1);
  
  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  tty = (int)jtty; 
  if (!jdriver)
    driver = NULL;
  else
    if (!(driver = (char *)(*env)->GetStringUTFChars(env, jdriver, NULL))) {
      throwException(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
      return -1;
    }

  result = brlapi__enterTtyMode(handle, tty,driver);
  if (result < 0) {
    throwError(env, __func__);
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
  GET_HANDLE(env, this, );
  
  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!jttys) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return;
  }
  if (!(ttys = (*env)->GetIntArrayElements(env, jttys, NULL))) {
    throwException(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
    return;
  }

  if (!jdriver) {
    driver = NULL;
  } else if (!(driver = (char *)(*env)->GetStringUTFChars(env, jdriver, NULL))) {
    throwException(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
    return;
  }

  result = brlapi__enterTtyModeWithPath(handle, ttys,(*env)->GetArrayLength(env,jttys),driver);
  (*env)->ReleaseIntArrayElements(env, jttys, ttys, JNI_ABORT);
  if (result < 0) {
    throwError(env, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, leaveTtyMode, void
) {
  SET_GLOBAL_JAVA_ENVIRONMENT(env);
  GET_HANDLE(env, this, );

  if (brlapi__leaveTtyMode(handle) < 0) {
    throwError(env, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, setFocus, void,
  jint jarg1
) {
  int arg1 ;
  GET_HANDLE(env, this, );
  
  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  arg1 = (int)jarg1; 
  if (brlapi__setFocus(handle, arg1) < 0) {
    throwError(env, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, writeText, void,
  jint jarg1, jstring jarg2
) {
  brlapi_writeArguments_t s = BRLAPI_WRITEARGUMENTS_INITIALIZER;
  int result;
  GET_HANDLE(env, this, );
  
  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  s.cursor = (int)jarg1; 

  if (jarg2) {
    s.regionBegin = 1;
    s.regionSize = (*env)->GetStringLength(env, jarg2);

    if (!(s.text = (char *)(*env)->GetStringUTFChars(env, jarg2, NULL))) {
      throwException(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
      return;
    }
    s.charset = "UTF-8";
  }

  result = brlapi__write(handle, &s);
  if (jarg2)
    (*env)->ReleaseStringUTFChars(env, jarg2, s.text); 

  if (result < 0) {
    throwError(env, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, writeDots, void,
  jbyteArray jarg1
) {
  jbyte *arg1;
  int result;
  GET_HANDLE(env, this, );
  
  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!jarg1) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return;
  }
  arg1 = (*env)->GetByteArrayElements(env, jarg1, NULL);
  if (!arg1) {
    throwException(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
    return;
  }

  result = brlapi__writeDots(handle, (const unsigned char *)arg1);
  (*env)->ReleaseByteArrayElements(env, jarg1, arg1, JNI_ABORT); 
  
  if (result < 0) {
    throwError(env, __func__);
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
  GET_HANDLE(env, this, );

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!jarguments) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return;
  }

  GET_CLASS(env, jcwriteArguments, jarguments, );
  GET_FIELD(env, displayNumberID, jcwriteArguments,
            "displayNumber", JAVA_SIG_INT, );
  GET_FIELD(env, regionBeginID, jcwriteArguments,
            "regionBegin", JAVA_SIG_INT, );
  GET_FIELD(env, regionSizeID, jcwriteArguments,
            "regionSize", JAVA_SIG_INT, );
  GET_FIELD(env, textID, jcwriteArguments,
            "text", JAVA_SIG_STRING, );
  GET_FIELD(env, andMaskID, jcwriteArguments,
            "andMask", JAVA_SIG_ARRAY(JAVA_SIG_BYTE), );
  GET_FIELD(env, orMaskID, jcwriteArguments,
            "orMask", JAVA_SIG_ARRAY(JAVA_SIG_BYTE), );
  GET_FIELD(env, cursorID, jcwriteArguments, 
            "cursor", JAVA_SIG_INT, );

  arguments.displayNumber = GET_VALUE(env, Int, jarguments, displayNumberID);
  arguments.regionBegin   = GET_VALUE(env, Int, jarguments, regionBeginID);
  arguments.regionSize    = GET_VALUE(env, Int, jarguments, regionSizeID);
  if ((text  = GET_VALUE(env, Object, jarguments, textID)))
    arguments.text   = (char *)(*env)->GetStringUTFChars(env, text, NULL);
  else 
    arguments.text  = NULL;
  if ((andMask = GET_VALUE(env, Object, jarguments, andMaskID)))
    arguments.andMask  = (unsigned char *)(*env)->GetByteArrayElements(env, andMask, NULL);
  else
    arguments.andMask = NULL;
  if ((orMask  = GET_VALUE(env, Object, jarguments, orMaskID)))
    arguments.orMask   = (unsigned char *)(*env)->GetByteArrayElements(env, orMask, NULL);
  else
    arguments.orMask  = NULL;
  arguments.cursor     = GET_VALUE(env, Int, jarguments, cursorID);
  arguments.charset = "UTF-8";

  result = brlapi__write(handle, &arguments);

  if (text)
    (*env)->ReleaseStringUTFChars(env, text, arguments.text); 
  if (andMask)
    (*env)->ReleaseByteArrayElements(env, andMask, (jbyte*) arguments.andMask, JNI_ABORT); 
  if (orMask)
    (*env)->ReleaseByteArrayElements(env, orMask,  (jbyte*) arguments.orMask,  JNI_ABORT); 

  if (result < 0) {
    throwError(env, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, readKey, jlong,
  jboolean jblock
) {
  brlapi_keyCode_t code;
  int result;
  GET_HANDLE(env, this, -1);

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  result = brlapi__readKey(handle, (int) jblock, &code);

  if (result < 0) {
    throwError(env, __func__);
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
  GET_HANDLE(env, this, -1);

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  result = brlapi__readKeyWithTimeout(handle, timeout_ms, &code);

  if (result < 0) {
    throwError(env, __func__);
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
  GET_HANDLE(env, this, );

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!js) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return;
  }

  n = (unsigned int) (*env)->GetArrayLength(env, js);
  s = (*env)->GetLongArrayElements(env, js, NULL);

  // XXX jlong != brlapi_keyCode_t probably
  result = brlapi__ignoreKeys(handle, jrange, (const brlapi_keyCode_t *)s, n);
  (*env)->ReleaseLongArrayElements(env, js, s, JNI_ABORT);
  
  if (result < 0) {
    throwError(env, __func__);
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
  GET_HANDLE(env, this, );

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!js) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return;
  }

  n = (unsigned int) (*env)->GetArrayLength(env, js);
  s = (*env)->GetLongArrayElements(env, js, NULL);

  // XXX jlong != brlapi_keyCode_t probably
  result = brlapi__acceptKeys(handle, jrange, (const brlapi_keyCode_t *)s, n);
  (*env)->ReleaseLongArrayElements(env, js, s, JNI_ABORT);

  if (result < 0) {
    throwError(env, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, ignoreAllKeys, void
) {
  GET_HANDLE(env, this, );

  if (brlapi__ignoreAllKeys(handle) < 0)
    throwError(env, __func__);
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, acceptAllKeys, void
) {
  GET_HANDLE(env, this, );

  if (brlapi__acceptAllKeys(handle) < 0)
    throwError(env, __func__);
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, ignoreKeyRanges, void,
  jobjectArray js
) {
  unsigned int n;
  GET_HANDLE(env, this, );

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!js) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return;
  }

  n = (unsigned int) (*env)->GetArrayLength(env, js);

  {
    unsigned int i;
    brlapi_range_t s[n];

    for (i=0; i<n; i++) {
      jlongArray jl = (*env)->GetObjectArrayElement(env, js, i);
      jlong *l = (*env)->GetLongArrayElements(env, jl, NULL);
      s[i].first = l[0];
      s[i].last = l[1];
      (*env)->ReleaseLongArrayElements(env, jl, l, JNI_ABORT);
    }
    if (brlapi__ignoreKeyRanges(handle, s, n)) {
      throwError(env, __func__);
      return;
    }
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, acceptKeyRanges, void,
  jobjectArray js
) {
  unsigned int n;
  GET_HANDLE(env, this, );

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!js) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return;
  }

  n = (unsigned int) (*env)->GetArrayLength(env, js);

  {
    unsigned int i;
    brlapi_range_t s[n];

    for (i=0; i<n; i++) {
      jlongArray jl = (*env)->GetObjectArrayElement(env, js, i);
      jlong *l = (*env)->GetLongArrayElements(env, jl, NULL);
      s[i].first = l[0];
      s[i].last = l[1];
      (*env)->ReleaseLongArrayElements(env, jl, l, JNI_ABORT);
    }
    if (brlapi__acceptKeyRanges(handle, s, n)) {
      throwError(env, __func__);
      return;
    }
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, enterRawMode, void,
  jstring jdriver
) {
  SET_GLOBAL_JAVA_ENVIRONMENT(env);
  char *driver;
  int res;
  GET_HANDLE(env, this, );

  if (!jdriver) {
    driver = NULL;
  } else if (!(driver = (char *)(*env)->GetStringUTFChars(env, jdriver, NULL))) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return;
  }
  res = brlapi__enterRawMode(handle, driver);
  if (jdriver) (*env)->ReleaseStringUTFChars(env, jdriver, driver);
  if (res < 0) {
    throwError(env, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Native, leaveRawMode, void
) {
  SET_GLOBAL_JAVA_ENVIRONMENT(env);
  GET_HANDLE(env, this, );

  if (brlapi__leaveRawMode(handle) < 0) {
    throwError(env, __func__);
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
  GET_HANDLE(env, this, -1);

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!jbuf) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return -1;
  }

  n = (unsigned int) (*env)->GetArrayLength(env, jbuf);
  buf = (*env)->GetByteArrayElements(env, jbuf, NULL);

  result = brlapi__sendRaw(handle, (const unsigned char *)buf, n);
  (*env)->ReleaseByteArrayElements(env, jbuf, buf, JNI_ABORT);

  if (result < 0) {
    throwError(env, __func__);
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
  GET_HANDLE(env, this, -1);

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!jbuf) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return -1;
  }

  n = (unsigned int) (*env)->GetArrayLength(env, jbuf);
  buf = (*env)->GetByteArrayElements(env, jbuf, NULL);

  result = brlapi__recvRaw(handle, (unsigned char *)buf, n);

  if (result < 0) {
    (*env)->ReleaseByteArrayElements(env, jbuf, buf, JNI_ABORT);
    throwError(env, __func__);
    return -1;
  }

  (*env)->ReleaseByteArrayElements(env, jbuf, buf, 0);
  return (jint) result;
}

JAVA_STATIC_METHOD(
  org_a11y_BrlAPI_Native, getPacketTypeName, jstring,
  jlong jtype
) {
  const char *type;

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!(type = brlapi_getPacketTypeName((brlapi_packetType_t) jtype))) {
    throwError(env, __func__);
    return NULL;
  }

  return (*env)->NewStringUTF(env, type);
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Error, toString, jstring
) {
  jstring jerrfun;
  brlapi_error_t error;
  const char *res;

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  GET_CLASS(env, jcerr, this, NULL);
  GET_FIELD(env, brlerrnoID,  jcerr, "brlerrno",  JAVA_SIG_INT,    NULL);
  GET_FIELD(env, libcerrnoID, jcerr, "libcerrno", JAVA_SIG_INT,    NULL);
  GET_FIELD(env, gaierrnoID,  jcerr, "gaierrno",  JAVA_SIG_INT,    NULL);
  GET_FIELD(env, errfunID,    jcerr, "errfun",    JAVA_SIG_STRING, NULL);

  error.brlerrno  = GET_VALUE(env, Int, this, brlerrnoID);
  error.libcerrno = GET_VALUE(env, Int, this, libcerrnoID);
  error.gaierrno  = GET_VALUE(env, Int, this, gaierrnoID);
  if (!(jerrfun = GET_VALUE(env, Object, this, errfunID))) {
    error.errfun = NULL;
  } else if (!(error.errfun = (char *)(*env)->GetStringUTFChars(env, jerrfun, NULL))) {
    throwException(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
    return NULL;
  }
  res = brlapi_strerror(&error);
  if (jerrfun)
    (*env)->ReleaseStringUTFChars(env, jerrfun, error.errfun);
  return (*env)->NewStringUTF(env, res);
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Exception, toString, jstring
) {
  jarray jbuf;
  long type;
  jbyte *buf;
  int size;
  char errmsg[256];

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!this) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return NULL;
  }
  GET_CLASS(env, jcerr, this, NULL);
  GET_FIELD(env, errnoID,  jcerr, "errno",  JAVA_SIG_INT, NULL);
  GET_FIELD(env, typeID,   jcerr, "type",   JAVA_SIG_INT, NULL);
  GET_FIELD(env, bufID,    jcerr, "buf",    JAVA_SIG_INT, NULL);

  GET_HANDLE(env, this, NULL);
  type  = GET_VALUE(env, Int, this, typeID);
  if (!(jbuf  = GET_VALUE(env, Object, this, typeID))) {
    throwException(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return NULL;
  }
  size  = (*env)->GetArrayLength(env, jbuf);
  buf = (*env)->GetByteArrayElements(env, jbuf, NULL);

  brlapi__strexception(handle, errmsg, sizeof(errmsg), errno, type, buf, size); 

  return (*env)->NewStringUTF(env, errmsg);
}

JAVA_INSTANCE_METHOD(
  org_a11y_BrlAPI_Key, expandKeyCode, void,
  jlong jkey
) {
  brlapi_keyCode_t key = jkey;
  brlapi_expandedKeyCode_t ekc;

  GET_CLASS(env, jckey, this, );
  GET_FIELD(env, typeID,     jckey, "typeValue",     JAVA_SIG_INT, );
  GET_FIELD(env, commandID,  jckey, "commandValue",  JAVA_SIG_INT, );
  GET_FIELD(env, argumentID, jckey, "argumentValue", JAVA_SIG_INT, );
  GET_FIELD(env, flagsID,    jckey, "flagsValue",    JAVA_SIG_INT, );

  brlapi_expandKeyCode(key, &ekc);
  SET_VALUE(env, Int, this, typeID,     ekc.type);
  SET_VALUE(env, Int, this, commandID,  ekc.command);
  SET_VALUE(env, Int, this, argumentID, ekc.argument);
  SET_VALUE(env, Int, this, flagsID,    ekc.flags);
}
