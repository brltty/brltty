/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

#ifndef _BRL_DRIVER_H
#define _BRL_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* this header file is used to create the driver structure
 * for a dynamically loadable braille display driver.
 * BRLNAME, BRLCODE, and BRLHELP must be defined - see driver make file
 */

#include "brl.h"

/* Routines provided by this braille display driver. */
static void brl_identify (void);
static int brl_open (BrailleDisplay *, char **parameters, const char *);
static void brl_close (BrailleDisplay *);
static int brl_readCommand (BrailleDisplay *, DriverCommandContext);
static void brl_writeWindow (BrailleDisplay *);
static void brl_writeStatus (BrailleDisplay *brl, const unsigned char *);

#ifdef BRL_HAVE_VISUAL_DISPLAY
  static void brl_writeVisual (BrailleDisplay *);
#endif /* BRL_HAVE_VISUAL_DISPLAY */

#ifdef BRL_HAVE_PACKET_IO
  static int brl_readPacket (BrailleDisplay *, unsigned char *, int);
  static int brl_writePacket (BrailleDisplay *, const unsigned char *, int);
  static int brl_reset (BrailleDisplay *);
#endif /* BRL_HAVE_PACKET_IO */

#ifdef BRL_HAVE_KEY_CODES
  static int brl_readKey (BrailleDisplay *);
  static int brl_keyToCommand (BrailleDisplay *, DriverCommandContext, int);
#endif /* BRL_HAVE_KEY_CODES */

#ifdef BRLPARMS
  static const char *const brl_parameters[] = {BRLPARMS, NULL};
#endif /* BRLPARMS */

#ifndef BRLSTAT
#  define BRLSTAT ST_None
#endif /* BRLSTAT */

#ifndef BRLSYMBOL
#  define BRLSYMBOL CONCATENATE(brl_driver_,BRLCODE)
#endif /* BRLSYMBOL */

#ifndef BRLCONST
#  define BRLCONST const
#endif /* BRLCONST */

extern BRLCONST BrailleDriver BRLSYMBOL;
BRLCONST BrailleDriver BRLSYMBOL = {
  STRINGIFY(BRLNAME),
  STRINGIFY(BRLCODE),
  __DATE__,
  __TIME__,

#ifdef BRLPARMS
  brl_parameters,
#else /* BRLPARMS */
  NULL,
#endif /* BRLPARMS */

  BRLHELP,
  BRLSTAT,

  brl_identify,
  brl_open,
  brl_close,
  brl_readCommand,
  brl_writeWindow,
  brl_writeStatus,

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
  brl_keyToCommand
#else /* BRL_HAVE_KEY_CODES */
  NULL, /* brl_readKey */
  NULL  /* brl_keyToCommand */
#endif /* BRL_HAVE_KEY_CODES */
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BRL_DRIVER_H */
