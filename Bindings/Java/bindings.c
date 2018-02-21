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
#define BRLAPI_OBJECT(name) "org/a11y/brlapi/" name

/* TODO: threads */
static JNIEnv *globalJavaEnvironment;
#define SET_GLOBAL_JAVA_ENVIRONMENT(env) (globalJavaEnvironment = (env))
#define GET_GLOBAL_JAVA_ENVIRONMENT(env) JNIEnv *env = globalJavaEnvironment

static void
throwJavaError (JNIEnv *env, const char *object, const char *message) {
  (*env)->ExceptionClear(env);
  jclass class = (*env)->FindClass(env, object);

  if (class) {
    (*env)->ThrowNew(env, class, message);
  } else {
    fprintf(stderr,"couldn't find exception object: %s: %s!\n", object, message);
  }
}

static void
throwConnectionError (JNIEnv *env, const char *function) {
  jclass class = (*env)->FindClass(env, BRLAPI_OBJECT("ConnectionError"));
  if (!class) return;

  jmethodID constructor = (*env)->GetMethodID(env, class,
    JAVA_CONSTRUCTOR_NAME,
    JAVA_SIG_CONSTRUCTOR(
      JAVA_SIG_INT // api error
      JAVA_SIG_INT // os error
      JAVA_SIG_INT // gai error
      JAVA_SIG_STRING // function name
    )
  );
  if (!constructor) return;

  jstring jFunction = (*env)->NewStringUTF(env, function);
  if (!jFunction) return;
  jobject object = (*env)->NewObject(env, class, constructor, brlapi_errno, brlapi_libcerrno, brlapi_gaierrno, jFunction);

  if (object) {
    (*env)->ExceptionClear(env);
    (*env)->Throw(env, object);
  } else if (jFunction) {
    (*env)->ReleaseStringUTFChars(env, jFunction, function);
  }
}

