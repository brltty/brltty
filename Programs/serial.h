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
extern void rawSerialDevice (struct termios *attributes);
extern int setSerialDevice (int descriptor, struct termios *attributes, speed_t baud);
extern int resetSerialDevice (int descriptor, struct termios *attributes, speed_t baud);

extern int flushSerialInput (int descriptor);
extern int flushSerialOutput (int descriptor);

extern int validateBaud (speed_t *value, const char *description, const char *word, const unsigned int *choices);
extern int baud2integer (speed_t baud);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SERIAL_H */
