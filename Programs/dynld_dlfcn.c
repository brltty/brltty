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

#ifdef HAVE_DLOPEN 
#include <dlfcn.h>
#else /* HAVE_DLOPEN */
#warning shared object support not available on this installation: no <dlfcn.h>
#endif /* HAVE_DLOPEN */

#include "log.h"
#include "dynld.h"

static inline int
getSharedObjectLoadFlags (void) {
  int flags = 0;

#ifdef DL_LAZY
  flags |= DL_LAZY;
#else /* DL_LAZY */
  flags |= RTLD_NOW | RTLD_GLOBAL;
#endif /* DL_LAZY */

  return flags;
}

void *
loadSharedObject (const char *path) {
#ifdef HAVE_DLOPEN 
  void *object = dlopen(path, getSharedObjectLoadFlags());
  if (object) return object;
  logMessage(LOG_ERR, "%s", dlerror());
#endif /* HAVE_DLOPEN */
  return NULL;
}

void 
unloadSharedObject (void *object) {
#ifdef HAVE_DLOPEN 
  dlclose((void *)object);
#endif /* HAVE_DLOPEN */
}

int 
findSharedSymbol (void *object, const char *symbol, void *pointerAddress) {
#ifdef HAVE_DLOPEN 
  void **address = pointerAddress;

  dlerror(); /* clear any previous error condition */
  *address = dlsym(object, symbol);

  {
    const char *error = dlerror();
    if (!error) return 1;
    logMessage(LOG_ERR, "%s", error);
  }
#endif /* HAVE_DLOPEN */
  return 0;
}