#define GET_CLASS(env, class, obj, ret) \
  jclass class; \
  do { \
    if (!((class) = (*(env))->GetObjectClass((env), (obj)))) { \
      throwJavaError((env), JAVA_OBJECT_NULL_POINTER_EXCEPTION, #obj " -> " #class); \
      return ret; \
    } \
  } while (0)

#define GET_FIELD(env, field, class, name, signature, ret) \
  jfieldID field; \
  do { \
    if (!(field = (*(env))->GetFieldID((env), (class), (name), (signature)))) {\
      throwJavaError((env), JAVA_OBJECT_NULL_POINTER_EXCEPTION, #class "." name); \
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
      throwJavaError((env), JAVA_OBJECT_NULL_POINTER_EXCEPTION, "connection has been closed"); \
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
exceptionHandler (brlapi_handle_t *handle, int error, brlapi_packetType_t type, const void *packet, size_t size) {
  GET_GLOBAL_JAVA_ENVIRONMENT(env);

  jbyteArray jPacket = (*env)->NewByteArray(env, size);
  if (!jPacket) return;
  (*env)->SetByteArrayRegion(env, jPacket, 0, size, (jbyte *) packet);

  jclass class = (*env)->FindClass(env, BRLAPI_OBJECT("ConnectionException"));
  if (!class) return;

  jmethodID constructor = (*env)->GetMethodID(env, class,
    JAVA_CONSTRUCTOR_NAME,
    JAVA_SIG_CONSTRUCTOR(
      JAVA_SIG_LONG // handle
      JAVA_SIG_INT // error
      JAVA_SIG_INT // type
      JAVA_SIG_ARRAY(JAVA_SIG_BYTE) // packet
    )
  );
  if (!constructor) return;

  jclass object = (*env)->NewObject(env, class, constructor,
    (jlong) (intptr_t) handle, error, type, jPacket
  );
  if (!object) return;

  (*env)->ExceptionClear(env);
  (*env)->Throw(env, object);
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
  org_a11y_brlapi_Version, getMajor, jint
) {
  getVersionNumbers();
  return majorVersion;
}

JAVA_STATIC_METHOD(
  org_a11y_brlapi_Version, getMinor, jint
) {
  getVersionNumbers();
  return minorVersion;
}

JAVA_STATIC_METHOD(
  org_a11y_brlapi_Version, getRevision, jint
) {
  getVersionNumbers();
  return revision;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, openConnection, jint,
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
    throwJavaError(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
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
	throwJavaError(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
	return -1;
      }
    } else clientSettings.auth = NULL;
    if ((host = GET_VALUE(env, Object, JclientSettings, clientHostID))) {
      if (!(clientSettings.host = (char *)(*env)->GetStringUTFChars(env, host, NULL))) {
	throwJavaError(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
	return -1;
      }
    } else clientSettings.host = NULL;
  } else PclientSettings = NULL;

  if (JusedSettings)
    PusedSettings = &usedSettings;
  else
    PusedSettings = NULL;

  if ((result = brlapi__openConnection(handle, PclientSettings, PusedSettings)) < 0) {
    throwConnectionError(env, __func__);
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
      throwJavaError(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
      return -1;
    }
    str = (*env)->GetStringUTFChars(env, auth, NULL);
    SET_VALUE(env, Object, JusedSettings, usedAuthID, auth);
    (*env)->ReleaseStringUTFChars(env, auth, str);

    host = (*env)->NewStringUTF(env, usedSettings.host);
    if (!host) {
      throwJavaError(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
      return -1;
    }
    str = (*env)->GetStringUTFChars(env, host, NULL);
    SET_VALUE(env, Object, JusedSettings, usedHostID, host);
    (*env)->ReleaseStringUTFChars(env, host, str);
  }

  return (jint) result;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, closeConnection, void
) {
  SET_GLOBAL_JAVA_ENVIRONMENT(env);
  GET_HANDLE(env, this, );
  brlapi__closeConnection(handle);
  free(handle);
  SET_HANDLE(env, this, NULL, );
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, getDriverName, jstring
) {
  char name[32];
  GET_HANDLE(env, this, NULL);

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (brlapi__getDriverName(handle, name, sizeof(name)) < 0) {
    throwConnectionError(env, __func__);
    return NULL;
  }

  name[sizeof(name)-1] = 0;
  return (*env)->NewStringUTF(env, name);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, getModelIdentifier, jstring
) {
  char identifier[32];
  GET_HANDLE(env, this, NULL);

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (brlapi__getModelIdentifier(handle, identifier, sizeof(identifier)) < 0) {
    throwConnectionError(env, __func__);
    return NULL;
  }

  identifier[sizeof(identifier)-1] = 0;
  return (*env)->NewStringUTF(env, identifier);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, getDisplaySize, jobject
) {
  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  GET_HANDLE(env, this, NULL);

  unsigned int width, height;
  if (brlapi__getDisplaySize(handle, &width, &height) < 0) {
    throwConnectionError(env, __func__);
    return NULL;
  }

  jclass class = (*env)->FindClass(env, BRLAPI_OBJECT("DisplaySize"));
  if (!class) return NULL;

  jmethodID constructor = (*env)->GetMethodID(env, class,
    JAVA_CONSTRUCTOR_NAME,
    JAVA_SIG_CONSTRUCTOR(
      JAVA_SIG_INT // width
      JAVA_SIG_INT // height
    )
  );
  if (!constructor) return NULL;

  jobject object = (*env)->NewObject(env, class, constructor, width, height);
  if (!object) return NULL;
  return object;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, enterTtyMode, jint,
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
      throwJavaError(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
      return -1;
    }

  result = brlapi__enterTtyMode(handle, tty,driver);
  if (result < 0) {
    throwConnectionError(env, __func__);
    return -1;
  }

  return (jint) result;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, enterTtyModeWithPath, void,
  jintArray jttys, jstring jdriver
) {
  jint *ttys ;
  char *driver;
  int result;
  GET_HANDLE(env, this, );
  
  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!jttys) {
    throwJavaError(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return;
  }
  if (!(ttys = (*env)->GetIntArrayElements(env, jttys, NULL))) {
    throwJavaError(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
    return;
  }

  if (!jdriver) {
    driver = NULL;
  } else if (!(driver = (char *)(*env)->GetStringUTFChars(env, jdriver, NULL))) {
    throwJavaError(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
    return;
  }

  result = brlapi__enterTtyModeWithPath(handle, ttys,(*env)->GetArrayLength(env,jttys),driver);
  (*env)->ReleaseIntArrayElements(env, jttys, ttys, JNI_ABORT);
  if (result < 0) {
    throwConnectionError(env, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, leaveTtyMode, void
) {
  SET_GLOBAL_JAVA_ENVIRONMENT(env);
  GET_HANDLE(env, this, );

  if (brlapi__leaveTtyMode(handle) < 0) {
    throwConnectionError(env, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, setFocus, void,
  jint jarg1
) {
  int arg1 ;
  GET_HANDLE(env, this, );
  
  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  arg1 = (int)jarg1; 
  if (brlapi__setFocus(handle, arg1) < 0) {
    throwConnectionError(env, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, writeText, void,
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
      throwJavaError(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
      return;
    }
    s.charset = "UTF-8";
  }

  result = brlapi__write(handle, &s);
  if (jarg2)
    (*env)->ReleaseStringUTFChars(env, jarg2, s.text); 

  if (result < 0) {
    throwConnectionError(env, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, writeDots, void,
  jbyteArray jarg1
) {
  jbyte *arg1;
  int result;
  GET_HANDLE(env, this, );
  
  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!jarg1) {
    throwJavaError(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return;
  }
  arg1 = (*env)->GetByteArrayElements(env, jarg1, NULL);
  if (!arg1) {
    throwJavaError(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
    return;
  }

  result = brlapi__writeDots(handle, (const unsigned char *)arg1);
  (*env)->ReleaseByteArrayElements(env, jarg1, arg1, JNI_ABORT); 
  
  if (result < 0) {
    throwConnectionError(env, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, write, void,
  jobject jarguments
) {
  brlapi_writeArguments_t arguments = BRLAPI_WRITEARGUMENTS_INITIALIZER;
  int result;
  jstring text, andMask, orMask;
  GET_HANDLE(env, this, );

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!jarguments) {
    throwJavaError(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
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
            "cursorPosition", JAVA_SIG_INT, );

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
    throwConnectionError(env, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, readKey, jlong,
  jboolean jblock
) {
  brlapi_keyCode_t code;
  int result;
  GET_HANDLE(env, this, -1);

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  result = brlapi__readKey(handle, (int) jblock, &code);

  if (result < 0) {
    throwConnectionError(env, __func__);
    return -1;
  }

  if (!result) return (jlong)(-1);
  return (jlong)code;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, readKeyWithTimeout, jlong,
  jint timeout_ms
) {
  brlapi_keyCode_t code;
  int result;
  GET_HANDLE(env, this, -1);

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  result = brlapi__readKeyWithTimeout(handle, timeout_ms, &code);

  if (result < 0) {
    throwConnectionError(env, __func__);
    return -1;
  }

  if (!result) return (jlong)(-1);
  return (jlong)code;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, ignoreKeys, void,
  jlong jrange, jlongArray js
) {
  jlong *s;
  unsigned int n;
  int result;
  GET_HANDLE(env, this, );

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!js) {
    throwJavaError(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return;
  }

  n = (unsigned int) (*env)->GetArrayLength(env, js);
  s = (*env)->GetLongArrayElements(env, js, NULL);

  // XXX jlong != brlapi_keyCode_t probably
  result = brlapi__ignoreKeys(handle, jrange, (const brlapi_keyCode_t *)s, n);
  (*env)->ReleaseLongArrayElements(env, js, s, JNI_ABORT);
  
  if (result < 0) {
    throwConnectionError(env, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, acceptKeys, void,
  jlong jrange, jlongArray js
) {
  jlong *s;
  unsigned int n;
  int result;
  GET_HANDLE(env, this, );

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!js) {
    throwJavaError(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return;
  }

  n = (unsigned int) (*env)->GetArrayLength(env, js);
  s = (*env)->GetLongArrayElements(env, js, NULL);

  // XXX jlong != brlapi_keyCode_t probably
  result = brlapi__acceptKeys(handle, jrange, (const brlapi_keyCode_t *)s, n);
  (*env)->ReleaseLongArrayElements(env, js, s, JNI_ABORT);

  if (result < 0) {
    throwConnectionError(env, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, ignoreAllKeys, void
) {
  GET_HANDLE(env, this, );

  if (brlapi__ignoreAllKeys(handle) < 0)
    throwConnectionError(env, __func__);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, acceptAllKeys, void
) {
  GET_HANDLE(env, this, );

  if (brlapi__acceptAllKeys(handle) < 0)
    throwConnectionError(env, __func__);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, ignoreKeyRanges, void,
  jobjectArray js
) {
  unsigned int n;
  GET_HANDLE(env, this, );

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!js) {
    throwJavaError(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
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
      throwConnectionError(env, __func__);
      return;
    }
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, acceptKeyRanges, void,
  jobjectArray js
) {
  unsigned int n;
  GET_HANDLE(env, this, );

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!js) {
    throwJavaError(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
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
      throwConnectionError(env, __func__);
      return;
    }
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, enterRawMode, void,
  jstring jdriver
) {
  SET_GLOBAL_JAVA_ENVIRONMENT(env);
  char *driver;
  int res;
  GET_HANDLE(env, this, );

  if (!jdriver) {
    driver = NULL;
  } else if (!(driver = (char *)(*env)->GetStringUTFChars(env, jdriver, NULL))) {
    throwJavaError(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return;
  }
  res = brlapi__enterRawMode(handle, driver);
  if (jdriver) (*env)->ReleaseStringUTFChars(env, jdriver, driver);
  if (res < 0) {
    throwConnectionError(env, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, leaveRawMode, void
) {
  SET_GLOBAL_JAVA_ENVIRONMENT(env);
  GET_HANDLE(env, this, );

  if (brlapi__leaveRawMode(handle) < 0) {
    throwConnectionError(env, __func__);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, sendRaw, jint,
  jbyteArray jbuf
) {
  jbyte *buf;
  unsigned int n;
  int result;
  GET_HANDLE(env, this, -1);

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!jbuf) {
    throwJavaError(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return -1;
  }

  n = (unsigned int) (*env)->GetArrayLength(env, jbuf);
  buf = (*env)->GetByteArrayElements(env, jbuf, NULL);

  result = brlapi__sendRaw(handle, (const unsigned char *)buf, n);
  (*env)->ReleaseByteArrayElements(env, jbuf, buf, JNI_ABORT);

  if (result < 0) {
    throwConnectionError(env, __func__);
    return -1;
  }

  return (jint) result;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, recvRaw, jint,
  jbyteArray jbuf
) {
  jbyte *buf;
  unsigned int n;
  int result;
  GET_HANDLE(env, this, -1);

  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  if (!jbuf) {
    throwJavaError(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return -1;
  }

  n = (unsigned int) (*env)->GetArrayLength(env, jbuf);
  buf = (*env)->GetByteArrayElements(env, jbuf, NULL);

  result = brlapi__recvRaw(handle, (unsigned char *)buf, n);

  if (result < 0) {
    (*env)->ReleaseByteArrayElements(env, jbuf, buf, JNI_ABORT);
    throwConnectionError(env, __func__);
    return -1;
  }

  (*env)->ReleaseByteArrayElements(env, jbuf, buf, 0);
  return (jint) result;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionError, toString, jstring
) {
  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  GET_CLASS(env, class, this, NULL);
  brlapi_error_t error;
  memset(&error, 0, sizeof(error));

  GET_FIELD(env, apiErrorID, class, "apiError", JAVA_SIG_INT, NULL);
  error.brlerrno = GET_VALUE(env, Int, this, apiErrorID);

  GET_FIELD(env, osErrorID, class, "osError", JAVA_SIG_INT, NULL);
  error.libcerrno = GET_VALUE(env, Int, this, osErrorID);

  GET_FIELD(env, gaiErrorID, class, "gaiError", JAVA_SIG_INT, NULL);
  error.gaierrno = GET_VALUE(env, Int, this, gaiErrorID);

  GET_FIELD(env, functionID, class, "functionName", JAVA_SIG_STRING, NULL);
  jstring jFunction = GET_VALUE(env, Object, this, functionID);

  if (!jFunction) {
    error.errfun = NULL;
  } else {
    const char *cFunction = (*env)->GetStringUTFChars(env, jFunction, NULL);

    if (!cFunction) return NULL;
    error.errfun = cFunction;
  }

  const char *cMessage = brlapi_strerror(&error);
  if (error.errfun) (*env)->ReleaseStringUTFChars(env, jFunction, error.errfun);

  return (*env)->NewStringUTF(env, cMessage);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionException, toString, jstring
) {
  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  GET_HANDLE(env, this, NULL);
  GET_CLASS(env, class, this, NULL);

  GET_FIELD(env, errorID, class, "errorNumber", JAVA_SIG_INT, NULL);
  jint error = GET_VALUE(env, Int, this, errorID);

  GET_FIELD(env, typeID, class, "packetType", JAVA_SIG_INT, NULL);
  jint type = GET_VALUE(env, Int, this, typeID);

  GET_FIELD(env, packetID, class, "failedPacket", JAVA_SIG_ARRAY(JAVA_SIG_BYTE), NULL);
  jbyteArray jPacket = GET_VALUE(env, Object, this, packetID);

  if (!jPacket) {
    throwJavaError(env, JAVA_OBJECT_NULL_POINTER_EXCEPTION, __func__);
    return NULL;
  }

  jint size = (*env)->GetArrayLength(env, jPacket);
  jbyte *cPacket = (*env)->GetByteArrayElements(env, jPacket, NULL);

  if (!cPacket) {
    throwJavaError(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
    return NULL;
  }

  char buffer[0X100];
  brlapi__strexception(handle, buffer, sizeof(buffer), error, type, cPacket, size); 
  jstring message = (*env)->NewStringUTF(env, buffer);
  (*env)->ReleaseByteArrayElements(env, jPacket, cPacket, JNI_ABORT);

  return message;
}

JAVA_STATIC_METHOD(
  org_a11y_brlapi_ConnectionException, getPacketTypeName, jstring,
  jint type
) {
  SET_GLOBAL_JAVA_ENVIRONMENT(env);

  const char *cName = brlapi_getPacketTypeName((brlapi_packetType_t) type);
  if (!cName) return NULL;
  jstring jName = (*env)->NewStringUTF(env, cName);

  if (!jName) {
    throwJavaError(env, JAVA_OBJECT_OUT_OF_MEMORY_ERROR, __func__);
    return NULL;
  }

  return jName;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_Key, expandKeyCode, void,
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
