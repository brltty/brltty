/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <mach/mach.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

#include "misc.h"
#include "usb.h"
#include "usb_internal.h"

typedef struct {
  UsbEndpoint *endpoint;
  void *context;
  void *buffer;
  int length;

  IOReturn result;
  int count;
} UsbAsynchronousRequest;

typedef struct {
  IOUSBDeviceInterface182 **device;
  unsigned opened:1;

  IOUSBInterfaceInterface190 **interface;
  UInt8 pipeCount;

  CFRunLoopRef runloopReference;
  CFStringRef runloopMode;
  CFRunLoopSourceRef runloopSource;
} UsbDeviceExtension;

typedef struct {
  UsbEndpoint *endpoint;
  Queue *completedRequests;

  UInt8 pipeNumber;
  UInt8 endpointNumber;
  UInt8 transferDirection;
  UInt8 transferMode;
  UInt8 pollInterval;
  UInt16 packetSize;
} UsbEndpointExtension;

static void
setErrno (long int result, const char *action) {
#define SET(to) errno = (to); break;
#define MAP(from,to) case (from): SET(to)
  switch (result) {
    default: SET(EIO)

  //MAP(KERN_SUCCESS, )
    MAP(KERN_INVALID_ADDRESS, EINVAL)
    MAP(KERN_PROTECTION_FAILURE, EFAULT)
    MAP(KERN_NO_SPACE, ENOSPC)
    MAP(KERN_INVALID_ARGUMENT, EINVAL)
  //MAP(KERN_FAILURE, )
    MAP(KERN_RESOURCE_SHORTAGE, EAGAIN)
  //MAP(KERN_NOT_RECEIVER, )
    MAP(KERN_NO_ACCESS, EACCES)
    MAP(KERN_MEMORY_FAILURE, EFAULT)
    MAP(KERN_MEMORY_ERROR, EFAULT)
  //MAP(KERN_ALREADY_IN_SET, )
  //MAP(KERN_NOT_IN_SET, )
    MAP(KERN_NAME_EXISTS, EEXIST)
    MAP(KERN_ABORTED, ECANCELED)
    MAP(KERN_INVALID_NAME, EINVAL)
    MAP(KERN_INVALID_TASK, EINVAL)
    MAP(KERN_INVALID_RIGHT, EINVAL)
    MAP(KERN_INVALID_VALUE, EINVAL)
  //MAP(KERN_UREFS_OVERFLOW, )
    MAP(KERN_INVALID_CAPABILITY, EINVAL)
  //MAP(KERN_RIGHT_EXISTS, )
    MAP(KERN_INVALID_HOST, EINVAL)
  //MAP(KERN_MEMORY_PRESENT, )
  //MAP(KERN_MEMORY_DATA_MOVED, )
  //MAP(KERN_MEMORY_RESTART_COPY, )
    MAP(KERN_INVALID_PROCESSOR_SET, EINVAL)
  //MAP(KERN_POLICY_LIMIT, )
    MAP(KERN_INVALID_POLICY, EINVAL)
    MAP(KERN_INVALID_OBJECT, EINVAL)
  //MAP(KERN_ALREADY_WAITING, )
  //MAP(KERN_DEFAULT_SET, )
  //MAP(KERN_EXCEPTION_PROTECTED, )
    MAP(KERN_INVALID_LEDGER, EINVAL)
    MAP(KERN_INVALID_MEMORY_CONTROL, EINVAL)
    MAP(KERN_INVALID_SECURITY, EINVAL)
  //MAP(KERN_NOT_DEPRESSED, )
  //MAP(KERN_TERMINATED, )
  //MAP(KERN_LOCK_SET_DESTROYED, )
  //MAP(KERN_LOCK_UNSTABLE, )
  //MAP(KERN_LOCK_OWNED, )
  //MAP(KERN_LOCK_OWNED_SELF, )
  //MAP(KERN_SEMAPHORE_DESTROYED, )
  //MAP(KERN_RPC_SERVER_TERMINATED, )
  //MAP(KERN_RPC_TERMINATE_ORPHAN, )
  //MAP(KERN_RPC_CONTINUE_ORPHAN, )
    MAP(KERN_NOT_SUPPORTED, ENOTSUP)
    MAP(KERN_NODE_DOWN, EHOSTDOWN)
  //MAP(KERN_NOT_WAITING, )
    MAP(KERN_OPERATION_TIMED_OUT, ETIMEDOUT)

  //MAP(kIOReturnSuccess, )
  //MAP(kIOReturnError, )
    MAP(kIOReturnNoMemory, ENOMEM)
    MAP(kIOReturnNoResources, EAGAIN)
  //MAP(kIOReturnIPCError, )
    MAP(kIOReturnNoDevice, ENODEV)
    MAP(kIOReturnNotPrivileged, EACCES)
    MAP(kIOReturnBadArgument, EINVAL)
    MAP(kIOReturnLockedRead, ENOLCK)
    MAP(kIOReturnLockedWrite, ENOLCK)
    MAP(kIOReturnExclusiveAccess, EBUSY)
  //MAP(kIOReturnBadMessageID, )
    MAP(kIOReturnUnsupported, ENOTSUP)
  //MAP(kIOReturnVMError, )
  //MAP(kIOReturnInternalError, )
    MAP(kIOReturnIOError, EIO)
    MAP(kIOReturnCannotLock, ENOLCK)
    MAP(kIOReturnNotOpen, EBADF)
    MAP(kIOReturnNotReadable, EACCES)
    MAP(kIOReturnNotWritable, EROFS)
  //MAP(kIOReturnNotAligned, )
    MAP(kIOReturnBadMedia, ENXIO)
  //MAP(kIOReturnStillOpen, )
  //MAP(kIOReturnRLDError, )
    MAP(kIOReturnDMAError, EDEVERR)
    MAP(kIOReturnBusy, EBUSY)
    MAP(kIOReturnTimeout, ETIMEDOUT)
    MAP(kIOReturnOffline, ENXIO)
    MAP(kIOReturnNotReady, ENXIO)
    MAP(kIOReturnNotAttached, ENXIO)
    MAP(kIOReturnNoChannels, EDEVERR)
    MAP(kIOReturnNoSpace, ENOSPC)
    MAP(kIOReturnPortExists, EADDRINUSE)
    MAP(kIOReturnCannotWire, ENOMEM)
  //MAP(kIOReturnNoInterrupt, )
    MAP(kIOReturnNoFrames, EDEVERR)
    MAP(kIOReturnMessageTooLarge, EMSGSIZE)
    MAP(kIOReturnNotPermitted, EPERM)
    MAP(kIOReturnNoPower, EPWROFF)
    MAP(kIOReturnNoMedia, ENXIO)
    MAP(kIOReturnUnformattedMedia, ENXIO)
    MAP(kIOReturnUnsupportedMode, ENOSYS)
    MAP(kIOReturnUnderrun, EDEVERR)
    MAP(kIOReturnOverrun, EDEVERR)
    MAP(kIOReturnDeviceError, EDEVERR)
  //MAP(kIOReturnNoCompletion, )
    MAP(kIOReturnAborted, ECANCELED)
    MAP(kIOReturnNoBandwidth, EDEVERR)
    MAP(kIOReturnNotResponding, EDEVERR)
    MAP(kIOReturnIsoTooOld, EDEVERR)
    MAP(kIOReturnIsoTooNew, EDEVERR)
    MAP(kIOReturnNotFound, ENOENT)
  //MAP(kIOReturnInvalid, )

  //MAP(kIOUSBUnknownPipeErr, )
  //MAP(kIOUSBTooManyPipesErr, )
  //MAP(kIOUSBNoAsyncPortErr, )
  //MAP(kIOUSBNotEnoughPipesErr, )
  //MAP(kIOUSBNotEnoughPowerErr, )
  //MAP(kIOUSBEndpointNotFound, )
  //MAP(kIOUSBConfigNotFound, )
    MAP(kIOUSBTransactionTimeout, ETIMEDOUT)
  //MAP(kIOUSBTransactionReturned, )
  //MAP(kIOUSBPipeStalled, )
  //MAP(kIOUSBInterfaceNotFound, )
  //MAP(kIOUSBLowLatencyBufferNotPreviouslyAllocated, )
  //MAP(kIOUSBLowLatencyFrameListNotPreviouslyAllocated, )
  //MAP(kIOUSBHighSpeedSplitError, )
  //MAP(kIOUSBLinkErr, )
  //MAP(kIOUSBNotSent2Err, )
  //MAP(kIOUSBNotSent1Err, )
  //MAP(kIOUSBBufferUnderrunErr, )
  //MAP(kIOUSBBufferOverrunErr, )
  //MAP(kIOUSBReserved2Err, )
  //MAP(kIOUSBReserved1Err, )
  //MAP(kIOUSBWrongPIDErr, )
  //MAP(kIOUSBPIDCheckErr, )
  //MAP(kIOUSBDataToggleErr, )
  //MAP(kIOUSBBitstufErr, )
  //MAP(kIOUSBCRCErr, )
  }
#undef MAP
#undef SET

  if (action) {
    LogPrint(LOG_WARNING, "Darwin error 0X%lX.", result);
    LogError(action);
  }
}

