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

#include <unistd.h>

#include "iodefs.h"

typedef struct SerialDeviceStruct SerialDevice;

extern int isSerialDevice (const char **path);
extern int serialValidateBaud (int *baud, const char *description, const char *word, const int *choices);

extern SerialDevice *serialOpenDevice (const char *path);
extern void serialCloseDevice (SerialDevice *serial);
extern int serialRestartDevice (SerialDevice *serial, int baud);
extern FILE *serialGetStream (SerialDevice *serial);

extern int serialAwaitInput (SerialDevice *serial, int timeout);
extern ssize_t serialReadData (
  SerialDevice *serial,
  void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
);
extern int serialReadChunk (
  SerialDevice *serial,
  void *buffer, size_t *offset, size_t count,
  int initialTimeout, int subsequentTimeout
);
extern ssize_t serialWriteData (
  SerialDevice *serial,
  const void *data, size_t size
);

extern int serialSetBaud (SerialDevice *serial, int baud);
extern int serialSetDataBits (SerialDevice *serial, int bits);
extern int serialSetStopBits (SerialDevice *serial, int bits);
extern int serialSetParity (SerialDevice *serial, SerialParity parity);
extern int serialSetFlowControl (SerialDevice *serial, SerialFlowControl flow);

extern int serialDiscardInput (SerialDevice *serial);
extern int serialDiscardOutput (SerialDevice *serial);
extern int serialFlushOutput (SerialDevice *serial);
extern int serialDrainOutput (SerialDevice *serial);

extern int serialTestLineCTS (SerialDevice *serial);
extern int serialTestLineDSR (SerialDevice *serial);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SERIAL_H */
