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

#include <termios.h>

extern int isSerialDevice (const char **path);
extern int openSerialDevice (const char *path, int *descriptor, struct termios *attributes);
extern int setSerialDevice (int descriptor, struct termios *attributes, speed_t baud);
extern int resetSerialDevice (int descriptor, struct termios *attributes, speed_t baud);
extern void rawSerialDevice (struct termios *attributes);

extern int validateBaud (speed_t *value, const char *description, const char *word, const unsigned int *choices);
extern int baud2integer (speed_t baud);

extern void initializeSerialAttributes (struct termios *attributes);
extern int setSerialSpeed (struct termios *attributes, int speed);
extern int setSerialDataBits (struct termios *attributes, int bits);
extern int setSerialStopBits (struct termios *attributes, int bits);

typedef enum {
  SERIAL_PARITY_NONE,
  SERIAL_PARITY_ODD,
  SERIAL_PARITY_EVEN
} SerialParity;
extern int setSerialParity (struct termios *attributes, SerialParity parity);

typedef enum {
  SERIAL_FLOW_HARDWARE          = 0X1,
  SERIAL_FLOW_SOFTWARE_INPUT    = 0X2,
  SERIAL_FLOW_SOFTWARE_OUTPUT   = 0X4,
  SERIAL_FLOW_NONE              = 0X0
} SerialFlowControl;
extern int setSerialFlowControl (struct termios *attributes, SerialFlowControl flow);

extern int applySerialAttributes (const struct termios *attributes, int descriptor);
extern int flushSerialInput (int descriptor);
extern int flushSerialOutput (int descriptor);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SERIAL_H */
