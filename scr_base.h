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

#ifndef _SCR_BASE_H
#define _SCR_BASE_H

/* scr_base.h - C++ header file for the screen drivers library
 */

extern "C"
{
#include "scr.h"
}
#include "help.h"		// help file header format


// abstract base class - useful for pointers
class Screen {
public:
  virtual void describe (ScreenDescription &) = 0;
  virtual unsigned char *read (ScreenBox, unsigned char *, ScreenMode) = 0;
  virtual int insert (unsigned short);
  virtual int route (int, int, int);
  virtual int selectvt (int);
  virtual int switchvt (int);
  virtual int execute (int);
};


// abstract base class - useful for screen source driver pointers
class RealScreen : public Screen {
public:
  virtual const char *const *parameters (void) = 0;
  virtual int prepare (char **parameters) = 0;
  virtual int open (void) = 0;
  virtual int setup (void) = 0;
  virtual void close (void) = 0;
  int route (int, int, int);
};


class FrozenScreen:public Screen {
  ScreenDescription description;
  unsigned char *text;
  unsigned char *attributes;
public:
  FrozenScreen ();
  int open (Screen *);		// called every time the screen is frozen
  void close (void);		// called to discard frozen screen image
  void describe (ScreenDescription &);
  unsigned char *read (ScreenBox, unsigned char *, ScreenMode);
};


class HelpScreen:public Screen {
  int fileDescriptor;
  short pageCount;
  short pageNumber;
  unsigned char cursorRow, cursorColumn;
  pageinfo *pageDescriptions;
  unsigned char **pages;
  unsigned char *characters;
  int loadPages (const char *);
public:
  HelpScreen ();
  int open (const char *helpfile);		// called every time the help screen is opened
  void close (void);		// called once to close the help screen
  void setPageNumber (short);
  short getPageNumber (void);
  short getPageCount (void);
  void describe (ScreenDescription &);
  unsigned char *read (ScreenBox, unsigned char *, ScreenMode);
  int insert (unsigned short);
  int route (int, int, int);
};


/* The Live Screen type is instanciated elsewhere and choosen at link time
 * from all available screen source drivers.
 */

extern RealScreen *live;

#endif /* !defined(_SCR_BASE_H) */
