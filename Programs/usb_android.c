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
  jobject interface;
};

struct UsbEndpointExtensionStruct {
  UsbEndpoint *endpoint;
  jobject object;
};

static jclass usbHelperClass = NULL;
static jclass usbDeviceClass = NULL;
static jclass usbInterfaceClass = NULL;
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
      clearJavaException(env, 1);
      errno = EIO;
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
      clearJavaException(env, 1);
    }
  }

  return NULL;
}

static jobject
usbGetDeviceInterface (JNIEnv *env, jobject device, jint identifier) {
  if (usbFindHelperClass(env)) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, usbHelperClass, "getDeviceInterface",
                             JAVA_SIG_METHOD(JAVA_SIG_OBJECT(android/hardware/usb/UsbInterface),
                                             JAVA_SIG_OBJECT(android/hardware/usb/UsbDevice) // device
                                             JAVA_SIG_INT // identifier
                                            ))) {
      jobject interface = (*env)->CallStaticObjectMethod(env, usbHelperClass, method, device, identifier);

      if (interface) return interface;
      clearJavaException(env, 1);
      errno = EIO;
    }
  }

  return NULL;
}

static jobject
usbGetInterfaceEndpoint (JNIEnv *env, jobject interface, jint address) {
  if (usbFindHelperClass(env)) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, usbHelperClass, "getInterfaceEndpoint",
                             JAVA_SIG_METHOD(JAVA_SIG_OBJECT(android/hardware/usb/UsbEndpoint),
                                             JAVA_SIG_OBJECT(android/hardware/usb/UsbInterface) // interface
                                             JAVA_SIG_INT // address
                                            ))) {
      jobject endpoint = (*env)->CallStaticObjectMethod(env, usbHelperClass, method, interface, address);

      if (endpoint) return endpoint;
      clearJavaException(env, 1);
      errno = EIO;
    }
  }

  return NULL;
}

static jobject
usbOpenDeviceConnection (JNIEnv *env, jobject device) {
  logMessage(LOG_DEBUG, "USB: opening device connection");

  if (usbFindHelperClass(env)) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, usbHelperClass, "openDeviceConnection",
                             JAVA_SIG_METHOD(JAVA_SIG_OBJECT(android/hardware/usb/UsbDeviceConnection), JAVA_SIG_OBJECT(android/hardware/usb/UsbDevice)))) {
      jobject connection = (*env)->CallStaticObjectMethod(env, usbHelperClass, method, device);

      if (connection) return connection;
      clearJavaException(env, 1);
      errno = EIO;
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
                               JAVA_SIG_METHOD(JAVA_SIG_INT, ))) {
      if (!clearJavaException(env, 1)) {
        *value = (*env)->CallIntMethod(env, device, *methodIdentifier);
        return 1;
      }

      errno = EIO;
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
  int ok = usbGetIntDeviceProperty(env, &class, device, "getDeviceClass", &method);

  if (ok) descriptor->bDeviceClass = class;
  return ok;
}

static int
usbGetDeviceSubclass (JNIEnv *env, jobject device, UsbDeviceDescriptor *descriptor) {
  static jmethodID method = 0;

  jint subclass;
  int ok = usbGetIntDeviceProperty(env, &subclass, device, "getDeviceSubclass", &method);

  if (ok) descriptor->bDeviceSubClass = subclass;
  return ok;
}

static int
usbGetDeviceProtocol (JNIEnv *env, jobject device, UsbDeviceDescriptor *descriptor) {
  static jmethodID method = 0;

  jint protocol;
  int ok = usbGetIntDeviceProperty(env, &protocol, device, "getDeviceProtocol", &method);

  if (ok) descriptor->bDeviceProtocol = protocol;
  return ok;
}

static int
usbFindInterfaceClass (JNIEnv *env) {
  return findJavaClass(env, &usbInterfaceClass, "android/hardware/usb/UsbInterface");
}

static int
usbGetInterfaceIdentifier (JNIEnv *env, uint8_t *identifier, jobject interface) {
  if (usbFindInterfaceClass(env)) {
    static jmethodID method = 0;

    if (findJavaInstanceMethod(env, &method, usbInterfaceClass, "getId",
                               JAVA_SIG_METHOD(JAVA_SIG_INT,
                                              ))) {
      jint result = (*env)->CallIntMethod(env, interface, method);

      if (!clearJavaException(env, 1)) {
        *identifier = result;
        return 1;
      }

      errno = EIO;
    }
  }

  return 0;
}

static int
usbFindConnectionClass (JNIEnv *env) {
  return findJavaClass(env, &usbConnectionClass, "android/hardware/usb/UsbDeviceConnection");
}

