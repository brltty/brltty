/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
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

#include "prologue.h"

#include <errno.h>
#include <sys/socket.h>

#include "misc.h"
#include "io_bluetooth.h"
#include "bluetooth_internal.h"

#include <bluetooth/rfcomm.h>

int
btConnect (bdaddr_t address, unsigned char channel) {
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
      bacpy(&remote.rc_bdaddr, &address);
      if (connect(connection, (struct sockaddr *)&remote, sizeof(remote)) != -1) {
        return connection;
      } else if ((errno != EHOSTDOWN) && (errno != EHOSTUNREACH)) {
        LogError("RFCOMM socket connect");
      }
    } else {
      LogError("RFCOMM socket bind");
    }

    close(connection);
  } else {
    LogError("RFCOMM socket allocate");
  }
  return -1;
}
