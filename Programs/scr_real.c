/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "misc.h"
#include "route.h"
#include "scr.h"
#include "scr_real.h"

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
  return csrjmp(column, row, screen);
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

static const char *const screenParameters[] = {
  NULL
};
static const char *const *
parameters_RealScreen (void) {
  return screenParameters;
}

static int
prepare_RealScreen (char **parameters) {
  return 1;
}

static int
open_RealScreen (void) {
  return 1;
}

static int
setup_RealScreen (void) {
  return 1;
}

static void
close_RealScreen (void) {
}

void
initializeRealScreen (RealScreen *real) {
  initializeBaseScreen(&real->base);
  real->base.route = route_RealScreen;
  real->base.point = point_RealScreen;
  real->base.pointer = pointer_RealScreen;
  real->parameters = parameters_RealScreen;
  real->prepare = prepare_RealScreen;
  real->open = open_RealScreen;
  real->setup = setup_RealScreen;
  real->close = close_RealScreen;
}
