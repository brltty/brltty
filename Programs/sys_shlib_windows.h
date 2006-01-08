/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Team. All rights reserved.
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

void *
loadSharedObject (const char *path) {
  HMODULE library;
  if (!(library = LoadLibrary(path)))
    LogWindowsError("loading library");
  return library;
}

void 
unloadSharedObject (void *object) {
  if (!(FreeLibrary((HMODULE) object)))
    LogWindowsError("unloading library");
}

int 
findSharedSymbol (void *object, const char *symbol, void *pointerAddress) {
  void **address = pointerAddress;
  if ((*address = GetProcAddress((HMODULE) object, symbol)))
    return 1;
  LogWindowsError("looking up symbol in library");
  return 0;
}
