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

/*
 * inskey_lnx.c - output to current tty -- Linux specific.
 */


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <linux/keyboard.h>
#include <linux/kd.h>

#include "config.h"
#include "inskey.h"


/* It might be necessary to define separate functions to insert ascii 
 * text and control movement keys because controls may differ for an inskey 
 * module to another.
 */


void inskey (unsigned char *string)
{
  int ins_fd;			/* file descriptor for current terminal */
  long kbmode;

  ins_fd = open (CONSOLE, O_RDONLY);
  if (ins_fd == -1) return;

  /* The ultimate goal for this function is to be able to insert characters,
   * as well as arrow key controls, even if the keyboard on the console is
   * in K_RAW mode (as in dosemu particularly).  
   * For instance, at least, the insertion is not performed if in 
   * raw keyboard to prevent garbage results.
   */
  ioctl( ins_fd, KDGKBMODE, &kbmode );
  if( kbmode != K_XLATE ) return;

  while (*string)
    if (ioctl (ins_fd, TIOCSTI, string++))
      break;

  close (ins_fd);
}


