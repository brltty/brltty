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
  IOUSBDeviceInterface182 **device;
  IOUSBInterfaceInterface190 **interface;
  unsigned opened:1;
} UsbDeviceExtension;

typedef struct {
  UInt8 reference;
  UInt8 number;
  UInt8 direction;
  UInt8 transfer;
  UInt8 interval;
  UInt16 size;
} UsbEndpointExtension;

static void
setErrnoIo (IOReturn result, const char *action) {
#define SET(to) errno = E##to; break;
#define MAP(from,to) case kIOReturn##from: SET(to)

  LogPrint(LOG_WARNING, "Darwin I/O error 0X%X.", result);
  switch (result) {
    default: SET(IO)
  //MAP(Success, )
  //MAP(Error, )
    MAP(NoMemory, NOMEM)
    MAP(NoResources, AGAIN)
  //MAP(IPCError, )
    MAP(NoDevice, NODEV)
    MAP(NotPrivileged, ACCES)
    MAP(BadArgument, INVAL)
    MAP(LockedRead, NOLCK)
    MAP(LockedWrite, NOLCK)
    MAP(ExclusiveAccess, BUSY)
  //MAP(BadMessageID, )
    MAP(Unsupported, NOTSUP)
  //MAP(VMError, )
  //MAP(InternalError, )
    MAP(IOError, IO)
    MAP(CannotLock, NOLCK)
    MAP(NotOpen, BADF)
    MAP(NotReadable, ACCES)
    MAP(NotWritable, ROFS)
  //MAP(NotAligned, )
    MAP(BadMedia, NXIO)
  //MAP(StillOpen, )
  //MAP(RLDError, )
    MAP(DMAError, DEVERR)
    MAP(Busy, BUSY)
    MAP(Timeout, TIMEDOUT)
    MAP(Offline, NXIO)
    MAP(NotReady, NXIO)
    MAP(NotAttached, NXIO)
    MAP(NoChannels, DEVERR)
    MAP(NoSpace, NOSPC)
    MAP(PortExists, ADDRINUSE)
    MAP(CannotWire, NOMEM)
  //MAP(NoInterrupt, )
    MAP(NoFrames, DEVERR)
    MAP(MessageTooLarge, MSGSIZE)
    MAP(NotPermitted, PERM)
    MAP(NoPower, PWROFF)
    MAP(NoMedia, NXIO)
    MAP(UnformattedMedia, NXIO)
    MAP(UnsupportedMode, NOSYS)
    MAP(Underrun, DEVERR)
    MAP(Overrun, DEVERR)
    MAP(DeviceError, DEVERR)
  //MAP(NoCompletion, )
    MAP(Aborted, CANCELED)
    MAP(NoBandwidth, DEVERR)
    MAP(NotResponding, DEVERR)
    MAP(IsoTooOld, DEVERR)
    MAP(IsoTooNew, DEVERR)
    MAP(NotFound, NOENT)
  //MAP(Invalid, )
  }
  if (action) LogError(action);

#undef MAP
#undef SET
}

