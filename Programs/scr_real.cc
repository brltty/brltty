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
    Gpm_Connect connection;
    memset(&connection, 0, sizeof(connection));
    connection.eventMask = GPM_MOVE;
    connection.defaultMask = ~0;
    connection.minMod = 0;
    connection.maxMod = ~0;

    int wasTried = gpm_tried;
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


int
RealScreen::route (int column, int row, int screen) {
  return csrjmp(column, row, screen);
}


int
RealScreen::point (int column, int row) {
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

int
RealScreen::pointer (int &column, int &row) {
#ifdef HAVE_LIBGPM
  if (gpmOpenConnection()) {
    if (gpm_fd >= 0) {
      int ok = 0;
      int error = 0;
      while (1) {
        fd_set mask;
        FD_ZERO(&mask);
        FD_SET(gpm_fd, &mask);

        struct timeval timeout;
        memset(&timeout, 0, sizeof(timeout));

        int count = select(gpm_fd+1, &mask, NULL, NULL, &timeout);
        if (count == 0) break;
        error = 1;
        if (count < 0) break;
        if (!FD_ISSET(gpm_fd, &mask)) break;

        Gpm_Event event;
        if (Gpm_GetEvent(&event) != 1) break;
        error = 0;

        column = event.x;
        row = event.y;
        ok = 1;
      }

      if (error) gpmCloseConnection();
      return ok;
    }
  }
#endif /* HAVE_LIBGPM */
  return 0;
}
