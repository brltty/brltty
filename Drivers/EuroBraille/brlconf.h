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

#define BRLNAME	"EuroBraille"

/* serial line baudrate 
 */
#include	"Programs/brl.h"


#define BAUDRATE B9600

/* Define the preferred/default status cells mode. */
#define PREFSTYLE ST_None

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
  int		brl_key;
  int		(*f)(BrailleDisplay *);
  int		res;
}		t_key;

static int	begblk(BrailleDisplay *);
static int	endblk(BrailleDisplay *);
static int	Program(BrailleDisplay *brl);
static int	ViewOn(BrailleDisplay *);
