/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2026 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
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

#include "prologue.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <mach/mach.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/usb/USB.h>

#include <mach/mach_time.h>
#include <fcntl.h>
#include <pthread.h>

#include "log.h"
#include "io_usb.h"
#include "usb_internal.h"
#include "system_darwin.h"
#include "async_wait.h"
#include "async_io.h"
#include "async_handle.h"

typedef struct {
  UsbEndpoint *endpoint;
  void *context;
  void *buffer;
  size_t length;

  IOReturn result;
  ssize_t count;
} UsbAsynchronousRequest;

struct UsbDeviceExtensionStruct {
  IOUSBDeviceInterface500 **device;
  unsigned deviceOpened:1;
  unsigned captured:1;
  UInt32 locationID;

  IOUSBInterfaceInterface190 **interface;
  unsigned interfaceOpened:1;
  UInt8 pipeCount;

  CFRunLoopSourceRef runloopSource;

  /* Dedicated thread that runs CFRunLoopRun() so IOKit async callbacks
   * fire immediately on packet arrival. Without it, callbacks only fire
   * when brltty's main thread explicitly pumps via executeRunLoop, which
   * is polling-based and adds latency. */
  pthread_t asyncThread;
  CFRunLoopRef asyncRunloop;       /* captured by asyncThread, read by main */
  CFRunLoopSourceRef wakeSource;   /* dummy source so CFRunLoopRun stays alive */
  pthread_mutex_t queueLock;       /* protects all eptx->completedRequests */
  pthread_cond_t completionCond;   /* signalled on each callback (for OUT waits) */
  unsigned asyncThreadActive:1;
  unsigned asyncShutdown:1;
};

#ifndef kUSBReEnumerateCaptureDeviceMask
#define kUSBReEnumerateCaptureDeviceMask (1U << 30)
#endif
#ifndef kUSBReEnumerateReleaseDeviceMask
#define kUSBReEnumerateReleaseDeviceMask (1U << 29)
#endif

struct UsbEndpointExtensionStruct {
  UsbEndpoint *endpoint;
  Queue *completedRequests;

  UInt8 pipeNumber;
  UInt8 endpointNumber;
  UInt8 transferDirection;
  UInt8 transferMode;
  UInt8 pollInterval;
  UInt16 packetSize;

  uint64_t lastWriteCompletedAbs;

  /* Self-pipe for IN endpoints: the IOKit completion callback writes one
   * byte here; brltty's main thread monitors the read fd via
   * asyncMonitorFileInput and wakes immediately to drain via usbReapResponse. */
  int wakeReadFd;
  int wakeWriteFd;
  AsyncHandle inputMonitor;

  /* Auto-pump state for bulk IN endpoints. macOS's ReadPipeTO only posts a
   * read for the duration of the call; between calls the host NAKs the
   * device, so devices that stop retrying (Focus 40) silently drop their
   * responses. We keep exactly one ReadPipeAsync pending at all times: the
   * prepare hook posts the first one, and usbReadBulkAsync re-arms after
   * each dequeue. Linux's usbfs effectively does the same via the kernel.
   * Flag is set once by prepare() — single-threaded init, no lock. */
  unsigned autoPumpArmed:1;
};

/* Tracks when *any* interrupt IN response was last delivered on the
 * device. Some HID-class devices (Baum VarioUltra) stall the interrupt
 * OUT pipe if a write lands too soon after an input packet arrived. */
static uint64_t lastInputResponseAbs = 0;

static uint64_t
machAbsToMs (uint64_t abs) {
  static mach_timebase_info_data_t timebase = {0};
  if (timebase.denom == 0) mach_timebase_info(&timebase);
  return (abs * timebase.numer) / (timebase.denom * 1000000ULL);
}

/* ------------------------------------------------------------------
 * Dedicated CFRunLoop thread for IOKit USB async callbacks.
 *
 * Background: brltty's main loop is poll/select-based and doesn't run
 * CFRunLoop. With the source attached to CFRunLoopGetMain(), IOKit
 * callbacks only fire when main explicitly pumps via executeRunLoop()
 * inside usbReapResponse. That made input polling-driven (40 ms ticks).
 *
 * Here we spawn a thread that runs CFRunLoop forever. Sources attached
 * to its runloop deliver callbacks in that thread. Each callback then
 * wakes the main thread via either a self-pipe (IN endpoints, monitored
 * by asyncMonitorFileInput) or a condition variable (OUT endpoints,
 * awaited synchronously by usbWriteInterruptAsync).
 * ------------------------------------------------------------------ */

typedef struct {
  UsbDeviceExtension *devx;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  unsigned ready:1;
} UsbAsyncThreadStartup;

static void
usbAsyncWakeSourcePerform (void *info) {
  /* No-op: the source exists only to prevent CFRunLoopRunInMode from
   * returning kCFRunLoopRunFinished when there are no sources yet. */
}

static void *
usbDarwinAsyncIoThread (void *arg) {
  UsbAsyncThreadStartup *startup = arg;
  UsbDeviceExtension *devx = startup->devx;

  CFRunLoopRef rl = CFRunLoopGetCurrent();

  CFRunLoopSourceContext ctx = {0};
  ctx.perform = usbAsyncWakeSourcePerform;
  CFRunLoopSourceRef wake = CFRunLoopSourceCreate(NULL, 0, &ctx);
  if (wake) {
    CFRunLoopAddSource(rl, wake, kCFRunLoopDefaultMode);
  }

  pthread_mutex_lock(&startup->lock);
  devx->asyncRunloop = rl;
  devx->wakeSource = wake;
  startup->ready = 1;
  pthread_cond_signal(&startup->cond);
  pthread_mutex_unlock(&startup->lock);

  while (!devx->asyncShutdown) {
    SInt32 r = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1.0e9, false);
    if (r == kCFRunLoopRunStopped) break;
  }

  if (wake) {
    CFRunLoopRemoveSource(rl, wake, kCFRunLoopDefaultMode);
    CFRelease(wake);
  }
  return NULL;
}

static int
usbStartAsyncIoThread (UsbDeviceExtension *devx) {
  if (devx->asyncThreadActive) return 1;

  if (pthread_mutex_init(&devx->queueLock, NULL) != 0) {
    logSystemError("USB queueLock init");
    return 0;
  }
  if (pthread_cond_init(&devx->completionCond, NULL) != 0) {
    logSystemError("USB completionCond init");
    pthread_mutex_destroy(&devx->queueLock);
    return 0;
  }

  UsbAsyncThreadStartup startup = { .devx = devx, .ready = 0 };
  if (pthread_mutex_init(&startup.lock, NULL) != 0
      || pthread_cond_init(&startup.cond, NULL) != 0) {
    logSystemError("USB async startup sync init");
    pthread_cond_destroy(&devx->completionCond);
    pthread_mutex_destroy(&devx->queueLock);
    return 0;
  }

  if (pthread_create(&devx->asyncThread, NULL, usbDarwinAsyncIoThread, &startup) != 0) {
    logSystemError("USB async thread create");
    pthread_cond_destroy(&startup.cond);
    pthread_mutex_destroy(&startup.lock);
    pthread_cond_destroy(&devx->completionCond);
    pthread_mutex_destroy(&devx->queueLock);
    return 0;
  }

  pthread_mutex_lock(&startup.lock);
  while (!startup.ready) pthread_cond_wait(&startup.cond, &startup.lock);
  pthread_mutex_unlock(&startup.lock);
  pthread_cond_destroy(&startup.cond);
  pthread_mutex_destroy(&startup.lock);

  devx->asyncThreadActive = 1;
  logMessage(LOG_CATEGORY(USB_IO), "USB async I/O thread started");
  return 1;
}

