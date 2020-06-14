/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2020 by
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
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "bindings.h"

#define BRLAPI_NO_DEPRECATED
#define BRLAPI_NO_SINGLE_SESSION
#include "brlapi.h"
#define BRLAPI_OBJECT(name) "org/a11y/brlapi/" name

static jint jniVersion = 0;
static int libraryVersion_major = 0;
static int libraryVersion_minor = 0;
static int libraryVersion_revision = 0;

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_NativeLibrary, initializeNativeData, void
) {
  jniVersion = (*env)->GetVersion(env);

  brlapi_getLibraryVersion(
    &libraryVersion_major,
    &libraryVersion_minor,
    &libraryVersion_revision
  );
}

static void
logJavaVirtualMachineError (jint error, const char *method) {
  const char *message;

  switch (error) {
    case JNI_OK:
      message = "success";
      break;

    default:
#ifdef JNI_ERR
    case JNI_ERR:
#endif /* JNI_ERR */
      message = "unknown error";
      break;

#ifdef JNI_EDETACHED
    case JNI_EDETACHED:
      message = "thread not attached to virtual machine";
      break;
#endif /* JNI_EDETACHED */

#ifdef JNI_EVERSION
    case JNI_EVERSION:
      message = "version error";
      break;
#endif /* JNI_EVERSION */

#ifdef JNI_ENOMEM
    case JNI_ENOMEM:
      message = "not enough memory";
      break;
#endif /* JNI_ENOMEM */

#ifdef JNI_EEXIST
    case JNI_EEXIST:
      message = "virtual machine already created";
      break;
#endif /* JNI_EEXIST */

#ifdef JNI_EINVAL
    case JNI_EINVAL:
      message = "invalid argument";
      break;
#endif /* JNI_EINVAL */
  }

  fprintf(stderr, "Java virtual machine error %d in %s: %s\n", error, method, message);
}

static pthread_key_t threadKey_vm;

static void
destroyThreadKey_vm (void *value) {
  JavaVM *vm = value;
  (*vm)->DetachCurrentThread(vm);
}

static void
createThreadKey_vm (void) {
  pthread_key_create(&threadKey_vm, destroyThreadKey_vm);
}

static void
setThreadExitHandler (JavaVM *vm) {
  static pthread_once_t once = PTHREAD_ONCE_INIT;
  pthread_once(&once, createThreadKey_vm);
  pthread_setspecific(threadKey_vm, vm);
}

static JNIEnv *
getJavaEnvironment (brlapi_handle_t *handle) {
  JavaVM *vm = brlapi__getClientData(handle);
  void *env = NULL;

  if (vm) {
    jint result = (*vm)->GetEnv(vm, &env, jniVersion);

    if (result != JNI_OK) {
      if (result == JNI_EDETACHED) {
        JavaVMAttachArgs args = {
          .version = jniVersion,
          .name = NULL,
          .group = NULL
        };

        if ((result = (*vm)->AttachCurrentThread(vm, &env, &args)) == JNI_OK) {
          setThreadExitHandler(env);
        } else {
          logJavaVirtualMachineError(result, "AttachCurrentThread");
        }
      } else {
        logJavaVirtualMachineError(result, "GetEnv");
      }
    }
  }

  return env;
}

static void
throwJavaError (JNIEnv *env, const char *object, const char *message) {
  (*env)->ExceptionClear(env);
  jclass class = (*env)->FindClass(env, object);

  if (class) {
    (*env)->ThrowNew(env, class, message);
  } else {
    fprintf(stderr,"couldn't find object: %s: %s!\n", object, message);
  }
}

static void
throwConnectionError (JNIEnv *env) {
  {
    const char *object = NULL;

    switch (brlapi_errno) {
      case BRLAPI_ERROR_LIBCERR:
        switch (brlapi_libcerrno) {
          case EINTR:
            object = JAVA_OBJ_INTERRUPTED_IO_EXCEPTION;
            break;
        }

        break;
    }

    if (object) {
      throwJavaError(env, object, brlapi_errfun);
      return;
    }
  }

  jclass class = (*env)->FindClass(env, BRLAPI_OBJECT("ConnectionError"));
  if (!class) return;

  jmethodID constructor = JAVA_GET_CONSTRUCTOR(env, class,
    JAVA_SIG_INT // api error
    JAVA_SIG_INT // os error
    JAVA_SIG_INT // gai error
    JAVA_SIG_STRING // function name
  );

  if (!constructor) return;
  jstring jFunction;

  if (!brlapi_errfun) {
    jFunction = NULL;
  } else if (!(jFunction = (*env)->NewStringUTF(env, brlapi_errfun))) {
    return;
  }

  jobject object = (*env)->NewObject(
    env, class, constructor,
    brlapi_errno, brlapi_libcerrno, brlapi_gaierrno, jFunction
  );

  if (object) {
    (*env)->ExceptionClear(env);
    (*env)->Throw(env, object);
  } else if (jFunction) {
    (*env)->ReleaseStringUTFChars(env, jFunction, brlapi_errfun);
  }
}