static int
openDevice (UsbDevice *device, int seize) {
  UsbDeviceExtension *devx = device->extension;
  IOReturn result;

  if (!devx->opened) {
    const char *action = "opened";
    int level = LOG_INFO;

    result = (*devx->device)->USBDeviceOpen(devx->device);
    if (result != kIOReturnSuccess) {
      setErrno(result, "USB device open");
      if ((result != kIOReturnExclusiveAccess) || !seize) return 0;

      result = (*devx->device)->USBDeviceOpenSeize(devx->device);
      if (result != kIOReturnSuccess) {
        setErrno(result, "USB device seize");
        return 0;
      }

      action = "seized";
      level = LOG_NOTICE;
    }

    LogPrint(level, "USB device %s: vendor=%04X product=%4X",
             action, device->descriptor.idVendor, device->descriptor.idProduct);
    devx->opened = 1;
  }

  return 1;
}

static int
unsetInterface (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  int ok = 1;

  if (devx->interface) {
    IOReturn result;

    {
      int pipe;
      for (pipe=1; pipe<=devx->pipeCount; ++pipe) {
        result = (*devx->interface)->AbortPipe(devx->interface, pipe);
        if (result != kIOReturnSuccess) {
          setErrno(result, "USB pipe abort");
        }
      }
    }

    result = (*devx->interface)->USBInterfaceClose(devx->interface);
    if (result != kIOReturnSuccess) {
      setErrno(result, "USB interface close");
      ok = 0;
    }

    result = (*devx->interface)->Release(devx->interface);
    if (result != kIOReturnSuccess) {
      setErrno(result, "USB interface release");
      ok = 0;
    }

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
    setErrno(result, "USB interface number query");
  } else if (num == number) {
    return 1;
  }

  return 0;
}

