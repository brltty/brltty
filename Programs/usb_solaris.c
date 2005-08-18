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

#include "prologue.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <strings.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/asynch.h>
#include <sys/usb/clients/ugen/usb_ugen.h>

#include "misc.h"
#include "usb.h"
#include "usb_internal.h"

typedef struct {
  char *path;
  int data;
  int status;
  unsigned char configuration;
  unsigned char interface;
  unsigned char alternative;
} UsbDeviceExtension;

typedef struct {
  Queue *requests;
  char *name;
  int data;
  int status;
} UsbEndpointExtension;

typedef struct {
  aio_result_t result; /* must be first for aiowait() */

  UsbEndpoint *endpoint;
  void *context;
  void *buffer;
  int length;
} UsbAsynchronousRequest;

static int
usbOpenStatusFile (const char *path, int *status) {
  if ((*status = open(path, O_RDONLY)) != -1) return 1;
  LogError("USB status file open");
  return 0;
}

static int
usbOpenEndpointFiles (
  const char *device,
  const char *endpoint,
  int *data,
  int *status,
  int flags
) {
  static const char *suffix = "stat";
  char path[strlen(device) + 1 + strlen(endpoint) + strlen(suffix) + 1];

  sprintf(path, "%s/%s", device, endpoint);
  if ((*data = open(path, flags)) != -1) {
    strcat(path, suffix);
    if (usbOpenStatusFile(path, status)) {
      return 1;
    } else {
      LogError("USB endpoint status open");
    }

    close(*data);
    *data = -1;
  } else {
    LogError("USB endpoint data open");
  }

  return 0;
}

static void
usbDeallocateAsynchronousRequest (void *item, void *data) {
  UsbAsynchronousRequest *request = item;
  free(request);
}

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
  UsbDeviceExtension *devx = device->extension;
  devx->configuration = configuration;
  return 1;
}

int
usbClaimInterface (
  UsbDevice *device,
  unsigned char interface
) {
  UsbDeviceExtension *devx = device->extension;
  devx->interface = interface;
  devx->alternative = 0;
  return 1;
}

int
usbReleaseInterface (
  UsbDevice *device,
  unsigned char interface
) {
  return 1;
}

