/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#define VBSIZE 40
#define LPTPORT 0x278
#define VBDELAY 3
#define VBCLOCK 5
#define VBREFRESHDELAY 40 // Min. time to wait before updating the display

// Don't touch the following definitions
#define VBLPTSTROBE 0x40
#define VBLPTCLOCK 0x20
#define VBLPTDATA 0x80
