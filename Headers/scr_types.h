/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2025 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_SCR_TYPES
#define BRLTTY_INCLUDED_SCR_TYPES

#include "color_types.h"
#include "vga.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  unsigned char vgaAttributes;

  RGBColor foreground;
  RGBColor background;

  unsigned char usingRGB:1;
  unsigned char isBlinking:1;
  unsigned char isBold:1;
  unsigned char isItalic:1;
  unsigned char hasUnderline:1;
  unsigned char hasStrikeThrough:1;
} ScreenColor;

typedef struct {
  wchar_t text;
  ScreenColor color;
} ScreenCharacter;

typedef enum {
  SCQ_NONE = 0,
  SCQ_LOW,
  SCQ_POOR,
  SCQ_FAIR,
  SCQ_GOOD,
  SCQ_HIGH,
} ScreenContentQuality;

typedef struct {
  const char *unreadable;
  ScreenContentQuality quality;

  int number;		      /* screen number */
  short cols, rows;	/* screen dimensions */
  short posx, posy;	/* cursor position */

  unsigned char hasCursor:1;
  unsigned char hasSelection:1;
} ScreenDescription;

typedef struct {
  short left, top;	/* top-left corner (offset from 0) */
  short width, height;	/* dimensions */
} ScreenBox;

#define SCR_KEY_SHIFT     0X40000000
#define SCR_KEY_UPPER     0X20000000
#define SCR_KEY_CONTROL   0X10000000
#define SCR_KEY_ALT_LEFT  0X08000000
#define SCR_KEY_ALT_RIGHT 0X04000000
#define SCR_KEY_GUI       0X02000000
#define SCR_KEY_CAPSLOCK  0X01000000
#define SCR_KEY_CHAR_MASK 0X000FFFFF

#define SCR_KEY_UNICODE_ROW 0XF800

typedef enum {
  SCR_KEY_ENTER = SCR_KEY_UNICODE_ROW,
  SCR_KEY_TAB,
  SCR_KEY_BACKSPACE,
  SCR_KEY_ESCAPE,
  SCR_KEY_CURSOR_LEFT,
  SCR_KEY_CURSOR_RIGHT,
  SCR_KEY_CURSOR_UP,
  SCR_KEY_CURSOR_DOWN,
  SCR_KEY_PAGE_UP,
  SCR_KEY_PAGE_DOWN,
  SCR_KEY_HOME,
  SCR_KEY_END,
  SCR_KEY_INSERT,
  SCR_KEY_DELETE,
  SCR_KEY_FUNCTION,

  SCR_KEY_F1 = SCR_KEY_FUNCTION,
  SCR_KEY_F2,
  SCR_KEY_F3,
  SCR_KEY_F4,
  SCR_KEY_F5,
  SCR_KEY_F6,
  SCR_KEY_F7,
  SCR_KEY_F8,
  SCR_KEY_F9,
  SCR_KEY_F10,
  SCR_KEY_F11,
  SCR_KEY_F12,
  SCR_KEY_F13,
  SCR_KEY_F14,
  SCR_KEY_F15,
  SCR_KEY_F16,
  SCR_KEY_F17,
  SCR_KEY_F18,
  SCR_KEY_F19,
  SCR_KEY_F20,
  SCR_KEY_F21,
  SCR_KEY_F22,
  SCR_KEY_F23,
  SCR_KEY_F24,
  SCR_KEY_F25,
  SCR_KEY_F26,
  SCR_KEY_F27,
  SCR_KEY_F28,
  SCR_KEY_F29,
  SCR_KEY_F30,
  SCR_KEY_F31,
  SCR_KEY_F32,
  SCR_KEY_F33,
  SCR_KEY_F34,
  SCR_KEY_F35,
  SCR_KEY_F36,
  SCR_KEY_F37,
  SCR_KEY_F38,
  SCR_KEY_F39,
  SCR_KEY_F40,
  SCR_KEY_F41,
  SCR_KEY_F42,
  SCR_KEY_F43,
  SCR_KEY_F44,
  SCR_KEY_F45,
  SCR_KEY_F46,
  SCR_KEY_F47,
  SCR_KEY_F48,
  SCR_KEY_F49,
  SCR_KEY_F50,
  SCR_KEY_F51,
  SCR_KEY_F52,
  SCR_KEY_F53,
  SCR_KEY_F54,
  SCR_KEY_F55,
  SCR_KEY_F56,
  SCR_KEY_F57,
  SCR_KEY_F58,
  SCR_KEY_F59,
  SCR_KEY_F60,
  SCR_KEY_F61,
  SCR_KEY_F62,
  SCR_KEY_F63,

  SCR_KEY_COUNT // not a key - must be last
} ScreenKey;

static inline int
isSpecialKey (ScreenKey key) {
  return (key & (SCR_KEY_CHAR_MASK & ~0XFF)) == SCR_KEY_UNICODE_ROW;
}

/* must be less than 0 */
#define SCR_NO_VT -1

typedef enum {
  SPM_UNKNOWN,
  SPM_PLAIN,
  SPM_BRACKETED,
} ScreenPasteMode;

typedef struct ScreenDriverStruct ScreenDriver;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SCR_TYPES */
