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

#ifndef _SERIAL_H
#define _SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>

typedef struct SerialDeviceStruct SerialDevice;

extern int isSerialDevice (const char **path);
extern int serialValidateBaud (int *baud, const char *description, const char *word, const int *choices);

extern SerialDevice *serialOpenDevice (const char *path);
extern void serialCloseDevice (SerialDevice *serial);
extern int serialRestartDevice (SerialDevice *serial, int baud);
extern FILE *serialGetStream (SerialDevice *serial);

extern int serialAwaitInput (SerialDevice *serial, int timeout);
extern int serialReadData (
  SerialDevice *serial,
  void *buffer, int size,
  int initialTimeout, int subsequentTimeout
);
extern int serialReadChunk (
  SerialDevice *serial,
  unsigned char *buffer, int *offset, int count,
  int initialTimeout, int subsequentTimeout
);
extern int serialWriteData (
  SerialDevice *serial,
  const void *data, int size
);

extern int serialSetBaud (SerialDevice *serial, int baud);
extern int serialSetDataBits (SerialDevice *serial, int bits);
extern int serialSetStopBits (SerialDevice *serial, int bits);

typedef enum {
  SERIAL_PARITY_NONE,
  SERIAL_PARITY_ODD,
  SERIAL_PARITY_EVEN
} SerialParity;
extern int serialSetParity (SerialDevice *serial, SerialParity parity);

typedef enum {
  SERIAL_FLOW_HARDWARE          = 0X1,
  SERIAL_FLOW_SOFTWARE_INPUT    = 0X2,
  SERIAL_FLOW_SOFTWARE_OUTPUT   = 0X4,
  SERIAL_FLOW_NONE              = 0X0
} SerialFlowControl;
extern int serialSetFlowControl (SerialDevice *serial, SerialFlowControl flow);

extern int serialDiscardInput (SerialDevice *serial);
extern int serialDiscardOutput (SerialDevice *serial);
extern int serialDrainOutput (SerialDevice *serial);

extern int serialTestLineCTS (SerialDevice *serial);
extern int serialTestLineDSR (SerialDevice *serial);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SERIAL_H */
