/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>

#ifndef USBDEVFS_DISCONNECT
#define USBDEVFS_DISCONNECT _IO('U', 22)
#endif /* USBDEVFS_DISCONNECT */

#ifndef USBDEVFS_CONNECT
#define USBDEVFS_CONNECT _IO('U', 23)
#endif /* USBDEVFS_CONNECT */

#include "misc.h"
#include "mount.h"
#include "io_usb.h"
#include "usb_internal.h"

struct UsbDeviceExtensionStruct {
  char *usbfsPath;
  char *sysfsPath;
  int usbfsFile;
};

struct UsbEndpointExtensionStruct {
  Queue *completedRequests;
};

int
usbResetDevice (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  if (ioctl(devx->usbfsFile, USBDEVFS_RESET, NULL) != -1) return 1;
  LogError("USB device reset");
  return 0;
}

int
usbDisableAutosuspend (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  int ok = 0;
  char *path = makePath(devx->sysfsPath, "device/power/autosuspend");

  if (path) {
    int file = open(path, O_WRONLY);

    if (file != -1) {
      static const char *const values[] = {"-1", "0", NULL};
      const char *const *value = values;

      while (*value) {
        size_t length = strlen(*value);
        ssize_t result = write(file, *value, length);

        if (result != -1) {
          ok = 1;
          break;
        }

        if (errno != EINVAL) {
          LogPrint(LOG_ERR, "write error: %s: %s", path, strerror(errno));
          break;
        }

        ++value;
      }

      close(file);
    } else {
      LogPrint((errno == ENOENT)? LOG_DEBUG: LOG_ERR,
               "open error: %s: %s", path, strerror(errno));
    }

    free(path);
  }

  return ok;
}

static char *
usbGetDriver (UsbDevice *device, unsigned char interface) {
  UsbDeviceExtension *devx = device->extension;
  struct usbdevfs_getdriver arg;
  memset(&arg, 0, sizeof(arg));
  arg.interface = interface;
  if (ioctl(devx->usbfsFile, USBDEVFS_GETDRIVER, &arg) == -1) return NULL;
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
  if (ioctl(devx->usbfsFile, USBDEVFS_IOCTL, &arg) != -1) return 1;
  LogError("USB driver control");
  return 0;
}

static int
usbDisconnectDriver (UsbDevice *device, unsigned char interface) {
#ifdef USBDEVFS_DISCONNECT
  if (usbControlDriver(device, interface, USBDEVFS_DISCONNECT, NULL)) return 1;
#else /* USBDEVFS_DISCONNECT */
  LogPrint(LOG_WARNING, "USB driver disconnection not available.");
#endif /* USBDEVFS_DISCONNECT */
  return 0;
}

static int
usbDisconnectInterface (UsbDevice *device, unsigned char interface) {
  char *driver = usbGetDriver(device, interface);

  if (driver) {
    LogPrint(LOG_WARNING, "USB interface in use: %s", driver);
    free(driver);

    if (usbDisconnectDriver(device, interface)) return 1;
  }

  return 0;
}

static int
usbDisconnectInterfaces (UsbDevice *device) {
  const UsbDescriptor *descriptor = NULL;
  int disconnected = 0;

  while (usbNextDescriptor(device, &descriptor)) {
    if (descriptor->interface.bDescriptorType == UsbDescriptorType_Interface) {
      if (usbDisconnectInterface(device, descriptor->interface.bInterfaceNumber)) disconnected = 1;
    }
  }

  return disconnected;
}

int
usbSetConfiguration (
  UsbDevice *device,
  unsigned char configuration
) {
  UsbDeviceExtension *devx = device->extension;
  int disconnected = 0;

  while (1) {
    unsigned int arg = configuration;

    if (ioctl(devx->usbfsFile, USBDEVFS_SETCONFIGURATION, &arg) != -1) return 1;
    if (errno != EBUSY) break;
    if (disconnected) break;

    if (!usbDisconnectInterfaces(device)) {
      errno = EBUSY;
      break;
    }
    disconnected = 1;
  }

  LogError("USB configuration set");
  return 0;
}

