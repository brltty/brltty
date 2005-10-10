/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#ifndef BRLTTY_INCLUDED_ROUTE
#define BRLTTY_INCLUDED_ROUTE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  ROUTE_NONE,
  ROUTE_DONE,
  ROUTE_WRONG_COLUMN,
  ROUTE_WRONG_ROW,
  ROUTE_ERROR
} RoutingStatus;

extern int startCursorRouting (int column, int row, int screen);
extern volatile pid_t routingProcess;
extern volatile RoutingStatus routingStatus;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_ROUTE */
