/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

/* serial line baudrate 
 */
#include "Programs/brl.h"


#define BAUDRATE	9600
#define CNTX_INTERNAL	0x80

/*
** main structure to alias braille keystrokes
*/

typedef struct	s_alias
{
  int		brl;
  int		key;
}		t_alias;

typedef struct	s_key
{
  short int	res;
  unsigned char	brl_key;
  int		(*f)(BrailleDisplay *);
}		t_key;

static int	begblk(BrailleDisplay *);
static int	endblk(BrailleDisplay *);
static int	Program(BrailleDisplay *brl);
static int	ViewOn(BrailleDisplay *);
static int	routing(BrailleDisplay *brl, int key);
static int	enter_routing(BrailleDisplay *brl);
static int	routing(BrailleDisplay *brl, int key);