static void
usbStopAsyncIoThread (UsbDeviceExtension *devx) {
  if (!devx->asyncThreadActive) return;
  devx->asyncShutdown = 1;
  if (devx->asyncRunloop) {
    CFRunLoopStop(devx->asyncRunloop);
  }
  pthread_join(devx->asyncThread, NULL);
  pthread_cond_destroy(&devx->completionCond);
  pthread_mutex_destroy(&devx->queueLock);
  devx->asyncThreadActive = 0;
}

static void
setUsbError (long int result, const char *action) {
  switch (result) {
    default: setDarwinSystemError(result); break;

  //MAP_DARWIN_ERROR(kIOUSBUnknownPipeErr, )
  //MAP_DARWIN_ERROR(kIOUSBTooManyPipesErr, )
  //MAP_DARWIN_ERROR(kIOUSBNoAsyncPortErr, )
  //MAP_DARWIN_ERROR(kIOUSBNotEnoughPipesErr, )
  //MAP_DARWIN_ERROR(kIOUSBNotEnoughPowerErr, )
  //MAP_DARWIN_ERROR(kIOUSBEndpointNotFound, )
  //MAP_DARWIN_ERROR(kIOUSBConfigNotFound, )
    MAP_DARWIN_ERROR(kIOUSBTransactionTimeout, ETIMEDOUT)
  //MAP_DARWIN_ERROR(kIOUSBTransactionReturned, )
  //MAP_DARWIN_ERROR(kIOUSBPipeStalled, )
  //MAP_DARWIN_ERROR(kIOUSBInterfaceNotFound, )
  //MAP_DARWIN_ERROR(kIOUSBLowLatencyBufferNotPreviouslyAllocated, )
  //MAP_DARWIN_ERROR(kIOUSBLowLatencyFrameListNotPreviouslyAllocated, )
  //MAP_DARWIN_ERROR(kIOUSBHighSpeedSplitError, )
  //MAP_DARWIN_ERROR(kIOUSBLinkErr, )
  //MAP_DARWIN_ERROR(kIOUSBNotSent2Err, )
  //MAP_DARWIN_ERROR(kIOUSBNotSent1Err, )
  //MAP_DARWIN_ERROR(kIOUSBBufferUnderrunErr, )
  //MAP_DARWIN_ERROR(kIOUSBBufferOverrunErr, )
  //MAP_DARWIN_ERROR(kIOUSBReserved2Err, )
  //MAP_DARWIN_ERROR(kIOUSBReserved1Err, )
  //MAP_DARWIN_ERROR(kIOUSBWrongPIDErr, )
  //MAP_DARWIN_ERROR(kIOUSBPIDCheckErr, )
  //MAP_DARWIN_ERROR(kIOUSBDataToggleErr, )
  //MAP_DARWIN_ERROR(kIOUSBBitstufErr, )
  //MAP_DARWIN_ERROR(kIOUSBCRCErr, )
  }

  if (action) {
    logMessage(LOG_WARNING, "Darwin error 0X%lX.", result);
    logSystemError(action);
  }
}

static IOUSBDeviceInterface500 **
acquireDeviceInterfaceMatching (uint16_t vendor, uint16_t product, UInt32 locationID) {
  CFMutableDictionaryRef match = IOServiceMatching(kIOUSBDeviceClassName);
  if (!match) return NULL;

  CFNumberRef vidNum = CFNumberCreate(NULL, kCFNumberSInt16Type, &vendor);
  CFNumberRef pidNum = CFNumberCreate(NULL, kCFNumberSInt16Type, &product);
  CFDictionarySetValue(match, CFSTR(kUSBVendorID), vidNum);
  CFDictionarySetValue(match, CFSTR(kUSBProductID), pidNum);
  CFRelease(vidNum);
  CFRelease(pidNum);

  io_iterator_t iterator = IO_OBJECT_NULL;
  kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault, match, &iterator);
  if (kr != KERN_SUCCESS) return NULL;

  IOUSBDeviceInterface500 **result = NULL;
  io_service_t service;
  while ((service = IOIteratorNext(iterator))) {
    IOCFPlugInInterface **plugin = NULL;
    SInt32 score = 0;
    IOReturn pluginResult = IOCreatePlugInInterfaceForService(
      service, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID,
      &plugin, &score
    );
    IOObjectRelease(service);

    if (pluginResult == kIOReturnSuccess && plugin) {
      IOUSBDeviceInterface500 **candidate = NULL;
      (*plugin)->QueryInterface(
        plugin,
        CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID500),
        (LPVOID)&candidate
      );
      (*plugin)->Release(plugin);

      if (candidate) {
        // If a locationID is provided, prefer that exact physical port so we
        // don't pick the wrong unit when several identical displays are
        // attached. A mismatch is non-fatal: we still try the first device.
        if (locationID != 0) {
          UInt32 loc = 0;
          if ((*candidate)->GetLocationID(candidate, &loc) == kIOReturnSuccess
              && loc == locationID) {
            result = candidate;
            break;
          }
          if (!result) {
            result = candidate;
            continue;
          }
          (*candidate)->Release(candidate);
        } else {
          result = candidate;
          break;
        }
      }
    }
  }
  IOObjectRelease(iterator);
  return result;
}

// Take exclusive ownership of a USB device that is currently bound to another
// driver (typically AppleUserUSBHostHIDDevice on macOS, which prevents libusb
// from opening the interface). The technique mirrors what VirtualBox / VMware
// Fusion use: USBDeviceReEnumerate with the capture flag tears down existing
// bindings, then we re-acquire a fresh IOUSBDeviceInterface for the same
// physical port (matched by locationID) before any other driver attaches.
static int
captureDevice (UsbDeviceExtension *devx) {
  if (devx->captured) return 1;

  UInt32 locationID = 0;
  uint16_t vendor = 0, product = 0;
  (*devx->device)->GetLocationID(devx->device, &locationID);
  (*devx->device)->GetDeviceVendor(devx->device, &vendor);
  (*devx->device)->GetDeviceProduct(devx->device, &product);

  IOReturn result = (*devx->device)->USBDeviceReEnumerate(
    devx->device, kUSBReEnumerateCaptureDeviceMask
  );
  if (result != kIOReturnSuccess) {
    setUsbError(result, "USB device capture (re-enumerate)");
    return 0;
  }

  // The kernel terminates the old IOService; our handle is now stale.
  (*devx->device)->Release(devx->device);
  devx->device = NULL;

  // Poll for the re-enumerated device by VID/PID, preferring the same
  // physical port when locationID is known. macOS typically completes
  // re-enumeration in well under a second; we wait up to ~5 seconds.
  IOUSBDeviceInterface500 **fresh = NULL;
  for (int attempt = 0; attempt < 100; attempt += 1) {
    fresh = acquireDeviceInterfaceMatching(vendor, product, locationID);
    if (fresh) break;
    usleep(50000);
  }

  if (!fresh) {
    logMessage(LOG_WARNING,
      "USB device did not reappear after capture (vendor=%04X product=%04X locationID=0x%08X)",
      vendor, product, locationID);
    return 0;
  }

  devx->device = fresh;
  devx->locationID = locationID;
  devx->captured = 1;
  logMessage(LOG_NOTICE,
    "USB device captured: vendor=%04X product=%04X locationID=0x%08X",
    vendor, product, locationID);
  return 1;
}

