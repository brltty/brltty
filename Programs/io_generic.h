/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2011 by The BRLTTY Developers.
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
} InputOutputEndpointAttributes;

typedef struct {
  struct {
    const SerialParameters *parameters;
    InputOutputEndpointAttributes attributes;
  } serial;

  struct {
    const UsbChannelDefinition *channelDefinitions;
    InputOutputEndpointAttributes attributes;
  } usb;

  struct {
    uint8_t channelNumber;
    InputOutputEndpointAttributes attributes;
  } bluetooth;
} InputOutputEndpointSpecification;

extern void ioInitializeEndpointSpecification (InputOutputEndpointSpecification *specification);
extern void ioInitializeSerialParameters (SerialParameters *parameters);

typedef struct InputOutputEndpointStruct InputOutputEndpoint;

extern InputOutputEndpoint *ioConnectResource (
  const char *identifier,
  const InputOutputEndpointSpecification *specification
);
extern int ioDisconnectResource (InputOutputEndpoint *endpoint);
extern const void *ioGetApplicationData (InputOutputEndpoint *endpoint);

extern ssize_t ioWriteData (InputOutputEndpoint *endpoint, const void *data, size_t size);
extern int ioAwaitInput (InputOutputEndpoint *endpoint, int timeout);
extern ssize_t ioReadData (InputOutputEndpoint *endpoint, void *buffer, size_t size, int wait);
extern int ioReadByte (InputOutputEndpoint *endpoint, unsigned char *byte, int wait);

extern int ioReconfigureResource (
  InputOutputEndpoint *endpoint,
  const SerialParameters *parameters
);

extern unsigned int ioGetBytesPerSecond (InputOutputEndpoint *endpoint);
extern unsigned int ioGetMillisecondsToTransfer (InputOutputEndpoint *endpoint, size_t bytes);

extern ssize_t ioTellResource (
  InputOutputEndpoint *endpoint,
  uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  const void *data, uint16_t size
);

extern ssize_t ioAskResource (
  InputOutputEndpoint *endpoint,
  uint8_t recipient, uint8_t type,
  uint8_t request, uint16_t value, uint16_t index,
  void *buffer, uint16_t size
);

extern size_t ioGetHidReportSize (InputOutputEndpoint *endpoint, unsigned char report);

extern ssize_t ioSetHidReport (
  InputOutputEndpoint *endpoint,
  unsigned char interface, unsigned char report,
  const void *data, uint16_t size
);

extern ssize_t ioGetHidReport (
  InputOutputEndpoint *endpoint,
  unsigned char interface, unsigned char report,
  void *buffer, uint16_t size
);

extern ssize_t ioSetHidFeature (
  InputOutputEndpoint *endpoint,
  unsigned char interface, unsigned char report,
  const void *data, uint16_t size
);

extern ssize_t ioGetHidFeature (
  InputOutputEndpoint *endpoint,
  unsigned char interface, unsigned char report,
  void *buffer, uint16_t size
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_IO_GENERIC */
