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

#ifndef BRLTTY_INCLUDED_BLUETOOTH_INTERNAL
#define BRLTTY_INCLUDED_BLUETOOTH_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define BDA_SIZE 6

typedef struct BluetoothConnectionExtensionStruct BluetoothConnectionExtension;

struct BluetoothConnectionStruct {
  uint64_t address;
  uint8_t channel;
  BluetoothConnectionExtension *extension;
};

extern BluetoothConnectionExtension *bthConnect (uint64_t bda, uint8_t channel);
extern void bthDisconnect (BluetoothConnectionExtension *bcx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BLUETOOTH_INTERNAL */
