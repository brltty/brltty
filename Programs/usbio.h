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

#include <stdint.h>

typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bcdUSB;
  uint8_t bDeviceClass;
  uint8_t bDeviceSubClass;
  uint8_t bDeviceProtocol;
  uint8_t bMaxPacketSize0;
  uint16_t idVendor;
  uint16_t idProduct;
  uint16_t bcdDevice;
  uint8_t iManufacturer;
  uint8_t iProduct;
  uint8_t iSerialNumber;
  uint8_t bNumConfigurations;
} __attribute__((packed)) UsbDeviceDescriptor;

typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t wTotalLength;
  uint8_t bNumInterfaces;
  uint8_t bConfigurationValue;
  uint8_t iConfiguration;
  uint8_t bmAttributes;
  uint8_t MaxPower;
} __attribute__((packed)) UsbConfigurationDescriptor;

typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bInterfaceNumber;
  uint8_t bAlternateSetting;
  uint8_t bNumEndpoints;
  uint8_t bInterfaceClass;
  uint8_t bInterfaceSubClass;
  uint8_t bInterfaceProtocol;
  uint8_t iInterface;
} __attribute__((packed)) UsbInterfaceDescriptor;

typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bEndpointAddress;
  uint8_t bmAttributes;
  uint16_t wMaxPacketSiz;
  uint8_t bInterval;
  uint8_t bRefresh;
  uint8_t bSynchAddress;
} __attribute__((packed)) UsbEndpointDescriptor;

typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t wData[1];
} __attribute__((packed)) UsbStringDescriptor;

typedef union {
  UsbDeviceDescriptor device;
  UsbConfigurationDescriptor configuration;
  UsbInterfaceDescriptor interface;
  UsbEndpointDescriptor endpoint;
  UsbStringDescriptor string;
  unsigned char bytes[0XFF];
} UsbDescriptor;

typedef struct {
  uint8_t bRequestType;
  uint8_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
} __attribute__((packed)) UsbSetupPacket;

typedef struct UsbDeviceStruct UsbDevice;
typedef int (*UsbDeviceChooser) (UsbDevice *device, void *data);

extern UsbDevice *usbOpenDevice (const char *path);
extern UsbDevice *usbFindDevice (UsbDeviceChooser chooser, void *data);
extern void usbCloseDevice (UsbDevice *device);
extern const UsbDeviceDescriptor *usbDeviceDescriptor (UsbDevice *device);
extern int usbResetDevice (UsbDevice *device);

extern int usbSetConfiguration (
  UsbDevice *device,
  unsigned int configuration
);

extern char *usbGetDriver (
  UsbDevice *device,
  unsigned int interface
);
extern int usbControlDriver (
  UsbDevice *device,
  unsigned int interface,
  int code,
  void *data
);
extern char *usbGetSerialDevice (
  UsbDevice *device,
  unsigned int interface
);

extern int usbClaimInterface (
  UsbDevice *device,
  unsigned int interface
);
extern int usbReleaseInterface (
  UsbDevice *device,
  unsigned int interface
);
extern int usbSetAlternative (
  UsbDevice *device,
  unsigned int interface,
  unsigned int alternative
);

extern int usbResetEndpoint (
  UsbDevice *device,
  unsigned int endpoint
);
extern int usbClearEndpoint (
  UsbDevice *device,
  unsigned int endpoint
);

extern int usbControlTransfer (
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
);
extern int usbGetDescriptor (
  UsbDevice *device,
  unsigned char type,
  unsigned char number,
  unsigned int index,
  UsbDescriptor *descriptor,
  int timeout
);
extern char *usbGetString (
  UsbDevice *device,
  unsigned char number,
  int timeout
);

extern int usbBulkRead (
  UsbDevice *device,
  unsigned char endpoint,
  void *data,
  int length,
  int timeout
);
extern int usbBulkWrite (
  UsbDevice *device,
  unsigned char endpoint,
  void *data,
  int length,
  int timeout
);

extern struct usbdevfs_urb *usbSubmitRequest (
  UsbDevice *device,
  unsigned char endpoint,
  unsigned char type,
  void *buffer,
  int length,
  unsigned int flags,
  void *data
);
extern struct usbdevfs_urb *usbReapRequest (
  UsbDevice *device,
  int wait
);
