/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
 *
 * Nicolas Pitre <nico@cam.org>
 * Stéphane Doyon <s.doyon@videotron.ca>
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
 */

/*
 * brl_static.c - handling of dynamic drivers
 *
 *    simulates dynamic loading for static linked brlttty executeables
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "brl.h"
#include "scr.h"
#include "misc.h"

extern driver brl_driver;

driver *thedriver = NULL;	/* filled by dynamic libs */
char* driver_libname = "built-in";	/* name of library */

/* load driver from libray - simulated */
/* return 0 on success */
int load_driver(void)
{
  thedriver = &brl_driver; /* locate struct driver - filled with all the data */
  return 0;
}

