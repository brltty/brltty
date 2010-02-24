/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#include "log.h"
#include "misc.h"

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
