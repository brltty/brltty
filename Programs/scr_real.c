/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else /* HAVE_SYS_SELECT_H */
#include <sys/time.h>
#endif /* HAVE_SYS_SELECT_H */

#include "misc.h"
#include "drivers.h"
#include "scr.h"
#include "scr_real.h"
#include "scr.auto.h"
#include "route.h"

#define SCRSYMBOL noScreen
#define DRIVER_NAME NoScreen
#define DRIVER_CODE no
#define DRIVER_COMMENT "no screen support"
#define DRIVER_VERSION ""
#define DRIVER_COPYRIGHT ""
#include "scr_driver.h"

static void
scr_initialize (MainScreen *main) {
  initializeMainScreen(main);
}

const ScreenDriver *
loadScreenDriver (const char *code, void **driverObject, const char *driverDirectory) {
  return loadDriver(code, driverObject,
                    driverDirectory, driverTable,
                    "screen", 'x', "scr_driver",
                    &noScreen, &noScreen.definition);
}

void
identifyScreenDriver (const ScreenDriver *driver, int full) {
  identifyDriver("Screen", &driver->definition, full);
}

void
identifyScreenDrivers (int full) {
  const DriverEntry *entry = driverTable;
  while (entry->address) {
    const ScreenDriver *driver = entry++->address;
    identifyScreenDriver(driver, full);
  }
}

#ifdef HAVE_LIBGPM
#include <gpm.h>
extern int gpm_tried;

static int
gpmOpenConnection (void) {
  if (!gpm_flag) {
    int wasTried = gpm_tried;
    Gpm_Connect connection;
    memset(&connection, 0, sizeof(connection));
    connection.eventMask = GPM_MOVE;
    connection.defaultMask = ~0;
    connection.minMod = 0;
    connection.maxMod = ~0;

    gpm_tried = 0;
    gpm_zerobased = 1;

    if (Gpm_Open(&connection, -1) == -1) {
      if (!wasTried) LogPrint(LOG_WARNING, "GPM open error: %s", strerror(errno));
      return 0;
    }
    LogPrint(LOG_DEBUG, "GPM opened: fd=%d", gpm_fd);
  }
  return 1;
}

static void
gpmCloseConnection (void) {
  Gpm_Close();
  LogPrint(LOG_DEBUG, "GPM closed.");
}
#endif /* HAVE_LIBGPM */

static int
route_RealScreen (int column, int row, int screen) {
  return startCursorRouting(column, row, screen);
}

static int
point_RealScreen (int column, int row) {
#ifdef HAVE_LIBGPM
  if (gpmOpenConnection()) {
    Gpm_Event event;
    event.x = column;
    event.y = row;
    if (GPM_DRAWPOINTER(&event) != -1) return 1;
    gpmCloseConnection();
  }
#endif /* HAVE_LIBGPM */
  return 0;
}

static int
pointer_RealScreen (int *column, int *row) {
#ifdef HAVE_LIBGPM
  if (gpmOpenConnection()) {
    if (gpm_fd >= 0) {
      int ok = 0;
      int error = 0;
      while (1) {
        fd_set mask;
        struct timeval timeout;
        int count;
        Gpm_Event event;
        FD_ZERO(&mask);
        FD_SET(gpm_fd, &mask);
        memset(&timeout, 0, sizeof(timeout));

        if ((count = select(gpm_fd+1, &mask, NULL, NULL, &timeout)) == 0) break;
        error = 1;
        if (count < 0) break;
        if (!FD_ISSET(gpm_fd, &mask)) break;

        if (Gpm_GetEvent(&event) != 1) break;
        error = 0;

        *column = event.x;
        *row = event.y;
        ok = 1;
      }

      if (error) gpmCloseConnection();
      return ok;
    }
  }
#endif /* HAVE_LIBGPM */
  return 0;
}

void
initializeRealScreen (MainScreen *main) {
  initializeMainScreen(main);
  main->base.route = route_RealScreen;
  main->base.point = point_RealScreen;
  main->base.pointer = pointer_RealScreen;
}
