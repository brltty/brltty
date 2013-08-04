/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>
#include <errno.h>

#ifdef HAVE_SHL_LOAD
#  include <dl.h>
#endif /* HAVE_SHL_LOAD */

#include "log.h"
#include "system.h"

#ifdef ENABLE_SHARED_OBJECTS
void *
loadSharedObject (const char *path) {
#ifdef HAVE_SHL_LOAD
  shl_t object = shl_load(path, BIND_IMMEDIATE|BIND_VERBOSE|DYNAMIC_PATH, 0L);
  if (object) return object;
  logMessage(LOG_ERR, "Shared library '%s' not loaded: %s",
             path, strerror(errno));
#endif /* HAVE_SHL_LOAD */
  return NULL;
}

void 
unloadSharedObject (void *object) {
#ifdef HAVE_SHL_LOAD
  if (shl_unload(object) == -1)
    logMessage(LOG_ERR, "Shared library unload error: %s",
               strerror(errno));
#endif /* HAVE_SHL_LOAD */
}

int 
findSharedSymbol (void *object, const char *symbol, const void **address) {
#ifdef HAVE_SHL_LOAD
  shl_t handle = object;
  if (shl_findsym(&handle, symbol, TYPE_UNDEFINED, address) != -1) return 1;
  logMessage(LOG_ERR, "Shared symbol '%s' not found: %s",
             symbol, strerror(errno));
#endif /* HAVE_SHL_LOAD */
  return 0;
}
#endif /* ENABLE_SHARED_OBJECTS */
