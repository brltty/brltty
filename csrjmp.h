/*
 * BrlTty - A daemon providing access to the Linux console (when in text
 *          mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BrlTty Team. All rights reserved.
 *
 * BrlTty comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */


extern volatile int csr_active;
extern volatile pid_t csr_pid;

void csrjmp (int x, int y, int scrno);

