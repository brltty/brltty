/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

int
usbResetDevice (UsbDevice *device) {
  if (ioctl(device->file, USBDEVFS_RESET, NULL) != -1) return 1;
  LogError("USB device reset");
  return 0;
}

int
usbSetConfiguration (
  UsbDevice *device,
  unsigned int configuration
) {
  if (ioctl(device->file, USBDEVFS_SETCONFIGURATION, &configuration) != -1) return 1;;
  LogError("USB configuration set");
  return 0;
}

static char *
usbGetDriver (
  UsbDevice *device,
  unsigned int interface
) {
  struct usbdevfs_getdriver arg;
  memset(&arg, 0, sizeof(arg));
  arg.interface = interface;
  if (ioctl(device->file, USBDEVFS_GETDRIVER, &arg) == -1) return NULL;
  return strdup(arg.driver);
}

int
usbIsSerialDevice (
  UsbDevice *device,
  unsigned int interface
) {
  int yes = 0;
  char *driver = usbGetDriver(device, interface);
  if (driver) {
    if (strcmp(driver, "serial") == 0) {
      yes = 1;
    }
    free(driver);
  }
  return yes;
}

static int
usbControlDriver (
  UsbDevice *device,
  unsigned int interface,
  int code,
  void *data
) {
  struct usbdevfs_ioctl arg;
  memset(&arg, 0, sizeof(arg));
  arg.ifno = interface;
  arg.ioctl_code = code;
  arg.data = data;
  if (ioctl(device->file, USBDEVFS_IOCTL, &arg) != -1) return 1;
  LogError("USB driver control");
  return 0;
}

static int
usbDisconnectDriver (
  UsbDevice *device,
  unsigned int interface
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
  unsigned int interface
) {
  int disconnected = 0;
claim:
  if (ioctl(device->file, USBDEVFS_CLAIMINTERFACE, &interface) != -1) return 1;

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
  unsigned int interface
) {
  if (ioctl(device->file, USBDEVFS_RELEASEINTERFACE, &interface) != -1) return 1;
  LogError("USB interface release");
  return 0;
}

int
usbSetAlternative (
  UsbDevice *device,
  unsigned int interface,
  unsigned int alternative
) {
  struct usbdevfs_setinterface arg;
  memset(&arg, 0, sizeof(arg));
  arg.interface = interface;
  arg.altsetting = alternative;
  if (ioctl(device->file, USBDEVFS_SETINTERFACE, &arg) != -1) return 1;
  LogError("USB alternative set");
  return 0;
}

int
usbResetEndpoint (
  UsbDevice *device,
  unsigned int endpoint
) {
  if (ioctl(device->file, USBDEVFS_RESETEP, &endpoint) != -1) return 1;
  LogError("USB endpoint reset");
  return 0;
}

int
usbClearEndpoint (
  UsbDevice *device,
  unsigned int endpoint
) {
  if (ioctl(device->file, USBDEVFS_CLEAR_HALT, &endpoint) != -1) return 1;
  LogError("USB endpoint clear");
  return 0;
}

int
usbControlTransfer (
  UsbDevice *device,
  unsigned char recipient,
  unsigned char direction,
  unsigned char type,
  unsigned char request,
  unsigned short value,
  unsigned short index,
  void *data,
  int length,
  int timeout
) {
  union {
    struct usbdevfs_ctrltransfer transfer;
    UsbSetupPacket setup;
  } arg;
  memset(&arg, 0, sizeof(arg));
  arg.setup.bRequestType = direction | type | recipient;
  arg.setup.bRequest = request;
  putLittleEndian(&arg.setup.wValue, value);
  putLittleEndian(&arg.setup.wIndex, index);
  putLittleEndian(&arg.setup.wLength, length);
  arg.transfer.data = data;
  arg.transfer.timeout = timeout;

  {
    int count = ioctl(device->file, USBDEVFS_CONTROL, &arg);
    if (count == -1) LogError("USB control transfer");
    return count;
  }
}

int
usbBulkTransfer (
  UsbDevice *device,
  unsigned char endpoint,
  void *data,
  int length,
  int timeout
) {
  struct usbdevfs_bulktransfer arg;
  memset(&arg, 0, sizeof(arg));
  arg.ep = endpoint;
  arg.data = data;
  arg.len = length;
  arg.timeout = timeout;

  {
    int count = ioctl(device->file, USBDEVFS_BULK, &arg);
    if (count == -1) LogError("USB bulk transfer");
    return count;
  }
}