int
usbClaimInterface (
  UsbDevice *device,
  unsigned char interface
) {
  UsbDeviceExtension *devx = device->extension;
  int disconnected = 0;

  while (1) {
    unsigned int arg = interface;

    if (ioctl(devx->usbfsFile, USBDEVFS_CLAIMINTERFACE, &arg) != -1) return 1;
    if (errno != EBUSY) break;
    if (disconnected) break;

    if (!usbDisconnectInterface(device, interface)) {
      errno = EBUSY;
      break;
    }
    disconnected = 1;
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
  if (ioctl(devx->usbfsFile, USBDEVFS_RELEASEINTERFACE, &arg) != -1) return 1;
  if (errno == ENODEV) return 1;
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
  if (ioctl(devx->usbfsFile, USBDEVFS_SETINTERFACE, &arg) != -1) return 1;
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
  if (ioctl(devx->usbfsFile, USBDEVFS_RESETEP, &arg) != -1) return 1;
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
  if (ioctl(devx->usbfsFile, USBDEVFS_CLEAR_HALT, &arg) != -1) return 1;
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
    int count = ioctl(devx->usbfsFile, USBDEVFS_CONTROL, &arg);
    if (count == -1) LogError("USB control transfer");
    return count;
  }
}

static int
usbReapUrb (
  UsbDevice *device,
  int wait
) {
  UsbDeviceExtension *devx = device->extension;
  struct usbdevfs_urb *urb;

  if (ioctl(devx->usbfsFile,
            wait? USBDEVFS_REAPURB: USBDEVFS_REAPURBNDELAY,
            &urb) != -1) {
    if (urb) {
      UsbEndpoint *endpoint;

      if ((endpoint = usbGetEndpoint(device, urb->endpoint))) {
        UsbEndpointExtension *eptx = endpoint->extension;

        if (enqueueItem(eptx->completedRequests, urb)) return 1;
        LogError("USB completed request enqueue");
        free(urb);
      }
    } else {
      errno = EAGAIN;
    }
  } else {
    if (wait || (errno != EAGAIN)) LogError("USB URB reap");
  }

  return 0;
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
      if (ioctl(devx->usbfsFile, USBDEVFS_SUBMITURB, urb) != -1) return urb;
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
      LogError("USB URB allocate");
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
  int reap = 1;

  if (ioctl(devx->usbfsFile, USBDEVFS_DISCARDURB, request) == -1) {
    if (errno == ENODEV)  {
      reap = 0;
    } else if (errno != EINVAL) {
      LogError("USB URB discard");
    }
  }
  
  {
    struct usbdevfs_urb *urb = request;
    UsbEndpoint *endpoint;

    if ((endpoint = usbGetEndpoint(device, urb->endpoint))) {
      UsbEndpointExtension *eptx = endpoint->extension;
      int found = 1;

      while (!deleteItem(eptx->completedRequests, request)) {
        if (!reap) break;
        if (!usbReapUrb(device, 0)) {
          found = 0;
          break;
        }
      }

      if (found) {
        free(request);
        return 1;
      }

      LogPrint(LOG_ERR, "USB request not found: urb=%p ept=%02X",
               urb, urb->endpoint);
    }
  }

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
    UsbEndpointExtension *eptx = endpoint->extension;
    struct usbdevfs_urb *urb;

    while (!(urb = dequeueItem(eptx->completedRequests))) {
      if (!usbReapUrb(device, wait)) return NULL;
    }

    response->context = urb->usercontext;
    response->buffer = urb->buffer;
    response->size = urb->buffer_length;

    if ((response->error = urb->status)) {
      if (response->error < 0) response->error = -response->error;
      errno = response->error;
      LogError("USB URB status");
      response->count = -1;
    } else {
      response->count = urb->actual_length;

      switch (USB_ENDPOINT_DIRECTION(endpoint->descriptor)) {
        case UsbEndpointDirection_Input:
          if (!usbApplyInputFilters(device, response->buffer, response->size, &response->count)) {
            response->error = EIO;
            response->count = -1;
          }
          break;
      }
    }

    return urb;
  }

  return NULL;
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
    int count = ioctl(devx->usbfsFile, USBDEVFS_BULK, &arg);
    if (count != -1) return count;
    LogError("USB bulk transfer");
  }
  return -1;
}

