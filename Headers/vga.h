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

#ifndef BRLTTY_INCLUDED_VGA
#define BRLTTY_INCLUDED_VGA

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  VGA_BIT_FG_BLUE   = 0X01,
  VGA_BIT_FG_GREEN  = 0X02,
  VGA_BIT_FG_RED    = 0X04,
  VGA_BIT_FG_BRIGHT = 0X08,
  VGA_BIT_BG_BLUE   = 0X10,
  VGA_BIT_BG_GREEN  = 0X20,
  VGA_BIT_BG_RED    = 0X40,
  VGA_BIT_BLINK     = 0X80,

  VGA_MASK_FG = VGA_BIT_FG_RED | VGA_BIT_FG_GREEN | VGA_BIT_FG_BLUE | VGA_BIT_FG_BRIGHT,
  VGA_MASK_BG = VGA_BIT_BG_RED | VGA_BIT_BG_GREEN | VGA_BIT_BG_BLUE,

  VGA_COLOR_FG_BLACK          = 0,
  VGA_COLOR_FG_BLUE           = VGA_BIT_FG_BLUE,
  VGA_COLOR_FG_GREEN          = VGA_BIT_FG_GREEN,
  VGA_COLOR_FG_CYAN           = VGA_BIT_FG_GREEN | VGA_BIT_FG_BLUE,
  VGA_COLOR_FG_RED            = VGA_BIT_FG_RED,
  VGA_COLOR_FG_MAGENTA        = VGA_BIT_FG_RED | VGA_BIT_FG_BLUE,
  VGA_COLOR_FG_BROWN          = VGA_BIT_FG_RED | VGA_BIT_FG_GREEN,
  VGA_COLOR_FG_LIGHT_GREY     = VGA_BIT_FG_RED | VGA_BIT_FG_GREEN | VGA_BIT_FG_BLUE,
  VGA_COLOR_FG_DARK_GREY      = VGA_BIT_FG_BRIGHT | VGA_COLOR_FG_BLACK,
  VGA_COLOR_FG_LIGHT_BLUE     = VGA_BIT_FG_BRIGHT | VGA_COLOR_FG_BLUE,
  VGA_COLOR_FG_LIGHT_GREEN    = VGA_BIT_FG_BRIGHT | VGA_COLOR_FG_GREEN,
  VGA_COLOR_FG_LIGHT_CYAN     = VGA_BIT_FG_BRIGHT | VGA_COLOR_FG_CYAN,
  VGA_COLOR_FG_LIGHT_RED      = VGA_BIT_FG_BRIGHT | VGA_COLOR_FG_RED,
  VGA_COLOR_FG_LIGHT_MAGENTA  = VGA_BIT_FG_BRIGHT | VGA_COLOR_FG_MAGENTA,
  VGA_COLOR_FG_YELLOW         = VGA_BIT_FG_BRIGHT | VGA_COLOR_FG_BROWN,
  VGA_COLOR_FG_WHITE          = VGA_BIT_FG_BRIGHT | VGA_COLOR_FG_LIGHT_GREY,

  VGA_COLOR_BG_BLACK      = 0,
  VGA_COLOR_BG_BLUE       = VGA_BIT_BG_BLUE,
  VGA_COLOR_BG_GREEN      = VGA_BIT_BG_GREEN,
  VGA_COLOR_BG_CYAN       = VGA_BIT_BG_GREEN | VGA_BIT_BG_BLUE,
  VGA_COLOR_BG_RED        = VGA_BIT_BG_RED,
  VGA_COLOR_BG_MAGENTA    = VGA_BIT_BG_RED | VGA_BIT_BG_BLUE,
  VGA_COLOR_BG_BROWN      = VGA_BIT_BG_RED | VGA_BIT_BG_GREEN,
  VGA_COLOR_BG_LIGHT_GREY = VGA_BIT_BG_RED | VGA_BIT_BG_GREEN | VGA_BIT_BG_BLUE,

  VGA_COLOR_DEFAULT = VGA_COLOR_FG_LIGHT_GREY | VGA_COLOR_BG_BLACK
} VGAAttributes;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_VGA */
