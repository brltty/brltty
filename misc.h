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

/* misc.h - Header file for miscellaneous all-purpose routines
 */

#define __EXTENSIONS__
#include <sys/time.h>

unsigned elapsed_msec (struct timeval *t1, struct timeval *t2);
void shortdelay (unsigned msec);
void delay (int msec);		/* sleep for `msec' milliseconds */
int timeout_yet (int msec);	/* test timeout condition */
