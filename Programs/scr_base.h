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

#ifndef _SCR_BASE_H
#define _SCR_BASE_H

/* scr_base.h - C++ header file for the screen drivers library
 */

/* abstract base class - useful for pointers */
class Screen {
public:
  virtual void describe (ScreenDescription &) = 0;
  virtual unsigned char *read (ScreenBox, unsigned char *, ScreenMode) = 0;
  virtual int insert (unsigned short);
  virtual int route (int, int, int);
  virtual int point (int, int);
  virtual int pointer (int &, int &);
  virtual int selectvt (int);
  virtual int switchvt (int);
  virtual int currentvt (void);
  virtual int execute (int);
};

#endif /* _SCR_BASE_H */