static struct usbdevfs_urb *
usbInterruptTransfer (
  UsbEndpoint *endpoint,
  void *buffer,
  int length,
  int timeout
) {
  UsbDevice *device = endpoint->device;
  struct usbdevfs_urb *urb = usbSubmitRequest(device,
                                              endpoint->descriptor->bEndpointAddress,
                                              buffer, length, NULL);

  if (urb) {
    UsbEndpointExtension *eptx = endpoint->extension;
    int interval = endpoint->descriptor->bInterval + 1;

    if (timeout) hasTimedOut(0);
    do {
      if (usbReapUrb(device, 0) &&
          deleteItem(eptx->completedRequests, urb)) {
        if (!urb->status) return urb;
        if ((errno = urb->status) < 0) errno = -errno;
        free(urb);
        break;
      }

      if (!timeout || hasTimedOut(timeout)) {
        usbCancelRequest(device, urb);
        errno = ETIMEDOUT;
        break;
      }

      approximateDelay(interval);
    } while (1);
  }

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
  int count = -1;
  UsbEndpoint *endpoint = usbGetInputEndpoint(device, endpointNumber);
  if (endpoint) {
    UsbEndpointTransfer transfer = USB_ENDPOINT_TRANSFER(endpoint->descriptor);
    switch (transfer) {
      case UsbEndpointTransfer_Bulk:
        count = usbBulkTransfer(endpoint, buffer, length, timeout);
        break;

      case UsbEndpointTransfer_Interrupt: {
        struct usbdevfs_urb *urb = usbInterruptTransfer(endpoint, NULL, length, timeout);

        if (urb) {
          count = urb->actual_length;
          if (count > length) count = length;
          memcpy(buffer, urb->buffer, count);
          free(urb);
        }
        break;
      }

      default:
        LogPrint(LOG_ERR, "USB input transfer not supported: %d", transfer);
        errno = ENOSYS;
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
    UsbEndpointTransfer transfer = USB_ENDPOINT_TRANSFER(endpoint->descriptor);
    switch (transfer) {
      case UsbEndpointTransfer_Bulk:
        return usbBulkTransfer(endpoint, (void *)buffer, length, timeout);

      case UsbEndpointTransfer_Interrupt: {
        struct usbdevfs_urb *urb = usbInterruptTransfer(endpoint, (void *)buffer, length, timeout);

        if (urb) {
          int count = urb->actual_length;
          free(urb);
          return count;
        }
        break;
      }

      default:
        LogPrint(LOG_ERR, "USB output transfer not supported: %d", transfer);
        errno = ENOSYS;
        break;
    }
  }
  return -1;
}

int
usbReadDeviceDescriptor (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;
  int count = read(devx->usbfsFile, &device->descriptor, UsbDescriptorSize_Device);
  if (count == -1) {
    LogError("USB device descriptor read");
  } else if (count != UsbDescriptorSize_Device) {
    LogPrint(LOG_ERR, "USB short device descriptor (%d).", count);
  } else {
    return 1;
  }
  return 0;
}

int
usbAllocateEndpointExtension (UsbEndpoint *endpoint) {
  UsbEndpointExtension *eptx;

  if ((eptx = malloc(sizeof(*eptx)))) {
    if ((eptx->completedRequests = newQueue(NULL, NULL))) {
      endpoint->extension = eptx;
      return 1;
    } else {
      LogError("USB endpoint completed request queue allocate");
    }
  } else {
    LogError("USB endpoint extension allocate");
  }

  return 0;
}

void
usbDeallocateEndpointExtension (UsbEndpointExtension *eptx) {
  if (eptx->completedRequests) {
    deallocateQueue(eptx->completedRequests);
    eptx->completedRequests = NULL;
  }

  free(eptx);
}

void
usbDeallocateDeviceExtension (UsbDeviceExtension *devx) {
  if (devx->usbfsFile != -1) {
    close(devx->usbfsFile);
    devx->usbfsFile = -1;
  }

  if (devx->usbfsPath) {
    free(devx->usbfsPath);
    devx->usbfsPath = NULL;
  }

  if (devx->sysfsPath) {
    free(devx->sysfsPath);
    devx->sysfsPath = NULL;
  }

  free(devx);
}

static char *
usbMakeSysfsPath (const char *usbfsPath) {
  const char *tail = usbfsPath + strlen(usbfsPath) - 8;
  unsigned int bus;
  unsigned int device;
  char extra;
  int count = sscanf(tail, "/%u/%u%c", &bus, &device, &extra);

  if (count == 2) {
    char *sysfsPath;

    {
      const char *format = "/sys/class/usb_device/usbdev%u.%u";
      char buffer[strlen(format) + (2 * 0X10) + 1];
      snprintf(buffer, sizeof(buffer), format, bus, device);
      sysfsPath= strdup(buffer);
    }

    if (sysfsPath) return sysfsPath;
  }

  return NULL;
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
      } else if (S_ISREG(status.st_mode) || S_ISCHR(status.st_mode)) {
        UsbDeviceExtension *devx;
        if ((devx = malloc(sizeof(*devx)))) {
          if ((devx->usbfsPath = strdup(path))) {
            if ((devx->sysfsPath = usbMakeSysfsPath(devx->usbfsPath))) {
              if ((devx->usbfsFile = open(devx->usbfsPath, O_RDWR)) != -1) {
                if ((device = usbTestDevice(devx, chooser, data))) break;
                close(devx->usbfsFile);
                devx->usbfsFile = -1;
              }
              free(devx->sysfsPath);
              devx->sysfsPath = NULL;
            }
            free(devx->usbfsPath);
            devx->usbfsPath = NULL;
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
usbVerifyDirectory (const char *path) {
  if (access(path, F_OK) != -1) return 1;
  return 0;
}

static int
usbVerifyUsbfs (const char *path) {
  struct statfs status;
  if (statfs(path, &status) != -1) {
    if (status.f_type == USBDEVICE_SUPER_MAGIC) return 1;
  }
  return 0;
}

static int
usbTestUsbfs (const char *path, const char *type) {
  if ((strcmp(type, "usbdevfs") == 0) ||
      (strcmp(type, "usbfs") == 0)) {
    if (usbVerifyUsbfs(path)) {
      return 1;
    }
  }
  return 0;
}

static char *
usbFindRoot (void) {
  {
    typedef struct {
      const char *path;
      int (*verify) (const char *path);
    } RootEntry;

    static const RootEntry roots[] = {
      {.path="/proc/bus/usb", .verify=usbVerifyUsbfs},
      {.path="/dev/bus/usb", .verify=usbVerifyDirectory},
      {.path=NULL, .verify=NULL}
    };
    const RootEntry *root = roots;

    while (root->path) {
      LogPrint(LOG_DEBUG, "verifying USB device root: %s", root->path);
      if (root->verify(root->path)) return strdupWrapper(root->path);
      ++root;
    }
  }

  return findMountPoint(usbTestUsbfs);
}

static char *
usbMakeRoot (void) {
  const char *type = "usbfs";
  char *directory = NULL;

  {
    char *workingDirectory = getWorkingDirectory();
    if (workingDirectory) {
      directory = makePath(workingDirectory, type);
      free(workingDirectory);
    }
  }

  if (directory) {
    if (makeDirectory(directory)) {
      if (usbVerifyUsbfs(directory)) return directory;
      if (makeMountPoint(directory, PACKAGE_NAME "-usbfs", type)) return directory;
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
    LogPrint(LOG_DEBUG, "USB Root: %s", root);
    device = usbSearchDevice(root, chooser, data);
    free(root);
  } else {
    LogPrint(LOG_WARNING, "USBFS not mounted");
  }
  return device;
}
