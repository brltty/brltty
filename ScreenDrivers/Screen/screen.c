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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef HAVE_SHMGET
#include <sys/ipc.h>
#include <sys/shm.h>
static key_t shmKey;
static int shmIdentifier;
#endif /* HAVE_SHMGET */

#ifdef HAVE_SHM_OPEN
#include <sys/mman.h>
static const char *shmPath = "/screen";
static int shmFileDescriptor = -1;
#endif /* HAVE_SHM_OPEN */

#include "Programs/misc.h"

#include "Programs/scr_driver.h"
#include "screen.h"

static char *shmAddress = NULL;
static const mode_t shmMode = S_IRWXU;
static const int shmSize = 4 + ((66 * 132) * 2);

static int
open_ScreenScreen (void) {
#ifdef HAVE_SHMGET
  key_t keys[2];
  int keyCount = 0;

  /* The original, static key. */
  keys[keyCount++] = 0xBACD072F;

  /* The new, dynamically generated, per user key. */
  {
    int project = 'b';
    const char *path = getenv("HOME");
    if (!path || !*path) path = "/";
    LogPrint(LOG_DEBUG, "Shared memory file system object: %s", path);
    if ((keys[keyCount] = ftok(path, project)) != -1) {
      keyCount++;
    } else {
      LogPrint(LOG_WARNING, "Per user shared memory key not generated: %s",
               strerror(errno));
    }
  }

  while (keyCount > 0) {
    shmKey = keys[--keyCount];
    LogPrint(LOG_DEBUG, "Trying shared memory key: 0X%X", shmKey);
    if ((shmIdentifier = shmget(shmKey, shmSize, shmMode)) != -1) {
      if ((shmAddress = shmat(shmIdentifier, NULL, 0)) != (char *)-1) {
        LogPrint(LOG_INFO, "Screen image shared memory key: 0X%X", shmKey);
        return 1;
      } else {
        LogPrint(LOG_WARNING, "Cannot attach shared memory segment 0X%X: %s",
                 shmKey, strerror(errno));
      }
    } else {
      LogPrint(LOG_WARNING, "Cannot access shared memory segment 0X%X: %s",
               shmKey, strerror(errno));
    }
  }
  shmIdentifier = -1;
#endif /* HAVE_SHMGET */

#ifdef HAVE_SHM_OPEN
  if ((shmFileDescriptor = shm_open(shmPath, O_RDONLY, shmMode)) != -1) {
    if ((shmAddress = mmap(0, shmSize, PROT_READ, MAP_SHARED, shmFileDescriptor, 0)) != MAP_FAILED) {
      return 1;
    } else {
      LogError("mmap");
    }

    close(shmFileDescriptor);
    shmFileDescriptor = -1;
  } else {
    LogError("shm_open");
  }
#endif /* HAVE_SHM_OPEN */

  return 0;
}

static int
currentvt_ScreenScreen (void) {
  return 0;
}

static void
describe_ScreenScreen (ScreenDescription *description) {
  description->cols = shmAddress[0];
  description->rows = shmAddress[1];
  description->posx = shmAddress[2];
  description->posy = shmAddress[3];
  description->no = currentvt_ScreenScreen();
}

static int
read_ScreenScreen (ScreenBox box, unsigned char *buffer, ScreenMode mode) {
  ScreenDescription description;                 /* screen statistics */
  describe_ScreenScreen(&description);
  if (validateScreenBox(&box, description.cols, description.rows)) {
    off_t start = 4 + (((mode == SCR_TEXT)? 0: 1) * description.cols * description.rows) + (box.top * description.cols) + box.left;
    int row;
    for (row=0; row<box.height; row++) {
      memcpy(buffer + (row * box.width),
             shmAddress + start + (row * description.cols),
             box.width);
    }
    return 1;
  }
  return 0;
}

static void
close_ScreenScreen (void) {
#ifdef HAVE_SHMGET
  if (shmIdentifier != -1) {
    shmdt(shmAddress);
  }
#endif /* HAVE_SHMGET */

#ifdef HAVE_SHM_OPEN
  if (shmFileDescriptor != -1) {
    munmap(shmAddress, shmSize);
    close(shmFileDescriptor);
    shmFileDescriptor = -1;
  }
#endif /* HAVE_SHM_OPEN */

  shmAddress = NULL;
}

static void
scr_initialize (MainScreen *main) {
  initializeRealScreen(main);
  main->base.currentvt = currentvt_ScreenScreen;
  main->base.describe = describe_ScreenScreen;
  main->base.read = read_ScreenScreen;
  main->open = open_ScreenScreen;
  main->close = close_ScreenScreen;
}
