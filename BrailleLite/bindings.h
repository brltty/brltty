/* BrailleLite/bindings.h - key bindings for BLazie Engineering's Braille Lite
 * N. Nair, 5 September 1998
 */

#ifndef _BINDINGS_H
#define _BINDINGS_H

#include "../brl.h"		/* for CMD_* codes */

/* When the Braille Lite sends braille key information, bits 0-5 represent
 * dots 1-6 and bit 6 represents the space bar.  For now, we mask out bit 6
 * and just use 64-byte tables.
 */

/* The static arrays must only be in brl.c, so just in case ... */
#ifdef BRL_C
#undef BRL_C

static unsigned char brltrans[64] =
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


/* This table is for global BRLTTY commands, to be passed straight back to
 * the main module.  If keyboard emulation is off, they will work with or
 * without a chord, unless they are marked as dangerous in dangcmd[] below.
 *
 * Note that key combinations used to initiate internal commands should be
 * left as 0 here.
 */

static unsigned char cmdtrans[64] =
{
  0, CMD_LNUP, CMD_KEY_LEFT, 0, CMD_CHRLT, 0, CMD_KEY_UP, CMD_TOP_LEFT,
  CMD_LNDN, CMD_CSRTRK, CMD_DISPMD, CMD_FREEZE, CMD_INFO, CMD_MUTE, 0, CMD_PASTE,
  CMD_KEY_RIGHT, 0, 0, CMD_HOME, 0, 0, CMD_LNBEG, CMD_RESTARTBRL,
  CMD_CSRJMP_VERT, 0, CMD_CSRJMP, 0, 0, 0, 0, 0,
  CMD_CHRRT, 0, 0, 0, CMD_SAY, 0, CMD_CUT_BEG, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  CMD_KEY_DOWN, 0, CMD_LNEND, 0, CMD_CUT_END, 0, 0, 0,
  CMD_BOT_LEFT, CMD_HELP, 0, 0, 0, 0, 0, 0
};

/* Dangerous commands; 1 bit per command, order as cmdtrans[], set if
 * the corresponding command is dangerous.
 */

static unsigned char dangcmd[8] =
{ 0x00, 0x88, 0x00, 0x05, 0x40, 0x00, 0x10, 0x00 };

#endif /* defined(BRL_C) */

/* Functions for the advance bar.  Currently, these are passed straight
 * back to the main module, so have to be global commands.
 */

#define BLT_BARLT CMD_FWINLT
#define BLT_BARRT CMD_FWINRT


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

/* For keyboard emulation mode: */
#define BLT_UPCASE 'u'
#define BLT_UPCOFF 'q'
#define BLT_CTRL 'x'
#define BLT_META '9'
#define BLT_BACKSP 'b'
#define BLT_DELETE 'd'
#define BLT_TAB 't'
#define BLT_ENTER '.'
/* Console - incr, decr, spawn.  Cursor keys? */

#endif /* !defined(_BINDINGS_H) */
