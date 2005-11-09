/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#include "prologue.h"

#include <errno.h>
#include <sys/socket.h>

#include "misc.h"
#include "io_bluetooth.h"
#include "bluetooth_internal.h"

#include <bluetooth/rfcomm.h>

int
openRfcommConnection (const char *address, unsigned char channel) {
  bdaddr_t bda;
  if (parseBluetoothAddress(&bda, address)) {
    int connection;
    if ((connection = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)) != -1) {
      struct sockaddr_rc local;
      local.rc_family = AF_BLUETOOTH;
      local.rc_channel = 0;
      bacpy(&local.rc_bdaddr, BDADDR_ANY); /* Any HCI. No support for explicit
                                            * interface specification yet.
                                            */
      if (bind(connection, (struct sockaddr *)&local, sizeof(local)) != -1) {
        struct sockaddr_rc remote;
        remote.rc_family = AF_BLUETOOTH;
        remote.rc_channel = channel;
        bacpy(&remote.rc_bdaddr, &bda);
        if (connect(connection, (struct sockaddr *)&remote, sizeof(remote)) != -1) {
          return connection;
        } else if (errno != EHOSTDOWN) {
          LogError("RFCOMM socket connection");
        }
      } else {
        LogError("RFCOMM socket bind");
      }

      close(connection);
    } else {
      LogError("RFCOMM socket creation");
    }
  } else {
    LogPrint(LOG_ERR, "Invalid Bluetooth address: %s", address);
  }
  return -1;
}
