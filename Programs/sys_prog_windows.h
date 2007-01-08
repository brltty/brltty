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

char *
getProgramPath (void) {
  char *path = NULL;
  HMODULE handle;

  if ((handle = GetModuleHandle(NULL))) {
    size_t size = 0X80;
    char *buffer = NULL;

    while (1) {
      buffer = reallocWrapper(buffer, size<<=1);

      {
        DWORD length = GetModuleFileName(handle, buffer, size);

        if (!length) {
          LogWindowsError("GetModuleFileName");
          break;
        }

        if (length < size) {
          buffer[length] = 0;
          path = strdupWrapper(buffer);

          while (length > 0)
            if (path[--length] == '\\')
              path[length] = '/';

          break;
        }
      }
    }

    free(buffer);
  } else {
    LogWindowsError("GetModuleHandle");
  }

  return path;
}