static int
setInterface (UsbDevice *device, UInt8 number) {
  UsbDeviceExtension *devx = device->extension;
  IOUSBFindInterfaceRequest request;
  io_iterator_t iterator;
  IOUSBInterfaceInterface190 **interface = NULL;
  IOReturn result;

  if (devx->interface)
    if (isInterface(devx->interface, number))
      return 1;

  request.bInterfaceClass = kIOUSBFindInterfaceDontCare;
  request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
  request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
  request.bAlternateSetting = kIOUSBFindInterfaceDontCare;

  result = (*devx->device)->CreateInterfaceIterator(devx->device, &request, &iterator);
  if (result == kIOReturnSuccess) {
    io_service_t service;

    while ((service = IOIteratorNext(iterator))) {
      IOCFPlugInInterface **plugin;
      SInt32 score;

      result = IOCreatePlugInInterfaceForService(service,
                                                 kIOUSBInterfaceUserClientTypeID,
                                                 kIOCFPlugInInterfaceID,
                                                 &plugin, &score);
      if ((result == kIOReturnSuccess) && plugin) {
        result = (*plugin)->QueryInterface(plugin,
                                           CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID190),
                                           (LPVOID)&interface);
        if ((result == kIOReturnSuccess) && interface) {
          if (isInterface(interface, number)) {
            unsetInterface(device);
            devx->interface = interface;
          } else {
            (*interface)->Release(interface);
            interface = NULL;
          }
        } else {
          setErrno(result, "USB interface interface create");
        }

        (*plugin)->Release(plugin);
      } else {
        setErrno(result, "USB interface service plugin create");
      }

      IOObjectRelease(service);
      if (interface) break;
    }
    if (!interface) LogPrint(LOG_ERR, "USB interface not found: %d", number);

    IOObjectRelease(iterator);
  } else {
    setErrno(result, "USB interface iterator create");
  }

  return interface != NULL;
}