static void BRLAPI_STDCALL
handleConnectionException (brlapi_handle_t *handle, int error, brlapi_packetType_t type, const void *packet, size_t size) {
  JNIEnv *env = getJavaEnvironment(handle);

  jbyteArray jPacket = (*env)->NewByteArray(env, size);
  if (!jPacket) return;
  (*env)->SetByteArrayRegion(env, jPacket, 0, size, (jbyte *) packet);

  jclass class = (*env)->FindClass(env, BRLAPI_OBJECT("ConnectionException"));
  if (!class) return;

  jmethodID constructor = JAVA_GET_CONSTRUCTOR(env, class,
    JAVA_SIG_LONG // handle
    JAVA_SIG_INT // error
    JAVA_SIG_INT // type
    JAVA_SIG_ARRAY(JAVA_SIG_BYTE) // packet
  );
  if (!constructor) return;

  jclass object = (*env)->NewObject(
    env, class, constructor,
    (jlong) (intptr_t) handle, error, type, jPacket
  );
  if (!object) return;

  (*env)->ExceptionClear(env);
  (*env)->Throw(env, object);
}

JAVA_STATIC_METHOD(
  org_a11y_brlapi_LibraryVersion, getMajor, jint
) {
  return libraryVersion_major;
}

JAVA_STATIC_METHOD(
  org_a11y_brlapi_LibraryVersion, getMinor, jint
) {
  return libraryVersion_minor;
}

JAVA_STATIC_METHOD(
  org_a11y_brlapi_LibraryVersion, getRevision, jint
) {
  return libraryVersion_revision;
}

#define GET_CLASS(env, class, object, ret) \
  jclass class; \
  do { \
    if (!((class) = (*(env))->GetObjectClass((env), (object)))) return ret; \
  } while (0)

#define FIND_FIELD(env, field, class, name, signature, ret) \
  jfieldID field; \
  do { \
    if (!(field = (*(env))->GetFieldID((env), (class), (name), (signature)))) return ret; \
  } while (0)

#define FIND_CONNECTION_HANDLE(env, object, ret) \
  GET_CLASS((env), class, (object), ret); \
  FIND_FIELD((env), field, class, "connectionHandle", JAVA_SIG_LONG, ret);

#define GET_CONNECTION_HANDLE(env, object, ret) \
  brlapi_handle_t *handle; \
  do { \
    FIND_CONNECTION_HANDLE((env), (object), ret); \
    handle = (void*) (intptr_t) JAVA_GET_FIELD((env), Long, (object), field); \
    if (!handle) { \
      throwJavaError((env), JAVA_OBJ_ILLEGAL_STATE_EXCEPTION, "connection has been closed"); \
      return ret; \
    } \
  } while (0)

