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
#include <string.h>
#include <errno.h>

#include "log.h"
#include "queue.h"

#include "io_usb.h"
#include "usb_internal.h"
#include "sys_android.h"

typedef struct {
  JNIEnv *env;
  jobject device;
  UsbDeviceDescriptor descriptor;
} UsbHostDevice;

static Queue *usbHostDevices = NULL;

struct UsbDeviceExtensionStruct {
  const UsbHostDevice *host;
  jobject connection;
};

static jclass usbHelperClass = NULL;
static jclass usbDeviceClass = NULL;
static jclass usbConnectionClass = NULL;

static int
usbFindHelperClass (JNIEnv *env) {
  return findJavaClass(env, &usbHelperClass, "org/a11y/brltty/android/UsbHelper");
}

static jobject
usbGetDeviceIterator (JNIEnv *env) {
  if (usbFindHelperClass(env)) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, usbHelperClass, "getDeviceIterator",
                             JAVA_SIG_METHOD(JAVA_SIG_OBJECT(java/util/Iterator), ))) {
      jobject iterator = (*env)->CallStaticObjectMethod(env, usbHelperClass, method);

      if (iterator) return iterator;
    }
  }

  return NULL;
}

static jobject
usbGetNextDevice (JNIEnv *env, jobject iterator) {
  if (usbFindHelperClass(env)) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, usbHelperClass, "getNextDevice",
                             JAVA_SIG_METHOD(JAVA_SIG_OBJECT(android/hardware/usb/UsbDevice), JAVA_SIG_OBJECT(java/util/Iterator)))) {
      jobject device = (*env)->CallStaticObjectMethod(env, usbHelperClass, method, iterator);

      if (device) return device;
    }
  }

  return NULL;
}

static jobject
usbOpenDeviceConnection (JNIEnv *env, jobject device) {
  if (usbFindHelperClass(env)) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, usbHelperClass, "openDeviceConnection",
                             JAVA_SIG_METHOD(JAVA_SIG_OBJECT(android/hardware/usb/UsbDeviceConnection), JAVA_SIG_OBJECT(android/hardware/usb/UsbDevice)))) {
      jobject connection = (*env)->CallStaticObjectMethod(env, usbHelperClass, method, device);

      if (connection) return connection;
    }
  }

  return NULL;
}

static int
usbFindDeviceClass (JNIEnv *env) {
  return findJavaClass(env, &usbDeviceClass, "android/hardware/usb/UsbDevice");
}

static int
usbGetIntDeviceProperty (
  JNIEnv *env, jint *value,
  jobject device, const char *methodName, jmethodID *methodIdentifier
) {
  if (usbFindDeviceClass(env)) {
    if (findJavaInstanceMethod(env, methodIdentifier, usbDeviceClass, methodName,
                               JAVA_SIG_METHOD(JAVA_SIG_INT, JAVA_SIG_OBJECT(android/hardware/usb/UsbDevice)))) {
      *value = (*env)->CallIntMethod(env, device, *methodIdentifier);
      if (!(*env)->ExceptionCheck(env)) return 1;
    }
  }

  return 0;
}

static int
usbGetDeviceVendor (JNIEnv *env, jobject device, UsbDeviceDescriptor *descriptor) {
  static jmethodID method = 0;

  jint vendor;
  int ok = usbGetIntDeviceProperty(env, &vendor, device, "getVendorId", &method);

  if (ok) putLittleEndian16(&descriptor->idVendor, vendor);
  return ok;
}

static int
usbGetDeviceProduct (JNIEnv *env, jobject device, UsbDeviceDescriptor *descriptor) {
  static jmethodID method = 0;

  jint product;
  int ok = usbGetIntDeviceProperty(env, &product, device, "getProductId", &method);

  if (ok) putLittleEndian16(&descriptor->idProduct, product);
  return ok;
}

static int
usbGetDeviceClass (JNIEnv *env, jobject device, UsbDeviceDescriptor *descriptor) {
  static jmethodID method = 0;

  jint class;
  int ok = usbGetIntDeviceProperty(env, &class, device, "getClass", &method);

  if (ok) descriptor->bDeviceClass = class;
  return ok;
}

static int
usbGetDeviceSubclass (JNIEnv *env, jobject device, UsbDeviceDescriptor *descriptor) {
  static jmethodID method = 0;

  jint subclass;
  int ok = usbGetIntDeviceProperty(env, &subclass, device, "getSubclass", &method);

  if (ok) descriptor->bDeviceSubClass = subclass;
  return ok;
}

static int
usbGetDeviceProtocol (JNIEnv *env, jobject device, UsbDeviceDescriptor *descriptor) {
  static jmethodID method = 0;

  jint protocol;
  int ok = usbGetIntDeviceProperty(env, &protocol, device, "getProtocol", &method);

  if (ok) descriptor->bDeviceProtocol = protocol;
  return ok;
}

