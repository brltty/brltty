/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_IO_USB
#define BRLTTY_INCLUDED_IO_USB

#include "prologue.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "io_defs.h"

/* Descriptor types. */
typedef enum {
  UsbDescriptorType_Device        = 0X01,
  UsbDescriptorType_Configuration = 0X02,
  UsbDescriptorType_String        = 0X03,
  UsbDescriptorType_Interface     = 0X04,
  UsbDescriptorType_Endpoint      = 0X05,
  UsbDescriptorType_HID           = 0X21,
  UsbDescriptorType_Report        = 0X22
} UsbDescriptorType;

/* Descriptor sizes. */
typedef enum {
  UsbDescriptorSize_Device        = 18,
  UsbDescriptorSize_Configuration =  9,
  UsbDescriptorSize_String        =  2,
  UsbDescriptorSize_Interface     =  9,
  UsbDescriptorSize_Endpoint      =  7,
  UsbDescriptorSize_HID           =  6,
  UsbDescriptorSize_Report        =  3
} UsbDescriptorSize;

/* Configuration attributes (bmAttributes). */
typedef enum {
  UsbConfigurationAttribute_BusPowered   = 0X80,
  UsbConfigurationAttribute_SelfPowered  = 0X40,
  UsbConfigurationAttribute_RemoteWakeup = 0X20
} UsbConfigurationAttribute;

/* Device and interface classes (bDeviceClass, bInterfaceClass). */
typedef enum {
  UsbClass_PerInterface = 0X00,
  UsbClass_Audio        = 0X01,
  UsbClass_Comm         = 0X02,
  UsbClass_Hid          = 0X03,
  UsbClass_Physical     = 0X05,
  UsbClass_Printer      = 0X07,
  UsbClass_MassStorage  = 0X08,
  UsbClass_Hub          = 0X09,
  UsbClass_Data         = 0X0A,
  UsbClass_AppSpec      = 0XFE,
  UsbClass_VendorSpec   = 0XFF
} UsbClass;

/* Endpoint numbers (bEndpointAddress). */
typedef enum {
  UsbEndpointNumber_Mask = 0X0F
} UsbEndpointNumber;
#define USB_ENDPOINT_NUMBER(descriptor) ((descriptor)->bEndpointAddress & UsbEndpointNumber_Mask)

/* Endpoint directions (bEndpointAddress). */
typedef enum {
  UsbEndpointDirection_Output = 0X00,
  UsbEndpointDirection_Input  = 0X80,
  UsbEndpointDirection_Mask   = 0X80
} UsbEndpointDirection;
#define USB_ENDPOINT_DIRECTION(descriptor) ((descriptor)->bEndpointAddress & UsbEndpointDirection_Mask)

/* Endpoint transfer types (bmAttributes). */
typedef enum {
  UsbEndpointTransfer_Control     = 0X00,
  UsbEndpointTransfer_Isochronous = 0X01,
  UsbEndpointTransfer_Bulk        = 0X02,
  UsbEndpointTransfer_Interrupt   = 0X03,
  UsbEndpointTransfer_Mask        = 0X03
} UsbEndpointTransfer;
#define USB_ENDPOINT_TRANSFER(descriptor) ((descriptor)->bmAttributes & UsbEndpointTransfer_Mask)

/* Endpoint isochronous types (bmAttributes). */
typedef enum {
  UsbEndpointIsochronous_Asynchronous = 0X04,
  UsbEndpointIsochronous_Adaptable    = 0X08,
  UsbEndpointIsochronous_Synchronous  = 0X0C,
  UsbEndpointIsochronous_Mask         = 0X0C
} UsbEndpointIsochronous;
#define USB_ENDPOINT_ISOCHRONOUS(descriptor) ((descriptor)->bmAttributes & UsbEndpointIsochronous_Mask)

/* Control transfer recipients. */
typedef enum {
  UsbControlRecipient_Device    = 0X00,
  UsbControlRecipient_Interface = 0X01,
  UsbControlRecipient_Endpoint  = 0X02,
  UsbControlRecipient_Other     = 0X03,
  UsbControlRecipient_Mask      = 0X1F
} UsbControlRecipient;