#define SET_CONNECTION_HANDLE(env, object, value, ret) \
  do { \
    FIND_CONNECTION_HANDLE((env), (object), ret); \
    JAVA_SET_FIELD((env), Long, (object), field, (jlong) (intptr_t) (value)); \
  } while (0)

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, openConnection, jint,
  jobject jRequestedSettings, jobject jActualSettings
) {
  brlapi_handle_t *handle;
  int fileDescriptor;

  brlapi_connectionSettings_t cActualSettings, *pActualSettings;
  pActualSettings = jActualSettings? &cActualSettings: NULL;

  {
    brlapi_connectionSettings_t cRequestedSettings, *pRequestedSettings;
    memset(&cRequestedSettings, 0, sizeof(cRequestedSettings));

    jstring jRequestedAuth;
    jstring jRequestedHost;

    if (jRequestedSettings) {
      GET_CLASS(env, class, jRequestedSettings, -1);

      {
        FIND_FIELD(env, field, class, "authorizationSchemes", JAVA_SIG_STRING, -1);

        if (!(jRequestedAuth = JAVA_GET_FIELD(env, Object, jRequestedSettings, field))) {
          cRequestedSettings.auth = NULL;
        } else if (!(cRequestedSettings.auth = (char *) (*env)->GetStringUTFChars(env, jRequestedAuth, NULL))) {
          return -1;
        }
      }

      {
        FIND_FIELD(env, field, class, "serverHost", JAVA_SIG_STRING, -1);

        if (!(jRequestedHost = JAVA_GET_FIELD(env, Object, jRequestedSettings, field))) {
          cRequestedSettings.host = NULL;
        } else if (!(cRequestedSettings.host = (char *) (*env)->GetStringUTFChars(env, jRequestedHost, NULL))) {
          return -1;
        }
      }

      pRequestedSettings = &cRequestedSettings;
    } else {
      pRequestedSettings = NULL;
      jRequestedAuth = NULL;
      jRequestedHost = NULL;
    }

    if (!(handle = malloc(brlapi_getHandleSize()))) {
      throwJavaError(env, JAVA_OBJ_OUT_OF_MEMORY_ERROR, __func__);
      return -1;
    }

    if ((fileDescriptor = brlapi__openConnection(handle, pRequestedSettings, pActualSettings)) < 0) {
      free(handle);
      throwConnectionError(env);
      return -1;
    }

    if (pRequestedSettings) {
      if (cRequestedSettings.auth) {
        (*env)->ReleaseStringUTFChars(env, jRequestedAuth,  cRequestedSettings.auth); 
      }

      if (cRequestedSettings.host) {
        (*env)->ReleaseStringUTFChars(env, jRequestedHost, cRequestedSettings.host); 
      }
    }
  }

  if (pActualSettings) {
    GET_CLASS(env, class, jActualSettings, -1);

    if (cActualSettings.auth) {
      jstring auth = (*env)->NewStringUTF(env, cActualSettings.auth);
      if (!auth) return -1;

      FIND_FIELD(env, field, class, "authorizationSchemes", JAVA_SIG_STRING, -1);
      JAVA_SET_FIELD(env, Object, jActualSettings, field, auth);
    }

    if (cActualSettings.host) {
      jstring host = (*env)->NewStringUTF(env, cActualSettings.host);
      if (!host) return -1;

      FIND_FIELD(env, field, class, "serverHost", JAVA_SIG_STRING, -1);
      JAVA_SET_FIELD(env, Object, jActualSettings, field, host);
    }
  }

  {
    JavaVM *vm;
    (*env)->GetJavaVM(env, &vm);
    brlapi__setClientData(handle, vm);
  }

  brlapi__setExceptionHandler(handle, handleConnectionException);
  SET_CONNECTION_HANDLE(env, this, handle, -1);
  return (jint) fileDescriptor;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, closeConnection, void
) {
  GET_CONNECTION_HANDLE(env, this, );
  brlapi__closeConnection(handle);
  free(handle);
  SET_CONNECTION_HANDLE(env, this, NULL, );
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, getDriverName, jstring
) {
  GET_CONNECTION_HANDLE(env, this, NULL);
  char name[0X20];

  if (brlapi__getDriverName(handle, name, sizeof(name)) < 0) {
    throwConnectionError(env);
    return NULL;
  }

  name[sizeof(name)-1] = 0;
  return (*env)->NewStringUTF(env, name);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, getModelIdentifier, jstring
) {
  GET_CONNECTION_HANDLE(env, this, NULL);
  char identifier[0X20];

  if (brlapi__getModelIdentifier(handle, identifier, sizeof(identifier)) < 0) {
    throwConnectionError(env);
    return NULL;
  }

  identifier[sizeof(identifier)-1] = 0;
  return (*env)->NewStringUTF(env, identifier);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, getDisplaySize, jobject
) {
  GET_CONNECTION_HANDLE(env, this, NULL);

  unsigned int width, height;
  if (brlapi__getDisplaySize(handle, &width, &height) < 0) {
    throwConnectionError(env);
    return NULL;
  }

  jclass class = (*env)->FindClass(env, BRLAPI_OBJECT("DisplaySize"));
  if (!class) return NULL;

  jmethodID constructor = JAVA_GET_CONSTRUCTOR(env, class,
    JAVA_SIG_INT // width
    JAVA_SIG_INT // height
  );
  if (!constructor) return NULL;

  jobject object = (*env)->NewObject(env, class, constructor, width, height);
  if (!object) return NULL;
  return object;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, pause, void,
  jint milliseconds
) {
  GET_CONNECTION_HANDLE(env, this, );
  int result = brlapi__pause(handle, milliseconds);

  if (result < 0) {
    throwConnectionError(env);
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, enterTtyMode, jint,
  jint jtty, jstring jdriver
) {
  int tty ;
  char *driver;
  int result;
  GET_CONNECTION_HANDLE(env, this, -1);
  
  tty = (int)jtty; 
  if (!jdriver)
    driver = NULL;
  else
    if (!(driver = (char *)(*env)->GetStringUTFChars(env, jdriver, NULL))) {
      throwJavaError(env, JAVA_OBJ_OUT_OF_MEMORY_ERROR, __func__);
      return -1;
    }

  result = brlapi__enterTtyMode(handle, tty,driver);
  if (result < 0) {
    throwConnectionError(env);
    return -1;
  }

  return (jint) result;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, enterTtyModeWithPath, void,
  jstring jdriver, jintArray jttys
) {
  jint *ttys ;
  char *driver;
  int result;
  GET_CONNECTION_HANDLE(env, this, );
  
  if (!jttys) {
    throwJavaError(env, JAVA_OBJ_NULL_POINTER_EXCEPTION, __func__);
    return;
  }
  if (!(ttys = (*env)->GetIntArrayElements(env, jttys, NULL))) {
    throwJavaError(env, JAVA_OBJ_OUT_OF_MEMORY_ERROR, __func__);
    return;
  }

  if (!jdriver) {
    driver = NULL;
  } else if (!(driver = (char *)(*env)->GetStringUTFChars(env, jdriver, NULL))) {
    throwJavaError(env, JAVA_OBJ_OUT_OF_MEMORY_ERROR, __func__);
    return;
  }

  result = brlapi__enterTtyModeWithPath(handle, ttys,(*env)->GetArrayLength(env,jttys),driver);
  (*env)->ReleaseIntArrayElements(env, jttys, ttys, JNI_ABORT);
  if (result < 0) {
    throwConnectionError(env);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, leaveTtyMode, void
) {
  GET_CONNECTION_HANDLE(env, this, );

  if (brlapi__leaveTtyMode(handle) < 0) {
    throwConnectionError(env);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, setFocus, void,
  jint jarg1
) {
  int arg1 ;
  GET_CONNECTION_HANDLE(env, this, );
  
  arg1 = (int)jarg1; 
  if (brlapi__setFocus(handle, arg1) < 0) {
    throwConnectionError(env);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, writeText, void,
  jint jarg1, jstring jarg2
) {
  brlapi_writeArguments_t s = BRLAPI_WRITEARGUMENTS_INITIALIZER;
  int result;
  GET_CONNECTION_HANDLE(env, this, );
  
  s.cursor = (int)jarg1; 

  if (jarg2) {
    s.regionBegin = 1;
    s.regionSize = (*env)->GetStringLength(env, jarg2);

    if (!(s.text = (char *)(*env)->GetStringUTFChars(env, jarg2, NULL))) {
      throwJavaError(env, JAVA_OBJ_OUT_OF_MEMORY_ERROR, __func__);
      return;
    }
    s.charset = "UTF-8";
  }

  result = brlapi__write(handle, &s);
  if (jarg2)
    (*env)->ReleaseStringUTFChars(env, jarg2, s.text); 

  if (result < 0) {
    throwConnectionError(env);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, writeDots, void,
  jbyteArray jarg1
) {
  jbyte *arg1;
  int result;
  GET_CONNECTION_HANDLE(env, this, );
  
  if (!jarg1) {
    throwJavaError(env, JAVA_OBJ_NULL_POINTER_EXCEPTION, __func__);
    return;
  }
  arg1 = (*env)->GetByteArrayElements(env, jarg1, NULL);
  if (!arg1) {
    throwJavaError(env, JAVA_OBJ_OUT_OF_MEMORY_ERROR, __func__);
    return;
  }

  result = brlapi__writeDots(handle, (const unsigned char *)arg1);
  (*env)->ReleaseByteArrayElements(env, jarg1, arg1, JNI_ABORT); 
  
  if (result < 0) {
    throwConnectionError(env);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, write, void,
  jobject jArguments
) {
  if (!jArguments) {
    throwJavaError(env, JAVA_OBJ_NULL_POINTER_EXCEPTION, __func__);
    return;
  }

  GET_CONNECTION_HANDLE(env, this, );
  GET_CLASS(env, class, jArguments, );
  brlapi_writeArguments_t cArguments = BRLAPI_WRITEARGUMENTS_INITIALIZER;

  {
    FIND_FIELD(env, field, class, "displayNumber", JAVA_SIG_INT, );
    cArguments.displayNumber = JAVA_GET_FIELD(env, Int, jArguments, field);
  }

  {
    FIND_FIELD(env, field, class, "regionBegin", JAVA_SIG_INT, );
    cArguments.regionBegin = JAVA_GET_FIELD(env, Int, jArguments, field);
  }

  {
    FIND_FIELD(env, field, class, "regionSize", JAVA_SIG_INT, );
    cArguments.regionSize = JAVA_GET_FIELD(env, Int, jArguments, field);
  }

  jstring jText;
  {
    FIND_FIELD(env, field, class, "text", JAVA_SIG_STRING, );

    if ((jText = JAVA_GET_FIELD(env, Object, jArguments, field))) {
      cArguments.text = (char *) (*env)->GetStringUTFChars(env, jText, NULL);
    } else {
      cArguments.text = NULL;
    }
  }

  jbyteArray jAndMask;
  {
    FIND_FIELD(env, field, class, "andMask", JAVA_SIG_ARRAY(JAVA_SIG_BYTE), );

    if ((jAndMask = JAVA_GET_FIELD(env, Object, jArguments, field))) {
      cArguments.andMask = (unsigned char *) (*env)->GetByteArrayElements(env, jAndMask, NULL);
    } else {
      cArguments.andMask = NULL;
    }
  }

  jbyteArray jOrMask;
  {
    FIND_FIELD(env, field, class, "orMask", JAVA_SIG_ARRAY(JAVA_SIG_BYTE), );

    if ((jOrMask = JAVA_GET_FIELD(env, Object, jArguments, field))) {
      cArguments.orMask = (unsigned char *) (*env)->GetByteArrayElements(env, jOrMask, NULL);
    } else {
      cArguments.orMask = NULL;
    }
  }

  {
    FIND_FIELD(env, field, class, "cursorPosition", JAVA_SIG_INT, );
    cArguments.cursor = JAVA_GET_FIELD(env, Int, jArguments, field);
  }

  cArguments.charset = "UTF-8";
  int result = brlapi__write(handle, &cArguments);

  if (jText) (*env)->ReleaseStringUTFChars(env, jText, cArguments.text); 
  if (jAndMask) (*env)->ReleaseByteArrayElements(env, jAndMask, (jbyte*) cArguments.andMask, JNI_ABORT); 
  if (jOrMask) (*env)->ReleaseByteArrayElements(env, jOrMask, (jbyte*) cArguments.orMask, JNI_ABORT); 

  if (result < 0) throwConnectionError(env);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, readKey, jlong,
  jboolean jblock
) {
  brlapi_keyCode_t code;
  int result;
  GET_CONNECTION_HANDLE(env, this, -1);

  result = brlapi__readKey(handle, (int) jblock, &code);

  if (result < 0) {
    throwConnectionError(env);
    return -1;
  }

  if (!result) return (jlong)(-1);
  return (jlong)code;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, readKeyWithTimeout, jlong,
  jint milliseconds
) {
  GET_CONNECTION_HANDLE(env, this, -1);

  brlapi_keyCode_t code;
  int result = brlapi__readKeyWithTimeout(handle, milliseconds, &code);

  if (result < 0) {
    throwConnectionError(env);
  } else if (!result) {
    throwJavaError(env, JAVA_OBJ_TIMEOUT_EXCEPTION, __func__);
  }

  return (jlong)code;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, ignoreKeys, void,
  jlong jrange, jlongArray js
) {
  jlong *s;
  unsigned int n;
  int result;
  GET_CONNECTION_HANDLE(env, this, );

  if (!js) {
    throwJavaError(env, JAVA_OBJ_NULL_POINTER_EXCEPTION, __func__);
    return;
  }

  n = (unsigned int) (*env)->GetArrayLength(env, js);
  s = (*env)->GetLongArrayElements(env, js, NULL);

  // XXX jlong != brlapi_keyCode_t probably
  result = brlapi__ignoreKeys(handle, jrange, (const brlapi_keyCode_t *)s, n);
  (*env)->ReleaseLongArrayElements(env, js, s, JNI_ABORT);
  
  if (result < 0) {
    throwConnectionError(env);
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
  GET_CONNECTION_HANDLE(env, this, );

  if (!js) {
    throwJavaError(env, JAVA_OBJ_NULL_POINTER_EXCEPTION, __func__);
    return;
  }

  n = (unsigned int) (*env)->GetArrayLength(env, js);
  s = (*env)->GetLongArrayElements(env, js, NULL);

  // XXX jlong != brlapi_keyCode_t probably
  result = brlapi__acceptKeys(handle, jrange, (const brlapi_keyCode_t *)s, n);
  (*env)->ReleaseLongArrayElements(env, js, s, JNI_ABORT);

  if (result < 0) {
    throwConnectionError(env);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, ignoreAllKeys, void
) {
  GET_CONNECTION_HANDLE(env, this, );

  if (brlapi__ignoreAllKeys(handle) < 0)
    throwConnectionError(env);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, acceptAllKeys, void
) {
  GET_CONNECTION_HANDLE(env, this, );

  if (brlapi__acceptAllKeys(handle) < 0)
    throwConnectionError(env);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, ignoreKeyRanges, void,
  jobjectArray js
) {
  unsigned int n;
  GET_CONNECTION_HANDLE(env, this, );

  if (!js) {
    throwJavaError(env, JAVA_OBJ_NULL_POINTER_EXCEPTION, __func__);
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
      throwConnectionError(env);
      return;
    }
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, acceptKeyRanges, void,
  jobjectArray js
) {
  unsigned int n;
  GET_CONNECTION_HANDLE(env, this, );

  if (!js) {
    throwJavaError(env, JAVA_OBJ_NULL_POINTER_EXCEPTION, __func__);
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
      throwConnectionError(env);
      return;
    }
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, enterRawMode, void,
  jstring jdriver
) {
  char *driver;
  int res;
  GET_CONNECTION_HANDLE(env, this, );

  if (!jdriver) {
    throwJavaError(env, JAVA_OBJ_NULL_POINTER_EXCEPTION, __func__);
    return;
  } else if (!(driver = (char *)(*env)->GetStringUTFChars(env, jdriver, NULL))) {
    throwJavaError(env, JAVA_OBJ_NULL_POINTER_EXCEPTION, __func__);
    return;
  }
  res = brlapi__enterRawMode(handle, driver);
  if (jdriver) (*env)->ReleaseStringUTFChars(env, jdriver, driver);
  if (res < 0) {
    throwConnectionError(env);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, leaveRawMode, void
) {
  GET_CONNECTION_HANDLE(env, this, );

  if (brlapi__leaveRawMode(handle) < 0) {
    throwConnectionError(env);
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
  GET_CONNECTION_HANDLE(env, this, -1);

  if (!jbuf) {
    throwJavaError(env, JAVA_OBJ_NULL_POINTER_EXCEPTION, __func__);
    return -1;
  }

  n = (unsigned int) (*env)->GetArrayLength(env, jbuf);
  buf = (*env)->GetByteArrayElements(env, jbuf, NULL);

  result = brlapi__sendRaw(handle, (const unsigned char *)buf, n);
  (*env)->ReleaseByteArrayElements(env, jbuf, buf, JNI_ABORT);

  if (result < 0) {
    throwConnectionError(env);
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
  GET_CONNECTION_HANDLE(env, this, -1);

  if (!jbuf) {
    throwJavaError(env, JAVA_OBJ_NULL_POINTER_EXCEPTION, __func__);
    return -1;
  }

  n = (unsigned int) (*env)->GetArrayLength(env, jbuf);
  buf = (*env)->GetByteArrayElements(env, jbuf, NULL);

  result = brlapi__recvRaw(handle, (unsigned char *)buf, n);

  if (result < 0) {
    (*env)->ReleaseByteArrayElements(env, jbuf, buf, JNI_ABORT);
    throwConnectionError(env);
    return -1;
  }

  (*env)->ReleaseByteArrayElements(env, jbuf, buf, 0);
  return (jint) result;
}

static int
checkParameter (
  JNIEnv *env,
  jint parameter, jlong subparam, jboolean global,
  const brlapi_param_properties_t **properties,
  brlapi_param_flags_t *flags
) {
  if (!(*properties = brlapi_getParameterProperties(parameter))) {
    throwJavaError(env, JAVA_OBJ_ILLEGAL_ARGUMENT_EXCEPTION, "parameter out of range");
    return 0;
  }

  if (!(*properties)->hasSubparam && (subparam != 0)) {
    throwJavaError(env, JAVA_OBJ_ILLEGAL_ARGUMENT_EXCEPTION, "nonzero subparam");
    return 0;
  }

  *flags = 0;
  if (global == JNI_TRUE) {
    *flags |= BRLAPI_PARAMF_GLOBAL;
  } else if (global == JNI_FALSE) {
    *flags |= BRLAPI_PARAMF_LOCAL;
  }

  return 1;
}

static jobject
newParameterValueObject (
  JNIEnv *env, const brlapi_param_properties_t *properties,
  const void *value, size_t size
) {
  jobject result = NULL;
  size_t count = size;

  switch (properties->type) {
    case BRLAPI_PARAM_TYPE_STRING: {
      result = (*env)->NewStringUTF(env, value);
      break;
    }

    case BRLAPI_PARAM_TYPE_BOOLEAN: {
      const brlapi_param_bool_t *cBooleans = value;
      count /= sizeof(*cBooleans);
      result = (*env)->NewBooleanArray(env, count);

      if (result && count) {
        jboolean jBooleans[count];

        for (jsize i=0; i<count; i+=1) {
          jBooleans[i] = cBooleans[i]? JNI_TRUE: JNI_FALSE;
        }

        (*env)->SetBooleanArrayRegion(env, result, 0, count, jBooleans);
      }

      break;
    }

    case BRLAPI_PARAM_TYPE_UINT8: {
      result = (*env)->NewByteArray(env, count);

      if (result && count) {
        (*env)->SetByteArrayRegion(env, result, 0, count, value);
      }

      break;
    }

    case BRLAPI_PARAM_TYPE_UINT16: {
      count /= 2;
      result = (*env)->NewShortArray(env, count);

      if (result && count) {
        (*env)->SetShortArrayRegion(env, result, 0, count, value);
      }

      break;
    }

    case BRLAPI_PARAM_TYPE_UINT32: {
      count /= 4;
      result = (*env)->NewIntArray(env, count);

      if (result && count) {
        (*env)->SetIntArrayRegion(env, result, 0, count, value);
      }

      break;
    }

    case BRLAPI_PARAM_TYPE_UINT64: {
      count /= 8;
      result = (*env)->NewLongArray(env, count);

      if (result && count) {
        (*env)->SetLongArrayRegion(env, result, 0, count, value);
      }

      break;
    }
  }

  return result;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, getParameter, jobject,
  jint parameter, jlong subparam, jboolean global
) {
  GET_CONNECTION_HANDLE(env, this, NULL);

  jobject result = NULL;
  const brlapi_param_properties_t *properties;
  brlapi_param_flags_t flags;

  if (checkParameter(env, parameter, subparam, global, &properties, &flags)) {
    void *value;
    size_t size;

    if ((value = brlapi__getParameterAlloc(handle, parameter, subparam, flags, &size))) {
      result = newParameterValueObject(env, properties, value, size);
      free(value);
    } else {
      throwConnectionError(env);
    }
  }

  return result;
}

static void
setParameter (
  JNIEnv *env, brlapi_handle_t *handle,
  jint parameter, jlong subparam, brlapi_param_flags_t flags,
  const void *data, size_t size
) {
  if (brlapi__setParameter(handle, parameter, subparam, flags, data, size) < 0) {
    throwConnectionError(env);
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, setParameter, void,
  jint parameter, jlong subparam, jboolean global, jobject value
) {
  GET_CONNECTION_HANDLE(env, this, );

  const brlapi_param_properties_t *properties;
  brlapi_param_flags_t flags;

  if (checkParameter(env, parameter, subparam, global, &properties, &flags)) {
    switch (properties->type) {
      case BRLAPI_PARAM_TYPE_STRING: {
        jboolean isCopy;
        const char *string = (*env)->GetStringUTFChars(env, value, &isCopy);

        if (string) {
          setParameter(
            env, handle,
            parameter, subparam, flags,
            string, strlen(string)
          );

          (*env)->ReleaseStringUTFChars(env, value, string);
        }

        break;
      }

      case BRLAPI_PARAM_TYPE_BOOLEAN: {
        jsize count = (*env)->GetArrayLength(env, value);

        if (!javaHasExceptionOccurred(env)) {
          jboolean values[count + 1];
          (*env)->GetBooleanArrayRegion(env, value, 0, count, values);

          if (!javaHasExceptionOccurred(env)) {
            brlapi_param_bool_t booleans[count + 1];

            for (unsigned int i=0; i<count; i+=1) {
              booleans[i] = values[i] != JNI_FALSE;
            }

            setParameter(
              env, handle,
              parameter, subparam, flags,
              booleans, count
            );
          }
        }

        break;
      }

      case BRLAPI_PARAM_TYPE_UINT8: {
        jsize count = (*env)->GetArrayLength(env, value);

        if (!javaHasExceptionOccurred(env)) {
          jbyte values[count + 1];
          (*env)->GetByteArrayRegion(env, value, 0, count, values);

          if (!javaHasExceptionOccurred(env)) {
            setParameter(
              env, handle,
              parameter, subparam, flags,
              values, count
            );
          }
        }

        break;
      }

      case BRLAPI_PARAM_TYPE_UINT16: {
        jsize count = (*env)->GetArrayLength(env, value);

        if (!javaHasExceptionOccurred(env)) {
          jshort values[count + 1];
          (*env)->GetShortArrayRegion(env, value, 0, count, values);

          if (!javaHasExceptionOccurred(env)) {
            setParameter(
              env, handle,
              parameter, subparam, flags,
              values, (count * 2)
            );
          }
        }

        break;
      }

      case BRLAPI_PARAM_TYPE_UINT32: {
        jsize count = (*env)->GetArrayLength(env, value);

        if (!javaHasExceptionOccurred(env)) {
          jint values[count + 1];
          (*env)->GetIntArrayRegion(env, value, 0, count, values);

          if (!javaHasExceptionOccurred(env)) {
            setParameter(
              env, handle,
              parameter, subparam, flags,
              values, (count * 4)
            );
          }
        }

        break;
      }

      case BRLAPI_PARAM_TYPE_UINT64: {
        jsize count = (*env)->GetArrayLength(env, value);

        if (!javaHasExceptionOccurred(env)) {
          jlong values[count + 1];
          (*env)->GetLongArrayRegion(env, value, 0, count, values);

          if (!javaHasExceptionOccurred(env)) {
            setParameter(
              env, handle,
              parameter, subparam, flags,
              values, (count * 8)
            );
          }
        }

        break;
      }
    }
  }
}

typedef struct {
  JNIEnv *env;
  brlapi_handle_t *handle;
  brlapi_paramCallbackDescriptor_t descriptor;

  struct {
    jobject object;
    jclass class;
    jmethodID method;
  } watcher;
} WatchedParameterData;

static void
handleWatchedParameter (
  brlapi_param_t parameter,
  brlapi_param_subparam_t subparam,
  brlapi_param_flags_t flags,
  void *identifier, const void *data, size_t length
) {
  WatchedParameterData *wpd = (WatchedParameterData *)identifier;
  JNIEnv *env = wpd->env;

  jobject value = newParameterValueObject(
    env, brlapi_getParameterProperties(parameter),
    data, length
  );

  if (value) {
    (*env)->CallVoidMethod(
      env, wpd->watcher.object, wpd->watcher.method,
      parameter, subparam, value
    );
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_BasicConnection, watchParameter, jlong,
  jint parameter, jlong subparam, jboolean global, jobject watcher
) {
  GET_CONNECTION_HANDLE(env, this, 0);

  const brlapi_param_properties_t *properties;
  brlapi_param_flags_t flags;

  if (checkParameter(env, parameter, subparam, global, &properties, &flags)) {
    WatchedParameterData *wpd;

    if ((wpd = malloc(sizeof(*wpd)))) {
      memset(wpd, 0, sizeof(*wpd));
      wpd->handle = handle;
      wpd->env = env;

      if ((wpd->watcher.object = (*env)->NewGlobalRef(env, watcher))) {
        wpd->watcher.class = (*env)->FindClass(
          env, BRLAPI_OBJECT("ParameterWatcher")
        );

        if (wpd->watcher.class) {
          wpd->watcher.method = (*env)->GetMethodID(
            env, wpd->watcher.class, "onParameterUpdated",
            JAVA_SIG_METHOD(JAVA_SIG_VOID,
              JAVA_SIG_INT // parameter
              JAVA_SIG_LONG // subparam
              JAVA_SIG_OBJECT(JAVA_OBJ_OBJECT) // value
            )
          );

          if (wpd->watcher.method) {
            wpd->descriptor = brlapi__watchParameter(
              handle, parameter, subparam, flags,
              handleWatchedParameter, wpd, NULL, 0
            );

            if (wpd->descriptor) return (intptr_t)wpd;
            throwConnectionError(env);
          }
        }

        (*env)->DeleteGlobalRef(env, wpd->watcher.object);
      }

      free(wpd);
    } else {
      throwJavaError(env, JAVA_OBJ_OUT_OF_MEMORY_ERROR, __func__);
    }
  }

  return 0;
}

JAVA_STATIC_METHOD(
  org_a11y_brlapi_BasicConnection, unwatchParameter, void,
  jlong identifier
) {
  WatchedParameterData *wpd = (WatchedParameterData *)(intptr_t)identifier;

  if (brlapi__unwatchParameter(wpd->handle, wpd->descriptor) < 0) {
    throwConnectionError(env);
  }

  (*env)->DeleteGlobalRef(env, wpd->watcher.object);
  free(wpd);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionError, toString, jstring
) {
  GET_CLASS(env, class, this, NULL);
  brlapi_error_t error;
  memset(&error, 0, sizeof(error));

  {
    FIND_FIELD(env, field, class, "apiError", JAVA_SIG_INT, NULL);
    error.brlerrno = JAVA_GET_FIELD(env, Int, this, field);
  }

  {
    FIND_FIELD(env, field, class, "osError", JAVA_SIG_INT, NULL);
    error.libcerrno = JAVA_GET_FIELD(env, Int, this, field);
  }

  {
    FIND_FIELD(env, field, class, "gaiError", JAVA_SIG_INT, NULL);
    error.gaierrno = JAVA_GET_FIELD(env, Int, this, field);
  }

  jstring jFunction;
  {
    FIND_FIELD(env, field, class, "functionName", JAVA_SIG_STRING, NULL);

    if ((jFunction = JAVA_GET_FIELD(env, Object, this, field))) {
      const char *cFunction = (*env)->GetStringUTFChars(env, jFunction, NULL);
      if (!cFunction) return NULL;
      error.errfun = cFunction;
    } else {
      error.errfun = NULL;
    }
  }

  const char *cMessage = brlapi_strerror(&error);
  if (jFunction) (*env)->ReleaseStringUTFChars(env, jFunction, error.errfun);
  if (!cMessage) return NULL;

  size_t length = strlen(cMessage);
  char buffer[length + 1];
  int copy = 0;

  while (length > 0) {
    size_t last = length - 1;
    if (cMessage[last] != '\n') break;
    length = last;
    copy = 1;
  }

  if (copy) {
    memcpy(buffer, cMessage, length);
    buffer[length] = 0;
    cMessage = buffer;
  }

  return (*env)->NewStringUTF(env, cMessage);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionException, toString, jstring
) {
  GET_CONNECTION_HANDLE(env, this, NULL);
  GET_CLASS(env, class, this, NULL);

  jint error;
  {
    FIND_FIELD(env, field, class, "errorNumber", JAVA_SIG_INT, NULL);
    error = JAVA_GET_FIELD(env, Int, this, field);
  }

  jint type;
  {
    FIND_FIELD(env, field, class, "packetType", JAVA_SIG_INT, NULL);
    type = JAVA_GET_FIELD(env, Int, this, field);
  }

  jbyteArray jPacket;
  {
    FIND_FIELD(env, field, class, "failedPacket", JAVA_SIG_ARRAY(JAVA_SIG_BYTE), NULL);

    if (!(jPacket = JAVA_GET_FIELD(env, Object, this, field))) {
      throwJavaError(env, JAVA_OBJ_NULL_POINTER_EXCEPTION, __func__);
      return NULL;
    }
  }

  jint size = (*env)->GetArrayLength(env, jPacket);
  jbyte *cPacket = (*env)->GetByteArrayElements(env, jPacket, NULL);

  if (!cPacket) {
    throwJavaError(env, JAVA_OBJ_OUT_OF_MEMORY_ERROR, __func__);
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
  const char *name = brlapi_getPacketTypeName((brlapi_packetType_t) type);
  if (!name) return NULL;
  return (*env)->NewStringUTF(env, name);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_Key, expandKeyCode, void,
  jlong code
) {
  brlapi_expandedKeyCode_t ekc;
  brlapi_expandKeyCode((brlapi_keyCode_t) code, &ekc);
  GET_CLASS(env, class, this, );

  {
    FIND_FIELD(env, field, class, "typeValue", JAVA_SIG_INT, );
    JAVA_SET_FIELD(env, Int, this, field, ekc.type);
  }

  {
    FIND_FIELD(env, field, class, "commandValue", JAVA_SIG_INT, );
    JAVA_SET_FIELD(env, Int, this, field, ekc.command);
  }

  {
    FIND_FIELD(env, field, class, "argumentValue", JAVA_SIG_INT, );
    JAVA_SET_FIELD(env, Int, this, field, ekc.argument);
  }

  {
    FIND_FIELD(env, field, class, "flagsValue", JAVA_SIG_INT, );
    JAVA_SET_FIELD(env, Int, this, field, ekc.flags);
  }
}
