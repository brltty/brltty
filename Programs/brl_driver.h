/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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
 * BRLNAME, BRLDRIVER, BRLHELP, and PREFSTYLE must be defined - see brlconf.h
 */

#include "brl.h"

/* Routines provided by this braille display driver. */
static void brl_identify (void);/* print start-up messages */
static void brl_initialize (char **parameters, brldim *, const char *); /* initialise Braille display */
static void brl_close (brldim *); /* close braille display */
static void brl_writeWindow (brldim *); /* write to braille display */
static int brl_read (DriverCommandContext);	/* get key press from braille display */
static void brl_writeStatus (const unsigned char *);	/* set status cells */

#ifdef BRLPARMS
  static const char *const brl_parameters[] = {BRLPARMS, NULL};
#endif /* BRLPARMS */

#ifndef BRLSYMBOL
#  define BRLSYMBOL brl_driver
#endif /* BRLSYMBOL */
#ifndef BRLCONST
#   define BRLCONST const
#endif /* BRLCONST */
extern BRLCONST BrailleDriver BRLSYMBOL;
BRLCONST BrailleDriver BRLSYMBOL = {
  BRLNAME,
  BRLDRIVER,

  #ifdef BRLPARMS
    brl_parameters,
  #else
    NULL,
  #endif

  BRLHELP,
  PREFSTYLE,

  brl_identify,
  brl_initialize,
  brl_close,
  brl_writeWindow,
  brl_read,
  brl_writeStatus
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BRL_DRIVER_H */
