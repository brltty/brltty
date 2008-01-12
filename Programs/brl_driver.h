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

#ifndef BRLTTY_INCLUDED_BRL_DRIVER
#define BRLTTY_INCLUDED_BRL_DRIVER

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* this header file is used to create the driver structure
 * for a dynamically loadable braille display driver.
 */

#include "brl.h"

/* Routines provided by this braille display driver. */
static int brl_construct (BrailleDisplay *, char **parameters, const char *);
static void brl_destruct (BrailleDisplay *);
static int brl_readCommand (BrailleDisplay *, BRL_DriverCommandContext);
static void brl_writeWindow (BrailleDisplay *);

#ifdef BRL_HAVE_STATUS_CELLS
  static void brl_writeStatus (BrailleDisplay *brl, const unsigned char *);
#endif /* BRL_HAVE_STATUS_CELLS */

#ifdef BRL_HAVE_VISUAL_DISPLAY
  static void brl_writeVisual (BrailleDisplay *);
#endif /* BRL_HAVE_VISUAL_DISPLAY */

#ifdef BRL_HAVE_PACKET_IO
  static ssize_t brl_readPacket (BrailleDisplay *, void *, size_t);
  static ssize_t brl_writePacket (BrailleDisplay *, const void *, size_t);
  static int brl_reset (BrailleDisplay *);
#endif /* BRL_HAVE_PACKET_IO */

#ifdef BRL_HAVE_KEY_CODES
  static int brl_readKey (BrailleDisplay *);
  static int brl_keyToCommand (BrailleDisplay *, BRL_DriverCommandContext, int);
#endif /* BRL_HAVE_KEY_CODES */

#ifdef BRL_HAVE_FIRMNESS
  static void brl_firmness (BrailleDisplay *brl, BrailleFirmness setting);
#endif /* BRL_HAVE_FIRMNESS */

#ifdef BRL_HAVE_SENSITIVITY
  static void brl_sensitivity (BrailleDisplay *brl, BrailleSensitivity setting);
#endif /* BRL_HAVE_SENSITIVITY */

#ifdef BRLPARMS
  static const char *const brl_parameters[] = {BRLPARMS, NULL};
#endif /* BRLPARMS */

#ifndef BRLSTAT
#  define BRLSTAT ST_None
#endif /* BRLSTAT */

#ifndef BRLSYMBOL
#  define BRLSYMBOL CONCATENATE(brl_driver_,DRIVER_CODE)
#endif /* BRLSYMBOL */

#ifndef BRLCONST
#  define BRLCONST const
#endif /* BRLCONST */

extern BRLCONST BrailleDriver BRLSYMBOL;
BRLCONST BrailleDriver BRLSYMBOL = {
  DRIVER_DEFINITION_INITIALIZER,

#ifdef BRLPARMS
  brl_parameters,
#else /* BRLPARMS */
  NULL,
#endif /* BRLPARMS */

  BRLHELP,
  BRLSTAT,

  brl_construct,
  brl_destruct,
  brl_readCommand,
  brl_writeWindow,

#ifdef BRL_HAVE_STATUS_CELLS
  brl_writeStatus,
#else /* BRL_HAVE_STATUS_CELLS */
  NULL, /* brl_writeStatus */
#endif /* BRL_HAVE_STATUS_CELLS */

#ifdef BRL_HAVE_VISUAL_DISPLAY
  brl_writeVisual,
#else /* BRL_HAVE_VISUAL_DISPLAY */
  NULL, /* brl_writeVisual */
#endif /* BRL_HAVE_VISUAL_DISPLAY */

#ifdef BRL_HAVE_PACKET_IO
  brl_readPacket,
  brl_writePacket,
  brl_reset,
#else /* BRL_HAVE_PACKET_IO */
  NULL, /* brl_readPacket */
  NULL, /* brl_writePacket */
  NULL, /* brl_reset */
#endif /* BRL_HAVE_PACKET_IO */

#ifdef BRL_HAVE_KEY_CODES
  brl_readKey,
  brl_keyToCommand,
#else /* BRL_HAVE_KEY_CODES */
  NULL, /* brl_readKey */
  NULL, /* brl_keyToCommand */
#endif /* BRL_HAVE_KEY_CODES */

#ifdef BRL_HAVE_FIRMNESS
  brl_firmness,
#else /* BRL_HAVE_FIRMNESS */
  NULL, /* brl_firmness */
#endif /* BRL_HAVE_FIRMNESS */

#ifdef BRL_HAVE_SENSITIVITY
  brl_sensitivity
#else /* BRL_HAVE_SENSITIVITY */
  NULL  /* brl_sensitivity */
#endif /* BRL_HAVE_SENSITIVITY */
};

DRIVER_VERSION_DECLARATION(brl);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BRL_DRIVER */
