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
#include "async.h"
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
#define DRIVER_DEVELOPERS ""
#include "scr_driver.h"

static void
scr_initialize (MainScreen *main) {
  initializeMainScreen(main);
}

const ScreenDriver *
loadScreenDriver (const char *code, void **driverObject, const char *driverDirectory) {
  return loadDriver(code, driverObject,
                    driverDirectory, driverTable,
                    "screen", 'x', "scr",
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

#define GPM_LOG_LEVEL LOG_WARNING

typedef enum {
  GCS_CLOSED,
  GCS_WAITING,
  GCS_OPENED
} GpmConnectionState;

static int gpmConnectionState = GCS_CLOSED;

static int gpmOpenConnection (void);

static void
gpmRetryConnection (void *data) {
  gpmConnectionState = GCS_CLOSED;
  gpmOpenConnection();
}

static int
gpmOpenConnection (void) {
  switch (gpmConnectionState) {
    case  GCS_CLOSED: {
      Gpm_Connect options = {
        .eventMask = GPM_MOVE,
        .defaultMask = ~0,
        .minMod = 0,
        .maxMod = ~0
      };

      gpm_tried = 0;
      gpm_zerobased = 1;

      if (Gpm_Open(&options, -1) == -1) {
        LogPrint(GPM_LOG_LEVEL, "GPM open error: %s", strerror(errno));
        asyncRelativeAlarm(5000, gpmRetryConnection, NULL);
        gpmConnectionState = GCS_WAITING;
        return 0;
      }

      LogPrint(GPM_LOG_LEVEL, "GPM opened: fd=%d con=%d", gpm_fd, gpm_consolefd);
      gpmConnectionState = GCS_OPENED;
    }

    case GCS_OPENED:
      return 1;
  }

  return 0;
}

static void
gpmCloseConnection (int alreadyClosed) {
  if (!alreadyClosed) Gpm_Close();
  gpmConnectionState = GCS_CLOSED;
  LogPrint(GPM_LOG_LEVEL, "GPM closed");
}
#endif /* HAVE_LIBGPM */

static int
routeCursor_RealScreen (int column, int row, int screen) {
  return startCursorRouting(column, row, screen);
}

static int
highlightRegion_RealScreen (int left, int right, int top, int bottom) {
  int console = getConsole();

  if (console != -1) {
#ifdef HAVE_LIBGPM
    if (gpmOpenConnection() && (gpm_fd >= 0)) {
      if (Gpm_DrawPointer(left, top, console) != -1) return 1;

      if (errno != EINVAL) {
        LogPrint(GPM_LOG_LEVEL, "Gpm_DrawPointer error: %s", strerror(errno));
        gpmCloseConnection(0);
        return 0;
      }
    }
#endif /* HAVE_LIBGPM */
  }

  return 0;
}

static int
getPointer_RealScreen (int *column, int *row) {
  int ok = 0;

#ifdef HAVE_LIBGPM
  if (gpmOpenConnection()) {
    if (gpm_fd >= 0) {
      int error = 0;

      while (1) {
        fd_set mask;
        struct timeval timeout;
        Gpm_Event event;
        int result;

        FD_ZERO(&mask);
        FD_SET(gpm_fd, &mask);
        memset(&timeout, 0, sizeof(timeout));

        if ((result = select(gpm_fd+1, &mask, NULL, NULL, &timeout)) == 0) break;
        error = 1;

        if (result == -1) {
          if (errno == EINTR) continue;
          LogError("select");
          break;
        }

        if (!FD_ISSET(gpm_fd, &mask)) {
          LogPrint(GPM_LOG_LEVEL, "GPM file descriptor not set: %d", gpm_fd);
          break;
        }

        if ((result = Gpm_GetEvent(&event)) == -1) {
          if (errno == EINTR) continue;
          LogError("Gpm_GetEvent");
          break;
        }

        error = 0;
        if (result == 0) {
          gpmCloseConnection(1);
          break;
        }

        *column = event.x;
        *row = event.y;
        ok = 1;
      }

      if (error) gpmCloseConnection(0);
    }
  }
#endif /* HAVE_LIBGPM */

  return ok;
}

void
initializeRealScreen (MainScreen *main) {
  initializeMainScreen(main);
  main->base.routeCursor = routeCursor_RealScreen;
  main->base.highlightRegion = highlightRegion_RealScreen;
  main->base.getPointer = getPointer_RealScreen;
}