static void
usbDeallocateAsynchronousRequest (void *item, void *data) {
  UsbAsynchronousRequest *request = item;
  free(request);
}

static void
usbAsynchronousRequestCallback (void *context, IOReturn result, void *arg) {
  UsbAsynchronousRequest *request = context;
  UsbEndpoint *endpoint = request->endpoint;
  UsbEndpointExtension *eptx = endpoint->extension;

  request->result = result;
  request->count = (int)arg;

  if (!enqueueItem(eptx->completedRequests, request)) {
    LogError("USB completed request enqueue");
    free(request);
  }
}

int
usbResetDevice (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  IOReturn result = (*devx->device)->ResetDevice(devx->device);
  if (result == kIOReturnSuccess) return 1;

  setErrno(result, "USB device reset");
  return 0;
}

int
usbSetConfiguration (
  UsbDevice *device,
  unsigned char configuration
) {
  UsbDeviceExtension *devx = device->extension;

  if (openDevice(device, 1)) {
    UInt8 arg = configuration;
    IOReturn result = (*devx->device)->SetConfiguration(devx->device, arg);
    if (result == kIOReturnSuccess) return 1;
    setErrno(result, "USB configuration set");
  }

  return 0;
}

int
usbClaimInterface (
  UsbDevice *device,
  unsigned char interface
) {
  UsbDeviceExtension *devx = device->extension;

  if (setInterface(device, interface)) {
    IOReturn result;

    result = (*devx->interface)->USBInterfaceOpen(devx->interface);
    if (result == kIOReturnSuccess) {
      result = (*devx->interface)->GetNumEndpoints(devx->interface, &devx->pipeCount);
      if (result == kIOReturnSuccess) {
        return 1;
      } else {
        setErrno(result, "USB pipe count query");
      }

      (*devx->interface)->USBInterfaceClose(devx->interface);
    } else {
      setErrno(result, "USB interface open");
    }
  }

  return 0;
}

int
usbReleaseInterface (
  UsbDevice *device,
  unsigned char interface
) {
  return setInterface(device, interface) && unsetInterface(device);
}

int
usbSetAlternative (
  UsbDevice *device,
  unsigned char interface,
  unsigned char alternative
) {
  UsbDeviceExtension *devx = device->extension;

  if (setInterface(device, interface)) {
    UInt8 arg = alternative;
    IOReturn result = (*devx->interface)->SetAlternateInterface(devx->interface, arg);
    if (result == kIOReturnSuccess) return 1;
    setErrno(result, "USB alternative set");
  }

  return 0;
}

int
usbResetEndpoint (
  UsbDevice *device,
  unsigned char endpointAddress
) {
  UsbDeviceExtension *devx = device->extension;
  UsbEndpoint *endpoint;

  if ((endpoint = usbGetEndpoint(device, endpointAddress))) {
    UsbEndpointExtension *eptx = endpoint->extension;
    IOReturn result;

    result = (*devx->interface)->ClearPipeStallBothEnds(devx->interface, eptx->pipeNumber);
    if (result == kIOReturnSuccess) return 1;
    setErrno(result, "USB endpoint reset");
  }

  return 0;
}