static void
releaseCapturedDevice (UsbDeviceExtension *devx) {
  if (!devx->captured) return;
  if (!devx->device) return;

  IOReturn result = (*devx->device)->USBDeviceReEnumerate(
    devx->device, kUSBReEnumerateReleaseDeviceMask
  );
  if (result != kIOReturnSuccess) {
    setUsbError(result, "USB device release (re-enumerate)");
  }
  devx->captured = 0;
}

static int
openDevice (UsbDevice *device, int seize) {
  UsbDeviceExtension *devx = device->extension;
  IOReturn result;

  if (!devx->deviceOpened) {
    const char *action = "opened";
    int level = LOG_INFO;

    // On macOS, refreshable braille displays present as USB-HID and the kernel
    // automatically binds them to AppleUserUSBHostHIDDevice, which then refuses
    // exclusive access. Capture the device away from that driver first.
    if (!captureDevice(devx)) {
      // Capture failed; the open attempt below will likely also fail with
      // kIOReturnExclusiveAccess, but we let it proceed so the error is
      // reported in the usual code path.
    }

    result = (*devx->device)->USBDeviceOpen(devx->device);
    if (result != kIOReturnSuccess) {
      setUsbError(result, "USB device open");
      if ((result != kIOReturnExclusiveAccess) || !seize) return 0;

      result = (*devx->device)->USBDeviceOpenSeize(devx->device);
      if (result != kIOReturnSuccess) {
        setUsbError(result, "USB device seize");
        return 0;
      }

      action = "seized";
      level = LOG_NOTICE;
    }

    logMessage(level, "USB device %s: vendor=%04X product=%04X",
               action, device->descriptor.idVendor, device->descriptor.idProduct);
    devx->deviceOpened = 1;
  }

  return 1;
}

static int
unsetInterface (UsbDeviceExtension *devx) {
  int ok = 1;

  if (devx->interface) {
    IOReturn result;

    if (devx->interfaceOpened) {
      if (devx->runloopSource) {
        {
          int pipe;
          for (pipe=1; pipe<=devx->pipeCount; ++pipe) {
            result = (*devx->interface)->AbortPipe(devx->interface, pipe);
            if (result != kIOReturnSuccess) {
              setUsbError(result, "USB pipe abort");
            }
          }
        }

        removeRunLoopSource(devx->runloopSource);
      }

      result = (*devx->interface)->USBInterfaceClose(devx->interface);
      if (result != kIOReturnSuccess) {
        setUsbError(result, "USB interface close");
        ok = 0;
      }
      devx->interfaceOpened = 0;
    }

    (*devx->interface)->Release(devx->interface);
    devx->interface = NULL;
  }

  return ok;
}

static int
isInterface (IOUSBInterfaceInterface190 **interface, UInt8 number) {
  IOReturn result;
  UInt8 num;

  result = (*interface)->GetInterfaceNumber(interface, &num);
  if (result != kIOReturnSuccess) {
    setUsbError(result, "USB interface number query");
  } else if (num == number) {
    return 1;
  }

  return 0;
}

static int
setInterface (UsbDeviceExtension *devx, UInt8 number) {
  int found = 0;
  IOReturn result;
  io_iterator_t iterator = 0;

  if (devx->interface)
    if (isInterface(devx->interface, number))
      return 1;

  {
    IOUSBFindInterfaceRequest request;

    request.bInterfaceClass = kIOUSBFindInterfaceDontCare;
    request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
    request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    request.bAlternateSetting = kIOUSBFindInterfaceDontCare;

    result = (*devx->device)->CreateInterfaceIterator(devx->device, &request, &iterator);
  }

  if ((result == kIOReturnSuccess) && iterator) {
    io_service_t service;

    while ((service = IOIteratorNext(iterator))) {
      IOCFPlugInInterface **plugin = NULL;
      SInt32 score;

      result = IOCreatePlugInInterfaceForService(service,
                                                 kIOUSBInterfaceUserClientTypeID,
                                                 kIOCFPlugInInterfaceID,
                                                 &plugin, &score);
      IOObjectRelease(service);
      service = 0;

      if ((result == kIOReturnSuccess) && plugin) {
        IOUSBInterfaceInterface190 **interface = NULL;

        result = (*plugin)->QueryInterface(plugin,
                                           CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID190),
                                           (LPVOID)&interface);
        (*plugin)->Release(plugin);
        plugin = NULL;

        if ((result == kIOReturnSuccess) && interface) {
          if (isInterface(interface, number)) {
            unsetInterface(devx);
            devx->interface = interface;
            found = 1;
            break;
          }

          (*interface)->Release(interface);
          interface = NULL;
        } else {
          setUsbError(result, "USB interface interface create");
        }
      } else {
        setUsbError(result, "USB interface service plugin create");
      }
    }
    if (!found) logMessage(LOG_ERR, "USB interface not found: %d", number);

    IOObjectRelease(iterator);
    iterator = 0;
  } else {
    setUsbError(result, "USB interface iterator create");
  }

  return found;
}

/* Deallocator for completedRequests queue: frees the malloc'd request
 * struct (which has its data buffer appended as a flexible-array tail,
 * so a single free suffices). */
static void
usbDeallocateAsyncRequest (void *item, void *data) {
  free(item);
}

static void
usbAsynchronousRequestCallback (void *context, IOReturn result, void *arg) {
  UsbAsynchronousRequest *request = context;
  UsbEndpoint *endpoint = request->endpoint;
  UsbEndpointExtension *eptx = endpoint->extension;
  UsbDeviceExtension *devx = endpoint->device->extension;

  request->result = result;
  request->count = (intptr_t)arg;

  if (eptx->transferDirection == kUSBIn && result == kIOReturnSuccess
      && (intptr_t)arg > 0) {
    lastInputResponseAbs = mach_absolute_time();
  }

  pthread_mutex_lock(&devx->queueLock);
  int enqueued = enqueueItem(eptx->completedRequests, request)? 1: 0;
  if (!enqueued) {
    logSystemError("USB completed request enqueue");
  }
  /* Signal any thread blocked in usbWriteInterruptAsync waiting on this
   * device's completion cond — they'll re-check their own queue. */
  pthread_cond_broadcast(&devx->completionCond);
  pthread_mutex_unlock(&devx->queueLock);

  if (!enqueued) {
    free(request);
    return;
  }

  /* Wake the main thread via per-endpoint self-pipe (IN endpoints only).
   * The byte is consumed by usbDarwinInputMonitorCallback before invoking
   * the user callback (gioInputMonitor → handleBrailleInput). */
  if (eptx->transferDirection == kUSBIn && eptx->wakeWriteFd >= 0) {
    uint8_t b = 0;
    ssize_t w = write(eptx->wakeWriteFd, &b, 1);
    (void)w;  /* EAGAIN means the pipe already has a pending byte — fine */
  }
}