int
usbSetAlternative (
  UsbDevice *device,
  unsigned char interface,
  unsigned char alternative
) {
  UsbDeviceExtension *devx = device->extension;
  devx->interface = interface;
  devx->alternative = alternative;
  return 1;
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
  UsbDeviceExtension *devx = device->extension;
  UsbSetupPacket setup;

  setup.bRequestType = direction | recipient | type;
  setup.bRequest = request;
  putLittleEndian(&setup.wValue, value);
  putLittleEndian(&setup.wIndex, index);
  putLittleEndian(&setup.wLength, length);

  switch (direction) {
    case UsbControlDirection_Input:
      if (write(devx->data, &setup, sizeof(setup)) != -1) {
        int count = read(devx->data, buffer, length);
        if (count != -1) return count;
        LogError("USB control read");
      } else {
        LogError("USB control request");
      }
      break;

    case UsbControlDirection_Output: {
      unsigned char packet[sizeof(setup) + length];
      int size = 0;

      memcpy(&packet[size], &setup, sizeof(setup));
      size += sizeof(setup);

      memcpy(&packet[size], buffer, length);
      size += length;

      if (write(devx->data, packet, size) != -1) return size;
      LogError("USB control write");
      break;
    }

    default:
      LogPrint(LOG_ERR, "USB unsupported control transfer direction: %02X", direction);
      errno = ENOSYS;
      break;
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
  UsbEndpoint *endpoint = usbGetEndpoint(device, endpointAddress);

  if (endpoint) {
    UsbEndpointExtension *eptx = endpoint->extension;
    UsbAsynchronousRequest *request;

    if ((request = malloc(sizeof(*request) + length))) {
      UsbEndpointDirection direction = USB_ENDPOINT_DIRECTION(endpoint->descriptor);

      request->endpoint = endpoint;
      request->context = context;
      request->buffer = (request->length = length)? (request + 1): NULL;
      request->result.aio_return = AIO_INPROGRESS;

      switch (direction) {
        case UsbEndpointDirection_Input:
          if (aioread(eptx->data, request->buffer, request->length, 0, SEEK_CUR, &request->result) != -1) return request;
          LogError("USB asynchronous read");
          break;

        case UsbEndpointDirection_Output:
          if (request->buffer) memcpy(request->buffer, buffer, length);
          if (aiowrite(eptx->data, request->buffer, request->length, 0, SEEK_CUR, &request->result) != -1) return request;
          LogError("USB asynchronous write");
          break;

        default:
          LogPrint(LOG_ERR, "USB unsupported endpoint transfer direction: %02X", direction);
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
  UsbAsynchronousRequest *req = request;
  UsbEndpoint *endpoint = req->endpoint;
  UsbEndpointExtension *eptx = endpoint->extension;

  if (!deleteItem(eptx->requests, req)) {
    if (aiocancel(&req->result) == -1) {
      LogError("USB asynchronous request cancel");
      return 0;
    }
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
  UsbEndpoint *endpoint = usbGetEndpoint(device, endpointAddress);

  if (endpoint) {
    UsbEndpointExtension *eptx = endpoint->extension;
    struct timeval timeout;
    UsbAsynchronousRequest *request;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    while (!(request = dequeueItem(eptx->requests))) {
      aio_result_t *result;

    retry:
      if (request) {
        free(request);
        request = NULL;
      }

      if ((int)(result = aiowait(wait? NULL: &timeout)) == -1) {
        if (errno == EINTR) goto retry;

        if (errno != EINVAL) {
          LogError("USB asynchronous request wait");
          return NULL;
        }

        result = NULL;
      }

      if (!result) {
        errno = EAGAIN;
        return NULL;
      }

      request = (UsbAsynchronousRequest *)result;
      if (result->aio_return == -1) {
        errno = result->aio_errno;
        LogError("USB asynchronous request completion");
        goto retry;
      }

      {
        int count = result->aio_return;

        if (!usbApplyInputFilters(device, request->buffer, request->length, &count)) goto retry;
        if (!count) goto retry;
        request->length = count;
      }

      {
        UsbEndpoint *ep = request->endpoint;
        UsbEndpointExtension *epx = ep->extension;
        if (!enqueueItem(epx->requests, request)) {
          LogError("USB asynchronous request enqueue");
          goto retry;
        }
      }
    }

    response->context = request->context;
    response->buffer = request->buffer;
    response->length = request->length;
    return request;
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
  UsbEndpoint *endpoint = usbGetInputEndpoint(device, endpointNumber);

  if (endpoint) {
    UsbEndpointExtension *eptx = endpoint->extension;
    int count = read(eptx->data, buffer, length);

    if (count == -1) {
      LogError("USB endpoint read");
    } else if (!usbApplyInputFilters(device, buffer, length, &count)) {
      errno = EIO;
    } else if (!count) {
      errno = EAGAIN;
    } else {
      return count;
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
  UsbEndpoint *endpoint = usbGetOutputEndpoint(device, endpointNumber);

  if (endpoint) {
    UsbEndpointExtension *eptx = endpoint->extension;
    int count = write(eptx->data, buffer, length);

    if (count == length) return count;
    if (count == -1) {
      LogError("USB endpoint write");
    } else {
    }
  }

  return -1;
}

int
usbReadDeviceDescriptor (UsbDevice *device) {
  int count = usbGetDeviceDescriptor(device, &device->descriptor);

  if (count == UsbDescriptorSize_Device) {
    device->descriptor.bcdUSB = getLittleEndian(device->descriptor.bcdUSB);
    device->descriptor.idVendor = getLittleEndian(device->descriptor.idVendor);
    device->descriptor.idProduct = getLittleEndian(device->descriptor.idProduct);
    device->descriptor.bcdDevice = getLittleEndian(device->descriptor.bcdDevice);
    return 1;
  }

  if (count != -1) LogPrint(LOG_ERR, "USB short device descriptor (%d).", count);
  return 0;
}

int
usbAllocateEndpointExtension (UsbEndpoint *endpoint) {
  UsbDevice *device = endpoint->device;
  UsbDeviceExtension *devx = device->extension;
  UsbEndpointExtension *eptx;

  if ((eptx = malloc(sizeof(*eptx)))) {
    if ((eptx->requests = newQueue(usbDeallocateAsynchronousRequest, NULL))) {
      int flags;
  
      {
        char name[0X80];
        int length = 0;
  
        if (devx->configuration != 1) {
          int count;
          sprintf(&name[length], "cfg%d%n", devx->configuration, &count);
          length += count;
        }
  
        {
          int count;
          sprintf(&name[length], "if%d%n", devx->interface, &count);
          length += count;
        }
  
        if (devx->alternative != 0) {
          int count;
          sprintf(&name[length], ".%d%n", devx->alternative, &count);
          length += count;
        }
  
        {
          const UsbEndpointDescriptor *descriptor = endpoint->descriptor;
          const char *direction;
          int count;
  
          switch (USB_ENDPOINT_DIRECTION(descriptor)) {
            case UsbEndpointDirection_Input:
              direction = "in";
              flags = O_RDONLY;
              break;
  
            case UsbEndpointDirection_Output:
              direction = "out";
              flags = O_WRONLY;
              break;
  
            default:
              direction = "x";
              flags = O_RDWR;
              break;
          }
  
          sprintf(&name[length], "%s%d%n", direction, USB_ENDPOINT_NUMBER(descriptor), &count);
          length += count;
        }
  
        eptx->name = strdup(name);
      }
  
      if (eptx->name) {
        if (usbOpenEndpointFiles(devx->path, eptx->name, &eptx->data, &eptx->status,  flags)) {
          endpoint->extension = eptx;
          return 1;
        }
  
        free(eptx->name);
      }

      deallocateQueue(eptx->requests);
    }

    free(eptx);
  }

  return 0;
}

void
usbDeallocateEndpointExtension (UsbEndpoint *endpoint) {
  UsbEndpointExtension *eptx = endpoint->extension;

  if (eptx->status != -1) {
    close(eptx->status);
    eptx->status = -1;
  }

  if (eptx->data != -1) {
    close(eptx->data);
    eptx->data = -1;
  }

  if (eptx->name) {
    free(eptx->name);
    eptx->name = NULL;
  }

  if (eptx->requests) {
    deallocateQueue(eptx->requests);
    eptx->requests = NULL;
  }

  free(eptx);
}

void
usbDeallocateDeviceExtension (UsbDevice *device) {
  UsbDeviceExtension *devx = device->extension;

  if (devx->status != -1) {
    close(devx->status);
    devx->status = -1;
  }

  if (devx->data != -1) {
    close(devx->data);
    devx->data = -1;
  }

  if (devx->path) {
    free(devx->path);
    devx->path = NULL;
  }

  free(devx);
}

UsbDevice *
usbFindDevice (UsbDeviceChooser chooser, void *data) {
  UsbDevice *device = NULL;
  static const char *rootPath = "/dev/usb";
  int rootLength = strlen(rootPath);
  DIR *rootDirectory;

  if ((rootDirectory = opendir(rootPath))) {
    struct dirent *deviceEntry;

    while ((deviceEntry = readdir(rootDirectory))) {
      const char *deviceName = deviceEntry->d_name;
      int deviceLength = strlen(deviceName);

      {
        unsigned int vendor;
        unsigned int product;
        int length;
        if (sscanf(deviceName, "%x.%x%n", &vendor, &product, &length) < 2) continue;
        if (length != deviceLength) continue;
      }

      deviceLength += rootLength + 1;
      {
        char devicePath[deviceLength + 1];
        DIR *deviceDirectory;

        sprintf(devicePath, "%s/%s", rootPath, deviceName);
        if ((deviceDirectory = opendir(devicePath))) {
          struct dirent *instanceEntry;

          while ((instanceEntry = readdir(deviceDirectory))) {
            const char *instanceName = instanceEntry->d_name;
            int instanceLength = strlen(instanceName);

            {
              unsigned int number;
              int length;
              if (sscanf(instanceName, "%u%n", &number, &length) < 1) continue;
              if (length != instanceLength) continue;
            }

            instanceLength += deviceLength + 1;
            {
              char instancePath[instanceLength + 1];
              UsbDeviceExtension *devx;

              sprintf(instancePath, "%s/%s", devicePath, instanceName);
              if ((devx = malloc(sizeof(*devx)))) {
                if ((devx->path = strdup(instancePath))) {
                  if (usbOpenEndpointFiles(devx->path, "cntrl0", &devx->data, &devx->status, O_RDWR)) {
                    if ((device = usbTestDevice(devx, chooser, data))) break;

                    close(devx->status);
                    close(devx->data);
                  }

                  free(devx->path);
                }

                free(devx);
              }
            }
          }

          closedir(deviceDirectory);
        }
      }

      if (device) break;
    }

    closedir(rootDirectory);
  }

  return device;
}
