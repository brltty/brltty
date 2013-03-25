/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#include <errno.h>

#include "io_bluetooth.h"
#include "bluetooth_internal.h"
#include "log.h"
#include "io_misc.h"
#include "sys_android.h"

struct BluetoothConnectionExtensionStruct {
  JNIEnv *env;
  jobject connection;
  int inputPipe[2];
};

static jclass connectionClass = NULL;
static jmethodID connectionConstructor = 0;
static jmethodID closeMethod = 0;
static jmethodID writeMethod = 0;

BluetoothConnectionExtension *
bthConnect (uint64_t bda, uint8_t channel) {
  BluetoothConnectionExtension *bcx;

  if ((bcx = malloc(sizeof(*bcx)))) {
    memset(bcx, 0, sizeof(*bcx));

    if ((bcx->env = getJavaNativeInterface())) {
      if (findJavaClass(bcx->env, &connectionClass, "org/a11y/brltty/android/BluetoothConnection")) {
        if (findJavaConstructor(bcx->env, &connectionConstructor, connectionClass,
                                JAVA_SIG_CONSTRUCTOR(JAVA_SIG_LONG JAVA_SIG_BYTE JAVA_SIG_INT))) {
          if (pipe(bcx->inputPipe) != -1) {
            if (setBlockingIo(bcx->inputPipe[0], 0)) {
              jobject localReference = (*bcx->env)->NewObject(bcx->env, connectionClass, connectionConstructor, bda, channel, bcx->inputPipe[1]);

              if (!clearJavaException(bcx->env, 0)) {
                jobject globalReference = (*bcx->env)->NewGlobalRef(bcx->env, localReference);

                (*bcx->env)->DeleteLocalRef(bcx->env, localReference);
                localReference = NULL;

                if ((globalReference)) {
                  bcx->connection = globalReference;
                  return bcx;
                } else {
                  logMallocError();
                  clearJavaException(bcx->env, 0);
                }
              }
            }

            close(bcx->inputPipe[0]);
            close(bcx->inputPipe[1]);
          } else {
            logSystemError("pipe");
          }
        }
      }
    }

    free(bcx);
  } else {
    logMallocError();
  }

  return NULL;
}

void
bthDisconnect (BluetoothConnectionExtension *bcx) {
  if (bcx->connection) {
    if (findJavaInstanceMethod(bcx->env, &closeMethod, connectionClass, "close",
                               JAVA_SIG_METHOD(JAVA_SIG_VOID, ))) {
      (*bcx->env)->CallVoidMethod(bcx->env, bcx->connection, closeMethod);
    }

    (*bcx->env)->DeleteGlobalRef(bcx->env, bcx->connection);
    clearJavaException(bcx->env, 1);
  }

  close(bcx->inputPipe[0]);
  close(bcx->inputPipe[1]);

  free(bcx);
}

int
bthAwaitInput (BluetoothConnection *connection, int milliseconds) {
  BluetoothConnectionExtension *bcx = connection->extension;

  return awaitInput(bcx->inputPipe[0], milliseconds);
}

ssize_t
bthReadData (
  BluetoothConnection *connection, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  BluetoothConnectionExtension *bcx = connection->extension;

  return readData(bcx->inputPipe[0], buffer, size, initialTimeout, subsequentTimeout);
}

ssize_t
bthWriteData (BluetoothConnection *connection, const void *buffer, size_t size) {
  BluetoothConnectionExtension *bcx = connection->extension;

  if (findJavaInstanceMethod(bcx->env, &writeMethod, connectionClass, "write",
                             JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN, JAVA_SIG_ARRAY(JAVA_SIG_BYTE)))) {
    jbyteArray bytes = (*bcx->env)->NewByteArray(bcx->env, size);

    if (bytes) {
      jboolean result;

      (*bcx->env)->SetByteArrayRegion(bcx->env, bytes, 0, size, buffer);
      result = (*bcx->env)->CallBooleanMethod(bcx->env, bcx->connection, writeMethod, bytes);
      (*bcx->env)->DeleteLocalRef(bcx->env, bytes);

      if (result) {
        return size;
      } else {
        errno = EIO;
      }
    } else {
      errno = ENOMEM;
    }
  } else {
    errno = ENOSYS;
  }

  logSystemError("Bluetooth write");
  clearJavaException(bcx->env, 1);
  return -1;
}
