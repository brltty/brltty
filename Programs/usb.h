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

#ifndef _USB_H
#define _USB_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <inttypes.h>

/* Descriptor types. */
#define USB_DESCRIPTOR_TYPE_DEVICE        0x01
#define USB_DESCRIPTOR_TYPE_CONFIGURATION 0x02
#define USB_DESCRIPTOR_TYPE_STRING        0x03
#define USB_DESCRIPTOR_TYPE_INTERFACE     0x04
#define USB_DESCRIPTOR_TYPE_ENDPOINT      0x05

/* Descriptor sizes. */
#define USB_DESCRIPTOR_SIZE_DEVICE        18
#define USB_DESCRIPTOR_SIZE_CONFIGURATION  9
#define USB_DESCRIPTOR_SIZE_STRING         2
#define USB_DESCRIPTOR_SIZE_INTERFACE      9
#define USB_DESCRIPTOR_SIZE_ENDPOINT       7

/* Device and interface classes. */
#define USB_CLASS_PER_INTERFACE 0X00
#define USB_CLASS_AUDIO         0X01
#define USB_CLASS_COMM          0X02
#define USB_CLASS_HID           0X03
#define USB_CLASS_PHYSICAL      0X05
#define USB_CLASS_PRINTER       0X07
#define USB_CLASS_MASS_STORAGE  0X08
#define USB_CLASS_HUB           0X09
#define USB_CLASS_DATA          0X0A
#define USB_CLASS_APP_SPEC      0XFE
#define USB_CLASS_VENDOR_SPEC   0XFF

/* Endpoint addresses (bEndpointAddress). */
#define USB_ENDPOINT_NUMBER_MASK    0X0F
#define USB_ENDPOINT_DIRECTION_MASK 0X80

/* Endpoint types (bmAttributes). */
#define USB_ENDPOINT_TRANSFER_MASK        0X03
#define USB_ENDPOINT_TRANSFER_CONTROL     0X00
#define USB_ENDPOINT_TRANSFER_ISOCHRONOUS 0X01
#define USB_ENDPOINT_TRANSFER_BULK        0X02
#define USB_ENDPOINT_TRANSFER_INTERRUPT   0X03

/* Control transfer recipients. */
#define USB_RECIPIENT_MASK      0X1F
#define USB_RECIPIENT_DEVICE    0X00
#define USB_RECIPIENT_INTERFACE 0X01
#define USB_RECIPIENT_ENDPOINT  0X02
#define USB_RECIPIENT_OTHER     0X03

/* Control transfer types. */
#define USB_TYPE_MASK     0X60
#define USB_TYPE_STANDARD 0X00
#define USB_TYPE_CLASS    0X20
#define USB_TYPE_VENDOR   0X40
#define USB_TYPE_RESERVED 0X60

/* Transfer directions. */
#define USB_DIR_OUT 0X00
#define USB_DIR_IN  0X80

/* Standard control requests. */
#define USB_REQ_GET_STATUS        0X00
#define USB_REQ_CLEAR_FEATURE     0X01
#define USB_REQ_SET_FEATURE       0X03
#define USB_REQ_SET_ADDRESS       0X05
#define USB_REQ_GET_DESCRIPTOR    0X06
#define USB_REQ_SET_DESCRIPTOR    0X07
#define USB_REQ_GET_CONFIGURATION 0X08
#define USB_REQ_SET_CONFIGURATION 0X09
#define USB_REQ_GET_INTERFACE     0X0A
#define USB_REQ_SET_INTERFACE     0X0B
#define USB_REQ_SYNCH_FRAME       0X0C

typedef struct {
  uint8_t bLength;         /* Descriptor size in bytes. */
  uint8_t bDescriptorType; /* Descriptor type. */
} __attribute__((packed)) UsbDescriptorHeader;

typedef struct {
  uint8_t bLength;            /* Descriptor size in bytes (18). */
  uint8_t bDescriptorType;    /* Descriptor type (1 == device). */
  uint16_t bcdUSB;            /* USB revision number. */
  uint8_t bDeviceClass;       /* Device class. */
  uint8_t bDeviceSubClass;    /* Device subclass. */
  uint8_t bDeviceProtocol;    /* Device protocol. */
  uint8_t bMaxPacketSize0;    /* Maximum packet size in bytes for endpoint 0. */
  uint16_t idVendor;          /* Vendor identifier. */
  uint16_t idProduct;         /* Product identifier. */
  uint16_t bcdDevice;         /* Product revision number. */
  uint8_t iManufacturer;      /* String index for manufacturer name. */
  uint8_t iProduct;           /* String index for product description. */
  uint8_t iSerialNumber;      /* String index for serial number. */
  uint8_t bNumConfigurations; /* Number of configurations. */
} __attribute__((packed)) UsbDeviceDescriptor;

