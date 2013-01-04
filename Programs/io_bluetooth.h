/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

extern void bthForgetConnectErrors (void);

extern BluetoothConnection *bthOpenConnection (const char *address, uint8_t channel, int force);
extern void bthCloseConnection (BluetoothConnection *connection);
extern int bthAwaitInput (BluetoothConnection *connection, int milliseconds);
extern ssize_t bthReadData (
  BluetoothConnection *connection, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
);
extern ssize_t bthWriteData (BluetoothConnection *conection, const void *buffer, size_t size);

extern int isBluetoothDevice (const char **identifier);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_IO_BLUETOOTH */
