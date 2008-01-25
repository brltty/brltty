/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#include <mach-o/dyld.h>

static void
logDyldError (const char *action) {
  NSLinkEditErrors errors;
  int number;
  const char *file;
  const char *message;
  NSLinkEditError(&errors, &number, &file, &message);
  LogPrint(LOG_ERR, "%.*s", (int)(strlen(message)-1), message);
}

void *
loadSharedObject (const char *path) {
  NSObjectFileImage image;
  switch (NSCreateObjectFileImageFromFile(path, &image)) {
    case NSObjectFileImageSuccess: {
      NSModule module = NSLinkModule(image, path, NSLINKMODULE_OPTION_RETURN_ON_ERROR);
      if (module) return module;
      logDyldError("link module");
      LogPrint(LOG_ERR, "shared object not linked: %s", path);
      break;
    }

    case NSObjectFileImageInappropriateFile:
      LogPrint(LOG_ERR, "inappropriate object type: %s", path);
      break;

    case NSObjectFileImageArch:
      LogPrint(LOG_ERR, "incorrect object architecture: %s", path);
      break;

    case NSObjectFileImageFormat:
      LogPrint(LOG_ERR, "invalid object format: %s", path);
      break;

    case NSObjectFileImageAccess:
      LogPrint(LOG_ERR, "inaccessible object: %s", path);
      break;

    case NSObjectFileImageFailure:
    default:
      LogPrint(LOG_ERR, "shared object not loaded: %s", path);
      break;
  }
  return NULL;
}

void 
unloadSharedObject (void *object) {
  NSModule module = object;
  NSUnLinkModule(module, NSUNLINKMODULE_OPTION_NONE);
}

int 
findSharedSymbol (void *object, const char *symbol, void *pointerAddress) {
  NSModule module = object;
  char name[strlen(symbol) + 2];
  snprintf(name, sizeof(name), "_%s", symbol);
  {
    NSSymbol sym = NSLookupSymbolInModule(module, name);
    if (sym) {
      void **address = pointerAddress;
      *address = NSAddressOfSymbol(sym);
      return 1;
    }
  }
  return 0;
}