typedef struct {
  uint8_t bLength;             /* Descriptor size in bytes (9). */
  uint8_t bDescriptorType;     /* Descriptor type (2 == configuration). */
  uint16_t wTotalLength;       /* Block size in bytes for all descriptors. */
  uint8_t bNumInterfaces;      /* Number of interfaces. */
  uint8_t bConfigurationValue; /* Configuration number. */
  uint8_t iConfiguration;
  uint8_t bmAttributes;        /* Configuration attributes. */
  uint8_t MaxPower;            /* Maximum power in milliamps. */
} __attribute__((packed)) UsbConfigurationDescriptor;

typedef struct {
  uint8_t bLength;         /* Descriptor size in bytes (2 + numchars/2). */
  uint8_t bDescriptorType; /* Descriptor type (3 == string). */
  uint16_t wData[1];       /* First 16-bit character. */
} __attribute__((packed)) UsbStringDescriptor;

typedef struct {
  uint8_t bLength;            /* Descriptor size in bytes (9). */
  uint8_t bDescriptorType;    /* Descriptor type (4 == interface). */
  uint8_t bInterfaceNumber;   /* Interface number. */
  uint8_t bAlternateSetting;  /* Interface alternative. */
  uint8_t bNumEndpoints;      /* Number of endpoints. */
  uint8_t bInterfaceClass;    /* Interface class. */
  uint8_t bInterfaceSubClass; /* Interface subclass. */
  uint8_t bInterfaceProtocol; /* Interface protocol. */
  uint8_t iInterface;
} __attribute__((packed)) UsbInterfaceDescriptor;

typedef struct {
  uint8_t bLength;          /* Descriptor size in bytes (7, 9 for audio). */
  uint8_t bDescriptorType;  /* Descriptor type (5 == endpoint). */
  uint8_t bEndpointAddress; /* Endpoint number (ored with 0X80 if input. */
  uint8_t bmAttributes;     /* Endpoint type and attributes. */
  uint16_t wMaxPacketSize;  /* Maximum packet size in bytes. */
  uint8_t bInterval;        /* Maximum interval in milliseconds between transfers. */
  uint8_t bRefresh;
  uint8_t bSynchAddress;
} __attribute__((packed)) UsbEndpointDescriptor;

typedef union {
  UsbDescriptorHeader header;
  UsbDeviceDescriptor device;
  UsbConfigurationDescriptor configuration;
  UsbStringDescriptor string;
  UsbInterfaceDescriptor interface;
  UsbEndpointDescriptor endpoint;
  unsigned char bytes[0XFF];
} UsbDescriptor;

typedef struct {
  uint8_t bRequestType; /* Recipient, direction, and type. */
  uint8_t bRequest;     /* Request code. */
  uint16_t wValue;      /* Request value. */
  uint16_t wIndex;      /* Recipient number (language for strings). */
  uint16_t wLength;     /* Data length in bytes. */
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

extern int usbIsSerialDevice (
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
extern int usbGetLanguage (
  UsbDevice *device,
  uint16_t *language,
  int timeout
);
extern char *usbGetString (
  UsbDevice *device,
  unsigned char number,
  int timeout
);
extern void usbLogString (
  UsbDevice *device,
  int index,
  const char *description
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
  const void *data,
  int length,
  int timeout
);

typedef struct {
  unsigned char *buffer;
  int length;
  void *context;
} UsbResponse;
extern void *usbSubmitRequest (
  UsbDevice *device,
  unsigned char endpoint,
  unsigned char transfer,
  void *buffer,
  int length,
  void *context
);
extern int usbCancelRequest (
  UsbDevice *device,
  void *request
);
extern void *usbReapResponse (
  UsbDevice *device,
  UsbResponse *response,
  int wait
);

extern int usbBeginInput (
  UsbDevice *device,
  unsigned char endpoint,
  int size,
  int count
);
extern int usbReapInput (
  UsbDevice *device,
  void *buffer,
  int length,
  int wait
);

typedef int (UsbStringVerifier) (const char *reference, const char *value);
extern UsbStringVerifier usbStringEquals;
extern UsbStringVerifier usbStringMatches;
extern int usbVerifyString (
  UsbDevice *device,
  UsbStringVerifier verify,
  unsigned char index,
  const char *value
);
extern int usbVerifyManufacturer (UsbDevice *device, const char *eRegExp);
extern int usbVerifyProduct (UsbDevice *device, const char *eRegExp);
extern int usbVerifySerialNumber (UsbDevice *device, const char *string);

extern int isUsbDevice (const char **path);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _USB_H */
