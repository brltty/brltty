/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2022 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>
#include <fcntl.h>

#include "log.h"
#include "pty_object.h"

struct PtyObjectStruct {
  char *path;
  int master;
};

PtyObject *
ptyNewObject (void) {
  PtyObject *pty;

  if ((pty = malloc(sizeof(*pty)))) {
    memset(pty, 0, sizeof(*pty));

    if ((pty->master = posix_openpt(O_RDWR)) != -1) {
      if ((pty->path = ptsname(pty->master))) {
        if ((pty->path = strdup(pty->path))) {
          if (grantpt(pty->master) != -1) {
            if (unlockpt(pty->master) != -1) {
              return pty;
            } else {
              logSystemError("unlockpt");
            }
          } else {
            logSystemError("grantpt");
          }

          free(pty->path);
        } else {
          logMallocError();
        }
      } else {
        logSystemError("ptsname");
      }

      close(pty->master);
    } else {
      logSystemError("posix_openpt");
    }

    free(pty);
  } else {
    logMallocError();
  }

  return NULL;
}

int
ptyOpenSlave (const PtyObject *pty, int *fileDescriptor) {
  int result = open(pty->path, O_RDWR);
  int opened = result != INVALID_FILE_DESCRIPTOR;

  if (opened) {
    *fileDescriptor = result;
  } else {
    logSystemError("pty slave open");
  }

  return opened;
}

void
ptyCloseMaster (PtyObject *pty) {
  if (pty->master != INVALID_FILE_DESCRIPTOR) {
    close(pty->master);
    pty->master = INVALID_FILE_DESCRIPTOR;
  }
}

void
ptyDestroyObject (PtyObject *pty) {
  ptyCloseMaster(pty);
  free(pty->path);
  free(pty);
}

const char *
ptyGetPath (const PtyObject *pty) {
  return pty->path;
}

int
ptyGetMaster (const PtyObject *pty) {
  return pty->master;
}

int
ptyWriteInput (PtyObject *pty, const void *data, size_t length) {
  if (write(pty->master, data, length) != -1) return 1;
  logSystemError("pty write input");
  return 0;
}
