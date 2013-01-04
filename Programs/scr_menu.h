/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_SCR_MENU
#define BRLTTY_INCLUDED_SCR_MENU

#include "scr_base.h"
#include "menu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  BaseScreen base;
  int (*construct) (Menu *menu);
  void (*destruct) (void);
} MenuScreen;

extern void initializeMenuScreen (MenuScreen *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SCR_MENU */