int
usbDisableAutosuspend (UsbDevice *device) {
  logUnsupportedFunction();
  return 0;
}

int
usbSetConfiguration (UsbDevice *device, unsigned char configuration) {
  UsbDeviceExtension *devx = device->extension;

  if (openDevice(device, 1)) {
    UInt8 arg = configuration;
    IOReturn result = (*devx->device)->SetConfiguration(devx->device, arg);
    if (result == kIOReturnSuccess) return 1;
    setUsbError(result, "USB configuration set");
  }

  return 0;
}

int
usbClaimInterface (UsbDevice *device, unsigned char interface) {
  UsbDeviceExtension *devx = device->extension;
  IOReturn result;

  if (setInterface(devx, interface)) {
    if (devx->interfaceOpened) return 1;

    result = (*devx->interface)->USBInterfaceOpen(devx->interface);
    if (result == kIOReturnExclusiveAccess) {
      setUsbError(result, "USB interface open");
      result = (*devx->interface)->USBInterfaceOpenSeize(devx->interface);
      if (result != kIOReturnSuccess) {
        setUsbError(result, "USB interface seize");
      } else {
        logMessage(LOG_NOTICE, "USB interface seized: %u", interface);
      }
    }
    if (result == kIOReturnSuccess) {
      result = (*devx->interface)->GetNumEndpoints(devx->interface, &devx->pipeCount);
      if (result == kIOReturnSuccess) {
        devx->interfaceOpened = 1;
        return 1;
      } else {
        setUsbError(result, "USB pipe count query");
      }

      (*devx->interface)->USBInterfaceClose(devx->interface);
    } else if (result != kIOReturnExclusiveAccess) {
      setUsbError(result, "USB interface open");
    }
  }

  return 0;
}

int
usbReleaseInterface (UsbDevice *device, unsigned char interface) {
  UsbDeviceExtension *devx = device->extension;
  return setInterface(devx, interface) && unsetInterface(devx);
}

int
usbSetAlternative (
  UsbDevice *device,
  unsigned char interface,
  unsigned char alternative
) {
  UsbDeviceExtension *devx = device->extension;

  if (setInterface(devx, interface)) {
    IOReturn result;
    UInt8 arg;

    result = (*devx->interface)->GetAlternateSetting(devx->interface, &arg);
    if (result == kIOReturnSuccess) {
      if (arg == alternative) return 1;

      arg = alternative;
      result = (*devx->interface)->SetAlternateInterface(devx->interface, arg);
      if (result == kIOReturnSuccess) return 1;
      setUsbError(result, "USB alternative set");
    } else {
      setUsbError(result, "USB alternative get");
    }
  }

  return 0;
}

int
usbResetDevice (UsbDevice *device) {
  logUnsupportedFunction();
  return 0;
}

int
usbClearHalt (UsbDevice *device, unsigned char endpointAddress) {
  UsbDeviceExtension *devx = device->extension;
  UsbEndpoint *endpoint;

  if ((endpoint = usbGetEndpoint(device, endpointAddress))) {
    UsbEndpointExtension *eptx = endpoint->extension;
    IOReturn result;

    result = (*devx->interface)->ClearPipeStallBothEnds(devx->interface, eptx->pipeNumber);
    if (result == kIOReturnSuccess) return 1;
    setUsbError(result, "USB endpoint clear");
  }

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
  UsbDeviceExtension *devx = device->extension;
  IOReturn result;
  IOUSBDevRequestTO arg;

  arg.bmRequestType = direction | recipient | type;
  arg.bRequest = request;
  arg.wValue = value;
  arg.wIndex = index;
  arg.wLength = length;

  arg.pData = buffer;
  arg.noDataTimeout = timeout;
  arg.completionTimeout = timeout;

  result = (*devx->device)->DeviceRequestTO(devx->device, &arg);
  if (result == kIOReturnSuccess) return arg.wLenDone;
  setUsbError(result, "USB control transfer");
  return -1;
}

/* Lazily start the async I/O thread and attach the IOKit event source to
 * its runloop, so async callbacks fire immediately on packet arrival
 * instead of waiting for the main thread to pump. Idempotent. */
static int
usbEnsureAsyncIoActive (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  if (devx->runloopSource) return 1;

  if (!usbStartAsyncIoThread(devx)) return 0;

  IOReturn result = (*devx->interface)->CreateInterfaceAsyncEventSource(devx->interface,
                                                                        &devx->runloopSource);
  if (result != kIOReturnSuccess) {
    setUsbError(result, "USB interface event source create");
    return 0;
  }

  CFRunLoopAddSource(devx->asyncRunloop, devx->runloopSource, kCFRunLoopDefaultMode);
  CFRunLoopWakeUp(devx->asyncRunloop);
  return 1;
}

/* Post a fresh ReadPipeAsync for a bulk IN endpoint's auto-pump. The
 * completion is enqueued onto eptx->completedRequests by the standard
 * callback; usbReadBulkAsync drains and re-arms. Always one request
 * pending in IOKit between calls, so the device's responses never NAK
 * for lack of a posted read. */
static int
usbDarwinPostBulkInRead (UsbDevice *device, UsbEndpoint *endpoint) {
  UsbDeviceExtension *devx = device->extension;
  UsbEndpointExtension *eptx = endpoint->extension;
  size_t length = eptx->packetSize;

  UsbAsynchronousRequest *request = malloc(sizeof(*request) + length);
  if (!request) {
    logSystemError("USB bulk-in auto-pump request allocate");
    return 0;
  }

  request->endpoint = endpoint;
  request->context = NULL;
  request->length = length;
  request->buffer = length ? (request + 1) : NULL;

  IOReturn result = (*devx->interface)->ReadPipeAsync(devx->interface, eptx->pipeNumber,
                                                       request->buffer, request->length,
                                                       usbAsynchronousRequestCallback, request);
  if (result != kIOReturnSuccess) {
    setUsbError(result, "USB bulk-in auto-pump ReadPipeAsync");
    free(request);
    return 0;
  }

  return 1;
}

/* endpoint->prepare hook for IN endpoints. For bulk IN, arms a permanent
 * auto-pump so polling-style readers (gioReadByte with short timeouts)
 * never miss device responses between sync ReadPipeTO calls. Interrupt IN
 * keeps the existing ReadPipe path — its polling-interval semantics give
 * IOKit enough buffering on its own. */