static int
usbDoClaimInterface (JNIEnv *env, jobject connection, jobject interface) {
  if (usbFindConnectionClass(env)) {
    static jmethodID method = 0;

    if (findJavaInstanceMethod(env, &method, usbConnectionClass, "claimInterface",
                               JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                               JAVA_SIG_OBJECT(android/hardware/usb/UsbInterface) // interface
                                               JAVA_SIG_BOOLEAN // force
                                              ))) {
      jboolean result = (*env)->CallBooleanMethod(env, connection, method, interface, JNI_TRUE);

      if (clearJavaException(env, 1)) {
        errno = EIO;
      } else if (result) {
        return 1;
      } else {
        logSystemError("USB claim interface");
      }
    }
  }

  return 0;
}

static int
usbDoReleaseInterface (JNIEnv *env, jobject connection, jobject interface) {
  if (usbFindConnectionClass(env)) {
    static jmethodID method = 0;

    if (findJavaInstanceMethod(env, &method, usbConnectionClass, "releaseInterface",
                               JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                               JAVA_SIG_OBJECT(android/hardware/usb/UsbInterface) // interface
                                              ))) {
      jboolean result = (*env)->CallBooleanMethod(env, connection, method, interface);

      if (clearJavaException(env, 1)) {
        errno = EIO;
      } else if (result) {
        return 1;
      } else {
        logSystemError("USB release interface");
      }
    }
  }

  return 0;
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
                                              ))) {
      jint result = (*env)->CallIntMethod(env, connection, method,
                                          type, request, value, index,
                                          buffer, length, timeout);

      if (!clearJavaException(env, 1)) return result;
      errno = EIO;
    }
  }

  return -1;
}

static int
usbDoBulkTransfer (
  JNIEnv *env, jobject connection, jobject endpoint,
  jbyteArray buffer, int length, int timeout
) {
  if (usbFindConnectionClass(env)) {
    static jmethodID method = 0;

    if (findJavaInstanceMethod(env, &method, usbConnectionClass, "bulkTransfer",
                               JAVA_SIG_METHOD(JAVA_SIG_INT,
                                               JAVA_SIG_OBJECT(android/hardware/usb/UsbEndpoint) // endpoint
                                               JAVA_SIG_ARRAY(JAVA_SIG_BYTE) // buffer
                                               JAVA_SIG_INT // length
                                               JAVA_SIG_INT // timeout
                                              ))) {
      jint result = (*env)->CallIntMethod(env, connection, method,
                                          endpoint, buffer, length, timeout);

      if (!clearJavaException(env, 1)) return result;
      errno = EIO;
    }
  }

  return -1;
}

static void
usbCloseDeviceConnection (JNIEnv *env, jobject connection) {
  if (usbFindConnectionClass(env)) {
    static jmethodID method = 0;

    if (findJavaInstanceMethod(env, &method, usbConnectionClass, "close",
                               JAVA_SIG_METHOD(JAVA_SIG_VOID, ))) {
      (*env)->CallVoidMethod(env, connection, method);
      clearJavaException(env, 1);
    }
  }
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
      logMallocError();
      clearJavaException(host->env, 0);
    }
  }

  return 0;
}

static void
usbUnsetInterface (UsbDeviceExtension *devx) {
  if (devx->interface) {
    JNIEnv *env = devx->host->env;

    (*env)->DeleteGlobalRef(env, devx->interface);
    devx->interface = NULL;
  }
}

static int
usbSetInterface (UsbDeviceExtension *devx, uint8_t identifier) {
  JNIEnv *env = devx->host->env;

  if (devx->interface) {
    uint8_t id;

    if (!usbGetInterfaceIdentifier(env, &id, devx->interface)) return 0;
    if (id == identifier) return 1;
  }

  {
    jobject interface = usbGetDeviceInterface(env, devx->host->device, identifier);

    if (interface) {
      usbUnsetInterface(devx);
      devx->interface = (*env)->NewGlobalRef(env, interface);

      (*env)->DeleteLocalRef(env, interface);
      interface = NULL;

      if (devx->interface) return 1;
      logMallocError();
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
  if (configuration == 1) return 1;

  errno = ENOSYS;
  logSystemError("USB configuration set");
  return 0;
}

int
usbClaimInterface (
  UsbDevice *device,
  unsigned char interface
) {
  UsbDeviceExtension *devx = device->extension;

  if (usbSetInterface(devx, interface))
    if (usbOpenConnection(devx))
      if (usbDoClaimInterface(devx->host->env, devx->connection, devx->interface))
        return 1;

  return 0;
}

int
usbReleaseInterface (
  UsbDevice *device,
  unsigned char interface
) {
  UsbDeviceExtension *devx = device->extension;

  if (usbSetInterface(devx, interface))
    if (usbOpenConnection(devx))
      if (usbDoReleaseInterface(devx->host->env, devx->connection, devx->interface))
        return 1;

  return 0;
}

int
usbSetAlternative (
  UsbDevice *device,
  unsigned char interface,
  unsigned char alternative
) {
  if (alternative == 0) return 1;

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

      result = usbDoControlTransfer(host->env, devx->connection,
                                    direction | recipient | type,
                                    request, value, index,
                                    bytes, length, timeout);

      if (direction == UsbControlDirection_Input) {
        if (result > 0) {
          (*host->env)->GetByteArrayRegion(host->env, bytes, 0, result, buffer);
        }
      }

      (*host->env)->DeleteLocalRef(host->env, bytes);
    } else {
      logMallocError();
    }
  }

  if (result == -1) logSystemError("USB control transfer");
  return result;
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
  ssize_t result = -1;
  UsbEndpoint *endpoint = usbGetInputEndpoint(device, endpointNumber);

  if (endpoint) {
    UsbDeviceExtension *devx = device->extension;

    if (usbOpenConnection(devx)) {
      const UsbHostDevice *host = devx->host;
      JNIEnv *env = host->env;
      jbyteArray bytes = (*env)->NewByteArray(env, length);

      if (bytes) {
        UsbEndpointExtension *eptx = endpoint->extension;

        result = usbDoBulkTransfer(env, devx->connection, eptx->object, bytes, length, timeout);
        if (result > 0) (*env)->GetByteArrayRegion(env, bytes, 0, result, buffer);

        (*env)->DeleteLocalRef(env, bytes);
      } else {
        logMallocError();
        clearJavaException(env, 0);
      }
    }
  }

  if (result >= 0) {
    if (!usbApplyInputFilters(device, buffer, length, &result)) {
      errno = EIO;
      result = -1;
    }
  }

  if (result == -1) {
    if (errno == ETIMEDOUT) errno = EAGAIN;
    if (errno != EAGAIN) logSystemError("USB bulk read");
  }

  return result;
}

