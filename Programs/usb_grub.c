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
#include <errno.h>

#include "log.h"
#include "io_usb.h"
#include "usb_internal.h"

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
  errno = ENOSYS;
  logSystemError("USB device descriptor read");
  return 0;
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
}

UsbDevice *
usbFindDevice (UsbDeviceChooser chooser, void *data) {
  return NULL;
}

void
usbForgetDevices (void) {
}