static void
setErrnoKernel (kern_return_t result, const char *action) {
#define SET(to) errno = E##to; break;
#define MAP(from,to) case KERN_##from: SET(to)

  LogPrint(LOG_WARNING, "Darwin kernel error 0X%X.", result);
  switch (result) {
    default: SET(IO)
  //MAP(SUCCESS, )
    MAP(INVALID_ADDRESS, INVAL)
    MAP(PROTECTION_FAILURE, FAULT)
    MAP(NO_SPACE, NOSPC)
    MAP(INVALID_ARGUMENT, INVAL)
  //MAP(FAILURE, )
    MAP(RESOURCE_SHORTAGE, AGAIN)
  //MAP(NOT_RECEIVER, )
    MAP(NO_ACCESS, ACCES)
    MAP(MEMORY_FAILURE, FAULT)
    MAP(MEMORY_ERROR, FAULT)
  //MAP(ALREADY_IN_SET, )
  //MAP(NOT_IN_SET, )
    MAP(NAME_EXISTS, EXIST)
    MAP(ABORTED, CANCELED)
    MAP(INVALID_NAME, INVAL)
    MAP(INVALID_TASK, INVAL)
    MAP(INVALID_RIGHT, INVAL)
    MAP(INVALID_VALUE, INVAL)
  //MAP(UREFS_OVERFLOW, )
    MAP(INVALID_CAPABILITY, INVAL)
  //MAP(RIGHT_EXISTS, )
    MAP(INVALID_HOST, INVAL)
  //MAP(MEMORY_PRESENT, )
  //MAP(MEMORY_DATA_MOVED, )
  //MAP(MEMORY_RESTART_COPY, )
    MAP(INVALID_PROCESSOR_SET, INVAL)
  //MAP(POLICY_LIMIT, )
    MAP(INVALID_POLICY, INVAL)
    MAP(INVALID_OBJECT, INVAL)
  //MAP(ALREADY_WAITING, )
  //MAP(DEFAULT_SET, )
  //MAP(EXCEPTION_PROTECTED, )
    MAP(INVALID_LEDGER, INVAL)
    MAP(INVALID_MEMORY_CONTROL, INVAL)
    MAP(INVALID_SECURITY, INVAL)
  //MAP(NOT_DEPRESSED, )
  //MAP(TERMINATED, )
  //MAP(LOCK_SET_DESTROYED, )
  //MAP(LOCK_UNSTABLE, )
  //MAP(LOCK_OWNED, )
  //MAP(LOCK_OWNED_SELF, )
  //MAP(SEMAPHORE_DESTROYED, )
  //MAP(RPC_SERVER_TERMINATED, )
  //MAP(RPC_TERMINATE_ORPHAN, )
  //MAP(RPC_CONTINUE_ORPHAN, )
    MAP(NOT_SUPPORTED, NOTSUP)
    MAP(NODE_DOWN, HOSTDOWN)
  //MAP(NOT_WAITING, )
    MAP(OPERATION_TIMED_OUT, TIMEDOUT)
  }
  if (action) LogError(action);

#undef MAP
#undef SET
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
      setErrnoIo(result, "USB device open");
      if ((result != kIOReturnExclusiveAccess) || !seize) return 0;

      result = (*devx->device)->USBDeviceOpenSeize(devx->device);
      if (result != kIOReturnSuccess) {
        setErrnoIo(result, "USB device seize");
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
closeInterface (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  int ok = 1;

  if (devx->interface) {
    IOReturn result;

    result = (*devx->interface)->USBInterfaceClose(devx->interface);
    if (result != kIOReturnSuccess) {
      setErrnoIo(result, "USB interface close");
      ok = 0;
    }

    result = (*devx->interface)->Release(devx->interface);
    if (result != kIOReturnSuccess) {
      setErrnoIo(result, "USB interface release");
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
  if (result == kIOReturnSuccess) {
    if (num == number) return 1;
  } else {
    setErrnoIo(result, "USB interface number query");
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
            closeInterface(device);
            devx->interface = interface;
          } else {
            (*interface)->Release(interface);
            interface = NULL;
          }
        } else {
          setErrnoIo(result, "USB interface interface create");
        }

        (*plugin)->Release(plugin);
      } else {
        setErrnoIo(result, "USB interface service plugin create");
      }

      IOObjectRelease(service);
      if (interface) break;
    }
    if (!interface) LogPrint(LOG_ERR, "USB interface not found: %d", number);

    IOObjectRelease(iterator);
  } else {
    setErrnoIo(result, "USB interface iterator create");
  }

  return interface != NULL;
}

int
usbResetDevice (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  IOReturn result = (*devx->device)->ResetDevice(devx->device);
  if (result == kIOReturnSuccess) return 1;

  setErrnoIo(result, "USB device reset");
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
    setErrnoIo(result, "USB configuration set");
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
    IOReturn result = (*devx->interface)->USBInterfaceOpen(devx->interface);
    if (result == kIOReturnSuccess) return 1;
    setErrnoIo(result, "USB interface open");
  }

  return 0;
}

int
usbReleaseInterface (
  UsbDevice *device,
  unsigned char interface
) {
  return setInterface(device, interface) && closeInterface(device);
}

