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
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <mntent.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/ioctl.h>
#include <linux/compiler.h>
#include <linux/usbdevice_fs.h>

#ifndef USBDEVFS_DISCONNECT
#define USBDEVFS_DISCONNECT _IO('U', 22)
#endif /* USBDEVFS_DISCONNECT */

#ifndef USBDEVFS_CONNECT
#define USBDEVFS_CONNECT _IO('U', 23)
#endif /* USBDEVFS_CONNECT */

#include "misc.h"
#include "usb.h"
#include "usb_internal.h"

typedef struct {
  char *path;
  int file;
} UsbDeviceExtension;

int
usbResetDevice (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  if (ioctl(devx->file, USBDEVFS_RESET, NULL) != -1) return 1;
  LogError("USB device reset");
  return 0;
}

int
usbSetConfiguration (
  UsbDevice *device,
  unsigned char configuration
) {
  UsbDeviceExtension *devx = device->extension;
  unsigned int arg = configuration;
  if (ioctl(devx->file, USBDEVFS_SETCONFIGURATION, &arg) != -1) return 1;
  LogError("USB configuration set");
  return 0;
}

static char *
usbGetDriver (
  UsbDevice *device,
  unsigned char interface
) {
  UsbDeviceExtension *devx = device->extension;
  struct usbdevfs_getdriver arg;
  memset(&arg, 0, sizeof(arg));
  arg.interface = interface;
  if (ioctl(devx->file, USBDEVFS_GETDRIVER, &arg) == -1) return NULL;
  return strdup(arg.driver);
}

static int
usbControlDriver (
  UsbDevice *device,
  unsigned char interface,
  int code,
  void *data
) {
  UsbDeviceExtension *devx = device->extension;
  struct usbdevfs_ioctl arg;
  memset(&arg, 0, sizeof(arg));
  arg.ifno = interface;
  arg.ioctl_code = code;
  arg.data = data;
  if (ioctl(devx->file, USBDEVFS_IOCTL, &arg) != -1) return 1;
  LogError("USB driver control");
  return 0;
}

static int
usbDisconnectDriver (
  UsbDevice *device,
  unsigned char interface
) {
#ifdef USBDEVFS_DISCONNECT
  if (usbControlDriver(device, interface, USBDEVFS_DISCONNECT, NULL)) return 1;
#else /* USBDEVFS_DISCONNECT */
  LogPrint(LOG_WARNING, "USB driver disconnection not available.");
#endif /* USBDEVFS_DISCONNECT */
  return 0;
}

int
usbClaimInterface (
  UsbDevice *device,
  unsigned char interface
) {
  UsbDeviceExtension *devx = device->extension;
  unsigned int arg = interface;
  int disconnected = 0;
claim:
  if (ioctl(devx->file, USBDEVFS_CLAIMINTERFACE, &arg) != -1) return 1;

  if (errno == EBUSY) {
    char *driver = usbGetDriver(device, interface);
    if (driver) {
      LogPrint(LOG_WARNING, "USB interface in use: %s", driver);
      free(driver);

      if (!disconnected) {
        disconnected = 1;
        if (usbDisconnectDriver(device, interface)) goto claim;
      }
    }
    errno = EBUSY;
  }

  LogError("USB interface claim");
  return 0;
}

int
usbReleaseInterface (
  UsbDevice *device,
  unsigned char interface
) {
  UsbDeviceExtension *devx = device->extension;
  unsigned int arg = interface;
  if (ioctl(devx->file, USBDEVFS_RELEASEINTERFACE, &arg) != -1) return 1;
  LogError("USB interface release");
  return 0;
}

int
usbSetAlternative (
  UsbDevice *device,
  unsigned char interface,
  unsigned char alternative
) {
  UsbDeviceExtension *devx = device->extension;
  struct usbdevfs_setinterface arg;
  memset(&arg, 0, sizeof(arg));
  arg.interface = interface;
  arg.altsetting = alternative;
  if (ioctl(devx->file, USBDEVFS_SETINTERFACE, &arg) != -1) return 1;
  LogError("USB alternative set");
  return 0;
}

int
usbResetEndpoint (
  UsbDevice *device,
  unsigned char endpointAddress
) {
  UsbDeviceExtension *devx = device->extension;
  unsigned int arg = endpointAddress;
  if (ioctl(devx->file, USBDEVFS_RESETEP, &arg) != -1) return 1;
  LogError("USB endpoint reset");
  return 0;
}

int
usbClearEndpoint (
  UsbDevice *device,
  unsigned char endpointAddress
) {
  UsbDeviceExtension *devx = device->extension;
  unsigned int arg = endpointAddress;
  if (ioctl(devx->file, USBDEVFS_CLEAR_HALT, &arg) != -1) return 1;
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
  UsbDeviceExtension *devx = device->extension;
  union {
    struct usbdevfs_ctrltransfer transfer;
    UsbSetupPacket setup;
  } arg;
  memset(&arg, 0, sizeof(arg));
  arg.setup.bRequestType = direction | recipient | type;
  arg.setup.bRequest = request;
  putLittleEndian(&arg.setup.wValue, value);
  putLittleEndian(&arg.setup.wIndex, index);
  putLittleEndian(&arg.setup.wLength, length);
  arg.transfer.data = buffer;
  arg.transfer.timeout = timeout;

  {
    int count = ioctl(devx->file, USBDEVFS_CONTROL, &arg);
    if (count == -1) LogError("USB control transfer");
    return count;
  }
}