static int
usbFindConnectionClass (JNIEnv *env) {
  return findJavaClass(env, &usbConnectionClass, "android/hardware/usb/UsbDeviceConnection");
}

static void
usbCloseDeviceConnection (JNIEnv *env, jobject connection) {
  if (usbFindConnectionClass(env)) {
    static jmethodID method = 0;

    if (findJavaInstanceMethod(env, &method, usbConnectionClass, "close",
                               JAVA_SIG_METHOD(JAVA_SIG_VOID, ))) {
      (*env)->CallVoidMethod(env, connection, method);
    }
  }
}

static int
usbDoControlTransfer (
  JNIEnv *env, jobject connection,
  int type, int request, int value, int index,
  jbyteArray buffer, int length, int timeout
) {
  if (usbFindConnectionClass(env)) {
    static jmethodID method = 0;

    if (findJavaInstanceMethod(env, &method, usbConnectionClass, "controlTransfer",
                               JAVA_SIG_METHOD(JAVA_SIG_INT,
                                               JAVA_SIG_INT // type
                                               JAVA_SIG_INT // request
                                               JAVA_SIG_INT // value
                                               JAVA_SIG_INT // index
                                               JAVA_SIG_ARRAY(JAVA_SIG_BYTE) // buffer
                                               JAVA_SIG_INT // length
                                               JAVA_SIG_INT // timeout
                                               JAVA_SIG_INT // type,
                                              ))) {
      return (*env)->CallIntMethod(env, connection, method,
                                   type, request, value, index,
                                   buffer, length, timeout);
    }
  }

  return -1;
}

static int
usbOpenConnection (UsbDeviceExtension *devx) {
  if (devx->connection) return 1;

  {
    const UsbHostDevice *host = devx->host;
    jobject connection = usbOpenDeviceConnection(host->env, host->device);

    if (connection) {
      devx->connection = (*host->env)->NewGlobalRef(host->env, connection);

      (*host->env)->DeleteLocalRef(host->env, connection);
      connection = NULL;

      if (devx->connection) return 1;
    }
  }

  return 0;
}

int
usbResetDevice (UsbDevice *device) {
  errno = ENOSYS;
  logSystemError("USB device reset");
  return 0;
}

int
usbDisableAutosuspend (UsbDevice *device) {
  errno = ENOSYS;
  logSystemError("USB device autosuspend disable");
  return 0;
}

int
usbSetConfiguration (
  UsbDevice *device,
  unsigned char configuration
) {
  errno = ENOSYS;
  logSystemError("USB configuration set");
  return 0;
}

int
usbClaimInterface (
  UsbDevice *device,
  unsigned char interface
) {
  errno = ENOSYS;
  logSystemError("USB interface claim");
  return 0;
}

int
usbReleaseInterface (
  UsbDevice *device,
  unsigned char interface
) {
  errno = ENOSYS;
  logSystemError("USB interface release");
  return 0;
}

int
usbSetAlternative (
  UsbDevice *device,
  unsigned char interface,
  unsigned char alternative
) {
  errno = ENOSYS;
  logSystemError("USB alternative set");
  return 0;
}

int
usbClearEndpoint (
  UsbDevice *device,
  unsigned char endpointAddress
) {
  errno = ENOSYS;
  logSystemError("USB endpoint clear");
  return 0;
}

ssize_t
usbControlTransfer (
  UsbDevice *device,
  uint8_t direction,
  uint8_t recipient,
  uint8_t type,
  uint8_t request,
  uint16_t value,
  uint16_t index,
  void *buffer,
  uint16_t length,
  int timeout
) {
  ssize_t result = -1;
  UsbDeviceExtension *devx = device->extension;

  if (usbOpenConnection(devx)) {
    const UsbHostDevice *host = devx->host;
    jbyteArray bytes = (*host->env)->NewByteArray(host->env, length);

    if (bytes) {
      if (direction == UsbControlDirection_Output) {
        (*host->env)->SetByteArrayRegion(host->env, bytes, 0, length, buffer);
      }

      result = usbDoControlTransfer(host->env, devx,
                                    direction | recipient | type,
                                    request, value, index,
                                    bytes, length, timeout);

      if (result > 0) {
        if (direction == UsbControlDirection_Input) {
          (*host->env)->GetByteArrayRegion(host->env, bytes, 0, result, buffer);
        }
      }

      (*host->env)->DeleteLocalRef(host->env, bytes);
    } else {
      logMallocError();
    }
  }

  errno = ENOSYS;
  logSystemError("USB control transfer");
  return -1;
}

void *
usbSubmitRequest (
  UsbDevice *device,
  unsigned char endpointAddress,
  void *buffer,
  size_t length,
  void *context
) {
  errno = ENOSYS;
  logSystemError("USB request submit");
  return NULL;
}

