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

/* brlkeydefs.h : Useful definitions to handle keys entered at */
/* VisioBraille's keyboard */ 

#ifndef _BRLKEYDEFS_H
#define _BRLKEYDEFS_H

#define BRLKEY_CHAR        0x100
#define BRLKEY_ROUTING     0x200
#define BRLKEY_FUNCTIONKEY 0x400
#define BRLKEY_OTHER       0x800

/* Symbolic definitions for VisioBraille's function keys */
#define keyA1 0x400
#define keyA2 0x401
#define keyA3 0x402
#define keyA4 0x403
#define keyA5 0x404
#define keyA6 0x405
#define keyA7 0x406
#define keyA8 0x407
#define keyB1 0x408
#define keyB2 0x409
#define keyB3 0x40a
#define keyB4 0x40b
#define keyB5 0x40c
#define keyB6 0x40d
#define keyB7 0x40e
#define keyB8 0x40f
#define keyC1 0x410
#define keyC2 0x411
#define keyC3 0x412
#define keyC4 0x413
#define keyC5 0x414
#define keyC6 0x415
#define keyC7 0x416
#define keyC8 0x417
#define keyD1 0x418
#define keyD2 0x419
#define keyD3 0x41a
#define keyD4 0x41b
#define keyD5 0x41c
#define keyD6 0x41d
#define keyD7 0x41e
#define keyD8 0x41f

#endif /* _BRLKEYDEFS_H */ 
