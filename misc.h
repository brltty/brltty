/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Version 1.0, 26 July 1996
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

/* misc.h - Header file for miscellaneous all-purpose routines
 * N. Nair, 17 September 1995
 */
#include <sys/time.h>

unsigned elapsed_msec (struct timeval *t1, struct timeval *t2);
void shortdelay (unsigned msec);
void delay (int msec);		/* sleep for `msec' milliseconds */
int timeout_yet (int msec);	/* test timeout condition */
