/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Team. All rights reserved.
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

#ifndef BRLTTY_INCLUDED_IO_BLUETOOTH
#define BRLTTY_INCLUDED_IO_BLUETOOTH

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int isBluetoothDevice (const char **path);

extern void btForgetConnectErrors (void);

extern int btOpenConnection (const char *address, unsigned char channel, int force);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_IO_BLUETOOTH */