void *
usbSubmitRequest (
  UsbDevice *device,
  unsigned char endpoint,
  unsigned char transfer,
  void *buffer,
  int length,
  void *data
) {
  struct usbdevfs_urb *urb;
  if ((urb = malloc(sizeof(*urb) + length))) {
    memset(urb, 0, sizeof(*urb));
    urb->endpoint = endpoint;
    switch (transfer) {
      case USB_ENDPOINT_TRANSFER_CONTROL:
        urb->type = USBDEVFS_URB_TYPE_CONTROL;
        break;
      case USB_ENDPOINT_TRANSFER_ISOCHRONOUS:
        urb->type = USBDEVFS_URB_TYPE_ISO;
        break;
      case USB_ENDPOINT_TRANSFER_BULK:
        urb->type = USBDEVFS_URB_TYPE_BULK;
        break;
      case USB_ENDPOINT_TRANSFER_INTERRUPT:
        urb->type = USBDEVFS_URB_TYPE_INTERRUPT;
        break;
    }
    urb->buffer = (urb->buffer_length = length)? (urb + 1): NULL;
    if (buffer) memcpy(urb->buffer, buffer, length);
    urb->flags = 0;
    urb->signr = 0;
    urb->usercontext = data;
    if (ioctl(device->file, USBDEVFS_SUBMITURB, urb) != -1) return urb;

    /* UHCI support returns ENXIO if a URB is already submitted. */
    if (errno != ENXIO) LogError("USB URB submit");

    free(urb);
  } else {
    LogError("USB URB allocation");
  }
  return NULL;
}

int
usbCancelRequest (
  UsbDevice *device,
  void *request
) {
  if (ioctl(device->file, USBDEVFS_DISCARDURB, request) == -1) {
    /* EINVAL is returned if the URB is already complete. */
    if (errno != EINVAL) {
      LogError("USB URB discard");
      return 0;
    }
  }
  free(request);
  return 1;
}

void *
usbReapResponse (
  UsbDevice *device,
  UsbResponse *response,
  int wait
) {
  struct usbdevfs_urb *urb;

  response->buffer = NULL;
  response->length = 0;
  response->context = NULL;

  if (ioctl(device->file,
            wait? USBDEVFS_REAPURB: USBDEVFS_REAPURBNDELAY,
            &urb) == -1) {
    urb = NULL;
    if (wait || (errno != EAGAIN)) LogError("USB URB reap");
  } else if (!urb) {
    errno = EAGAIN;
  } else if (urb->status) {
    if ((errno = urb->status) < 0) errno = -errno;
    LogError("USB URB status");
    urb = NULL;
  } else {
    response->buffer = urb->buffer;
    response->length = urb->actual_length;
    response->context = urb->usercontext;
  }
  return urb;
}

int
usbReadDeviceDescriptor (UsbDevice *device) {
  int count = read(device->file, &device->descriptor, USB_DESCRIPTOR_SIZE_DEVICE);
  if (count == -1) {
    LogError("USB device descriptor read");
  } else if (count != USB_DESCRIPTOR_SIZE_DEVICE) {
    LogPrint(LOG_ERR, "USB short device descriptor (%d).", count);
  } else {
    return 1;
  }
  return 0;
}

static UsbDevice *
usbSearchDevice (const char *root, UsbDeviceChooser chooser, void *data) {
  UsbDevice *device = NULL;
  DIR *directory;
  if ((directory = opendir(root))) {
    struct dirent *entry;
    while ((entry = readdir(directory))) {
      struct stat status;
      char path[PATH_MAX+1];
      if (strlen(entry->d_name) != 3) continue;
      if (!isdigit(entry->d_name[0]) ||
          !isdigit(entry->d_name[1]) ||
          !isdigit(entry->d_name[2])) continue;
      snprintf(path, sizeof(path), "%s/%s", root, entry->d_name);
      if (stat(path, &status) == -1) continue;
      if (S_ISDIR(status.st_mode)) {
        if ((device = usbSearchDevice(path, chooser, data))) break;
      } else if (S_ISREG(status.st_mode)) {
        if ((device = usbTestDevice(path, chooser, data))) break;
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
    if (!root) LogPrint(LOG_WARNING, "USBFS file system not mounted.");
  } else {
    LogPrint((errno == ENOENT)? LOG_WARNING: LOG_ERR,
             "Mounted file system table open erorr: %s: %s",
             path, strerror(errno));
  }
  return root;
}

static char *
usbMakeRoot (void) {
  char *root = makePath(DATA_DIRECTORY, "usbfs");

  if (access(root, F_OK) == -1) {
    if (errno != ENOENT) {
      LogPrint(LOG_ERR, "USBFS root access error: %s: %s", root, strerror(errno));
      goto error;
    }

    LogPrint(LOG_NOTICE, "Creating USBFS root: %s", root);
    if (mkdir(root, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) == -1) {
      LogPrint(LOG_ERR, "USBFS root creation error: %s: %s", root, strerror(errno));
      goto error;
    }
  }

  if (!usbTestPath(root)) {
    LogPrint(LOG_NOTICE, "Mounting USBFS: %s", root);
    if (mount("usbfs", root, "usbfs", 0, NULL) == -1) {
      LogPrint(LOG_ERR, "USBFS mount error: %s: %s", root, strerror(errno));

    error:
      free(root);
      root = NULL;
    }
  }
  return root;
}

UsbDevice *
usbFindDevice (UsbDeviceChooser chooser, void *data) {
  UsbDevice *device = NULL;
  char *root;
  if ((root = usbFindRoot()) || (root = usbMakeRoot())) {
    device = usbSearchDevice(root, chooser, data);
    free(root);
  }
  return device;
}