/* Control transfer types. */
typedef enum {
  UsbControlType_Standard = 0X00,
  UsbControlType_Class    = 0X20,
  UsbControlType_Vendor   = 0X40,
  UsbControlType_Reserved = 0X60,
  UsbControlType_Mask     = 0X60
} UsbControlType;

/* Transfer directions. */
typedef enum {
  UsbControlDirection_Output = 0X00,
  UsbControlDirection_Input  = 0X80,
  UsbControlDirection_Mask   = 0X80
} UsbControlDirection;

/* Standard control requests. */
typedef enum {
  UsbStandardRequest_GetStatus        = 0X00,
  UsbStandardRequest_ClearFeature     = 0X01,
  UsbStandardRequest_GetState         = 0X02,
  UsbStandardRequest_SetFeature       = 0X03,
  UsbStandardRequest_SetAddress       = 0X05,
  UsbStandardRequest_GetDescriptor    = 0X06,
  UsbStandardRequest_SetDescriptor    = 0X07,
  UsbStandardRequest_GetConfiguration = 0X08,
  UsbStandardRequest_SetConfiguration = 0X09,
  UsbStandardRequest_GetInterface     = 0X0A,
  UsbStandardRequest_SetInterface     = 0X0B,
  UsbStandardRequest_SynchFrame       = 0X0C
} UsbStandardRequest;

/* Standard features. */
typedef enum {
  UsbFeature_Endpoint_Stall      = 0X00,
  UsbFeature_Device_RemoteWakeup = 0X01
} UsbFeature;

typedef struct {
  uint8_t bLength;         /* Descriptor size in bytes. */
  uint8_t bDescriptorType; /* Descriptor type. */
} PACKED UsbDescriptorHeader;

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
} PACKED UsbDeviceDescriptor;

typedef struct {
  uint8_t bLength;             /* Descriptor size in bytes (9). */
  uint8_t bDescriptorType;     /* Descriptor type (2 == configuration). */
  uint16_t wTotalLength;       /* Block size in bytes for all descriptors. */
  uint8_t bNumInterfaces;      /* Number of interfaces. */
  uint8_t bConfigurationValue; /* Configuration number. */
  uint8_t iConfiguration;      /* String index for configuration description. */
  uint8_t bmAttributes;        /* Configuration attributes. */
  uint8_t bMaxPower;           /* Maximum power in 2 milliamp units. */
} PACKED UsbConfigurationDescriptor;

typedef struct {
  uint8_t bLength;         /* Descriptor size in bytes (2 + numchars/2). */
  uint8_t bDescriptorType; /* Descriptor type (3 == string). */
  uint16_t wData[127];     /* 16-bit characters. */
} PACKED UsbStringDescriptor;

typedef struct {
  uint8_t bLength;            /* Descriptor size in bytes (9). */
  uint8_t bDescriptorType;    /* Descriptor type (4 == interface). */
  uint8_t bInterfaceNumber;   /* Interface number. */
  uint8_t bAlternateSetting;  /* Interface alternative. */
  uint8_t bNumEndpoints;      /* Number of endpoints. */
  uint8_t bInterfaceClass;    /* Interface class. */
  uint8_t bInterfaceSubClass; /* Interface subclass. */
  uint8_t bInterfaceProtocol; /* Interface protocol. */
  uint8_t iInterface;         /* String index for interface description. */
} PACKED UsbInterfaceDescriptor;

typedef struct {
  uint8_t bLength;          /* Descriptor size in bytes (7, 9 for audio). */
  uint8_t bDescriptorType;  /* Descriptor type (5 == endpoint). */
  uint8_t bEndpointAddress; /* Endpoint number (ored with 0X80 if input. */
  uint8_t bmAttributes;     /* Endpoint type and attributes. */
  uint16_t wMaxPacketSize;  /* Maximum packet size in bytes. */
  uint8_t bInterval;        /* Maximum interval in milliseconds between transfers. */
  uint8_t bRefresh;
  uint8_t bSynchAddress;
} PACKED UsbEndpointDescriptor;

