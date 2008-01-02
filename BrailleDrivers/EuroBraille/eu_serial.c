/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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

/** EuroBraille/eu_serial.c
 ** Implements the Serial low-level transport methods
 */

#include "prologue.h"

#include "eu_io.h"
#include "eu_protocol.h"
#include "io_serial.h"

/** Static Members **/
static SerialDevice *serialDevice = NULL;

int
eubrl_serialInit(BrailleDisplay *brl, char **parameters, const char *device) 
{
  if ((serialDevice = serialOpenDevice(device))) 
    {
      serialSetParity(serialDevice, SERIAL_PARITY_EVEN);
      if (serialRestartDevice(serialDevice, SPEED)) 
	{
	  return 1;
	}
      serialCloseDevice(serialDevice);
      serialDevice = NULL;
    }
  return 0;
}

int
eubrl_serialClose (BrailleDisplay *brl) {
  if (serialDevice) 
    {
      serialCloseDevice(serialDevice);
      serialDevice = NULL;
    }
  return 0;
}

ssize_t
eubrl_serialRead (BrailleDisplay *brl, void *buf, size_t length) 
{
  return serialReadData(serialDevice, buf, length, 0, 0);
}

ssize_t
eubrl_serialWrite (BrailleDisplay *brl, const void *buffer, size_t length)
{
  return serialWriteData(serialDevice, buffer, length);
}