int
usbAllocateEndpointExtension (UsbEndpoint *endpoint) {
  return 1;
}

void
usbDeallocateEndpointExtension (UsbEndpoint *endpoint) {
}

static int
usbBulkTransfer (
  UsbEndpoint *endpoint,
  void *buffer,
  int length,
  int timeout
) {
  UsbDeviceExtension *devx = endpoint->device->extension;
  struct usbdevfs_bulktransfer arg;
  memset(&arg, 0, sizeof(arg));
  arg.ep = endpoint->descriptor->bEndpointAddress;
  arg.data = buffer;
  arg.len = length;
  arg.timeout = timeout;

  {
    int count = ioctl(devx->file, USBDEVFS_BULK, &arg);
    if (count != -1) return count;
    LogError("USB bulk transfer");
  }
  return -1;
}

int
usbReadEndpoint (
  UsbDevice *device,
  unsigned char endpointNumber,
  void *buffer,
  int length,
  int timeout
) {
  int count = -1;
  UsbEndpoint *endpoint = usbGetInputEndpoint(device, endpointNumber);
  if (endpoint) {
    switch (USB_ENDPOINT_TRANSFER(endpoint->descriptor)) {
      case UsbEndpointTransfer_Bulk:
        count = usbBulkTransfer(endpoint, buffer, length, timeout);
        break;

      default:
        errno = EIO;
        break;
    }

    if (count != -1) {
      if (!usbApplyInputFilters(device, buffer, length, &count)) {
        errno = EIO;
        count = -1;
      }
    }
  }
  return count;
}

