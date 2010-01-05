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

/**
 ** io.h -- Implements the IO structures and functions headers.
 */

#ifndef __EU_IO_H__
#define __EU_IO_H__

#include	"eu_braille.h"




/*
** IO structure operations
*/

typedef enum	e_eubrl_iotype {
  IO_UNINITIALIZED = 0,
  IO_SERIAL,
  IO_USB,
  IO_BLUETOOTH,
  IO_ETHERNET
}		t_eubrl_iotype;

typedef struct	s_eubrl_io
{
  int			(*init)(BrailleDisplay *brl, char **params, const char* device);
  int			(*close)(BrailleDisplay *brl);
  ssize_t		(*read)(BrailleDisplay *brl, void *buffer, size_t bufsize);
  ssize_t		(*write)(BrailleDisplay *brl, const void *buf, size_t size);
  
  t_eubrl_iotype	ioType;
  void*			*private;
}			t_eubrl_io;

/*
** IO functions headers
*/

/* USB IO */
int		eubrl_usbInit(BrailleDisplay *brl, char **params, const char *device);
int		eubrl_usbClose(BrailleDisplay *brl);
ssize_t		eubrl_usbRead(BrailleDisplay *brl, void *buf, size_t size);
ssize_t		eubrl_usbWrite(BrailleDisplay *brl, const void *buf, size_t size);


/* Serial  IO */

int		eubrl_serialInit(BrailleDisplay *brl, char **params, const char *device);
int		eubrl_serialClose(BrailleDisplay *brl);
ssize_t		eubrl_serialRead(BrailleDisplay *brl, void *buf, size_t size);
ssize_t		eubrl_serialWrite(BrailleDisplay *brl, const void *buf, size_t size);

/* Bluetooth IO */
int		eubrl_bluetoothInit(BrailleDisplay *brl, char **params, const char *device);
int		eubrl_bluetoothClose(BrailleDisplay *brl);
ssize_t		eubrl_bluetoothRead(BrailleDisplay *brl, void *buf, size_t size);
ssize_t		eubrl_bluetoothWrite(BrailleDisplay *brl, const void *buf, size_t size);

/* Ethernet IO */

int		eubrl_netInit(BrailleDisplay *brl, char **params, const char *device);
int		eubrl_netClose(BrailleDisplay *brl);
ssize_t		eubrl_netRead(BrailleDisplay *brl, void *buf, size_t size);
ssize_t		eubrl_netWrite(BrailleDisplay *brl, const void *buf, size_t size);

#endif /* __EU_IO_H__ */
