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

#ifndef BRLTTY_INCLUDED_SCR_REAL
#define BRLTTY_INCLUDED_SCR_REAL

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "driver.h"
#include "scr_main.h"

typedef struct {
  DRIVER_DEFINITION_DECLARATION;
  const char *const *parameters;

  void (*initialize) (MainScreen *main);		/* initialize speech device */
} ScreenDriver;

extern void initializeRealScreen (MainScreen *);

extern const ScreenDriver noScreen;
extern const ScreenDriver *loadScreenDriver (const char *code, void **driverObject, const char *driverDirectory);
extern void identifyScreenDrivers (int full);
extern void identifyScreenDriver (const ScreenDriver *driver, int full);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SCR_REAL */
