/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2023 by
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
#include <netdb.h>
#include <pthread.h>

#include "bindings.h"

#define NEW_PRIMITIVE_WRAPPER(name, type, sig) \
static jobject \
new##name (JNIEnv *env, type value) { \
  static JAVA_CLASS_VARIABLE(class); \
  static JAVA_METHOD_VARIABLE(constructor); \
  \
  if (!javaFindClassAndMethod( \
    env, \
    &class, JAVA_OBJ_LANG(#name), \
    &constructor, JAVA_CONSTRUCTOR_NAME, \
    JAVA_SIG_CONSTRUCTOR(JAVA_SIG_##sig) \
  )) return NULL; \
  \
  return (*env)->NewObject(env, class, constructor, value); \
}

NEW_PRIMITIVE_WRAPPER(Long, jlong, LONG)

#define BRLAPI_NO_DEPRECATED
#define BRLAPI_NO_SINGLE_SESSION
#include "brlapi.h"
#define BRLAPI_OBJECT(name) "org/a11y/brlapi/" name

static jint jniVersion = 0;
static int libraryVersion_major = 0;
static int libraryVersion_minor = 0;
static int libraryVersion_revision = 0;

JAVA_STATIC_METHOD(
  org_a11y_brlapi_NativeComponent, initializeNativeData, void
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

        #ifdef __ANDROID__
          JNIEnv *e = env;
          result = (*vm)->AttachCurrentThread(vm, &e, &args);
          env = e;
        #else /* __ANDROID__ */
          result = (*vm)->AttachCurrentThread(vm, &env, &args);
        #endif /* __ANDROID__ */

        if (result == JNI_OK) {
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
  if ((*env)->ExceptionCheck(env)) return;
  jclass class = (*env)->FindClass(env, object);
  if (class) (*env)->ThrowNew(env, class, message);
}

static void
logBrlapiError (const char *label) {
  size_t size = brlapi_strerror_r(&brlapi_error, NULL, 0);
  char msg[size+1];
  brlapi_strerror_r(&brlapi_error, msg, sizeof(msg));
  fprintf(stderr,
    "%s: API=%d Libc=%d GAI=%d: %s\n",
    label, brlapi_errno, brlapi_libcerrno, brlapi_gaierrno, msg
  );
}

static void
throwAPIError (JNIEnv *env) {
  if (0) logBrlapiError("API Error");
  if ((*env)->ExceptionCheck(env)) return;

  {
    const char *object = NULL;

    switch (brlapi_errno) {
      case BRLAPI_ERROR_SUCCESS:
        break;

      case BRLAPI_ERROR_NOMEM:
        object = JAVA_OBJ_OUT_OF_MEMORY_ERROR;
        break;

      case BRLAPI_ERROR_EOF:
        object = BRLAPI_OBJECT("LostConnectionException");
        break;

      case BRLAPI_ERROR_LIBCERR: {
        switch (brlapi_libcerrno) {
          case EINTR:
            object = JAVA_OBJ_INTERRUPTED_IO_EXCEPTION;
            break;
        }

        break;
      }

      default:
        break;
    }

    if (object) {
      throwJavaError(env, object, brlapi_errfun);
      return;
    }
  }

  static JAVA_CLASS_VARIABLE(class);
  if (!javaFindClass(env, &class, BRLAPI_OBJECT("APIError"))) return;

  static JAVA_METHOD_VARIABLE(constructor);
  if (!JAVA_FIND_CONSTRUCTOR(env, &constructor, class,
    JAVA_SIG_INT // api error
    JAVA_SIG_INT // os error
    JAVA_SIG_INT // gai error
    JAVA_SIG_STRING // function name
  )) return;

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
    (*env)->Throw(env, object);
  } else if (jFunction) {
    (*env)->ReleaseStringUTFChars(env, jFunction, brlapi_errfun);
  }
}

static void
throwConnectError (JNIEnv *env, const brlapi_connectionSettings_t *settings) {
  if (0) logBrlapiError("Connect Error");

  const char *object = NULL;
  const char *message = NULL;

  const char *host = NULL;
  const char *auth = NULL;

  if (settings) {
    host = settings->host;
    auth = settings->auth;
  }

  switch (brlapi_errno) {
    case BRLAPI_ERROR_SUCCESS:
      break;

    case BRLAPI_ERROR_CONNREFUSED:
      object = BRLAPI_OBJECT("UnavailableServiceException");
      message = host;
      break;

    case BRLAPI_ERROR_AUTHENTICATION:
      object = BRLAPI_OBJECT("AuthenticationException");
      message = auth;
      break;

    case BRLAPI_ERROR_GAIERR: {
      switch (brlapi_gaierrno) {
        case EAI_SYSTEM:
          goto SYSTEM_ERROR;

        case EAI_MEMORY:
          object = JAVA_OBJ_OUT_OF_MEMORY_ERROR;
          break;

        #ifdef EAI_NODATA
        case EAI_NODATA: // obsoleted on RFC 2553bis-02
        #endif /* EAI_NODATA */

        case EAI_NONAME:
          object = BRLAPI_OBJECT("UnknownHostException");
          message = host;
          break;
      }

      break;
    }

    SYSTEM_ERROR:
    case BRLAPI_ERROR_LIBCERR: {
      switch (brlapi_libcerrno) {
      }

      break;
    }

    default:
      break;
  }

  if (object) {
    if (!message) message = "";
    throwJavaError(env, object, message);
  } else {
    throwAPIError(env);
  }
}

static void BRLAPI_STDCALL
handleAPIException (brlapi_handle_t *handle, int error, brlapi_packetType_t type, const void *packet, size_t size) {
  JNIEnv *env = getJavaEnvironment(handle);
  if ((*env)->ExceptionCheck(env)) return;

  jbyteArray jPacket = (*env)->NewByteArray(env, size);
  if (!jPacket) return;
  (*env)->SetByteArrayRegion(env, jPacket, 0, size, (jbyte *) packet);

  static JAVA_CLASS_VARIABLE(class);
  if (!javaFindClass(env, &class, BRLAPI_OBJECT("APIException"))) return;

  static JAVA_METHOD_VARIABLE(constructor);
  if (!JAVA_FIND_CONSTRUCTOR(env, &constructor, class,
    JAVA_SIG_LONG // handle
    JAVA_SIG_INT // error
    JAVA_SIG_INT // type
    JAVA_SIG_ARRAY(JAVA_SIG_BYTE) // packet
  )) return;

  jclass object = (*env)->NewObject(
    env, class, constructor,
    (jlong) (intptr_t) handle, error, type, jPacket
  );
  if (!object) return;

  (*env)->Throw(env, object);
}

JAVA_STATIC_METHOD(
  org_a11y_brlapi_APIVersion, getMajor, jint
) {
  return libraryVersion_major;
}

JAVA_STATIC_METHOD(
  org_a11y_brlapi_APIVersion, getMinor, jint
) {
  return libraryVersion_minor;
}

JAVA_STATIC_METHOD(
  org_a11y_brlapi_APIVersion, getRevision, jint
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

JAVA_STATIC_METHOD(
  org_a11y_brlapi_ConnectionSettings, getKeyfileDirectory, jstring
) {
  return (*env)->NewStringUTF(env, BRLAPI_ETCDIR);
}

JAVA_STATIC_METHOD(
  org_a11y_brlapi_ConnectionSettings, getKeyfileName, jstring
) {
  return (*env)->NewStringUTF(env, BRLAPI_AUTHKEYFILE);
}

static int
openConnection (
  JNIEnv *env, jobject connection,
  jobject jRequestedSettings, brlapi_connectionSettings_t *cRequestedSettings,
  jobject jActualSettings, brlapi_connectionSettings_t *cActualSettings,
  brlapi_handle_t **handle, int *fileDescriptor,
  jobject *jRequestedHost, jobject *jRequestedAuth
) {
  if (jRequestedSettings) {
    GET_CLASS(env, class, jRequestedSettings, 0);

    {
      FIND_FIELD(env, field, class, "serverHost", JAVA_SIG_STRING, 0);
      *jRequestedHost = JAVA_GET_FIELD(env, Object, jRequestedSettings, field);

      if (*jRequestedHost) {
        if (!(cRequestedSettings->host = (*env)->GetStringUTFChars(env, *jRequestedHost, NULL))) {
          return 0;
        }
      }
    }

    {
      FIND_FIELD(env, field, class, "authenticationScheme", JAVA_SIG_STRING, 0);
      *jRequestedAuth = JAVA_GET_FIELD(env, Object, jRequestedSettings, field);

      if (*jRequestedAuth) {
        if (!(cRequestedSettings->auth = (*env)->GetStringUTFChars(env, *jRequestedAuth, NULL))) {
          return 0;
        }
      }
    }
  }

  if (!(*handle = malloc(brlapi_getHandleSize()))) {
    throwJavaError(env, JAVA_OBJ_OUT_OF_MEMORY_ERROR, __func__);
    return 0;
  }

  *fileDescriptor = brlapi__openConnection(
    *handle, cRequestedSettings, cActualSettings
  );

  if (*fileDescriptor < 0) {
    throwConnectError(env, cRequestedSettings);
    return 0;
  }

  if (cActualSettings) {
    GET_CLASS(env, class, jActualSettings, 0);

    if (cActualSettings->host) {
      jstring host = (*env)->NewStringUTF(env, cActualSettings->host);
      if (!host) return 0;

      FIND_FIELD(env, field, class, "serverHost", JAVA_SIG_STRING, 0);
      JAVA_SET_FIELD(env, Object, jActualSettings, field, host);
      if ((*env)->ExceptionCheck(env)) return 0;
    }

    if (cActualSettings->auth) {
      jstring auth = (*env)->NewStringUTF(env, cActualSettings->auth);
      if (!auth) return 0;

      FIND_FIELD(env, field, class, "authenticationScheme", JAVA_SIG_STRING, 0);
      JAVA_SET_FIELD(env, Object, jActualSettings, field, auth);
      if ((*env)->ExceptionCheck(env)) return 0;
    }
  }

  {
    JavaVM *vm;
    (*env)->GetJavaVM(env, &vm);
    brlapi__setClientData(*handle, vm);
  }

  brlapi__setExceptionHandler(*handle, handleAPIException);
  SET_CONNECTION_HANDLE(env, connection, *handle, 0);
  return 1;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, openConnection, jint,
  jobject jRequestedSettings, jobject jActualSettings
) {
  brlapi_connectionSettings_t cRequestedSettings, *pRequestedSettings;
  memset(&cRequestedSettings, 0, sizeof(cRequestedSettings));
  pRequestedSettings = jRequestedSettings? &cRequestedSettings: NULL;

  brlapi_connectionSettings_t cActualSettings, *pActualSettings;
  memset(&cActualSettings, 0, sizeof(cActualSettings));
  pActualSettings = jActualSettings? &cActualSettings: NULL;

  brlapi_handle_t *handle = NULL;
  int fileDescriptor = -1;

  jobject jRequestedHost = NULL;
  jobject jRequestedAuth = NULL;

  int opened = openConnection(
    env, this,
    jRequestedSettings, pRequestedSettings,
    jActualSettings, pActualSettings,
    &handle, &fileDescriptor,
    &jRequestedHost, &jRequestedAuth
  );

  if (cRequestedSettings.host) {
    (*env)->ReleaseStringUTFChars(env, jRequestedHost, cRequestedSettings.host); 
  }

  if (cRequestedSettings.auth) {
    (*env)->ReleaseStringUTFChars(env, jRequestedAuth,  cRequestedSettings.auth); 
  }

  if (opened) return fileDescriptor;
  if (fileDescriptor >= 0) brlapi__closeConnection(handle);
  if (handle) free(handle);
  return -1;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, closeConnection, void
) {
  GET_CONNECTION_HANDLE(env, this, );
  brlapi__closeConnection(handle);
  free(handle);
  SET_CONNECTION_HANDLE(env, this, NULL, );
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, getDriverName, jstring
) {
  GET_CONNECTION_HANDLE(env, this, NULL);
  char name[0X20];

  if (brlapi__getDriverName(handle, name, sizeof(name)) < 0) {
    throwAPIError(env);
    return NULL;
  }

  name[sizeof(name)-1] = 0;
  return (*env)->NewStringUTF(env, name);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, getModelIdentifier, jstring
) {
  GET_CONNECTION_HANDLE(env, this, NULL);
  char identifier[0X20];

  if (brlapi__getModelIdentifier(handle, identifier, sizeof(identifier)) < 0) {
    throwAPIError(env);
    return NULL;
  }

  identifier[sizeof(identifier)-1] = 0;
  return (*env)->NewStringUTF(env, identifier);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, getDisplaySize, jobject
) {
  GET_CONNECTION_HANDLE(env, this, NULL);

  unsigned int width, height;
  if (brlapi__getDisplaySize(handle, &width, &height) < 0) {
    throwAPIError(env);
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
  org_a11y_brlapi_ConnectionBase, pause, void,
  jint milliseconds
) {
  GET_CONNECTION_HANDLE(env, this, );
  int result = brlapi__pause(handle, milliseconds);

  if (result < 0) {
    throwAPIError(env);
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, enterTtyMode, jint,
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
    throwAPIError(env);
    return -1;
  }

  return (jint) result;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, enterTtyModeWithPath, void,
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
    throwAPIError(env);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, leaveTtyMode, void
) {
  GET_CONNECTION_HANDLE(env, this, );

  if (brlapi__leaveTtyMode(handle) < 0) {
    throwAPIError(env);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, setFocus, void,
  jint tty
) {
  GET_CONNECTION_HANDLE(env, this, );
  if (brlapi__setFocus(handle, tty) < 0) throwAPIError(env);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, writeText, void,
  jint cursor, jstring jText
) {
  GET_CONNECTION_HANDLE(env, this, );
  
  const char *cText;
  if (!jText) {
    cText = NULL;
  } else if (!(cText = (*env)->GetStringUTFChars(env, jText, NULL))) {
    throwJavaError(env, JAVA_OBJ_OUT_OF_MEMORY_ERROR, __func__);
    return;
  }

  int result = brlapi__writeText(handle, cursor, cText);
  if (jText) (*env)->ReleaseStringUTFChars(env, jText, cText); 
  if (result < 0) throwAPIError(env);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, writeDots, void,
  jbyteArray jDots
) {
  GET_CONNECTION_HANDLE(env, this, );
  
  if (!jDots) {
    throwJavaError(env, JAVA_OBJ_NULL_POINTER_EXCEPTION, __func__);
    return;
  }

  jbyte *cDots = (*env)->GetByteArrayElements(env, jDots, NULL);
  if (!cDots) return;

  int result = brlapi__writeDots(handle, (const unsigned char *)cDots);
  (*env)->ReleaseByteArrayElements(env, jDots, cDots, JNI_ABORT); 
  
  if (result < 0) {
    throwAPIError(env);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, write, void,
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
      cArguments.charset = "UTF-8";
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

  int result = brlapi__write(handle, &cArguments);
  if (jText) (*env)->ReleaseStringUTFChars(env, jText, cArguments.text); 
  if (jAndMask) (*env)->ReleaseByteArrayElements(env, jAndMask, (jbyte*) cArguments.andMask, JNI_ABORT); 
  if (jOrMask) (*env)->ReleaseByteArrayElements(env, jOrMask, (jbyte*) cArguments.orMask, JNI_ABORT); 

  if (result < 0) throwAPIError(env);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, readKey, jobject,
  jboolean jWait
) {
  GET_CONNECTION_HANDLE(env, this, NULL);

  int cWait = jWait != JNI_FALSE;
  brlapi_keyCode_t code;

  int result = brlapi__readKey(handle, cWait, &code);
  if (result < 0) throwAPIError(env);
  if (!result) return NULL;
  return newLong(env, code);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, readKeyWithTimeout, jlong,
  jint milliseconds
) {
  GET_CONNECTION_HANDLE(env, this, -1);

  brlapi_keyCode_t code;
  int result = brlapi__readKeyWithTimeout(handle, milliseconds, &code);

  if (result < 0) {
    throwAPIError(env);
  } else if (!result) {
    throwJavaError(env, JAVA_OBJ_TIMEOUT_EXCEPTION, __func__);
  }

  return (jlong)code;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, ignoreKeys, void,
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
    throwAPIError(env);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, acceptKeys, void,
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
    throwAPIError(env);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, ignoreAllKeys, void
) {
  GET_CONNECTION_HANDLE(env, this, );

  if (brlapi__ignoreAllKeys(handle) < 0)
    throwAPIError(env);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, acceptAllKeys, void
) {
  GET_CONNECTION_HANDLE(env, this, );

  if (brlapi__acceptAllKeys(handle) < 0)
    throwAPIError(env);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, ignoreKeyRanges, void,
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
      throwAPIError(env);
      return;
    }
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, acceptKeyRanges, void,
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
      throwAPIError(env);
      return;
    }
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, enterRawMode, void,
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
    throwAPIError(env);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, leaveRawMode, void
) {
  GET_CONNECTION_HANDLE(env, this, );

  if (brlapi__leaveRawMode(handle) < 0) {
    throwAPIError(env);
    return;
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, sendRaw, jint,
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
    throwAPIError(env);
    return -1;
  }

  return (jint) result;
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, recvRaw, jint,
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
    throwAPIError(env);
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
  org_a11y_brlapi_ConnectionBase, getParameter, jobject,
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
      throwAPIError(env);
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
    throwAPIError(env);
  }
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_ConnectionBase, setParameter, void,
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
  brlapi_handle_t *handle = wpd->handle;
  JNIEnv *env = getJavaEnvironment(handle);
  if (!env) return;

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
  org_a11y_brlapi_ConnectionBase, watchParameter, jlong,
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
            throwAPIError(env);
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
  org_a11y_brlapi_ConnectionBase, unwatchParameter, void,
  jlong identifier
) {
  WatchedParameterData *wpd = (WatchedParameterData *)(intptr_t)identifier;

  if (brlapi__unwatchParameter(wpd->handle, wpd->descriptor) < 0) {
    throwAPIError(env);
  }

  (*env)->DeleteGlobalRef(env, wpd->watcher.object);
  free(wpd);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_APIError, toString, jstring
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

  size_t size = brlapi_strerror_r(&error, NULL, 0);
  char cMessage[size+1];

  brlapi_strerror_r(&error, cMessage, sizeof(cMessage));
  if (jFunction) (*env)->ReleaseStringUTFChars(env, jFunction, error.errfun);

  return (*env)->NewStringUTF(env, cMessage);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_APIException, toString, jstring
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
  org_a11y_brlapi_APIException, getPacketTypeName, jstring,
  jint type
) {
  const char *name = brlapi_getPacketTypeName((brlapi_packetType_t) type);
  if (!name) return NULL;
  return (*env)->NewStringUTF(env, name);
}

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_CommandKeycode, expandKeycode, void,
  jlong code
) {
  brlapi_expandedKeyCode_t ekc;
  GET_CLASS(env, class, this, );

  if (brlapi_expandKeyCode((brlapi_keyCode_t)code, &ekc) < 0) {
    memset(&ekc, 0, sizeof(ekc));
    ekc.type = code & BRLAPI_KEY_TYPE_MASK;
    ekc.command = code & BRLAPI_KEY_CODE_MASK;
    ekc.flags = (code & BRLAPI_KEY_FLAGS_MASK) >> BRLAPI_KEY_FLAGS_SHIFT;
  }

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

JAVA_INSTANCE_METHOD(
  org_a11y_brlapi_CommandKeycode, describeKeycode, void,
  jlong code
) {
  brlapi_describedKeyCode_t dkc;
  GET_CLASS(env, class, this, );

  if (brlapi_describeKeyCode((brlapi_keyCode_t)code, &dkc) < 0) {
    memset(&dkc, 0, sizeof(dkc));
    dkc.type = "UNSUPPORTED";
  }

  {
    jstring name = (*env)->NewStringUTF(env, dkc.type);
    if (!name) return;

    FIND_FIELD(env, field, class, "typeName", JAVA_SIG_STRING, );
    JAVA_SET_FIELD(env, Object, this, field, name);
  }

  {
    jstring name = (*env)->NewStringUTF(env, dkc.command);
    if (!name) return;

    FIND_FIELD(env, field, class, "commandName", JAVA_SIG_STRING, );
    JAVA_SET_FIELD(env, Object, this, field, name);
  }

  {
    jclass stringClass = (*env)->FindClass(env, JAVA_OBJ_STRING);
    if (!stringClass) return;

    jsize count = dkc.flags;
    jobjectArray names = (*env)->NewObjectArray(env, count, stringClass, NULL);
    if (!names) return;

    for (unsigned int index=0; index<count; index+=1) {
      jstring name = (*env)->NewStringUTF(env, dkc.flag[index]);
      if (!name) return;

      (*env)->SetObjectArrayElement(env, names, index, name);
      if ((*env)->ExceptionCheck(env)) return;
    }

    {
      FIND_FIELD(env, field, class, "flagNames", JAVA_SIG_ARRAY(JAVA_SIG_STRING), );
      JAVA_SET_FIELD(env, Object, this, field, names);
    }
  }
}

JAVA_STATIC_METHOD(
  org_a11y_brlapi_DriverKeycode, isPress, jboolean,
  jlong code
) {
  return (code & BRLAPI_DRV_KEY_PRESS)? JNI_TRUE: JNI_FALSE;
}

JAVA_STATIC_METHOD(
  org_a11y_brlapi_DriverKeycode, getValue, jlong,
  jlong code
) {
  return code & BRLAPI_DRV_KEY_VALUE_MASK;
}

JAVA_STATIC_METHOD(
  org_a11y_brlapi_DriverKeycode, getGroup, jint,
  jlong code
) {
  return BRLAPI_DRV_KEY_GROUP(code);
}

JAVA_STATIC_METHOD(
  org_a11y_brlapi_DriverKeycode, getNumber, jint,
  jlong code
) {
  return BRLAPI_DRV_KEY_NUMBER(code);
}

JAVA_STATIC_METHOD(
  org_a11y_brlapi_DriverKeycode, getNumberAny, jint
) {
  return BRLAPI_DRV_KEY_NUMBER_ANY;
}
