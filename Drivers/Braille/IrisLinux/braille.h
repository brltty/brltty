/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* serial line baudrate 
 */
#include "brl.h"


#define BAUDRATE	9600
#define CNTX_INTERNAL	0x80
#define		CTX_KEYCODES	1
#define		CTX_COMMANDS	2

#define		EB_LINEAR	0x00010000
#define		EB_BRAILLE	0x00020000
#define		EB_ROUTING	0x00040000

#define		IN_LEVEL1	0x1000
#define		IN_LEVEL2	0x2000
#define		IN_LEVEL3	0x4000
#define		IN_LEVEL4	0x8000

/*
** Next defines represent linear keys, DO NOT MODIFY THIS 
*/

#define		VK_NONE		0x0000
#define		VK_L1		0x0001
#define         VK_L2		0x0002
#define         VK_L3		0x0004
#define         VK_L4		0x0008
#define         VK_L5OLD	0x0010
#define         VK_L5		0x0020
#define         VK_L6		0x0040
#define         VK_L7		0x0080
#define         VK_L8		0x0100

/*
** Arrow keys
*/
#define         VK_FH		0x0200
#define         VK_FG		0x0400
#define		VK_FD		0x0800
#define         VK_FB		0x1000

/*
** Combinations
*/

#define		VK_L12		(VK_L1 | VK_L2)
#define		VK_L13		(VK_L1 | VK_L3)
#define		VK_L14		(VK_L1 | VK_L4)
#define		VK_L23		(VK_L2 | VK_L3)
#define		VK_L24		(VK_L2 | VK_L4)
#define		VK_L34		(VK_L3 | VK_L4)

#define		VK_L56		(VK_L5 | VK_L6)
#define		VK_L57		(VK_L5 | VK_L7)
#define		VK_L58		(VK_L5 | VK_L8)
#define		VK_L67		(VK_L6 | VK_L7)
#define		VK_L68		(VK_L6 | VK_L8)
#define		VK_L78		(VK_L7 | VK_L8)

#define		VK_L123		(VK_L1 | VK_L2 | VK_L3)
#define		VK_L124		(VK_L1 | VK_L2 | VK_L4)
#define		VK_L134		(VK_L1 | VK_L3 | VK_L4)
#define		VK_L234		(VK_L2 | VK_L3 | VK_L4)
#define		VK_L567		(VK_L5 | VK_L6 | VK_L7)
#define		VK_L568		(VK_L5 | VK_L6 | VK_L8)
#define		VK_L578		(VK_L5 | VK_L7 | VK_L8)
#define		VK_L678		(VK_L6 | VK_L7 | VK_L8)

#define		VK_L1234	(VK_L1 | VK_L2 | VK_L3 | VK_L4)
#define		VK_L5678	(VK_L5 | VK_L6 | VK_L7 | VK_L8)

#define		VK_FDB		(VK_FD | VK_FB)
#define		VK_FGB		(VK_FG | VK_FB)
#define		VK_FDH		(VK_FD | VK_FH)
#define		VK_FGH		(VK_FG | VK_FH)
#define		VK_FBH		(VK_FB | VK_FH)

/*
** main structure to alias braille keystrokes
*/

#include "gio_ioctl.h"

typedef struct	s_alias
{
  int		brl;
  int		key;
}		t_alias;

typedef struct	s_key
{
  unsigned int		res;
  unsigned short        brl_key;
  int		(*f)(BrailleDisplay *);
}		t_key;

static int	begblk(BrailleDisplay *);
static int	endblk(BrailleDisplay *);
static int	key_handle(BrailleDisplay *, unsigned char *, char);
static int	linear_handle(BrailleDisplay *, unsigned int, char);
static int	handle_routekey(BrailleDisplay *, int);
static int	Program(BrailleDisplay *brl);
static int	SynthControl(BrailleDisplay *brl);
static int	ViewOn(BrailleDisplay *);
static int	routing(BrailleDisplay *brl, int key, char);