int
usbClearEndpoint (
  UsbDevice *device,
  unsigned char endpointAddress
) {
  UsbDeviceExtension *devx = device->extension;
  UsbEndpoint *endpoint;

  if ((endpoint = usbGetEndpoint(device, endpointAddress))) {
    UsbEndpointExtension *eptx = endpoint->extension;
    IOReturn result;

    result = (*devx->interface)->ClearPipeStall(devx->interface, eptx->pipeNumber);
    if (result == kIOReturnSuccess) return 1;
    setErrno(result, "USB endpoint clear");
  }

  return 0;
}

int
usbControlTransfer (
  UsbDevice *device,
  unsigned char direction,
  unsigned char recipient,
  unsigned char type,
  unsigned char request,
  unsigned short value,
  unsigned short index,
  void *buffer,
  int length,
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
  setErrno(result, "USB control transfer");
  return -1;
}

int
usbAllocateEndpointExtension (UsbEndpoint *endpoint) {
  UsbDeviceExtension *devx = endpoint->device->extension;
  UsbEndpointExtension *eptx;

  if ((eptx = malloc(sizeof(*eptx)))) {
    if ((eptx->completedRequests = newQueue(usbDeallocateAsynchronousRequest, NULL))) {
      IOReturn result;
      unsigned char number = USB_ENDPOINT_NUMBER(endpoint->descriptor);
      unsigned char output = USB_ENDPOINT_DIRECTION(endpoint->descriptor) == USB_ENDPOINT_DIRECTION_OUTPUT;

      for (eptx->pipeNumber=1; eptx->pipeNumber<=devx->pipeCount; ++eptx->pipeNumber) {
        result = (*devx->interface)->GetPipeProperties(devx->interface, eptx->pipeNumber,
                                                       &eptx->transferDirection, &eptx->endpointNumber,
                                                       &eptx->transferMode, &eptx->packetSize, &eptx->pollInterval);
        if (result == kIOReturnSuccess) {
          if ((eptx->endpointNumber == number) &&
              ((eptx->transferDirection == kUSBOut) == output)) {
            LogPrint(LOG_DEBUG, "USB: ept=%02X -> pip=%d (num=%d dir=%d xfr=%d int=%d pkt=%d)",
                     endpoint->descriptor->bEndpointAddress, eptx->pipeNumber,
                     eptx->endpointNumber, eptx->transferDirection, eptx->transferMode,
                     eptx->pollInterval, eptx->packetSize);

            eptx->endpoint = endpoint;
            endpoint->extension = eptx;
            return 1;
          }
        } else {
          setErrno(result, "USB pipe properties query");
        }
      }

      errno = EIO;
      LogPrint(LOG_ERR, "USB pipe not found: ept=%02X",
               endpoint->descriptor->bEndpointAddress);

      deallocateQueue(eptx->completedRequests);
    } else {
      LogError("USB completed request queue allocate");
    }

    free(eptx);
  } else {
    LogError("USB endpoint extension allocate");
  }

  return 0;
}

void
usbDeallocateEndpointExtension (UsbEndpoint *endpoint) {
  UsbEndpointExtension *eptx = endpoint->extension;
  deallocateQueue(eptx->completedRequests);
  endpoint->extension = NULL;
  free(eptx);
}

int
usbReadEndpoint (
  UsbDevice *device,
  unsigned char endpointNumber,
  void *buffer,
  int length,
  int timeout
) {
  UsbDeviceExtension *devx = device->extension;
  UsbEndpoint *endpoint;

  if ((endpoint = usbGetInputEndpoint(device, endpointNumber))) {
    UsbEndpointExtension *eptx = endpoint->extension;
    IOReturn result;
    UInt32 count;
    int stalled = 0;

  read:
    count = length;
    result = (*devx->interface)->ReadPipeTO(devx->interface, eptx->pipeNumber,
                                            buffer, &count,
                                            timeout, timeout);

    switch (result) {
      case kIOReturnSuccess:
        length = count;
        if (usbApplyInputFilters(device, buffer, length, &length)) return length;
        errno = EIO;
        break;

      case kIOUSBTransactionTimeout:
        errno = EAGAIN;
        break;

      case kIOUSBPipeStalled:
        if (!stalled) {
          result = (*devx->interface)->ClearPipeStall(devx->interface, eptx->pipeNumber);
          if (result == kIOReturnSuccess) {
            stalled = 1;
            goto read;
          }

          setErrno(result, "USB stall clear");
          break;
        }

      default:
        setErrno(result, "USB endpoint read");
        break;
    }
  }

  return -1;
}

