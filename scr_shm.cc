/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
 *
 * Web Page: http://www.cam.org/~nico/brltty
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
 */

/* scr_shm.cc - screen type library for use with the "screen" package
 *              through IPC shared memory. See in the patches directory
 *              for details.
 *
 * Note: Although C++, this code requires no standard C++ library.
 * This is important as BRLTTY *must not* rely on too many
 * run-time shared libraries, nor be a huge executable.
 */

#define SCR_C 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "scrdev.h"
#include "scr_shm.h"
#include "config.h"



/* Instanciation of the shm driver here */
static shm_Screen shm;

/* Pointer for external reference */
RealScreen *live = &shm;



#define MAX(a, b) (((a) > (b)) ? (a) : (b))


inline int
shm_Screen::open (int for_csr_routing)
{
  /* this should be generated from ftok(), but... */
  key = 0xBACD072F;     /* random static IPC key */
  /* Allocation of shared mem for 18000 bytes (screen text and attributes
   * + few coord.).  We supose no screen will be wider than 132x66.
   * 0x1C0 = [rwx------].
   */
  if( (shmid = shmget( key, 18000, 0x1C0 )) >= 0 )
    {
      if( (shm = shmat( shmid, 0, 0)) != (void*)-1)
	{
	  return( 0 );
	}
    }
  return( 1 );
}


void
shm_Screen::getstat (scrstat &stat)
{
  stat.cols = shm[0];	/* scrdim x */
  stat.rows = shm[1];   /* scrdim y */
  stat.posx = shm[2];   /* csrpos x */
  stat.posy = shm[3];   /* csrpos y */
  stat.no = 1;  /* not yet implemented */
}


unsigned char *
shm_Screen::getscr (winpos pos, unsigned char *buffer, short mode)
{
  scrstat stat;                 /* screen statistics */
  off_t start;                  /* start offset */

  getstat (stat);
  if (pos.left < 0 || pos.top < 0 || pos.width < 1 || pos.height < 1 \
      || mode < 0 || mode > 1 || pos.left + pos.width > stat.cols \
      || pos.top + pos.height > stat.rows)
    return NULL;
  start = 4 + (mode * stat.cols * stat.rows) + (pos.top * stat.cols + pos.left);
  for (int i = 0; i < pos.height; i++)
    {
      memcpy (buffer + i * pos.width, shm + start + i * stat.cols, pos.width);
    }
  return buffer;
}


inline void
shm_Screen::close (void)
{
  shmdt( shm );
}

