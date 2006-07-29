/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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

#include "prologue.h"

#include <stdio.h>
#include <errno.h>
#include <mach/mach.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

#include "misc.h"
#include "io_usb.h"
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
  unsigned deviceOpened:1;

  IOUSBInterfaceInterface190 **interface;
  unsigned interfaceOpened:1;
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
setUnixError (long int result, const char *action) {
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

  if (!devx->deviceOpened) {
    const char *action = "opened";
    int level = LOG_INFO;

    result = (*devx->device)->USBDeviceOpen(devx->device);
    if (result != kIOReturnSuccess) {
      setUnixError(result, "USB device open");
      if ((result != kIOReturnExclusiveAccess) || !seize) return 0;

      result = (*devx->device)->USBDeviceOpenSeize(devx->device);
      if (result != kIOReturnSuccess) {
        setUnixError(result, "USB device seize");
        return 0;
      }

      action = "seized";
      level = LOG_NOTICE;
    }

    LogPrint(level, "USB device %s: vendor=%04X product=%04X",
             action, device->descriptor.idVendor, device->descriptor.idProduct);
    devx->deviceOpened = 1;
  }

  return 1;
}

static int
unsetInterface (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
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
              setUnixError(result, "USB pipe abort");
            }
          }
        }

        CFRunLoopRemoveSource(devx->runloopReference,
                              devx->runloopSource,
                              devx->runloopMode);
        devx->runloopReference = NULL;
      }

      result = (*devx->interface)->USBInterfaceClose(devx->interface);
      if (result != kIOReturnSuccess) {
        setUnixError(result, "USB interface close");
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
    setUnixError(result, "USB interface number query");
  } else if (num == number) {
    return 1;
  }

  return 0;
}

static int
setInterface (UsbDevice *device, UInt8 number) {
  UsbDeviceExtension *devx = device->extension;
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
            unsetInterface(device);
            devx->interface = interface;
            found = 1;
            break;
          }

          (*interface)->Release(interface);
          interface = NULL;
        } else {
          setUnixError(result, "USB interface interface create");
        }
      } else {
        setUnixError(result, "USB interface service plugin create");
      }
    }
    if (!found) LogPrint(LOG_ERR, "USB interface not found: %d", number);

    IOObjectRelease(iterator);
    iterator = 0;
  } else {
    setUnixError(result, "USB interface iterator create");
  }

  return found;
}

static void
usbAsynchronousRequestCallback (void *context, IOReturn result, void *arg) {
  UsbAsynchronousRequest *request = context;
  UsbEndpoint *endpoint = request->endpoint;
  UsbEndpointExtension *eptx = endpoint->extension;

  request->result = result;
  request->count = (UInt32)arg;

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

  setUnixError(result, "USB device reset");
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
    setUnixError(result, "USB configuration set");
  }

  return 0;
}

