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

/* scr_vcsa.h - C++ header file for the Linux vcsa screen type library
 */

#ifndef _SCR_VCSA_H
#define _SCR_VCSA_H


#include "scrdev.h"



class vcsa_Screen:public RealScreen
{
  int fd, cons_fd;
  const unsigned char *translation_table;
  void set_screen_translation_table (void);
    public:
  int open (void);		// called once to initialise screen reading
  void getstat (scrstat &);
  unsigned char *getscr (winpos, unsigned char *, short);
  void close (void);		// called once to close screen reading
};


#endif  /* _SCR_VCSA_H */
