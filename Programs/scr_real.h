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

#ifndef _SCR_REAL_H
#define _SCR_REAL_H

#include "scr_base.h"

/* abstract base class - useful for screen source driver pointers */
class RealScreen : public Screen {
public:
  virtual const char *const *parameters (void) = 0;
  virtual int prepare (char **parameters) = 0;
  virtual int open (void) = 0;
  virtual int setup (void) = 0;
  virtual void close (void) = 0;
  int route (int, int, int);
  int point (int, int);
  int pointer (int &, int &);
};


/* The Live Screen type is instanciated elsewhere and choosen at link time
 * from all available screen source drivers.
 */

extern RealScreen *live;

#endif /* _SCR_REAL_H */
