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

typedef struct {
  void (*describe) (ScreenDescription *);
  unsigned char * (*read) (ScreenBox, unsigned char *, ScreenMode);
  int (*insert) (unsigned short);
  int (*route) (int, int, int);
  int (*point) (int, int);
  int (*pointer) (int *, int *);
  int (*selectvt) (int);
  int (*switchvt) (int);
  int (*currentvt) (void);
  int (*execute) (int);
} BaseScreen;

extern void initializeBaseScreen (BaseScreen *);

#endif /* _SCR_BASE_H */
