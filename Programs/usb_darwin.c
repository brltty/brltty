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
  IOUSBDeviceInterface182 **interface;
} UsbDeviceExtension;

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

int
usbResetDevice (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  IOReturn result = (*devx->interface)->ResetDevice(devx->interface);
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
  UInt8 arg = configuration;
  IOReturn result = (*devx->interface)->SetConfiguration(devx->interface, arg);
  if (result == kIOReturnSuccess) return 1;

  setErrnoIo(result, "USB configuration set");
  return 0;
}

int
usbClaimInterface (
  UsbDevice *device,
  unsigned char interface
) {
  errno = ENOSYS;
  LogError("USB interface claim");
  return 0;
}

int
usbReleaseInterface (
  UsbDevice *device,
  unsigned char interface
) {
  errno = ENOSYS;
  LogError("USB interface release");
  return 0;
}

int
usbSetAlternative (
  UsbDevice *device,
  unsigned char interface,
  unsigned char alternative
) {
  errno = ENOSYS;
  LogError("USB alternative set");
  return 0;
}

int
usbResetEndpoint (
  UsbDevice *device,
  unsigned char endpointAddress
) {
  errno = ENOSYS;
  LogError("USB endpoint reset");
  return 0;
}

int
usbClearEndpoint (
  UsbDevice *device,
  unsigned char endpointAddress
) {
  errno = ENOSYS;
  LogError("USB endpoint clear");
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
  errno = ENOSYS;
  LogError("USB control transfer");
  return -1;
}

int
usbAllocateEndpointExtension (UsbEndpoint *endpoint) {
  return 1;
}

void
usbDeallocateEndpointExtension (UsbEndpoint *endpoint) {
}

int
usbReadEndpoint (
  UsbDevice *device,
  unsigned char endpointNumber,
  void *buffer,
  int length,
  int timeout
) {
  errno = ENOSYS;
  LogError("USB endpoint read");
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
  errno = ENOSYS;
  LogError("USB endpoint write");
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
  LogError("USB request submission");
  return NULL;
}

int
usbCancelRequest (
  UsbDevice *device,
  void *request
) {
  errno = ENOSYS;
  LogError("USB request cancellation");
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
    if ((result = (*devx->interface)->GetDeviceSpeed(devx->interface, &speed)) != kIOReturnSuccess) goto error;
    switch (speed) {
      default                 : device->descriptor.bcdUSB = 0X0000; break;
      case kUSBDeviceSpeedLow : device->descriptor.bcdUSB = 0X0100; break;
      case kUSBDeviceSpeedFull: device->descriptor.bcdUSB = 0X0110; break;
      case kUSBDeviceSpeedHigh: device->descriptor.bcdUSB = 0X0200; break;
    }
  }

  if ((result = (*devx->interface)->GetDeviceClass(devx->interface, &device->descriptor.bDeviceClass)) != kIOReturnSuccess) goto error;
  if ((result = (*devx->interface)->GetDeviceSubClass(devx->interface, &device->descriptor.bDeviceSubClass)) != kIOReturnSuccess) goto error;
  if ((result = (*devx->interface)->GetDeviceProtocol(devx->interface, &device->descriptor.bDeviceProtocol)) != kIOReturnSuccess) goto error;

  if ((result = (*devx->interface)->GetDeviceVendor(devx->interface, &device->descriptor.idVendor)) != kIOReturnSuccess) goto error;
  if ((result = (*devx->interface)->GetDeviceProduct(devx->interface, &device->descriptor.idProduct)) != kIOReturnSuccess) goto error;
  if ((result = (*devx->interface)->GetDeviceReleaseNumber(devx->interface, &device->descriptor.bcdDevice)) != kIOReturnSuccess) goto error;

  if ((result = (*devx->interface)->USBGetManufacturerStringIndex(devx->interface, &device->descriptor.iManufacturer)) != kIOReturnSuccess) goto error;
  if ((result = (*devx->interface)->USBGetProductStringIndex(devx->interface, &device->descriptor.iProduct)) != kIOReturnSuccess) goto error;
  if ((result = (*devx->interface)->USBGetSerialNumberStringIndex(devx->interface, &device->descriptor.iSerialNumber)) != kIOReturnSuccess) goto error;

  if ((result = (*devx->interface)->GetNumberOfConfigurations(devx->interface, &device->descriptor.bNumConfigurations)) != kIOReturnSuccess) goto error;
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
  (*devx->interface)->USBDeviceClose(devx->interface);
  (*devx->interface)->Release(devx->interface);
  free(devx);
}

UsbDevice *
usbFindDevice (UsbDeviceChooser chooser, void *data) {
  UsbDevice *device = NULL;
  kern_return_t kernelResult;
  IOReturn ioResult;
  mach_port_t masterPort;

  kernelResult = IOMasterPort(MACH_PORT_NULL, &masterPort);
  if (!kernelResult) {
    CFMutableDictionaryRef matchingDictionary;

    if ((matchingDictionary = IOServiceMatching(kIOUSBDeviceClassName))) {
      io_iterator_t serviceIterator;

      kernelResult = IOServiceGetMatchingServices(masterPort,
                                                  matchingDictionary,
                                                  &serviceIterator);
      if (!kernelResult) {
        io_service_t service;

        while ((service = IOIteratorNext(serviceIterator))) {
          IOCFPlugInInterface **servicePlugin;
          SInt32 score;

          ioResult = IOCreatePlugInInterfaceForService(service,
                                                       kIOUSBDeviceUserClientTypeID,
                                                       kIOCFPlugInInterfaceID,
                                                       &servicePlugin, &score);
          if (!ioResult && servicePlugin) {
            IOUSBDeviceInterface182 **deviceInterface;

            ioResult = (*servicePlugin)->QueryInterface(servicePlugin,
                                                        CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID182),
                                                        (LPVOID)&deviceInterface);
            if (!ioResult && deviceInterface) {
              if (!(ioResult = (*deviceInterface)->USBDeviceOpen(deviceInterface))) {
                UsbDeviceExtension *devx;
                if ((devx = malloc(sizeof(*devx)))) {
                  devx->interface = deviceInterface;
                  device = usbTestDevice(devx, chooser, data);

                  if (!device) free(devx);
                } else {
                  LogPrint(LOG_ERR, "USB: cannot allocate device extension.");
                }

                if (!device) (*deviceInterface)->USBDeviceClose(deviceInterface);
              } else {
                setErrnoIo(ioResult, "USB: device interface open");
              }

              if (!device) (*deviceInterface)->Release(deviceInterface);
            } else {
              setErrnoIo(ioResult, "USB: device interface create");
            }

            (*servicePlugin)->Release(servicePlugin);
          } else {
            setErrnoIo(ioResult, "USB: service plugin create");
          }

          IOObjectRelease(service);
          if (device) break;
        }

        IOObjectRelease(serviceIterator);
      } else {
        setErrnoKernel(kernelResult, "USB: service iterator create");
      }
    } else {
      LogPrint(LOG_ERR, "USB: cannot create matching dictionary.");
    }

    mach_port_deallocate(mach_task_self(), masterPort);
  } else {
    setErrnoKernel(kernelResult, "USB: master port create");
  }

  return device;
}