static int
usbDarwinPrepareInputEndpoint (UsbEndpoint *endpoint) {
  UsbDevice *device = endpoint->device;
  UsbEndpointExtension *eptx = endpoint->extension;

  if (eptx->transferMode != kUSBBulk) return 1;
  if (eptx->autoPumpArmed) return 1;

  if (!usbEnsureAsyncIoActive(device)) return 0;
  if (!usbDarwinPostBulkInRead(device, endpoint)) return 0;

  eptx->autoPumpArmed = 1;
  logMessage(LOG_CATEGORY(USB_IO),
             "bulk IN auto-pump armed: ept=%02X pkt=%u",
             endpoint->descriptor->bEndpointAddress, eptx->packetSize);
  return 1;
}

void *
usbSubmitRequest (
  UsbDevice *device,
  unsigned char endpointAddress,
  void *buffer,
  size_t length,
  void *context
) {
  UsbDeviceExtension *devx = device->extension;
  UsbEndpoint *endpoint;

  if ((endpoint = usbGetEndpoint(device, endpointAddress))) {
    UsbEndpointExtension *eptx = endpoint->extension;
    IOReturn result;
    UsbAsynchronousRequest *request;

    if (!usbEnsureAsyncIoActive(device)) return NULL;

    if ((request = malloc(sizeof(*request) + length))) {
      request->endpoint = endpoint;
      request->context = context;
      request->buffer = (request->length = length)? (request + 1): NULL;

      switch (eptx->transferDirection) {
        case kUSBIn:
          result = (*devx->interface)->ReadPipeAsync(devx->interface, eptx->pipeNumber,
                                                     request->buffer, request->length,
                                                     usbAsynchronousRequestCallback, request);
          if (result == kIOReturnSuccess) return request;
          setUsbError(result, "USB endpoint asynchronous read");
          break;

        case kUSBOut:
          if (request->buffer) memcpy(request->buffer, buffer, length);
          result = (*devx->interface)->WritePipeAsync(devx->interface, eptx->pipeNumber,
                                                      request->buffer, request->length,
                                                      usbAsynchronousRequestCallback, request);
          if (result == kIOReturnSuccess) return request;
          setUsbError(result, "USB endpoint asynchronous write");
          break;

        default:
          logMessage(LOG_ERR, "USB endpoint direction not supported: %d",
                     eptx->transferDirection);
          errno = ENOSYS;
          break;
      }

      free(request);
    } else {
      logSystemError("USB asynchronous request allocate");
    }
  }

  return NULL;
}

int
usbCancelRequest (UsbDevice *device, void *request) {
  logUnsupportedFunction();
  return 0;
}

void *
usbReapResponse (
  UsbDevice *device,
  unsigned char endpointAddress,
  UsbResponse *response,
  int wait
) {
  UsbEndpoint *endpoint;

  if ((endpoint = usbGetEndpoint(device, endpointAddress))) {
    UsbDeviceExtension *devx = device->extension;
    UsbEndpointExtension *eptx = endpoint->extension;
    UsbAsynchronousRequest *request = NULL;

    pthread_mutex_lock(&devx->queueLock);
    request = dequeueItem(eptx->completedRequests);
    if (!request && wait) {
      /* Wait on the device-wide completion cond. Spurious wakes are fine
       * because we re-check the queue each loop. */
      while (!request) {
        pthread_cond_wait(&devx->completionCond, &devx->queueLock);
        request = dequeueItem(eptx->completedRequests);
      }
    }
    pthread_mutex_unlock(&devx->queueLock);

    if (!request) {
      errno = EAGAIN;
      return NULL;
    }

    response->context = request->context;
    response->buffer = request->buffer;
    response->size = request->length;

    if (request->result == kIOReturnSuccess) {
      response->error = 0;
      response->count = request->count;

      if (!usbApplyInputFilters(endpoint, response->buffer, response->size, &response->count)) {
        response->error = EIO;
        response->count = -1;
      }
    } else {
      setUsbError(request->result, "USB asynchronous response");
      response->error = errno;
      response->count = -1;
    }

    return request;
  }

  return NULL;
}

typedef struct {
  UsbEndpointExtension *eptx;
  AsyncMonitorCallback *userCallback;
  void *userData;
} UsbDarwinMonitorContext;

ASYNC_MONITOR_CALLBACK(usbDarwinInputMonitorCallback) {
  UsbDarwinMonitorContext *ctx = parameters->data;
  UsbEndpointExtension *eptx = ctx->eptx;

  /* Drain the wake pipe — the async I/O thread may have written several
   * bytes if multiple responses arrived before we returned to the loop. */
  if (eptx->wakeReadFd >= 0) {
    uint8_t buf[64];
    while (read(eptx->wakeReadFd, buf, sizeof(buf)) > 0) ;
  }

  AsyncMonitorCallbackParameters userParams = {
    .error = parameters->error,
    .data = ctx->userData
  };
  return ctx->userCallback(&userParams);
}

int
usbMonitorInputEndpoint (
  UsbDevice *device, unsigned char endpointNumber,
  AsyncMonitorCallback *callback, void *data
) {
  UsbEndpoint *endpoint = usbGetInputEndpoint(device, endpointNumber);
  if (!endpoint) return 0;

  UsbEndpointExtension *eptx = endpoint->extension;
  if (!eptx) return 0;

  /* Tear down any prior monitor. */
  if (eptx->inputMonitor) {
    asyncCancelRequest(eptx->inputMonitor);
    eptx->inputMonitor = NULL;
  }
  if (!callback) return 1;

  /* Lazy-create the self-pipe on first attach. */
  if (eptx->wakeReadFd < 0) {
    int fds[2];
    if (pipe(fds) == -1) {
      logSystemError("USB wake pipe");
      return 0;
    }
    fcntl(fds[0], F_SETFL, O_NONBLOCK);
    fcntl(fds[1], F_SETFL, O_NONBLOCK);
    eptx->wakeReadFd = fds[0];
    eptx->wakeWriteFd = fds[1];
  }

  UsbDarwinMonitorContext *ctx = malloc(sizeof(*ctx));
  if (!ctx) {
    logMallocError();
    return 0;
  }
  ctx->eptx = eptx;
  ctx->userCallback = callback;
  ctx->userData = data;

  if (!asyncMonitorFileInput(&eptx->inputMonitor, eptx->wakeReadFd,
                             usbDarwinInputMonitorCallback, ctx)) {
    free(ctx);
    return 0;
  }

  return 1;
}

/* Bulk IN read backed by the always-armed ReadPipeAsync auto-pump. Replaces
 * the previous synchronous ReadPipeTO path: between sync calls no host
 * read was pending and bulk IN packets sent by the device were NAK'd,
 * which the Focus 40 firmware treated as "host gone" — no retransmission.
 * Now exactly one ReadPipeAsync is always pending; this function dequeues
 * its completion (waiting up to timeout ms via the device-wide cond) and
 * re-arms a fresh one before returning. */
