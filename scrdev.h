/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 * Nicolas Pitre <nico@cam.org>
 * Stephane Doyon <doyons@jsp.umontreal.ca>
 *
 * Version 1.0.2, 17 September 1996
 *
 * Copyright (C) 1995, 1996 by Nikhil Nair and others.  All rights reserved.
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nikhil Nair <nn201@cus.cam.ac.uk>.
 */

/* scrdev.h - C++ header file for the screen types library
 * $Id: scrdev.h,v 1.3 1996/09/24 01:04:27 nn201 Exp $
 */

#ifndef _SCRDEV_H
#define _SCRDEV_H

extern "C"
  {
    #include "scr.h"
  }
#include "helphdr.h"		// help file header format

class Screen			// abstract base class - useful for pointers
{
public:
  virtual void getstat (scrstat &) = 0;
  virtual unsigned char *getscr (winpos, unsigned char *, short) = 0;
};


class LiveScreen: public Screen
{
  int fd;
public:
  int open (void);		// called once to initialise screen reading
  void getstat (scrstat &);
  unsigned char *getscr (winpos, unsigned char *, short);
  void close (void);		// called once to close screen reading
};


class FrozenScreen: public Screen
{
  scrstat stat;
  unsigned char *text;
  unsigned char *attrib;
public:
  FrozenScreen ();
  int open (LiveScreen &);	// called every time the screen is frozen
  void getstat (scrstat &);
  unsigned char *getscr (winpos, unsigned char *, short);
  void close (void);		// called to discard frozen screen image
};


class HelpScreen: public Screen
{
  int fd;
  short numpages;
  pageinfo *psz;
  unsigned char **page;
  unsigned char *buffer;
  short scrno;
  int gethelp (void);
public:
  HelpScreen ();
  void setscrno (short);
  short numscreens (void);
  int open (void);		// called every time the help screen is opened
  void getstat (scrstat &);
  unsigned char *getscr (winpos, unsigned char *, short);
  void close (void);		// called once to close the help screen
};


#endif				// !_SCRDEV_H
