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

/* BrailleLite/bindings.h - key bindings for BLazie Engineering's Braille Lite
 * N. Nair, 5 September 1998
 */

#ifndef _BINDINGS_H
#define _BINDINGS_H

#include "Programs/brl.h"		/* for BRL_CMD_* codes */

/* When the Braille Lite sends braille key information, bits 0-5 represent
 * dots 1-6 and bit 6 represents the space bar.  For now, we mask out bit 6
 * and just use 64-byte tables.
 */

/* The static arrays must only be in braille.c, so just in case ... */
#ifdef BL_NEED_ARRAYS
#undef BL_NEED_ARRAYS

static const unsigned char brltrans[64] =
{
  ' ', 'a', '1', 'b', '\'', 'k', '2', 'l',
  '`', 'c', 'i', 'f', '/', 'm', 's', 'p',
  '"', 'e', '3', 'h', '9', 'o', '6', 'r',
  '~', 'd', 'j', 'g', '>', 'n', 't', 'q',
  ',', '*', '5', '<', '-', 'u', '8', 'v',
  '.', '%', '{', '$', '+', 'x', '!', '&',
  ';', ':', '4', '|', '0', 'z', '7', '(',
  '_', '?', 'w', '}', '#', 'y', ')', '='
};

#ifdef USE_TEXTTRANS
/* Map from key representations (bits 0-5 for dots 1-6) to BRLTTY dot
   pattern representation (dot 1 bit 0, dot 4 bit 1, dot 2 bit 2, etc) */
static const unsigned char keys_to_dots[64] =
{
  0x0, 0x1, 0x4, 0x5, 0x10, 0x11, 0x14, 0x15,
  0x2, 0x3, 0x6, 0x7, 0x12, 0x13, 0x16, 0x17,
  0x8, 0x9, 0xc, 0xd, 0x18, 0x19, 0x1c, 0x1d,
  0xa, 0xb, 0xe, 0xf, 0x1a, 0x1b, 0x1e, 0x1f,
  0x20, 0x21, 0x24, 0x25, 0x30, 0x31, 0x34, 0x35,
  0x22, 0x23, 0x26, 0x27, 0x32, 0x33, 0x36, 0x37,
  0x28, 0x29, 0x2c, 0x2d, 0x38, 0x39, 0x3c, 0x3d,
  0x2a, 0x2b, 0x2e, 0x2f, 0x3a, 0x3b, 0x3e, 0x3f
};
#endif /* USE_TEXTTRANS */

/* This table is for global BRLTTY commands, to be passed straight back to
 * the main module.  If keyboard emulation is off, they will work with or
 * without a chord, unless they are marked as dangerous in dangcmd[] below.
 *
 * Note that key combinations used to initiate internal commands should be
 * left as 0 here.
 */

static const int cmdtrans[64] =
{
  0, BRL_CMD_LNUP, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT, BRL_BLK_PASSKEY+BRL_KEY_BACKSPACE, BRL_CMD_CHRLT, 0 /*k: kbemu*/, BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP, BRL_CMD_TOP_LEFT,
  BRL_CMD_LNDN, BRL_CMD_CSRTRK, BRL_CMD_DISPMD, BRL_CMD_FREEZE, BRL_CMD_INFO, BRL_CMD_MUTE, BRL_CMD_NXSEARCH, BRL_CMD_PASTE,
  BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT, 0 /*e: endcmd*/, 0 /*25: prefs*/, BRL_CMD_HOME, 0 /*35: meta*/, 0 /*o: number*/, BRL_CMD_LNBEG, BRL_CMD_RESTARTBRL,
  BRL_CMD_CSRJMP_VERT, BRL_BLK_PASSKEY+BRL_KEY_DELETE, BRL_BLK_ROUTE, 0 /*g: internal cursor*/, 0 /*345: blite internal speechbox*/, BRL_CMD_NXPGRPH, BRL_BLK_PASSKEY+BRL_KEY_TAB, 0 /*q: finish uppercase*/,
  BRL_CMD_CHRRT, BRL_BLK_CUTLINE, 0 /*26: dot8shift*/, BRL_BLK_CUTAPPEND, BRL_CMD_SAY_LINE, 0 /*u: uppsercase*/, BRL_BLK_CUTBEGIN, BRL_CMD_SWITCHVT_NEXT,
  BRL_BLK_PASSKEY+BRL_KEY_ENTER, 0 /*145: free*/, BRL_BLK_PASSKEY+BRL_KEY_ESCAPE, BRL_CMD_PRPGRPH, 0 /*346: free*/, 0 /*x: xtrl*/, BRL_CMD_SIXDOTS, 0 /*12346: free*/,
  BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN, BRL_CMD_PRSEARCH, BRL_CMD_LNEND, BRL_CMD_BACK, BRL_BLK_CUTRECT, 0 /*z: abort*/, 0 /*2356: rotate*/, BRL_CMD_NXPROMPT,
  BRL_CMD_BOT_LEFT, BRL_CMD_HELP, 0 /*w: free*/, BRL_CMD_LEARN, BRL_CMD_SWITCHVT_PREV, 0 /*y: free*/, BRL_CMD_PRPROMPT, 0 /*123456: noop*/
};

