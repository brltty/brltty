/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 * Nicolas Pitre <nico@cam.org>
 * Stephane Doyon <doyons@jsp.umontreal.ca>
 *
 * Version 1.0.2, 17 September 1996
 *
 * Copyright (C) 1995, 1996 by Nikhil Nair and others.  All rights reserved.
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nikhil Nair <nn201@cus.cam.ac.uk>.
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