int
usbWriteEndpoint (
  UsbDevice *device,
  unsigned char endpointNumber,
  const void *buffer,
  int length,
  int timeout
) {
  UsbDeviceExtension *devx = device->extension;
  UsbEndpoint *endpoint;

  if ((endpoint = usbGetOutputEndpoint(device, endpointNumber))) {
    UsbEndpointExtension *eptx = endpoint->extension;
    IOReturn result;

    result = (*devx->interface)->WritePipeTO(devx->interface, eptx->pipeNumber,
                                             (void *)buffer, length,
                                             timeout, timeout);
    if (result == kIOReturnSuccess) return length;
    setErrno(result, "USB endpoint write");
  }

  return -1;
}

void *
usbSubmitRequest (
  UsbDevice *device,
  unsigned char endpointAddress,
  void *buffer,
  int length,
  void *context
) {
  UsbDeviceExtension *devx = device->extension;
  UsbEndpoint *endpoint;

  if ((endpoint = usbGetEndpoint(device, endpointAddress))) {
    UsbEndpointExtension *eptx = endpoint->extension;
    IOReturn result;
    UsbAsynchronousRequest *request;

    if (!devx->runloopSource) {
      devx->runloopReference = CFRunLoopGetCurrent();
      devx->runloopMode = kCFRunLoopDefaultMode;

      result = (*devx->interface)->CreateInterfaceAsyncEventSource(devx->interface,
                                                                   &devx->runloopSource);
      if (result != kIOReturnSuccess) {
        setErrno(result, "USB interface event source create");
        return NULL;
      }
      CFRunLoopAddSource(devx->runloopReference,
                         devx->runloopSource,
                         devx->runloopMode);
    }

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
          setErrno(result, "USB endpoint asynchronous read");
          break;

        case kUSBOut:
          if (request->buffer) memcpy(request->buffer, buffer, length);
          result = (*devx->interface)->WritePipeAsync(devx->interface, eptx->pipeNumber,
                                                      request->buffer, request->length,
                                                      usbAsynchronousRequestCallback, request);
          if (result == kIOReturnSuccess) return request;
          setErrno(result, "USB endpoint asynchronous write");
          break;

        default:
          LogPrint(LOG_ERR, "USB endpoint direction not suppported: %d",
                   eptx->transferDirection);
          errno = ENOSYS;
          break;
      }

      free(request);
    } else {
      LogError("USB asynchronous request allocate");
    }
  }

  return NULL;
}

int
usbCancelRequest (
  UsbDevice *device,
  void *request
) {
  errno = ENOSYS;
  return 0;
}

