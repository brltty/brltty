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

#ifndef BRLTTY_INCLUDED_GIO_INTERNAL
#define BRLTTY_INCLUDED_GIO_INTERNAL

#include "io_serial.h"
#include "io_usb.h"
#include "io_bluetooth.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef union {
  struct {
    SerialDevice *device;
  } serial;

  struct {
    UsbChannel *channel;
  } usb;

  struct {
    BluetoothConnection *connection;
  } bluetooth;
} GioHandle;

typedef struct {
  void *address;
  size_t size;
} HidReportItemsData;

typedef int DisconnectResourceMethod (GioHandle *handle);

typedef char *GetResourceNameMethod (GioHandle *handle, int timeout);

typedef ssize_t WriteDataMethod (GioHandle *handle, const void *data, size_t size, int timeout);

typedef int AwaitInputMethod (GioHandle *handle, int timeout);

typedef ssize_t ReadDataMethod (
  GioHandle *handle, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
);

typedef int ReconfigureResourceMethod (GioHandle *handle, const SerialParameters *parameters);

typedef ssize_t TellResourceMethod (
  GioHandle *handle, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  const void *data, uint16_t size, int timeout
);

typedef ssize_t AskResourceMethod (
  GioHandle *handle, uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  void *buffer, uint16_t size, int timeout
);

typedef int GetHidReportItemsMethod (GioHandle *handle, HidReportItemsData *items, int timeout);

typedef size_t GetHidReportSizeMethod (const HidReportItemsData *items, unsigned char report);

typedef ssize_t SetHidReportMethod (
  GioHandle *handle, unsigned char report,
  const void *data, uint16_t size, int timeout
);

typedef ssize_t GetHidReportMethod (
  GioHandle *handle, unsigned char report,
  void *buffer, uint16_t size, int timeout
);

typedef ssize_t SetHidFeatureMethod (
  GioHandle *handle, unsigned char report,
  const void *data, uint16_t size, int timeout
);

typedef ssize_t GetHidFeatureMethod (
  GioHandle *handle, unsigned char report,
  void *buffer, uint16_t size, int timeout
);

typedef struct {
  DisconnectResourceMethod *disconnectResource;
  GetResourceNameMethod *getResourceName;

  WriteDataMethod *writeData;
  AwaitInputMethod *awaitInput;
  ReadDataMethod *readData;

  ReconfigureResourceMethod *reconfigureResource;

  TellResourceMethod *tellResource;
  AskResourceMethod *askResource;

  GetHidReportItemsMethod *getHidReportItems;
  GetHidReportSizeMethod *getHidReportSize;

  SetHidReportMethod *setHidReport;
  GetHidReportMethod *getHidReport;

  SetHidFeatureMethod *setHidFeature;
  GetHidFeatureMethod *getHidFeature;
} GioMethods;

struct GioEndpointStruct {
  GioHandle handle;
  const GioMethods *methods;
  GioOptions options;
  unsigned int bytesPerSecond;
  HidReportItemsData hidReportItems;

  struct {
    int error;
    unsigned int from;
    unsigned int to;
    unsigned char buffer[0X40];
  } input;
};

extern const GioMethods gioSerialMethods;
extern const GioMethods gioUsbMethods;
extern const GioMethods gioBluetoothMethods;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_GIO_INTERNAL */