ssize_t
usbWriteEndpoint (
  UsbDevice *device,
  unsigned char endpointNumber,
  const void *buffer,
  size_t length,
  int timeout
) {
  ssize_t result = -1;
  UsbEndpoint *endpoint = usbGetOutputEndpoint(device, endpointNumber);

  if (endpoint) {
    UsbDeviceExtension *devx = device->extension;

    if (usbOpenConnection(devx)) {
      const UsbHostDevice *host = devx->host;
      JNIEnv *env = host->env;
      jbyteArray bytes = (*env)->NewByteArray(env, length);

      if (bytes) {
        UsbEndpointExtension *eptx = endpoint->extension;

        (*env)->SetByteArrayRegion(env, bytes, 0, length, buffer);
        result = usbDoBulkTransfer(env, devx->connection, eptx->object, bytes, length, timeout);

        (*env)->DeleteLocalRef(env, bytes);
      } else {
        logMallocError();
        clearJavaException(env, 0);
      }
    }
  }

  if (result == -1) logSystemError("USB bulk write");
  return result;
}

int
usbReadDeviceDescriptor (UsbDevice *device) {
  device->descriptor = device->extension->host->descriptor;
  return 1;
}

int
usbAllocateEndpointExtension (UsbEndpoint *endpoint) {
  UsbDevice *device = endpoint->device;
  UsbDeviceExtension *devx = device->extension;
  const UsbHostDevice *host = devx->host;
  JNIEnv *env = host->env;

  if (devx->interface) {
    jobject localReference = usbGetInterfaceEndpoint(env, devx->interface, endpoint->descriptor->bEndpointAddress);

    if (localReference) {
      jobject globalReference = (*env)->NewGlobalRef(env, localReference);

      (*env)->DeleteLocalRef(env, localReference);
      localReference = NULL;

      if (globalReference) {
        UsbEndpointExtension *eptx = malloc(sizeof(*eptx));

        if (eptx) {
          memset(eptx, 0, sizeof(*eptx));
          eptx->endpoint = endpoint;
          eptx->object = globalReference;

          endpoint->extension = eptx;
          return 1;
        } else {
          logMallocError();
        }

        (*env)->DeleteGlobalRef(env, globalReference);
        globalReference = NULL;
      } else {
        logMallocError();
        clearJavaException(env, 0);
      }
    }
  } else {
    errno = ENOSYS;
  }

  return 0;
}

void
usbDeallocateEndpointExtension (UsbEndpointExtension *eptx) {
  UsbEndpoint *endpoint = eptx->endpoint;
  UsbDevice *device = endpoint->device;
  UsbDeviceExtension *devx = device->extension;
  const UsbHostDevice *host = devx->host;
  JNIEnv *env = host->env;

  if (eptx->object) {
    (*env)->DeleteGlobalRef(env, eptx->object);
    eptx->object = NULL;
  }

  free(eptx);
}

void
usbDeallocateDeviceExtension (UsbDeviceExtension *devx) {
  usbUnsetInterface(devx);

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

    host->descriptor.bLength = UsbDescriptorSize_Device;
    host->descriptor.bDescriptorType = UsbDescriptorType_Device;
    host->descriptor.bNumConfigurations = 1;

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
    devx->interface = NULL;

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