static ssize_t
usbReadBulkAsync (
  UsbDevice *device, UsbEndpoint *endpoint,
  void *buffer, size_t length, int timeout
) {
  UsbDeviceExtension *devx = device->extension;
  UsbEndpointExtension *eptx = endpoint->extension;

  /* Belt-and-braces: if prepare() never armed (e.g. brl driver opened a
   * bulk IN endpoint via a path that skips endpoint->prepare), arm now. */
  if (!eptx->autoPumpArmed) {
    if (!usbEnsureAsyncIoActive(device)) return -1;
    if (!usbDarwinPostBulkInRead(device, endpoint)) return -1;
    eptx->autoPumpArmed = 1;
  }

  uint64_t deadlineMs = machAbsToMs(mach_absolute_time())
                       + (uint64_t)(timeout > 0 ? timeout : 0);
  UsbAsynchronousRequest *request = NULL;

  pthread_mutex_lock(&devx->queueLock);
  for (;;) {
    request = dequeueItem(eptx->completedRequests);
    if (request) break;

    uint64_t nowMs = machAbsToMs(mach_absolute_time());
    if (nowMs >= deadlineMs) break;

    long deltaMs = (long)(deadlineMs - nowMs);
    struct timespec abs_ts;
    clock_gettime(CLOCK_REALTIME, &abs_ts);
    abs_ts.tv_sec  += deltaMs / 1000;
    abs_ts.tv_nsec += (deltaMs % 1000) * 1000000L;
    if (abs_ts.tv_nsec >= 1000000000L) { abs_ts.tv_sec++; abs_ts.tv_nsec -= 1000000000L; }

    int rc = pthread_cond_timedwait(&devx->completionCond, &devx->queueLock, &abs_ts);
    if (rc == ETIMEDOUT) {
      request = dequeueItem(eptx->completedRequests);
      break;
    }
  }
  pthread_mutex_unlock(&devx->queueLock);

  if (!request) {
    errno = EAGAIN;
    return -1;
  }

  IOReturn result = request->result;
  ssize_t actual = -1;
  if (result == kIOReturnSuccess) {
    size_t toCopy = ((size_t)request->count < length) ? (size_t)request->count : length;
    if (toCopy && buffer) memcpy(buffer, request->buffer, toCopy);
    actual = (ssize_t)toCopy;
  }
  free(request);

  /* Re-arm immediately so the next bulk IN packet has somewhere to land.
   * If this fails (e.g. device went away mid-call), subsequent reads will
   * time out and brltty's higher layer will reset the driver. */
  usbDarwinPostBulkInRead(device, endpoint);

  switch (result) {
    case kIOReturnSuccess:
      if (usbApplyInputFilters(endpoint, buffer, length, &actual)) return actual;
      errno = EIO;
      return -1;

    case kIOUSBTransactionTimeout:
      errno = EAGAIN;
      return -1;

    case kIOUSBPipeStalled:
      (*devx->interface)->ClearPipeStallBothEnds(devx->interface, eptx->pipeNumber);
      errno = EAGAIN;
      return -1;

    case kIOReturnAborted:
      errno = EAGAIN;
      return -1;

    default:
      setUsbError(result, "USB bulk endpoint async read");
      return -1;
  }
}

ssize_t
usbReadEndpoint (
  UsbDevice *device,
  unsigned char endpointNumber,
  void *buffer,
  size_t length,
  int timeout
) {
  UsbDeviceExtension *devx = device->extension;
  UsbEndpoint *endpoint;

  if ((endpoint = usbGetInputEndpoint(device, endpointNumber))) {
    UsbEndpointExtension *eptx = endpoint->extension;
    IOReturn result;
    UInt32 count;
    int stalled = 0;

    if (eptx->transferMode == kUSBBulk) {
      return usbReadBulkAsync(device, endpoint, buffer, length, timeout);
    }

  read:
    count = length;
    // Interrupt pipes use ReadPipe (no timeout argument — they complete on
    // the next polling interval or when data is available).
    result = (*devx->interface)->ReadPipe(devx->interface, eptx->pipeNumber,
                                          buffer, &count);

    switch (result) {
      case kIOReturnSuccess:
        {
          ssize_t actual = count;

          if (usbApplyInputFilters(endpoint, buffer, length, &actual)) return actual;
        }

        errno = EIO;
        break;

      case kIOUSBTransactionTimeout:
        errno = EAGAIN;
        break;

      case kIOUSBPipeStalled:
        if (!stalled) {
          result = (*devx->interface)->ClearPipeStallBothEnds(devx->interface, eptx->pipeNumber);
          if (result == kIOReturnSuccess) {
            stalled = 1;
            goto read;
          }

          setUsbError(result, "USB stall clear");
          break;
        }

      default:
        setUsbError(result, "USB endpoint read");
        break;
    }
  }

  return -1;
}

/* How long to wait for an async interrupt write to complete before
 * giving up. IOKit's synchronous WritePipe waits indefinitely if the
 * device NAKs (busy sending IN reports), so we drive WritePipeAsync
 * ourselves with a bounded timeout and recovery. */
#define USB_DARWIN_INTERRUPT_WRITE_TIMEOUT_MS 250

