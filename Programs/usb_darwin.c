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

int
usbResetDevice (UsbDevice *device) {
  errno = ENOSYS;
  LogError("USB device reset");
  return 0;
}

int
usbSetConfiguration (
  UsbDevice *device,
  unsigned char configuration
) {
  errno = ENOSYS;
  LogError("USB configuration set");
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
  errno = ENOSYS;
  LogError("USB device descriptor read");
  return 0;
}

void
usbDeallocateDeviceExtension (UsbDevice *device) {
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
            IOUSBDeviceInterface **deviceInterface;

            ioResult = (*servicePlugin)->QueryInterface(servicePlugin,
                                                        CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID),
                                                        (LPVOID)&deviceInterface);
            if (!ioResult && deviceInterface) {
              if (!(ioResult = (*deviceInterface)->USBDeviceOpen(deviceInterface))) {
                LogPrint(LOG_NOTICE, "intf=%p", (void*)deviceInterface);

                (*deviceInterface)->USBDeviceClose(deviceInterface);
              } else {
                LogPrint(LOG_ERR, "USB: cannot open device interface: err=%d", ioResult);
              }

              (*deviceInterface)->Release(deviceInterface);
            } else {
              LogPrint(LOG_ERR, "USB: cannot create device interface: err=%d", ioResult);
            }

            (*servicePlugin)->Release(servicePlugin);
          } else {
            LogPrint(LOG_ERR, "USB: cannot create service plugin: err=%d", ioResult);
          }

          IOObjectRelease(service);
        }

        IOObjectRelease(serviceIterator);
      } else {
        LogPrint(LOG_ERR, "USB: cannot create service iterator: err=%d", kernelResult);
      }
    } else {
      LogPrint(LOG_ERR, "USB: cannot create matching dictionary.");
    }

    mach_port_deallocate(mach_task_self(), masterPort);
  } else {
    LogPrint(LOG_ERR, "USB: cannot create master port: err=%d", kernelResult);
  }

  return device;
}