void *
usbReapResponse (
  UsbDevice *device,
  unsigned char endpointAddress,
  UsbResponse *response,
  int wait
) {
  UsbDeviceExtension *devx = device->extension;
  UsbEndpoint *endpoint;

  if ((endpoint = usbGetEndpoint(device, endpointAddress))) {
    UsbEndpointExtension *eptx = endpoint->extension;
    UsbAsynchronousRequest *request;

    while (!(request = dequeueItem(eptx->completedRequests))) {
      switch (CFRunLoopRunInMode(devx->runloopMode, (wait? 60: 0), 1)) {
        case kCFRunLoopRunTimedOut:
          if (wait) continue;
        case kCFRunLoopRunFinished:
          errno = EAGAIN;
          goto none;

        case kCFRunLoopRunStopped:
        case kCFRunLoopRunHandledSource:
        default:
          continue;
      }
    }

    if (request->result == kIOReturnSuccess) {
      int length = request->count;
      if (usbApplyInputFilters(device, request->buffer, request->length, &length)) {
        response->buffer = request->buffer;
        response->length = length;
        response->context = request->context;
        return request;
      }
    } else {
      setErrno(request->result, "USB asynchronous response");
    }

    free(request);
  }

none:
  return NULL;
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
      case kUSBDeviceSpeedLow : device->descriptor.bcdUSB = 0X0100; break;
      case kUSBDeviceSpeedFull: device->descriptor.bcdUSB = 0X0110; break;
      case kUSBDeviceSpeedHigh: device->descriptor.bcdUSB = 0X0200; break;
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

  device->descriptor.bLength = USB_DESCRIPTOR_SIZE_DEVICE;
  device->descriptor.bDescriptorType = USB_DESCRIPTOR_TYPE_DEVICE;
  return 1;

error:
  setErrno(result, "USB device descriptor read");
  return 0;
}

void
usbDeallocateDeviceExtension (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;

  if (devx->runloopSource) {
    CFRunLoopRemoveSource(devx->runloopReference,
                          devx->runloopSource,
                          devx->runloopMode);
  }

  if (devx->opened) {
    unsetInterface(device);
    (*devx->device)->USBDeviceClose(devx->device);
  }

  (*devx->device)->Release(devx->device);
  free(devx);
}

UsbDevice *
usbFindDevice (UsbDeviceChooser chooser, void *data) {
  UsbDevice *device = NULL;
  kern_return_t kernelResult;
  IOReturn ioResult;
  mach_port_t masterPort;

  kernelResult = IOMasterPort(MACH_PORT_NULL, &masterPort);
  if (kernelResult == KERN_SUCCESS) {
    CFMutableDictionaryRef matchingDictionary;

    if ((matchingDictionary = IOServiceMatching(kIOUSBDeviceClassName))) {
      io_iterator_t serviceIterator;

      kernelResult = IOServiceGetMatchingServices(masterPort,
                                                  matchingDictionary,
                                                  &serviceIterator);
      matchingDictionary = NULL;
      if (kernelResult == KERN_SUCCESS) {
        io_service_t service;

        while ((service = IOIteratorNext(serviceIterator))) {
          IOCFPlugInInterface **servicePlugin;
          SInt32 score;

          ioResult = IOCreatePlugInInterfaceForService(service,
                                                       kIOUSBDeviceUserClientTypeID,
                                                       kIOCFPlugInInterfaceID,
                                                       &servicePlugin, &score);
          if ((ioResult == kIOReturnSuccess) && servicePlugin) {
            IOUSBDeviceInterface182 **deviceInterface;

            ioResult = (*servicePlugin)->QueryInterface(servicePlugin,
                                                        CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID182),
                                                        (LPVOID)&deviceInterface);
            if ((ioResult == kIOReturnSuccess) && deviceInterface) {
              UsbDeviceExtension *devx;
              if ((devx = malloc(sizeof(*devx)))) {
                devx->device = deviceInterface;
                devx->interface = NULL;
                devx->runloopReference = NULL;
                devx->runloopMode = NULL;
                devx->runloopSource = NULL;
                devx->opened = 0;
                device = usbTestDevice(devx, chooser, data);

                if (!device) free(devx);
              } else {
                LogError("USB device extension allocate");
              }

              if (!device) (*deviceInterface)->Release(deviceInterface);
            } else {
              setErrno(ioResult, "USB device interface create");
            }

            (*servicePlugin)->Release(servicePlugin);
          } else {
            setErrno(ioResult, "USB device service plugin create");
          }

          IOObjectRelease(service);
          if (device) break;
        }

        IOObjectRelease(serviceIterator);
      } else {
        setErrno(kernelResult, "USB device iterator create");
      }
    } else {
      LogPrint(LOG_ERR, "USB device matching dictionary create error.");
    }

    mach_port_deallocate(mach_task_self(), masterPort);
  } else {
    setErrno(kernelResult, "Darwin master port create");
  }

  return device;
}