static ssize_t
usbWriteInterruptAsync (
  UsbDevice *device, UsbEndpoint *endpoint,
  unsigned char endpointAddress,
  const void *buffer, size_t length
) {
  UsbDeviceExtension *devx = device->extension;
  UsbEndpointExtension *eptx = endpoint->extension;

  /* Drain any stale completions left over from a previous timeout/abort. */
  pthread_mutex_lock(&devx->queueLock);
  {
    UsbAsynchronousRequest *stale;
    while ((stale = dequeueItem(eptx->completedRequests))) {
      free(stale);
    }
  }
  pthread_mutex_unlock(&devx->queueLock);

  logMessage(LOG_DEBUG, "WritePipe enter: ep=%02X len=%zu", endpointAddress, length);

  void *handle = usbSubmitRequest(device, endpointAddress,
                                  (void *)buffer, length, NULL);
  if (!handle) {
    logMessage(LOG_DEBUG, "WritePipe exit:  ep=%02X submit failed", endpointAddress);
    return -1;
  }

  /* Wait on the device-wide completion cond until our request shows up in
   * our endpoint's queue or the deadline passes. The callback fires on the
   * async-I/O thread, enqueues the completion, and broadcasts the cond. */
  uint64_t deadlineMs = machAbsToMs(mach_absolute_time())
                       + USB_DARWIN_INTERRUPT_WRITE_TIMEOUT_MS;
  UsbAsynchronousRequest *request = NULL;

  pthread_mutex_lock(&devx->queueLock);
  for (;;) {
    request = dequeueItem(eptx->completedRequests);
    if (request) break;

    uint64_t nowMs = machAbsToMs(mach_absolute_time());
    if (nowMs >= deadlineMs) break;

    struct timespec abs;
    uint64_t targetNs = (uint64_t)deadlineMs * 1000000ULL;
    abs.tv_sec  = targetNs / 1000000000ULL;
    abs.tv_nsec = targetNs % 1000000000ULL;
    /* CLOCK_REALTIME for pthread_cond_timedwait on macOS (default). We
     * convert our monotonic deadline to wall-clock once at entry: a brief
     * wall-clock jump just terminates the wait early, which is harmless. */
    struct timespec now_ts;
    clock_gettime(CLOCK_REALTIME, &now_ts);
    uint64_t monoNowMs = machAbsToMs(mach_absolute_time());
    long deltaMs = (long)deadlineMs - (long)monoNowMs;
    if (deltaMs <= 0) break;
    abs.tv_sec  = now_ts.tv_sec  + deltaMs / 1000;
    abs.tv_nsec = now_ts.tv_nsec + (deltaMs % 1000) * 1000000L;
    if (abs.tv_nsec >= 1000000000L) { abs.tv_sec++; abs.tv_nsec -= 1000000000L; }

    int rc = pthread_cond_timedwait(&devx->completionCond, &devx->queueLock, &abs);
    if (rc == ETIMEDOUT) {
      request = dequeueItem(eptx->completedRequests);
      break;
    }
  }
  pthread_mutex_unlock(&devx->queueLock);

  if (!request) {
    /* Timed out. Cancel the pipe so the request unblocks, clear any stall
     * state on both ends, then drain the completion that AbortPipe leaves
     * behind so subsequent writes start clean. */
    logMessage(LOG_WARNING, "WritePipe timed out: ep=%02X len=%zu — aborting pipe",
               endpointAddress, length);
    IOReturn ar = (*devx->interface)->AbortPipe(devx->interface, eptx->pipeNumber);
    if (ar != kIOReturnSuccess) {
      logMessage(LOG_WARNING, "AbortPipe failed: 0X%X", ar);
    }
    IOReturn cr = (*devx->interface)->ClearPipeStallBothEnds(devx->interface, eptx->pipeNumber);
    if (cr != kIOReturnSuccess) {
      logMessage(LOG_WARNING, "ClearPipeStallBothEnds failed: 0X%X", cr);
    }

    /* Reap the abort-completion from the async-I/O thread. Bounded wait. */
    uint64_t reapDeadlineMs = machAbsToMs(mach_absolute_time()) + 100;
    pthread_mutex_lock(&devx->queueLock);
    for (;;) {
      request = dequeueItem(eptx->completedRequests);
      if (request) { free(request); request = NULL; break; }
      uint64_t nowMs = machAbsToMs(mach_absolute_time());
      if (nowMs >= reapDeadlineMs) break;
      struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
      long dMs = (long)(reapDeadlineMs - nowMs);
      ts.tv_sec += dMs / 1000;
      ts.tv_nsec += (dMs % 1000) * 1000000L;
      if (ts.tv_nsec >= 1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }
      pthread_cond_timedwait(&devx->completionCond, &devx->queueLock, &ts);
    }
    pthread_mutex_unlock(&devx->queueLock);

    errno = ETIMEDOUT;
    return -1;
  }

  IOReturn result = request->result;
  ssize_t count = (result == kIOReturnSuccess)? (ssize_t)length: -1;
  free(request);

  logMessage(LOG_DEBUG, "WritePipe exit:  ep=%02X result=0X%X", endpointAddress, result);

  if (result != kIOReturnSuccess) {
    setUsbError(result, "USB endpoint async write");
    return -1;
  }

  eptx->lastWriteCompletedAbs = mach_absolute_time();
  return count;
}

ssize_t
usbWriteEndpoint (
  UsbDevice *device,
  unsigned char endpointNumber,
  const void *buffer,
  size_t length,
  int timeout
) {
  UsbDeviceExtension *devx = device->extension;
  UsbEndpoint *endpoint;

  if ((endpoint = usbGetOutputEndpoint(device, endpointNumber))) {
    UsbEndpointExtension *eptx = endpoint->extension;
    IOReturn result;

    // WritePipeTO only supports bulk pipes on macOS; interrupt pipes go
    // through WritePipeAsync (via usbSubmitRequest) with our own timeout
    // and recovery, because the sync WritePipe blocks indefinitely on NAK.
    if (eptx->transferMode == kUSBInterrupt) {
      return usbWriteInterruptAsync(device, endpoint,
                                    endpoint->descriptor->bEndpointAddress,
                                    buffer, length);
    } else {
      result = (*devx->interface)->WritePipeTO(devx->interface, eptx->pipeNumber,
                                               (void *)buffer, length,
                                               timeout, timeout);
    }
    if (result == kIOReturnSuccess) return length;
    setUsbError(result, "USB endpoint write");
  }

  return -1;
}

int
usbReadDeviceDescriptor (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  IOReturn result;

  {
    uint8_t speed;
    if ((result = (*devx->device)->GetDeviceSpeed(devx->device, &speed)) != kIOReturnSuccess) goto error;
    switch (speed) {
      default                 : device->descriptor.bcdUSB = 0X0000; break;
      case kUSBDeviceSpeedLow : device->descriptor.bcdUSB = kUSBRel10; break;
      case kUSBDeviceSpeedFull: device->descriptor.bcdUSB = kUSBRel11; break;
      case kUSBDeviceSpeedHigh: device->descriptor.bcdUSB = kUSBRel20; break;
    }
  }

  if ((result = (*devx->device)->GetDeviceClass(devx->device, &device->descriptor.bDeviceClass)) != kIOReturnSuccess) goto error;
  if ((result = (*devx->device)->GetDeviceSubClass(devx->device, &device->descriptor.bDeviceSubClass)) != kIOReturnSuccess) goto error;
  if ((result = (*devx->device)->GetDeviceProtocol(devx->device, &device->descriptor.bDeviceProtocol)) != kIOReturnSuccess) goto error;

  if ((result = (*devx->device)->GetDeviceVendor(devx->device, &device->descriptor.idVendor)) != kIOReturnSuccess) goto error;
  if ((result = (*devx->device)->GetDeviceProduct(devx->device, &device->descriptor.idProduct)) != kIOReturnSuccess) goto error;
  if ((result = (*devx->device)->GetDeviceReleaseNumber(devx->device, &device->descriptor.bcdDevice)) != kIOReturnSuccess) goto error;

  if ((result = (*devx->device)->USBGetManufacturerStringIndex(devx->device, &device->descriptor.iManufacturer)) != kIOReturnSuccess) goto error;
  if ((result = (*devx->device)->USBGetProductStringIndex(devx->device, &device->descriptor.iProduct)) != kIOReturnSuccess) goto error;
  if ((result = (*devx->device)->USBGetSerialNumberStringIndex(devx->device, &device->descriptor.iSerialNumber)) != kIOReturnSuccess) goto error;

  if ((result = (*devx->device)->GetNumberOfConfigurations(devx->device, &device->descriptor.bNumConfigurations)) != kIOReturnSuccess) goto error;
  device->descriptor.bMaxPacketSize0 = 0;

  device->descriptor.bLength = UsbDescriptorSize_Device;
  device->descriptor.bDescriptorType = UsbDescriptorType_Device;
  return 1;

error:
  setUsbError(result, "USB device descriptor read");
  return 0;
}