int
usbWriteEndpoint (
  UsbDevice *device,
  unsigned char endpointNumber,
  const void *buffer,
  int length,
  int timeout
) {
  UsbEndpoint *endpoint = usbGetOutputEndpoint(device, endpointNumber);
  if (endpoint) {
    switch (USB_ENDPOINT_TRANSFER(endpoint->descriptor)) {
      case UsbEndpointTransfer_Bulk:
        return usbBulkTransfer(endpoint, (void *)buffer, length, timeout);

      default:
        errno = EINVAL;
        break;
    }
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
  UsbEndpoint *endpoint = usbGetEndpoint(device, endpointAddress);
  if (endpoint) {
    struct usbdevfs_urb *urb;
    if ((urb = malloc(sizeof(*urb) + length))) {
      memset(urb, 0, sizeof(*urb));
      urb->endpoint = endpointAddress;
      switch (USB_ENDPOINT_TRANSFER(endpoint->descriptor)) {
        case UsbEndpointTransfer_Control:
          urb->type = USBDEVFS_URB_TYPE_CONTROL;
          break;
        case UsbEndpointTransfer_Isochronous:
          urb->type = USBDEVFS_URB_TYPE_ISO;
          break;
        case UsbEndpointTransfer_Interrupt:
        case UsbEndpointTransfer_Bulk:
          urb->type = USBDEVFS_URB_TYPE_BULK;
          break;
      }
      urb->buffer = (urb->buffer_length = length)? (urb + 1): NULL;
      if (buffer)
        if (USB_ENDPOINT_DIRECTION(endpoint->descriptor) == UsbEndpointDirection_Output)
          memcpy(urb->buffer, buffer, length);
      urb->flags = 0;
      urb->signr = 0;
      urb->usercontext = context;

    /*
      LogPrint(LOG_DEBUG, "USB submit: urb=%p typ=%02X ept=%02X flg=%X sig=%d buf=%p len=%d ctx=%p",
               urb, urb->type, urb->endpoint, urb->flags, urb->signr,
               urb->buffer, urb->buffer_length, urb->usercontext);
    */
    submit:
      if (ioctl(devx->file, USBDEVFS_SUBMITURB, urb) != -1) return urb;
      if ((errno == EINVAL) &&
          (USB_ENDPOINT_TRANSFER(endpoint->descriptor) == UsbEndpointTransfer_Interrupt) &&
          (urb->type == USBDEVFS_URB_TYPE_BULK)) {
        urb->type = USBDEVFS_URB_TYPE_INTERRUPT;
        goto submit;
      }

      /* UHCI support returns ENXIO if a URB is already submitted. */
      if (errno != ENXIO) LogError("USB URB submit");

      free(urb);
    } else {
      LogError("USB URB allocation");
    }
  }
  return NULL;
}

int
usbCancelRequest (
  UsbDevice *device,
  void *request
) {
  UsbDeviceExtension *devx = device->extension;
  if (ioctl(devx->file, USBDEVFS_DISCARDURB, request) == -1) {
    /* EINVAL is returned if the URB is already complete. */
    if (errno != EINVAL) LogError("USB URB discard");
    return 0;
  }
  free(request);
  return 1;
}

void *
usbReapResponse (
  UsbDevice *device,
  unsigned char endpointAddress,
  UsbResponse *response,
  int wait
) {
  UsbDeviceExtension *devx = device->extension;
  struct usbdevfs_urb *urb;

  response->buffer = NULL;
  response->length = 0;
  response->context = NULL;

  if (ioctl(devx->file,
            wait? USBDEVFS_REAPURB: USBDEVFS_REAPURBNDELAY,
            &urb) == -1) {
    if (wait || (errno != EAGAIN)) LogError("USB URB reap");
    urb = NULL;
  } else if (!urb) {
    errno = EAGAIN;
  } else if (urb->status) {
    if ((errno = urb->status) < 0) errno = -errno;
    LogError("USB URB status");
  } else {
    int length = urb->actual_length;
    if (usbApplyInputFilters(device, urb->buffer, urb->buffer_length, &length)) {
      response->buffer = urb->buffer;
      response->length = length;
      response->context = urb->usercontext;
      return urb;
    }

    errno = EIO;
  }

  if (urb) free(urb);
  return NULL;
}

int
usbReadDeviceDescriptor (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  int count = read(devx->file, &device->descriptor, UsbDescriptorSize_Device);
  if (count == -1) {
    LogError("USB device descriptor read");
  } else if (count != UsbDescriptorSize_Device) {
    LogPrint(LOG_ERR, "USB short device descriptor (%d).", count);
  } else {
    return 1;
  }
  return 0;
}

void
usbDeallocateDeviceExtension (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  close(devx->file);
  free(devx->path);
  free(devx);
}

static UsbDevice *
usbSearchDevice (const char *root, UsbDeviceChooser chooser, void *data) {
  size_t rootLength = strlen(root);
  UsbDevice *device = NULL;
  DIR *directory;
  if ((directory = opendir(root))) {
    struct dirent *entry;
    while ((entry = readdir(directory))) {
      size_t nameLength = strlen(entry->d_name);
      struct stat status;
      char path[rootLength + 1 + nameLength + 1];
      if (nameLength != 3) continue;
      if (!isdigit(entry->d_name[0]) ||
          !isdigit(entry->d_name[1]) ||
          !isdigit(entry->d_name[2])) continue;
      snprintf(path, sizeof(path), "%s/%s", root, entry->d_name);
      if (stat(path, &status) == -1) continue;
      if (S_ISDIR(status.st_mode)) {
        if ((device = usbSearchDevice(path, chooser, data))) break;
      } else if (S_ISREG(status.st_mode)) {
        UsbDeviceExtension *devx;
        if ((devx = malloc(sizeof(*devx)))) {
          if ((devx->path = strdup(path))) {
            if ((devx->file = open(devx->path, O_RDWR)) != -1) {
              if ((device = usbTestDevice(devx, chooser, data))) break;
              close(devx->file);
            }
            free(devx->path);
          }
          free(devx);
        }
      }
    }
    closedir(directory);
  }
  return device;
}

static int
usbTestPath (const char *path) {
  struct statfs status;
  if (statfs(path, &status) != -1) {
    if (status.f_type == USBDEVICE_SUPER_MAGIC) return 1;
  }
  return 0;
}

static char *
usbFindRoot (void) {
  char *root = NULL;
  const char *path = MOUNTED;
  FILE *table;
  if ((table = setmntent(path, "r"))) {
    struct mntent *entry;
    while ((entry = getmntent(table))) {
      if ((strcmp(entry->mnt_type, "usbdevfs") == 0) ||
          (strcmp(entry->mnt_type, "usbfs") == 0)) {
        if (usbTestPath(entry->mnt_dir)) {
          root = strdupWrapper(entry->mnt_dir);
          break;
        }
      }
    }
    endmntent(table);
  } else {
    LogPrint((errno == ENOENT)? LOG_WARNING: LOG_ERR,
             "Mounted file system table open erorr: %s: %s",
             path, strerror(errno));
  }
  return root;
}

static char *
usbMakeRoot (void) {
  const char *type = "usbfs";
  const char *name = type;
  char *directory = makePath(DATA_DIRECTORY, type);
  if (directory) {
    if (makeDirectory(directory)) {
      if (usbTestPath(directory)) return directory;
      LogPrint(LOG_NOTICE, "Mounting USBFS: %s", directory);
      if (mount(name, directory, type, 0, NULL) != -1) return directory;
      LogPrint(LOG_ERR, "USBFS mount error: %s: %s", directory, strerror(errno));
    }
    free(directory);
  }
  return NULL;
}

UsbDevice *
usbFindDevice (UsbDeviceChooser chooser, void *data) {
  UsbDevice *device = NULL;
  char *root;
  if ((root = usbFindRoot()) || (root = usbMakeRoot())) {
    device = usbSearchDevice(root, chooser, data);
    free(root);
  } else {
    LogPrint(LOG_WARNING, "USBFS not mounted.");
  }
  return device;
}
