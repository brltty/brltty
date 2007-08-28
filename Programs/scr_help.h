/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_SCR_HELP
#define BRLTTY_INCLUDED_SCR_HELP

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "scr_base.h"
#include "help.h"

typedef struct {
  BaseScreen base;
  int (*construct) (const char *);
  void (*destruct) (void);		   /* called once to close the help screen */
  void (*setPageNumber) (short);
  short (*getPageNumber) (void);
  short (*getPageCount) (void);
} HelpScreen;

extern void initializeHelpScreen (HelpScreen *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SCR_HELP */