typedef struct {
  uint8_t bDescriptorType;  /* Descriptor type (34 == report). */
  uint16_t wLength;         /* Descriptor size in bytes (6). */
} PACKED UsbReportDescriptor;

typedef struct {
  uint8_t bLength;          /* Descriptor size in bytes (6). */
  uint8_t bDescriptorType;  /* Descriptor type (33 == HID). */
  uint16_t bcdHID;
  uint8_t bCountryCode;
  uint8_t bNumDescriptors;
  UsbReportDescriptor descriptors[(0XFF - UsbDescriptorSize_HID) / UsbDescriptorSize_Report];
} PACKED UsbHidDescriptor;

typedef union {
  UsbDescriptorHeader header;
  UsbDeviceDescriptor device;
  UsbConfigurationDescriptor configuration;
  UsbStringDescriptor string;
  UsbInterfaceDescriptor interface;
  UsbEndpointDescriptor endpoint;
  UsbHidDescriptor hid;
  unsigned char bytes[0XFF];
} UsbDescriptor;

typedef struct {
  uint8_t bRequestType; /* Recipient, direction, and type. */
  uint8_t bRequest;     /* Request code. */
  uint16_t wValue;      /* Request value. */
  uint16_t wIndex;      /* Recipient number (language for strings). */
  uint16_t wLength;     /* Data length in bytes. */
} PACKED UsbSetupPacket;

extern void usbForgetDevices (void);

typedef struct UsbDeviceStruct UsbDevice;
typedef int (*UsbDeviceChooser) (UsbDevice *device, void *data);
extern UsbDevice *usbFindDevice (UsbDeviceChooser chooser, void *data);
extern void usbCloseDevice (UsbDevice *device);
extern int usbResetDevice (UsbDevice *device);
extern int usbDisableAutosuspend (UsbDevice *device);

extern const UsbDeviceDescriptor *usbDeviceDescriptor (UsbDevice *device);
#define USB_IS_PRODUCT(descriptor,vendor,product) (((descriptor)->idVendor == (vendor)) && ((descriptor)->idProduct == (product)))

extern int usbNextDescriptor (
  UsbDevice *device,
  const UsbDescriptor **descriptor
);
extern const UsbConfigurationDescriptor *usbConfigurationDescriptor (
  UsbDevice *device
);
extern const UsbInterfaceDescriptor *usbInterfaceDescriptor (
  UsbDevice *device,
  unsigned char interface,
  unsigned char alternative
);
extern unsigned int usbAlternativeCount (
  UsbDevice *device,
  unsigned char interface
);
extern const UsbEndpointDescriptor *usbEndpointDescriptor (
  UsbDevice *device,
  unsigned char endpointAddress
);
extern const UsbHidDescriptor *usbHidDescriptor (UsbDevice *device);

extern int usbConfigureDevice (
  UsbDevice *device,
  unsigned char configuration
);
extern int usbGetConfiguration (
  UsbDevice *device,
  unsigned char *configuration
);

extern int usbOpenInterface (
  UsbDevice *device,
  unsigned char interface,
  unsigned char alternative
);
extern void usbCloseInterface (
  UsbDevice *device
);

extern int usbClearEndpoint (
  UsbDevice *device,
  unsigned char endpointAddress
);