int
usbAllocateEndpointExtension (UsbEndpoint *endpoint) {
  UsbDeviceExtension *devx = endpoint->device->extension;
  UsbEndpointExtension *eptx;

  if ((eptx = malloc(sizeof(*eptx)))) {
    memset(eptx, 0, sizeof(*eptx));
    eptx->wakeReadFd = -1;
    eptx->wakeWriteFd = -1;
    /* Free any unreaped completions on queue destroy (e.g. arrival after
     * device tear-down, or our own bulk-IN auto-pump request still in
     * flight at endpoint deallocation). Existing consumers free their
     * own dequeued request, so the deallocator only runs on leftovers. */
    if ((eptx->completedRequests = newQueue(usbDeallocateAsyncRequest, NULL))) {
      IOReturn result;
      unsigned char number = USB_ENDPOINT_NUMBER(endpoint->descriptor);
      unsigned char direction = USB_ENDPOINT_DIRECTION(endpoint->descriptor);

      for (eptx->pipeNumber=1; eptx->pipeNumber<=devx->pipeCount; ++eptx->pipeNumber) {
        result = (*devx->interface)->GetPipeProperties(devx->interface, eptx->pipeNumber,
                                                       &eptx->transferDirection, &eptx->endpointNumber,
                                                       &eptx->transferMode, &eptx->packetSize, &eptx->pollInterval);
        if (result == kIOReturnSuccess) {
          if ((eptx->endpointNumber == number) &&
              (((eptx->transferDirection == kUSBIn) && (direction == UsbEndpointDirection_Input)) ||
               ((eptx->transferDirection == kUSBOut) && (direction == UsbEndpointDirection_Output)))) {
            logMessage(LOG_CATEGORY(USB_IO), "ept=%02X -> pip=%d (num=%d dir=%d xfr=%d int=%d pkt=%d)",
                       endpoint->descriptor->bEndpointAddress, eptx->pipeNumber,
                       eptx->endpointNumber, eptx->transferDirection, eptx->transferMode,
                       eptx->pollInterval, eptx->packetSize);

            eptx->endpoint = endpoint;
            endpoint->extension = eptx;

            /* For bulk IN endpoints, install a prepare hook that arms a
             * permanent ReadPipeAsync auto-pump (see comment at
             * usbDarwinPrepareInputEndpoint). Interrupt IN endpoints work
             * fine with the existing ReadPipe path. */
            if (eptx->transferDirection == kUSBIn && eptx->transferMode == kUSBBulk) {
              endpoint->prepare = usbDarwinPrepareInputEndpoint;
            }

            return 1;
          }
        } else {
          setUsbError(result, "USB pipe properties query");
        }
      }

      errno = EIO;
      logMessage(LOG_ERR, "USB pipe not found: ept=%02X",
                 endpoint->descriptor->bEndpointAddress);

      destroyQueue(eptx->completedRequests);
    } else {
      logSystemError("USB completed request queue allocate");
    }

    free(eptx);
  } else {
    logSystemError("USB endpoint extension allocate");
  }

  return 0;
}

void
usbDeallocateEndpointExtension (UsbEndpointExtension *eptx) {
  if (eptx->inputMonitor) {
    asyncCancelRequest(eptx->inputMonitor);
    eptx->inputMonitor = NULL;
  }
  if (eptx->wakeReadFd >= 0) { close(eptx->wakeReadFd); eptx->wakeReadFd = -1; }
  if (eptx->wakeWriteFd >= 0) { close(eptx->wakeWriteFd); eptx->wakeWriteFd = -1; }
  if (eptx->completedRequests) {
    destroyQueue(eptx->completedRequests);
    eptx->completedRequests = NULL;
  }

  free(eptx);
}

void
usbDeallocateDeviceExtension (UsbDeviceExtension *devx) {
  IOReturn result;

  /* If the async-thread is up, remove the IOKit event source from its
   * runloop, then stop and join the thread before releasing the interface. */
  if (devx->runloopSource && devx->asyncRunloop) {
    CFRunLoopRemoveSource(devx->asyncRunloop, devx->runloopSource, kCFRunLoopDefaultMode);
  }
  if (devx->runloopSource) {
    CFRelease(devx->runloopSource);
    devx->runloopSource = NULL;
  }
  usbStopAsyncIoThread(devx);

  unsetInterface(devx);

  if (devx->deviceOpened) {
    result = (*devx->device)->USBDeviceClose(devx->device);
    if (result != kIOReturnSuccess) setUsbError(result, "USB device close");
    devx->deviceOpened = 0;
  }

  // Hand the device back to the kernel before releasing the interface so
  // VoiceOver (or any other userland HID client) can pick it up again.
  releaseCapturedDevice(devx);

  if (devx->device) {
    (*devx->device)->Release(devx->device);
    devx->device = NULL;
  }

  free(devx);
}

UsbDevice *
usbFindDevice (UsbDeviceChooser *chooser, UsbChooseChannelData *data) {
  UsbDevice *device = NULL;
  kern_return_t kernelResult;
  IOReturn ioResult;
  mach_port_t port;

  kernelResult = IOMasterPort(MACH_PORT_NULL, &port);
  if (kernelResult == KERN_SUCCESS) {
    CFMutableDictionaryRef dictionary;

    if ((dictionary = IOServiceMatching(kIOUSBDeviceClassName))) {
      io_iterator_t iterator = 0;

      kernelResult = IOServiceGetMatchingServices(port, dictionary, &iterator);
      dictionary = NULL;

      if ((kernelResult == KERN_SUCCESS) && iterator) {
        io_service_t service;

        while ((service = IOIteratorNext(iterator))) {
          IOCFPlugInInterface **plugin = NULL;
          SInt32 score;

          ioResult = IOCreatePlugInInterfaceForService(service,
                                                       kIOUSBDeviceUserClientTypeID,
                                                       kIOCFPlugInInterfaceID,
                                                       &plugin, &score);
          IOObjectRelease(service);
          service = 0;

          if ((ioResult == kIOReturnSuccess) && plugin) {
            IOUSBDeviceInterface500 **interface = NULL;

            ioResult = (*plugin)->QueryInterface(plugin,
                                                 CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID500),
                                                 (LPVOID)&interface);
            (*plugin)->Release(plugin);
            plugin = NULL;

            if ((ioResult == kIOReturnSuccess) && interface) {
              UsbDeviceExtension *devx;

              if ((devx = malloc(sizeof(*devx)))) {
                devx->device = interface;
                devx->deviceOpened = 0;
                devx->captured = 0;
                devx->locationID = 0;

                devx->interface = NULL;
                devx->interfaceOpened = 0;

                devx->runloopSource = NULL;

                if ((device = usbTestDevice(devx, chooser, data))) break;
                free(devx);
                devx = NULL;
              } else {
                logSystemError("USB device extension allocate");
              }

              (*interface)->Release(interface);
              interface = NULL;
            } else {
              setUsbError(ioResult, "USB device interface create");
            }
          } else {
            setUsbError(ioResult, "USB device service plugin create");
          }
        }

        IOObjectRelease(iterator);
        iterator = 0;
      } else {
        setUsbError(kernelResult, "USB device iterator create");
      }
    } else {
      logMessage(LOG_ERR, "USB device matching dictionary create error.");
    }

    mach_port_deallocate(mach_task_self(), port);
  } else {
    setUsbError(kernelResult, "Darwin master port create");
  }

  return device;
}

void
usbForgetDevices (void) {
}
