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

/* scr_shm.cc - screen type library for use with the "screen" package
 *              through IPC shared memory. See in the patches directory
 *              for details.
 *
 * Note: Although C++, this code requires no standard C++ library.
 * This is important as BRLTTY *must not* rely on too many
 * run-time shared libraries, nor be a huge executable.
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

#include "scr.h"
#include "scr_shm.h"


/* Instanciation of the shm driver here */
static ShmScreen shm;

/* Pointer for external reference */
RealScreen *live = &shm;


static char *shmParameters[] = {
  NULL
};

char ** ShmScreen::parameters (void) {
  return shmParameters;
}


int ShmScreen::prepare (char **parameters) {
  return 1;
}


int ShmScreen::open (void) {
  /* this should be generated from ftok(), but... */
  key = 0xBACD072F;     /* random static IPC key */
  /* Allocation of shared mem for 18000 bytes (screen text and attributes
   * + few coord.).  We supose no screen will be wider than 132x66.
   * 0700 = [rwx------].
   */
  if ((shmid = shmget(key, 18000, 0700)) != -1) {
    if ((shm = (char *)shmat(shmid, NULL, 0)) != (char *)-1) {
      return 1;
    }
  }
  return 0;
}


int ShmScreen::setup (void) {
  return 1;
}


void ShmScreen::describe (ScreenDescription &desc) {
  desc.cols = shm[0];	/* scrdim x */
  desc.rows = shm[1];   /* scrdim y */
  desc.posx = shm[2];   /* csrpos x */
  desc.posy = shm[3];   /* csrpos y */
  desc.no = 1;  /* not yet implemented */
}


unsigned char *ShmScreen::read (ScreenBox box, unsigned char *buffer, ScreenMode mode) {
  ScreenDescription description;                 /* screen statistics */
  describe(description);
  if ((box.left >= 0) && (box.width > 0) && ((box.left + box.width) <= description.cols) &&
      (box.top >= 0) && (box.height > 0) && ((box.top + box.height) <= description.rows)) {
    off_t start = 4 + (((mode == SCR_TEXT)? 0: 1) * description.cols * description.rows) + (box.top * description.cols) + box.left;
    for (int row=0; row<box.height; row++) {
      memcpy(buffer + (row * box.width), shm + start + (row * description.cols), box.width);
    }
    return buffer;
  }
  return NULL;
}


void ShmScreen::close (void) {
  shmdt(shm);
  shm = NULL;
}


int ShmScreen::insert (unsigned short) {
  return 0;
}


int ShmScreen::selectvt (int vt) {
  return 0;
}

int ShmScreen::switchvt (int vt) {
  return 0;
}