int
usbSetAlternative (
  UsbDevice *device,
  unsigned char interface,
  unsigned char alternative
) {
  UsbDeviceExtension *devx = device->extension;

  if (setInterface(device, interface)) {
    IOReturn result = (*devx->interface)->SetAlternateInterface(devx->interface, alternative);
    if (result == kIOReturnSuccess) return 1;
    setErrnoIo(result, "USB alternative set");
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

    result = (*devx->interface)->ClearPipeStallBothEnds(devx->interface, eptx->reference);
    if (result == kIOReturnSuccess) return 1;
    setErrnoIo(result, "USB endpoint reset");
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

    result = (*devx->interface)->ClearPipeStall(devx->interface, eptx->reference);
    if (result == kIOReturnSuccess) return 1;
    setErrnoIo(result, "USB endpoint clear");
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
  setErrnoIo(result, "USB control transfer");
  return -1;
}

int
usbAllocateEndpointExtension (UsbEndpoint *endpoint) {
  UsbDeviceExtension *devx = endpoint->device->extension;
  UsbEndpointExtension *eptx;

  if ((eptx = malloc(sizeof(*eptx)))) {
    IOReturn result;
    UInt8 count;

    result = (*devx->interface)->GetNumEndpoints(devx->interface, &count);
    if (result == kIOReturnSuccess) {
      unsigned char number = USB_ENDPOINT_NUMBER(endpoint->descriptor);
      unsigned char output = USB_ENDPOINT_DIRECTION(endpoint->descriptor) == USB_ENDPOINT_DIRECTION_OUTPUT;

      for (eptx->reference=1; eptx->reference<=count; ++eptx->reference) {
        result = (*devx->interface)->GetPipeProperties(devx->interface, eptx->reference,
                                                       &eptx->direction, &eptx->number,
                                                       &eptx->transfer, &eptx->size, &eptx->interval);
        if (result == kIOReturnSuccess) {
          if ((eptx->number == number) &&
              ((eptx->direction == kUSBOut) == output)) {
            LogPrint(LOG_DEBUG, "USB: ept=%02X -> ref=%d (num=%d dir=%d xfr=%d int=%d pkt=%d)",
                     endpoint->descriptor->bEndpointAddress, eptx->reference,
                     eptx->number, eptx->direction, eptx->transfer,
                     eptx->interval, eptx->size);

            endpoint->extension = eptx;
            return 1;
          }
        } else {
          setErrnoIo(result, "USB endpoint properties query");
        }
      }

      errno = EIO;
      LogPrint(LOG_ERR, "USB endpoint not found: %02X",
               endpoint->descriptor->bEndpointAddress);
    } else {
      setErrnoIo(result, "USB endpoint count query");
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
    result = (*devx->interface)->ReadPipeTO(devx->interface, eptx->reference,
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
          result = (*devx->interface)->ClearPipeStall(devx->interface, eptx->reference);
          if (result != kIOReturnSuccess) {
            setErrnoIo(result, "USB stall clear");
            return 0;
          }

          stalled = 1;
          goto read;
        }

      default:
        setErrnoIo(result, "USB endpoint read");
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

    result = (*devx->interface)->WritePipeTO(devx->interface, eptx->reference,
                                             (void *)buffer, length,
                                             timeout, timeout);
    if (result == kIOReturnSuccess) return length;
    setErrnoIo(result, "USB endpoint write");
  }

  return -1;
}

void *
usbSubmitRequest (
  UsbDevice *device,
  unsigned char endpointAddress,
  void *buffer,
  int length,
  void *data
) {
  errno = ENOSYS;
  LogError("USB request submit");
  return NULL;
}

int
usbCancelRequest (
  UsbDevice *device,
  void *request
) {
  errno = ENOSYS;
  LogError("USB request cancel");
  return 0;
}

void *
usbReapResponse (
  UsbDevice *device,
  UsbResponse *response,
  int wait
) {
  errno = ENOSYS;
  LogError("USB request reap");
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
  setErrnoIo(result, "USB device descriptor read");
  return 0;
}

void
usbDeallocateDeviceExtension (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  if (devx->opened) {
    closeInterface(device);
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
                devx->opened = 0;
                device = usbTestDevice(devx, chooser, data);

                if (!device) free(devx);
              } else {
                LogError("USB device extension allocate");
              }

              if (!device) (*deviceInterface)->Release(deviceInterface);
            } else {
              setErrnoIo(ioResult, "USB device interface create");
            }

            (*servicePlugin)->Release(servicePlugin);
          } else {
            setErrnoIo(ioResult, "USB device service plugin create");
          }

          IOObjectRelease(service);
          if (device) break;
        }

        IOObjectRelease(serviceIterator);
      } else {
        setErrnoKernel(kernelResult, "USB device iterator create");
      }
    } else {
      LogPrint(LOG_ERR, "USB device matching dictionary create error.");
    }

    mach_port_deallocate(mach_task_self(), masterPort);
  } else {
    setErrnoKernel(kernelResult, "Darwin master port create");
  }

  return device;
}
