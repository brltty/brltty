/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

/** EuroBraille/eu_bluetooth.c 
 ** Implements the Bluetooth low-level transport methods
 */

#include "prologue.h"

#include "misc.h"
#include "io_misc.h"
#include "io_bluetooth.h"
#include "eu_io.h"

#ifdef ENABLE_BLUETOOTH_SUPPORT

#define	DEFAULT_ESYS_CHANNEL	1

static int bluetoothConnection = -1;

int
eubrl_bluetoothInit (BrailleDisplay *brl, char **parameters, const char *device) 
{
  bluetoothConnection = btOpenConnection(device, DEFAULT_ESYS_CHANNEL, 0);
  if (bluetoothConnection < 0)
    {
      LogPrint(LOG_ERR, "eu: Failed to initialize bluetooth connection.");
      return (0);
    }
  return (1);
}

ssize_t
eubrl_bluetoothRead (BrailleDisplay *brl, void *buffer, size_t length)
{
  const int timeout = 100;
  return readData(bluetoothConnection, buffer, length, 0, timeout);
}

ssize_t
eubrl_bluetoothWrite (BrailleDisplay *brl, const void *buf, size_t length)
{
  ssize_t count = writeData(bluetoothConnection, buf, length);
  if (count != length) 
    {
      if (count == -1) 
	{
	  LogError("EuroBraille Bluetooth write error");
	} 
      else 
	{
	  LogPrint(LOG_WARNING, "Trunccated bluetooth write: %d < %d", 
		   count, length);
	}
    }
  return count;
}

int
eubrl_bluetoothClose (BrailleDisplay *brl) 
{
  close(bluetoothConnection);
  bluetoothConnection = -1;
  return 0;
}

#else /* No bluetooth support enabled */


int
eubrl_bluetoothInit (BrailleDisplay *brl, char **parameters, const char *device) 
{
  unsupportedDevice(device);
  return -1;
}

ssize_t
eubrl_bluetoothRead(BrailleDisplay *brl, void *buffer, size_t length)
{
  return -1;
}

ssize_t
eubrl_bluetoothWrite (BrailleDisplay *brl, const void *buf, size_t length)
{
  return 0;
}

int
eubrl_bluetoothClose (BrailleDisplay *brl) 
{
  return 0;
}


#endif /* ENABLE_BLUETOOTH_SUPPORT */
