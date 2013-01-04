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

#ifndef BRLTTY_INCLUDED_IO_GENERIC
#define BRLTTY_INCLUDED_IO_GENERIC

#include "serialdefs.h"
#include "usbdefs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  const void *applicationData;
  int readyDelay;
  int inputTimeout;
  int outputTimeout;
} GioOptions;

typedef struct {
  struct {
    const SerialParameters *parameters;
    GioOptions options;
  } serial;

  struct {
    const UsbChannelDefinition *channelDefinitions;
    GioOptions options;
  } usb;

  struct {
    uint8_t channelNumber;
    GioOptions options;
  } bluetooth;
} GioDescriptor;

extern void gioInitializeDescriptor (GioDescriptor *descriptor);
extern void gioInitializeSerialParameters (SerialParameters *parameters);

typedef struct GioEndpointStruct GioEndpoint;

extern GioEndpoint *gioConnectResource (
  const char *identifier,
  const GioDescriptor *descriptor
);
extern int gioDisconnectResource (GioEndpoint *endpoint);
extern const void *gioGetApplicationData (GioEndpoint *endpoint);

extern ssize_t gioWriteData (GioEndpoint *endpoint, const void *data, size_t size);
extern int gioAwaitInput (GioEndpoint *endpoint, int timeout);
extern ssize_t gioReadData (GioEndpoint *endpoint, void *buffer, size_t size, int wait);
extern int gioReadByte (GioEndpoint *endpoint, unsigned char *byte, int wait);
extern int gioDiscardInput (GioEndpoint *endpoint);

extern int gioReconfigureResource (
  GioEndpoint *endpoint,
  const SerialParameters *parameters
);

extern unsigned int gioGetBytesPerSecond (GioEndpoint *endpoint);
extern unsigned int gioGetMillisecondsToTransfer (GioEndpoint *endpoint, size_t bytes);

extern ssize_t gioTellResource (
  GioEndpoint *endpoint,
  uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  const void *data, uint16_t size
);

extern ssize_t gioAskResource (
  GioEndpoint *endpoint,
  uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  void *buffer, uint16_t size
);

extern size_t gioGetHidReportSize (GioEndpoint *endpoint, unsigned char report);

extern ssize_t gioSetHidReport (
  GioEndpoint *endpoint, unsigned char report,
  const void *data, uint16_t size
);

extern ssize_t gioGetHidReport (
  GioEndpoint *endpoint, unsigned char report,
  void *buffer, uint16_t size
);

extern ssize_t gioSetHidFeature (
  GioEndpoint *endpoint, unsigned char report,
  const void *data, uint16_t size
);

extern ssize_t gioGetHidFeature (
  GioEndpoint *endpoint, unsigned char report,
  void *buffer, uint16_t size
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_IO_GENERIC */
