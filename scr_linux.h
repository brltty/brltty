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

#ifndef _SCR_LINUX_H
#define _SCR_LINUX_H

/*
 * scr_linux.h - C++ header file for the Linux vcsa screen type library
 */

#include <linux/kd.h>

typedef unsigned short int UnicodeNumber;
typedef UnicodeNumber ApplicationCharacterMap[0X100];

class LinuxScreen:public RealScreen {
  int setScreenPath (void);
  const char *screenPath;

  int openScreen (unsigned char vt);
  void closeScreen (void);
  int screenDescriptor;
  unsigned char virtualTerminal;

  int setConsolePath (void);
  const char *consolePath;

  int openConsole (unsigned char vt);
  void closeConsole (void);
  int consoleDescriptor;
  int rebindConsole (void);
  int controlConsole (int operation, void *argument);

  int setTranslationTable (int force);
  unsigned char translationTable[0X100];

  ApplicationCharacterMap applicationCharacterMap;
  int (LinuxScreen::*setApplicationCharacterMap) (int force);
  int getUserAcm (int force);
  int determineApplicationCharacterMap (int force);
  void logApplicationCharacterMap (void);

  int setScreenFontMap (int force);
  struct unipair *screenFontMapTable;
  unsigned short screenFontMapCount;
  unsigned short screenFontMapSize;

  void getScreenDescription (ScreenDescription &desc);
  void getConsoleDescription (ScreenDescription &desc);

  int insertCode (unsigned short key, int raw);
  int insertMapped (unsigned short key, int (LinuxScreen::*byteInserter)(unsigned char byte));
  int insertUtf8 (unsigned char byte);
  int insertByte (unsigned char byte);

public:
  const char *const *parameters (void);
  int prepare (char **parameters);
  int open (void);
  int setup (void);
  void close (void);
  void describe (ScreenDescription &);
  unsigned char *read (ScreenBox, unsigned char *, ScreenMode);
  int insert (unsigned short);
  int selectvt (int);
  int switchvt (int);
};

#endif /* !defined(_SCR_LINUX_H) */
