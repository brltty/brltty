/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

#ifndef _SCR_SHM_H
#define _SCR_SHM_H

/* scr_shm.h - C++ header file for the screen drivers library
 */

#include <sys/ipc.h>
#include <sys/shm.h>

class ShmScreen:public RealScreen
{
  key_t key;
  int shmid;
  char *shm;
public:
  char **parameters (void);
  int prepare (char **parameters);
  int open (void);
  int setup (void);
  void describe (ScreenDescription &);
  unsigned char *read (ScreenBox, unsigned char *, ScreenMode);
  int insert (unsigned short);
  int selectvt (int);
  int switchvt (int);
  void close (void);		// called once to close screen reading
};

#endif /* !defined(_SCR_SHM_H) */
