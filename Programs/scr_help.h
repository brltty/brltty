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

#ifndef _SCR_HELP_H
#define _SCR_HELP_H

#include "scr_base.h"
#include "help.h"

class HelpScreen:public Screen {
  int fileDescriptor;
  HelpFileHeader fileHeader;
  short pageNumber;
  unsigned char cursorRow, cursorColumn;
  HelpPageEntry *pageDescriptions;
  unsigned char **pages;
  unsigned char *characters;
  int loadPages (const char *);
public:
  HelpScreen ();
  int open (const char *helpfile); /* called every time the help screen is opened */
  void close (void);		   /* called once to close the help screen */
  void setPageNumber (short);
  short getPageNumber (void);
  short getPageCount (void);
  void describe (ScreenDescription &);
  unsigned char *read (ScreenBox, unsigned char *, ScreenMode);
  int insert (unsigned short);
  int route (int, int, int);
};

#endif /* _SCR_HELP_H */
