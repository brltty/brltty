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
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* EuroBraille -- Input/Output handling
** 
** In this file, we will find input and output generic routines for :
** - Serial IO - for Clios, Scribas, and IRISs models
** - USB IO - For ESYS model when in USB mode.
** - Bluetooth IO - For ESYS models in Bluetooth mode.
** - Ethernet IO - For IRIS models in Ethernet mode.
** 
** All of these different parts are merged into a `eubrl_io' structure, 
** which allows generic access for reading/writing to the high-level protocol
** stuff. See protocols.c for more details.
**
** Maintained by Yannick PLASSIARD <yan@mistigri.org>
*/

#include "prologue.h"

#include "eu_io.h"

/*
** Static structure io operations
*/

t_eubrl_io	eubrl_usbIos = {
  .init = eubrl_usbInit,
  .read = eubrl_usbRead,
  .write = eubrl_usbWrite,
  .close = eubrl_usbClose,
  .ioType = IO_USB,
  .private = NULL
};

t_eubrl_io	eubrl_serialIos = {
  .init = eubrl_serialInit,
  .read = eubrl_serialRead,
  .write = eubrl_serialWrite,
  .close = eubrl_serialClose,
  .ioType = IO_SERIAL,
  .private = NULL
};

t_eubrl_io	eubrl_bluetoothIos = {
  .init = eubrl_bluetoothInit,
  .read = eubrl_bluetoothRead,
  .write = eubrl_bluetoothWrite,
  .close = eubrl_bluetoothClose,
  .ioType = IO_BLUETOOTH,
  .private = NULL
};

#ifndef __MSDOS__
t_eubrl_io	eubrl_ethernetIos = {
  .init = eubrl_netInit,
  .read = eubrl_netRead,
  .write = eubrl_netWrite,
  .close = eubrl_netClose,
  .ioType = IO_ETHERNET,
  .private = NULL
};
#endif /* __MSDOS__ */

/*
** All the IO code is separated into the io_*.c files
*/
