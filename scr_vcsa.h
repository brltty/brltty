/*
 * BrlTty - A daemon providing access to the Linux console (when in text
 *          mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BrlTty Team. All rights reserved.
 *
 * BrlTty comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/*
 * scr_vcsa.h - C++ header file for the Linux vcsa screen type library
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
  int open (int);		// called once to initialise screen reading
  void getstat (scrstat &);
  unsigned char *getscr (winpos, unsigned char *, short);
  void close (void);		// called once to close screen reading
};


#endif  /* _SCR_VCSA_H */
