/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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
#include <sys/ipc.h>
#include <sys/shm.h>

#include "scr.h"
#include "scr_real.h"
#include "scr_shm.h"

static key_t shmKey;
static int shmIdentifier;
static char *shmAddress;

static int
open_ShmScreen (void) {
  /* this should be generated from ftok(), but... */
  shmKey = 0xBACD072F;     /* random static IPC key */
  /* Allocation of shared mem for 18000 bytes (screen text and attributes
   * + few coord.).  We supose no screen will be wider than 132x66.
   * 0700 = [rwx------].
   */
  if ((shmIdentifier = shmget(shmKey, 18000, 0700)) != -1) {
    if ((shmAddress = (char *)shmat(shmIdentifier, NULL, 0)) != (char *)-1) {
      return 1;
    }
  }
  return 0;
}

static void
describe_ShmScreen (ScreenDescription *description) {
  description->cols = shmAddress[0];	/* scrdim x */
  description->rows = shmAddress[1];   /* scrdim y */
  description->posx = shmAddress[2];   /* csrpos x */
  description->posy = shmAddress[3];   /* csrpos y */
  description->no = 1;  /* not yet implemented */
}

static unsigned char *
read_ShmScreen (ScreenBox box, unsigned char *buffer, ScreenMode mode) {
  ScreenDescription description;                 /* screen statistics */
  describe_ShmScreen(&description);
  if ((box.left >= 0) && (box.width > 0) && ((box.left + box.width) <= description.cols) &&
      (box.top >= 0) && (box.height > 0) && ((box.top + box.height) <= description.rows)) {
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
  shmdt(shmAddress);
  shmAddress = NULL;
}

void
initializeLiveScreen (RealScreen *real) {
  initializeRealScreen(real);
  real->base.describe = describe_ShmScreen;
  real->base.read = read_ShmScreen;
  real->open = open_ShmScreen;
  real->close = close_ShmScreen;
}
