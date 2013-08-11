/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2013 Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_MM_BRLDEFS
#define BRLTTY_INCLUDED_MM_BRLDEFS

typedef enum {
  MM_CMD_QueryLineSize    = 0X13,
  MM_CMD_StartDisplayMode = 0X20,
  MM_CMD_EndDisplayMode   = 0X28,
  MM_CMD_SendBrailleData  = 0X31,
  MM_CMD_SendDisplayData  = 0X32,
  MM_CMD_KeyCombination   = 0Xf0,
  MM_CMD_ShiftPress       = 0Xf2,
  MM_CMD_ShiftRelease     = 0Xf3
} MM_CommandCode;

typedef enum {
  MM_BLINK_NO   = 0,
  MM_BLINK_SLOW = 1,
  MM_BLINK_FAST = 2
} MM_BlinkMode;

typedef struct {
  unsigned char ID1;
  unsigned char ID2;
  unsigned char code;
  unsigned char subcode;
  unsigned char lengthLow;
  unsigned char lengthHigh;
} MM_CommandHeader;

typedef union {
  unsigned char bytes[1];

  struct {
    MM_CommandHeader header;
    unsigned char data[3];
  } fields;
} MM_CommandPacket;

typedef enum {
  MM_GROUP_NONE    = 0,
  MM_GROUP_BRAILLE = 1,
  MM_GROUP_EDIT    = 2,
  MM_GROUP_ARROW   = 3,
  MM_GROUP_CURSOR  = 4,
  MM_GROUP_ERROR   = 5,
  MM_GROUP_DISPLAY = 6
} MM_KeyGroup;

typedef enum {
  MM_SHIFT_F1      = 0,
  MM_SHIFT_F4      = 1,
  MM_SHIFT_CONTROL = 2,
  MM_SHIFT_ALT     = 3,
  MM_SHIFT_SELECT  = 4,
  MM_SHIFT_READ    = 5,
  MM_SHIFT_F2      = 6,
  MM_SHIFT_F3      = 7
} MM_ShiftKey;

typedef enum {
  MM_DOT_8 = 0,
  MM_DOT_6 = 1,
  MM_DOT_5 = 2,
  MM_DOT_4 = 3,
  MM_DOT_7 = 4,
  MM_DOT_3 = 5,
  MM_DOT_2 = 6,
  MM_DOT_1 = 7
} MM_DotKey;

typedef enum {
  MM_EDIT_ESC    = 0,
  MM_EDIT_INF    = 1,
  MM_EDIT_BS     = 2,
  MM_EDIT_DEL    = 3,
  MM_EDIT_INS    = 4,
  MM_EDIT_CHANGE = 5,
  MM_EDIT_OK     = 6,
  MM_EDIT_SET    = 7
} MM_EditKey;

typedef enum {
  MM_ARROW_UP    = 0,
  MM_ARROW_DOWN  = 1,
  MM_ARROW_LEFT  = 2,
  MM_ARROW_RIGHT = 3
} MM_ArrowKey;

typedef enum {
  MM_DISPLAY_BACKWARD = 0,
  MM_DISPLAY_FORWARD  = 1,
  MM_DISPLAY_LSCROLL  = 2,
  MM_DISPLAY_RSCROLL  = 3
} MM_DisplayKey;

typedef enum {
  MM_SET_SHIFT   = 0,
  MM_SET_DOT     = 1,
  MM_SET_EDIT    = 2,
  MM_SET_ARROW   = 3,
  MM_SET_ROUTE   = 4,
  MM_SET_DISPLAY = 6
} MM_KeySet;

#endif /* BRLTTY_INCLUDED_MM_BRLDEFS */ 