int
usbCancelRequest (
  UsbDevice *device,
  void *request
) {
  errno = ENOSYS;
  logSystemError("USB request cancel");
  return 0;
}

void *
usbReapResponse (
  UsbDevice *device,
  unsigned char endpointAddress,
  UsbResponse *response,
  int wait
) {
  errno = ENOSYS;
  logSystemError("USB request reap");
  return NULL;
}

ssize_t
usbReadEndpoint (
  UsbDevice *device,
  unsigned char endpointNumber,
  void *buffer,
  size_t length,
  int timeout
) {
  errno = ENOSYS;
  logSystemError("USB endpoint read");
  return -1;
}

ssize_t
usbWriteEndpoint (
  UsbDevice *device,
  unsigned char endpointNumber,
  const void *buffer,
  size_t length,
  int timeout
) {
  errno = ENOSYS;
  logSystemError("USB endpoint write");
  return -1;
}

int
usbReadDeviceDescriptor (UsbDevice *device) {
  device->descriptor = device->extension->host->descriptor;
  return 1;
}

int
usbAllocateEndpointExtension (UsbEndpoint *endpoint) {
  return 1;
}

void
usbDeallocateEndpointExtension (UsbEndpointExtension *eptx) {
}

void
usbDeallocateDeviceExtension (UsbDeviceExtension *devx) {
  if (devx->connection) {
    const UsbHostDevice *host = devx->host;

    usbCloseDeviceConnection(host->env, devx->connection);
    (*host->env)->DeleteGlobalRef(host->env, devx->connection);
    devx->connection = NULL;
  }

  free(devx);
}

static void
usbDeallocateHostDevice (void *item, void *data) {
  UsbHostDevice *host = item;

  (*host->env)->DeleteGlobalRef(host->env, host->device);
  free(host);
}

static int
usbAddHostDevice (JNIEnv *env, jobject device) {
  UsbHostDevice *host;

  if ((host = malloc(sizeof(*host)))) {
    memset(host, 0, sizeof(*host));
    host->env = env;

    if ((host->device = (*host->env)->NewGlobalRef(host->env, device))) {
      if (usbGetDeviceVendor(host->env, host->device, &host->descriptor)) {
        if (usbGetDeviceProduct(host->env, host->device, &host->descriptor)) {
          if (usbGetDeviceClass(host->env, host->device, &host->descriptor)) {
            if (usbGetDeviceSubclass(host->env, host->device, &host->descriptor)) {
              if (usbGetDeviceProtocol(host->env, host->device, &host->descriptor)) {
                if (enqueueItem(usbHostDevices, host)) {
                  return 1;
                }
              }
            }
          }
        }
      }

      (*host->env)->DeleteGlobalRef(host->env, host->device);
    }

    free(host);
  } else {
    logMallocError();
  }

  return 0;
}

typedef struct {
  UsbDeviceChooser chooser;
  void *data;
  UsbDevice *device;
} UsbTestHostDeviceData;

static int
usbTestHostDevice (void *item, void *data) {
  const UsbHostDevice *host = item;
  UsbTestHostDeviceData *test = data;
  UsbDeviceExtension *devx;

  if ((devx = malloc(sizeof(*devx)))) {
    memset(devx, 0, sizeof(*devx));
    devx->host = host;
    devx->connection = NULL;

    if ((test->device = usbTestDevice(devx, test->chooser, test->data))) return 1;

    usbDeallocateDeviceExtension(devx);
  } else {
    logMallocError();
  }

  return 0;
}

UsbDevice *
usbFindDevice (UsbDeviceChooser chooser, void *data) {
  if (!usbHostDevices) {
    int ok = 0;

    if ((usbHostDevices = newQueue(usbDeallocateHostDevice, NULL))) {
      JNIEnv *env = getJavaNativeInterface();

      if (env) {
        jobject iterator = usbGetDeviceIterator(env);

        if (iterator) {
          jobject device;

          ok = 1;
          while ((device = usbGetNextDevice(env, iterator))) {
            int added = usbAddHostDevice(env, device);
            (*env)->DeleteLocalRef(env, device);

            if (!added) {
              ok = 0;
              break;
            }
          }

          (*env)->DeleteLocalRef(env, iterator);
        }

        clearJavaException(env, 1);
      }

      if (!ok) {
        deallocateQueue(usbHostDevices);
        usbHostDevices = NULL;
      }
    }
  }

  if (usbHostDevices) {
    UsbTestHostDeviceData test = {
      .chooser = chooser,
      .data = data,
      .device = NULL
    };

    if (processQueue(usbHostDevices, usbTestHostDevice, &test)) return test.device;
  }

  return NULL;
}

void
usbForgetDevices (void) {
  if (usbHostDevices) {
    deallocateQueue(usbHostDevices);
    usbHostDevices = NULL;
  }
}
