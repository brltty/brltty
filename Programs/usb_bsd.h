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

typedef struct {
  int timeout;
} BsdDeviceExtension;

typedef struct {
  int file;
  int timeout;
} BsdEndpointExtension;

static int
usbSetTimeout (int file, int new, int *old) {
  if (!old || (new != *old)) {
    int arg = new;
    if (ioctl(file, USB_SET_TIMEOUT, &arg) == -1) {
      LogError("USB timeout set");
      return 0;
    }
    if (old) *old = new;
  }
  return 1;
}

static int
usbSetShortTransfers (int file, int arg) {
  if (ioctl(file, USB_SET_SHORT_XFER, &arg) != -1) return 1;
  LogError("USB set short transfers");
  return 0;
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
  int arg = configuration;
  if (ioctl(device->file, USB_SET_CONFIG, &arg) != -1) return 1;
  LogError("USB configuration set");
  return 0;
}

int
usbClaimInterface (
  UsbDevice *device,
  unsigned char interface
) {
  return 1;
/*
  errno = ENOSYS;
  LogError("USB interface claim");
  return 0;
*/
}

int
usbReleaseInterface (
  UsbDevice *device,
  unsigned char interface
) {
  return 1;
/*
  errno = ENOSYS;
  LogError("USB interface release");
  return 0;
*/
}

int
usbSetAlternative (
  UsbDevice *device,
  unsigned char interface,
  unsigned char alternative
) {
  struct usb_alt_interface arg;
  memset(&arg, 0, sizeof(arg));
  arg.uai_interface_index = interface;
  arg.uai_alt_no = alternative;
  if (ioctl(device->file, USB_SET_ALTINTERFACE, &arg) != -1) return 1;
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
  BsdDeviceExtension *bsd = device->extension;
  struct usb_ctl_request arg;
  memset(&arg, 0, sizeof(arg));
  arg.ucr_request.bmRequestType = direction | recipient | type;
  arg.ucr_request.bRequest = request;
  USETW(arg.ucr_request.wValue, value);
  USETW(arg.ucr_request.wIndex, index);
  USETW(arg.ucr_request.wLength, length);
  arg.ucr_data = buffer;
  arg.ucr_flags = USBD_SHORT_XFER_OK;
  if (usbSetTimeout(device->file, timeout, &bsd->timeout)) {
    if (ioctl(device->file, USB_DO_REQUEST, &arg) != -1) return arg.ucr_actlen;
    LogError("USB control transfer");
  }
  return -1;
}

int
usbAllocateEndpointExtension (UsbEndpoint *endpoint) {
  BsdEndpointExtension *bsd;

  if ((bsd = malloc(sizeof(*bsd)))) {
    const char *prefix = endpoint->device->path;
    const char *dot = strchr(prefix, '.');
    int length = dot? (dot - prefix): strlen(prefix);
    char path[PATH_MAX+1];
    int flags = O_RDWR;

    snprintf(path, sizeof(path), USB_ENDPOINT_PATH_FORMAT,
             length, prefix, USB_ENDPOINT_NUMBER(endpoint->descriptor));

    switch (USB_ENDPOINT_DIRECTION(endpoint->descriptor)) {
      case USB_ENDPOINT_DIRECTION_INPUT : flags = O_RDONLY; break;
      case USB_ENDPOINT_DIRECTION_OUTPUT: flags = O_WRONLY; break;
    }

    if ((bsd->file = open(path, flags)) != -1) {
      if (usbSetShortTransfers(bsd->file, 1)) {
        bsd->timeout = -1;

        endpoint->extension = bsd;
        return 1;
      }

      close(bsd->file);
    }

    free(bsd);
  }

  return 0;
}

void
usbDeallocateEndpointExtension (UsbEndpoint *endpoint) {
  BsdEndpointExtension *bsd = endpoint->extension;
  close(bsd->file);
  free(bsd);
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
    BsdEndpointExtension *bsd = endpoint->extension;
    if (usbSetTimeout(bsd->file, timeout, &bsd->timeout)) {
      if ((count = read(bsd->file, buffer, length)) != -1) {
        if (!usbApplyInputFilters(device, buffer, length, &count)) {
          errno = EIO;
          count = -1;
        }
      } else {
        LogError("USB endpoint read");
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
    BsdEndpointExtension *bsd = endpoint->extension;
    if (usbSetTimeout(bsd->file, timeout, &bsd->timeout)) {
      int count = write(bsd->file, buffer, length);
      if (count != -1) return count;
      LogError("USB endpoint write");
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
  if (ioctl(device->file, USB_GET_DEVICE_DESC, &device->descriptor) != -1) {
    device->descriptor.bcdUSB = getLittleEndian(device->descriptor.bcdUSB);
    device->descriptor.idVendor = getLittleEndian(device->descriptor.idVendor);
    device->descriptor.idProduct = getLittleEndian(device->descriptor.idProduct);
    device->descriptor.bcdDevice = getLittleEndian(device->descriptor.bcdDevice);
    return 1;
  }
  LogError("USB device descriptor read");
  return 0;
}

int
usbAllocateDeviceExtension (UsbDevice *device) {
  BsdDeviceExtension *bsd;

  if ((bsd = malloc(sizeof(*bsd)))) {
    bsd->timeout = -1;

    device->extension = bsd;
    return 1;

    free(bsd);
  }

  return 0;
}

void
usbDeallocateDeviceExtension (UsbDevice *device) {
  BsdDeviceExtension *bsd = device->extension;
  free(bsd);
}

UsbDevice *
usbFindDevice (UsbDeviceChooser chooser, void *data) {
  UsbDevice *device = NULL;
  int busNumber = 0;
  while (1) {
    char busPath[PATH_MAX+1];
    int bus;
    snprintf(busPath, sizeof(busPath), "/dev/usb%d", busNumber);
    if ((bus = open(busPath, O_RDONLY)) != -1) {
      int deviceNumber;
      for (deviceNumber=1; deviceNumber<USB_MAX_DEVICES; deviceNumber++) {
        struct usb_device_info info;
        memset(&info, 0, sizeof(info));
        info.udi_addr = deviceNumber;
        if (ioctl(bus, USB_DEVICEINFO, &info) != -1) {
          static const char *driver = "ugen";
          const char *deviceName = info.udi_devnames[0];

          LogPrint(LOG_DEBUG, "USB device [%d,%d]: vendor=%s product=%s",
                   busNumber, deviceNumber, info.udi_vendor, info.udi_product);
          {
            int nameNumber;
            for (nameNumber=0; nameNumber<USB_MAX_DEVNAMES; nameNumber++) {
              const char *name = info.udi_devnames[nameNumber];
              if (*name)
                LogPrint(LOG_DEBUG, "USB name %d: %s", nameNumber, name);
            }
          }

          if (strncmp(deviceName, driver, strlen(driver)) == 0) {
            char devicePath[PATH_MAX+1];
            snprintf(devicePath, sizeof(devicePath), USB_CONTROL_PATH_FORMAT, deviceName);
            if ((device = usbTestDevice(devicePath, chooser, data))) {
              close(bus);
              return device;
            }
          }
        } else if (errno != ENXIO) {
          LogError("USB device query");
        }
      }
      close(bus);
    } else if (errno == ENOENT) {
      break;
    } else if (errno != ENXIO) {
      LogError("USB bus open");
    }
    busNumber++;
  }
  return device;
}
