/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

/*
 * scr_vcsa.h - C++ header file for the Linux vcsa screen type library
 */

#ifndef _SCR_VCSA_H
#define _SCR_VCSA_H

#include <linux/kd.h>

#include "scrdev.h"

class VcsaScreen:public RealScreen
{
  int setScreenDevice (void);
  const char *screenDevice;

  void closeScreen (void);
  int screenDescriptor;

  int setConsoleDevice (void);
  const char *consoleDevice;

  int rebindConsole(void);
  void closeConsole (void);
  int consoleDescriptor;

  int insertCode (unsigned short key, int raw);
  int insertMapped (unsigned short key, int (VcsaScreen::*byteInserter)(unsigned char byte));
  int insertUtf8 (unsigned char byte);
  int insertByte (unsigned char byte);

  int setTranslationTable (int opening);
  unsigned char translationTable[0X100];

  unsigned short characterMap[0X100];
  int (VcsaScreen::*setCharacterMap) (int opening);
  int setApplicationCharacterMap (int opening);

  int setScreenFontMap (int opening);
  struct unipair *screenFontMapTable;
  unsigned short screenFontMapCount;
  unsigned short screenFontMapSize;

public:
  char **parameters (void);
  int prepare (char **parameters);
  int open (void);
  int setup (void);
  void getstat (ScreenStatus &);
  unsigned char *getscr (winpos, unsigned char *, short);
  int insert (unsigned short);
  int switchvt (int);
  void close (void);
};

#endif  /* _SCR_VCSA_H */
