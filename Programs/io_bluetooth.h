/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_IO_BLUETOOTH
#define BRLTTY_INCLUDED_IO_BLUETOOTH

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct BluetoothConnectionStruct BluetoothConnection;

typedef struct BluetoothDeviceAddressStruct BluetoothDeviceAddress;

extern int isBluetoothDevice (const char **path);

extern void btForgetConnectErrors (void);

extern BluetoothConnection *btOpenConnection (const char *address, unsigned char channel, int force);
extern void btCloseConnection (BluetoothConnection *connection);
extern int btAwaitInput (BluetoothConnection *connection, int milliseconds);
extern ssize_t btReadData (
  BluetoothConnection *connection, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
);
extern ssize_t btWriteData (BluetoothConnection *conection, const void *buffer, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_IO_BLUETOOTH */
