/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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
  int			(*read)(BrailleDisplay *brl, char *buffer, int bufsize);
  int			(*write)(BrailleDisplay *brl, char *buf, int size);
  
  t_eubrl_iotype	ioType;
  void*			*private;
}			t_eubrl_io;

/*
** IO functions headers
*/

/* USB IO */
int		eubrl_usbInit(BrailleDisplay *brl, char **params, const char *device);
int		eubrl_usbClose(BrailleDisplay *brl);
int		eubrl_usbRead(BrailleDisplay *brl, char *buf, int size);
int		eubrl_usbWrite(BrailleDisplay *brl, char *buf, int size);


/* Serial  IO */

int		eubrl_serialInit(BrailleDisplay *brl, char **params, const char *device);
int		eubrl_serialClose(BrailleDisplay *brl);
int		eubrl_serialRead(BrailleDisplay *brl, char *buf, int size);
int		eubrl_serialWrite(BrailleDisplay *brl, char *buf, int size);

/* Bluetooth IO */
int		eubrl_bluetoothInit(BrailleDisplay *brl, char **params, const char *device);
int		eubrl_bluetoothClose(BrailleDisplay *brl);
int		eubrl_bluetoothRead(BrailleDisplay *brl, char *buf, int size);
int		eubrl_bluetoothWrite(BrailleDisplay *brl, char *buf, int size);

/* Ethernet IO */

int		eubrl_netInit(BrailleDisplay *brl, char **params, const char *device);
int		eubrl_netClose(BrailleDisplay *brl);
int		eubrl_netRead(BrailleDisplay *brl, char *buf, int size);
int		eubrl_netWrite(BrailleDisplay *brl, char *buf, int size);

#endif /* __EU_IO_H__ */
