
/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Version 1.9.0, 06 April 1998
 *
 * Copyright (C) 1995-1998 by The BRLTTY Team, All rights reserved.
 *
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 * Nicolas Pitre <nico@cam.org>
 * Stephane Doyon <s.doyon@videotron.ca>
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
 */

/* beeps.h - console beeps used by the system
 * $Id: beeps.h,v 1.3 1996/09/24 01:04:24 nn201 Exp $
 */

void soundstat (short);		/* set sound on/off */

void play (int *);		/* make a specific beep */

extern int snd_link[];
extern int snd_unlink[];
extern int snd_wrap_down[];
extern int snd_wrap_up[];
extern int snd_bounce[];
extern int snd_freeze[];
extern int snd_unfreeze[];
extern int snd_cut_beg[];
extern int snd_cut_end[];
extern int snd_toggleon[];
extern int snd_toggleoff[];
extern int snd_done[];
extern int snd_skip[];
extern int snd_skipmore[];