/* Dangerous commands; 1 bit per command, order as cmdtrans[], set if
 * the corresponding command is dangerous.
 */

static const unsigned char dangcmd[8] =
{ 0x00, 0x88, 0x80, 0x05, 0x40, 0x00, 0x10, 0x00 };

typedef int BarCmds[16];
const BarCmds *barcmds;

/* Two advance bar commands. */
static const BarCmds bar2cmds =
{
/* LeftBar\ RightBar> None         Right        Left         Both         */
/*         None    */ 0          , BRL_CMD_FWINRT , BRL_CMD_LNDN   , BRL_CMD_HWINRT ,
/*         Right   */ BRL_CMD_LNUP   , BRL_CMD_ATTRDN , BRL_CMD_ATTRUP , 0          ,
/*         Left    */ BRL_CMD_FWINLT , BRL_CMD_NXDIFLN, BRL_CMD_PRDIFLN, 0          ,
/*         Both    */ BRL_CMD_HWINLT , BRL_CMD_BOT    , BRL_CMD_TOP    , 0
};

/* One advance bar commands. */
static const BarCmds bar1cmds =
{
/*          None         Left         Right        Both         */
/* None  */ 0          , BRL_CMD_FWINLT , BRL_CMD_FWINRT , 0          ,
/* Right */ BRL_CMD_FWINRT , 0          , 0          , 0          ,
/* Left  */ BRL_CMD_FWINLT , 0          , 0          , 0          ,
/* Both  */ 0          , 0          , 0          , 0
};

/* Left whiz wheel commands. */
static const int lwwcmds[] =
/* None         Up           Down         Press        */
  {0          , BRL_CMD_LNUP   , BRL_CMD_LNDN   , BRL_CMD_ATTRVIS};

/* Right whiz wheel commands. */
static const int rwwcmds[] =
/* None         Up           Down         Press        */
  {0          , BRL_CMD_FWINLT , BRL_CMD_FWINRT , BRL_CMD_CSRVIS };

#endif /* BL_NEED_ARRAYS */


/*
 * Functions for the advance bar.  Currently, these are passed straight
 * back to the main module, so have to be global commands.
 */

/* BrailleLite 18 */
#define BLT_BARLT BRL_CMD_FWINLT
#define BLT_BARRT BRL_CMD_FWINRT


/* Internal commands.  The definitions use the ASCII codes from brltrans[]
 * above.  All must be chorded.
 */

#define BLT_KBEMU 'k'
#define BLT_ROTATE '7'
#define BLT_POSITN 'g'
#define BLT_REPEAT 'o'
#define BLT_CONFIG '3'
#define BLT_ENDCMD 'e'
#define BLT_ABORT 'z'
#define SWITCHVT_NEXT 'v'
#define SWITCHVT_PREV '#'
/* These are valid only in repeat input mode. They duplicate existing
   normal commands. */
#define O_SETMARK 's'
#define O_GOTOMARK 'm'

/* For keyboard emulation mode: */
#define BLT_UPCASE 'u'
#define BLT_UPCOFF 'q'
#define BLT_CTRL 'x'
#ifdef USE_TEXTTRANS
#define BLT_DOT8SHIFT '5'
#endif /* USE_TEXTTRANS */
#define BLT_META '9'

#endif /* _BINDINGS_H */
