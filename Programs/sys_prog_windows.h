/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

char *
getProgramPath (void) {
  CHAR path[MAX_PATH];
  DWORD length = GetModuleFileName(GetModuleHandle(NULL), path, sizeof(path));

  while (length > 0)
    if (path[--length] == '\\')
      path[length] = '/';

  return strdupWrapper(path);
}
