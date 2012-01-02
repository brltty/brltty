/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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

/** EuroBraille/eu_bluetooth.c 
 ** Implements the Bluetooth low-level transport methods
 */

#include "prologue.h"

#include "log.h"
#include "io_bluetooth.h"
#include "eu_io.h"

#define	DEFAULT_ESYS_CHANNEL	1

static BluetoothConnection *bluetoothConnection = NULL;

int
eubrl_bluetoothInit (BrailleDisplay *brl, char **parameters, const char *device) 
{
  bluetoothConnection = bthOpenConnection(device, DEFAULT_ESYS_CHANNEL, 0);
  if (!bluetoothConnection)
    {
      logMessage(LOG_ERR, "eu: Failed to initialize bluetooth connection.");
      return (0);
    }
  return (1);
}

ssize_t
eubrl_bluetoothRead (BrailleDisplay *brl, void *buffer, size_t length, int wait)
{
  int timeout = 10;
  return bthReadData(bluetoothConnection, buffer, length, (wait? timeout: 0), timeout);
}

ssize_t
eubrl_bluetoothWrite (BrailleDisplay *brl, const void *buf, size_t length)
{
  ssize_t count = bthWriteData(bluetoothConnection, buf, length);
  if (count != length) 
    {
      if (count == -1) 
	{
	  logSystemError("EuroBraille Bluetooth write");
	} 
      else 
	{
	  logMessage(LOG_WARNING, "Trunccated bluetooth write: %d < %u", 
		     (int)count, (unsigned int)length);
	}
    }
  return count;
}

int
eubrl_bluetoothClose (BrailleDisplay *brl) 
{
  bthCloseConnection(bluetoothConnection);
  bluetoothConnection = NULL;
  return 0;
}