int
usbClaimInterface (
  UsbDevice *device,
  unsigned char interface
) {
  UsbDeviceExtension *devx = device->extension;
  IOReturn result;

  if (setInterface(device, interface)) {
    if (devx->interfaceOpened) return 1;

    result = (*devx->interface)->USBInterfaceOpen(devx->interface);
    if (result == kIOReturnSuccess) {
      result = (*devx->interface)->GetNumEndpoints(devx->interface, &devx->pipeCount);
      if (result == kIOReturnSuccess) {
        devx->interfaceOpened = 1;
        return 1;
      } else {
        setUnixError(result, "USB pipe count query");
      }

      (*devx->interface)->USBInterfaceClose(devx->interface);
    } else {
      setUnixError(result, "USB interface open");
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
    IOReturn result;
    UInt8 arg;

    result = (*devx->interface)->GetAlternateSetting(devx->interface, &arg);
    if (result == kIOReturnSuccess) {
      if (arg == alternative) return 1;

      arg = alternative;
      result = (*devx->interface)->SetAlternateInterface(devx->interface, arg);
      if (result == kIOReturnSuccess) return 1;
      setUnixError(result, "USB alternative set");
    } else {
      setUnixError(result, "USB alternative get");
    }
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
    setUnixError(result, "USB endpoint reset");
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
    setUnixError(result, "USB endpoint clear");
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
  setUnixError(result, "USB control transfer");
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
      if (!devx->runloopReference) {
        devx->runloopReference = CFRunLoopGetCurrent();
      }

      if (!devx->runloopMode) {
        devx->runloopMode = kCFRunLoopDefaultMode;
      }

      result = (*devx->interface)->CreateInterfaceAsyncEventSource(devx->interface,
                                                                   &devx->runloopSource);
      if (result != kIOReturnSuccess) {
        setUnixError(result, "USB interface event source create");
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
          setUnixError(result, "USB endpoint asynchronous read");
          break;

        case kUSBOut:
          if (request->buffer) memcpy(request->buffer, buffer, length);
          result = (*devx->interface)->WritePipeAsync(devx->interface, eptx->pipeNumber,
                                                      request->buffer, request->length,
                                                      usbAsynchronousRequestCallback, request);
          if (result == kIOReturnSuccess) return request;
          setUnixError(result, "USB endpoint asynchronous write");
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

    response->context = request->context;
    response->buffer = request->buffer;
    response->size = request->length;

    if (request->result == kIOReturnSuccess) {
      response->error = 0;
      response->count = request->count;

      if (!usbApplyInputFilters(device, response->buffer, response->size, &response->count)) {
        response->error = EIO;
        response->count = -1;
      }
    } else {
      setUnixError(request->result, "USB asynchronous response");
      response->error = errno;
      response->count = -1;
    }

    return request;
  }

none:
  return NULL;
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

          setUnixError(result, "USB stall clear");
          break;
        }

      default:
        setUnixError(result, "USB endpoint read");
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
    setUnixError(result, "USB endpoint write");
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
  setUnixError(result, "USB device descriptor read");
  return 0;
}

int
usbAllocateEndpointExtension (UsbEndpoint *endpoint) {
  UsbDeviceExtension *devx = endpoint->device->extension;
  UsbEndpointExtension *eptx;

  if ((eptx = malloc(sizeof(*eptx)))) {
    if ((eptx->completedRequests = newQueue(NULL, NULL))) {
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
            LogPrint(LOG_DEBUG, "USB: ept=%02X -> pip=%d (num=%d dir=%d xfr=%d int=%d pkt=%d)",
                     endpoint->descriptor->bEndpointAddress, eptx->pipeNumber,
                     eptx->endpointNumber, eptx->transferDirection, eptx->transferMode,
                     eptx->pollInterval, eptx->packetSize);

            eptx->endpoint = endpoint;
            endpoint->extension = eptx;
            return 1;
          }
        } else {
          setUnixError(result, "USB pipe properties query");
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

  if (eptx->completedRequests) {
    deallocateQueue(eptx->completedRequests);
    eptx->completedRequests = NULL;
  }

  free(eptx);
}

void
usbDeallocateDeviceExtension (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  IOReturn result;

  unsetInterface(device);

  if (devx->deviceOpened) {
    result = (*devx->device)->USBDeviceClose(devx->device);
    if (result != kIOReturnSuccess) setUnixError(result, "USB device close");
    devx->deviceOpened = 0;
  }

  (*devx->device)->Release(devx->device);
  devx->device = NULL;

  free(devx);
}

UsbDevice *
usbFindDevice (UsbDeviceChooser chooser, void *data) {
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
            IOUSBDeviceInterface182 **interface = NULL;

            ioResult = (*plugin)->QueryInterface(plugin,
                                                 CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID182),
                                                 (LPVOID)&interface);
            (*plugin)->Release(plugin);
            plugin = NULL;

            if ((ioResult == kIOReturnSuccess) && interface) {
              UsbDeviceExtension *devx;

              if ((devx = malloc(sizeof(*devx)))) {
                devx->device = interface;
                devx->deviceOpened = 0;

                devx->interface = NULL;
                devx->interfaceOpened = 0;

                devx->runloopReference = NULL;
                devx->runloopMode = NULL;
                devx->runloopSource = NULL;

                if ((device = usbTestDevice(devx, chooser, data))) break;
                free(devx);
                devx = NULL;
              } else {
                LogError("USB device extension allocate");
              }

              (*interface)->Release(interface);
              interface = NULL;
            } else {
              setUnixError(ioResult, "USB device interface create");
            }
          } else {
            setUnixError(ioResult, "USB device service plugin create");
          }
        }

        IOObjectRelease(iterator);
        iterator = 0;
      } else {
        setUnixError(kernelResult, "USB device iterator create");
      }
    } else {
      LogPrint(LOG_ERR, "USB device matching dictionary create error.");
    }

    mach_port_deallocate(mach_task_self(), port);
  } else {
    setUnixError(kernelResult, "Darwin master port create");
  }

  return device;
}