extern int usbControlRead (
  UsbDevice *device,
  unsigned char recipient,
  unsigned char type,
  unsigned char request,
  unsigned short value,
  unsigned short index,
  void *buffer,
  int length,
  int timeout
);
extern int usbControlWrite (
  UsbDevice *device,
  unsigned char recipient,
  unsigned char type,
  unsigned char request,
  unsigned short value,
  unsigned short index,
  const void *buffer,
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
extern int usbGetDeviceDescriptor (
  UsbDevice *device,
  UsbDeviceDescriptor *descriptor
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
extern char *usbDecodeString (const UsbStringDescriptor *descriptor);
extern char *usbGetManufacturer (UsbDevice *device, int timeout);
extern char *usbGetProduct (UsbDevice *device, int timeout);
extern char *usbGetSerialNumber (UsbDevice *device, int timeout);

extern void usbLogString (
  UsbDevice *device,
  unsigned char number,
  const char *description
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

extern int usbReadEndpoint (
  UsbDevice *device,
  unsigned char endpointNumber,
  void *buffer,
  int length,
  int timeout
);
extern int usbWriteEndpoint (
  UsbDevice *device,
  unsigned char endpointNumber,
  const void *buffer,
  int length,
  int timeout
);

typedef struct {
  void *context;
  void *buffer;
  int size;
  int count;
  int error;
} UsbResponse;
extern void *usbSubmitRequest (
  UsbDevice *device,
  unsigned char endpointAddress,
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
  unsigned char endpointAddress,
  UsbResponse *response,
  int wait
);

extern int usbAwaitInput (
  UsbDevice *device,
  unsigned char endpointNumber,
  int timeout
);
extern int usbReapInput (
  UsbDevice *device,
  unsigned char endpointNumber,
  void *buffer,
  int length,
  int initialTimeout,
  int subsequentTimeout
);

typedef struct {
  void *const buffer;
  int const size;
  int length;
} UsbInputFilterData;
typedef int (*UsbInputFilter) (UsbInputFilterData *data);
extern int usbAddInputFilter (UsbDevice *device, UsbInputFilter filter);

typedef enum {
  UsbHidRequest_GetReport   = 0X01,
  UsbHidRequest_GetIdle     = 0X02,
  UsbHidRequest_GetProtocol = 0X03,
  UsbHidRequest_SetReport   = 0X09,
  UsbHidRequest_SetIdle     = 0X0A,
  UsbHidRequest_SetProtocol = 0X0B
} UsbHidRequest;

typedef enum {
  UsbHidReportType_Input   = 0X01,
  UsbHidReportType_Output  = 0X02,
  UsbHidReportType_Feature = 0X03
} UsbHidReportType;

extern int usbHidGetReport (
  UsbDevice *device,
  unsigned char interface,
  unsigned char report,
  void *buffer,
  int length,
  int timeout
);

extern int usbHidSetReport (
  UsbDevice *device,
  unsigned char interface,
  unsigned char report,
  const void *buffer,
  int length,
  int timeout
);

extern int usbHidGetFeature (
  UsbDevice *device,
  unsigned char interface,
  unsigned char report,
  void *buffer,
  int length,
  int timeout
);

extern int usbHidSetFeature (
  UsbDevice *device,
  unsigned char interface,
  unsigned char report,
  const void *buffer,
  int length,
  int timeout
);

typedef struct {
  int (*setBaud) (UsbDevice *device, int rate);
  int (*setFlowControl) (UsbDevice *device, SerialFlowControl flow);
  int (*setDataFormat) (UsbDevice *device, int dataBits, int stopBits, SerialParity parity);
  int (*setDtrState) (UsbDevice *device, int state);
  int (*setRtsState) (UsbDevice *device, int state);
  int (*enableAdapter) (UsbDevice *device);
} UsbSerialOperations;

extern const UsbSerialOperations *usbGetSerialOperations (UsbDevice *device);
extern int usbSetSerialParameters (UsbDevice *device, const SerialParameters *parameters);

typedef struct {
  uint16_t vendor;
  uint16_t product;
  unsigned char configuration;
  unsigned char interface;
  unsigned char alternative;
  unsigned char inputEndpoint;
  unsigned char outputEndpoint;
  unsigned disableAutosuspend:1;
  const SerialParameters *serial;
  const void *data;
} UsbChannelDefinition;

typedef struct {
  UsbChannelDefinition definition;
  UsbDevice *device;
} UsbChannel;

extern UsbChannel *usbFindChannel (const UsbChannelDefinition *definitions, const char *device);
extern void usbCloseChannel (UsbChannel *channel);

extern int isUsbDevice (const char **path);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_IO_USB */
