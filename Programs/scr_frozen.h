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

#ifndef _SCR_FROZEN_H
#define _SCR_FROZEN_H

#include "scr_base.h"

class FrozenScreen:public Screen {
  ScreenDescription description;
  unsigned char *text;
  unsigned char *attributes;
public:
  FrozenScreen ();
  int open (Screen *);		/* called every time the screen is frozen */
  void close (void);		/* called to discard frozen screen image */
  void describe (ScreenDescription &);
  unsigned char *read (ScreenBox, unsigned char *, ScreenMode);
};

#endif /* _SCR_FROZEN_H */
