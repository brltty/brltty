/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

/** EuroBraille/eu_keys.h '- Key defines for all models **/

#ifndef __EU_KEYS_H__
#define __EU_KEYS_H__

# define	EUBRL_BRAILLE_KEY	0x10000000
# define	EUBRL_ROUTING_KEY	0x20000000
# define	EUBRL_COMMAND_KEY	0x40000000


/** IRIS models **/


#define		VK_NONE		0x0000
#define		VK_L1		0x0001
#define         VK_L2		0x0002
#define         VK_L3		0x0004
#define         VK_L4		0x0008
#define         VK_L5		0x0010
#define         VK_L6		0x0020
#define         VK_L7		0x0040
#define         VK_L8		0x0080

/*
** Arrow keys
*/
#define         VK_FH		0x0100
#define         VK_FB		0x0200
#define		VK_FD		0x0400
#define         VK_FG		0x0800

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

/* Old protocol keys : clio/scriba/iris <= 1.70 */
#define		CL_NONE		0x0000
# define	CL_1		'1'
# define	CL_2		'2'
# define	CL_3		'3'
# define	CL_4		'4'
# define	CL_5		'5'
# define	CL_6		'6'
# define	CL_7		'7'
# define	CL_8		'8'
# define	CL_9		'9'
# define	CL_0		'0'
# define	CL_STAR		'*'
# define	CL_SHARP	'#'
# define	CL_A		'A'
# define	CL_B		'B'
# define	CL_C		'C'
# define	CL_D		'D'
# define	CL_E		'E'
# define	CL_F		'F'
# define	CL_G		'G'
# define	CL_H		'H'
# define	CL_I		'I'
# define	CL_J		'J'
# define	CL_K		'K'
# define	CL_L		'L'
# define	CL_M		'M'
# define	CL_FG		'4'
# define	CL_FD		'6'
# define	CL_FB		'8'
# define	CL_FH		'2'


/** Esys Model keys **/

#endif /* __EU_KEYS_H__ */
