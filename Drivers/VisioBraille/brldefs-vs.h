/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2004 Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 * All rights reserved.
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 * Please see the file COPYING-API for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* brldefs-vs.h : Useful definitions to handle keys entered at */
/* VisioBraille's keyboard */ 

#ifndef _BRLDEFS_VS_H
#define _BRLDEFS_VS_H

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

#define PLOC_LT 0x801
#define BACKSPACE 0x808
#define TAB 0x809
#define RETURN 0x80d

#define PLOC_PLOC_A 0x8A1
#define PLOC_PLOC_B 0x8A2
#define PLOC_PLOC_C 0x8A3
#define PLOC_PLOC_D 0x8A4
#define PLOC_PLOC_E 0x8A5
#define PLOC_PLOC_F 0x8A6
#define PLOC_PLOC_G 0x8A7
#define PLOC_PLOC_H 0x8A8
#define PLOC_PLOC_I 0x8A9
#define PLOC_PLOC_J 0x8AA
#define PLOC_PLOC_K 0x8AB
#define PLOC_PLOC_L 0x8AC
#define PLOC_PLOC_M 0x8AD
#define PLOC_PLOC_N 0x8AE
#define PLOC_PLOC_O 0x8AF
#define PLOC_PLOC_P 0x8B0
#define PLOC_PLOC_Q 0x8B1
#define PLOC_PLOC_R 0x8B2
#define PLOC_PLOC_S 0x8B3
#define PLOC_PLOC_T 0x8B4
#define PLOC_PLOC_U 0x8B5
#define PLOC_PLOC_V 0x8B6
#define PLOC_PLOC_W 0x8B7
#define PLOC_PLOC_X 0x8B8
#define PLOC_PLOC_Y 0x8B9
#define PLOC_PLOC_Z 0x8BA

#define CONTROL 0x8BE
#define ALT 0x8BF
#define ESCAPE 0x8e0

#endif /* _BRLDEFS_VS_H */ 
