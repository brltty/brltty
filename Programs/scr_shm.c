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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

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

#include "misc.h"
#include "scr.h"
#include "scr_real.h"
#include "scr_shm.h"

static char *shmAddress = NULL;
static const mode_t shmMode = S_IRWXU;
static const int shmSize = 4 + ((66 * 132) * 2);

static int
open_ShmScreen (void) {
#ifdef HAVE_SHMGET
  /* this should be generated from ftok(), but... */
  shmKey = 0xBACD072F;     /* random static IPC key */

  if ((shmIdentifier = shmget(shmKey, shmSize, shmMode)) != -1) {
    if ((shmAddress = shmat(shmIdentifier, NULL, 0)) != (char *)-1) {
      return 1;
    } else {
      LogError("shmat");
    }
  } else {
    LogError("shmget");
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

static void
describe_ShmScreen (ScreenDescription *description) {
  description->cols = shmAddress[0];
  description->rows = shmAddress[1];
  description->posx = shmAddress[2];
  description->posy = shmAddress[3];
  description->no = 1;  /* not yet implemented */
}

static unsigned char *
read_ShmScreen (ScreenBox box, unsigned char *buffer, ScreenMode mode) {
  ScreenDescription description;                 /* screen statistics */
  describe_ShmScreen(&description);
  if (validateScreenBox(&box, description.cols, description.rows)) {
    off_t start = 4 + (((mode == SCR_TEXT)? 0: 1) * description.cols * description.rows) + (box.top * description.cols) + box.left;
    int row;
    for (row=0; row<box.height; row++) {
      memcpy(buffer + (row * box.width),
             shmAddress + start + (row * description.cols),
             box.width);
    }
    return buffer;
  }
  return NULL;
}

static void
close_ShmScreen (void) {
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

void
initializeLiveScreen (MainScreen *main) {
  initializeRealScreen(main);
  main->base.describe = describe_ShmScreen;
  main->base.read = read_ShmScreen;
  main->open = open_ShmScreen;
  main->close = close_ShmScreen;
}
