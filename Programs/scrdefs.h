/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_SCRDEFS
#define BRLTTY_INCLUDED_SCRDEFS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SCR_ATTR_FG_BLUE   0X01
#define SCR_ATTR_FG_GREEN  0X02
#define SCR_ATTR_FG_RED    0X04
#define SCR_ATTR_FG_BRIGHT 0X08
#define SCR_ATTR_BG_BLUE   0X10
#define SCR_ATTR_BG_GREEN  0X20
#define SCR_ATTR_BG_RED    0X40
#define SCR_ATTR_BLINK     0X80

#define SCR_COLOUR_FG_BLACK 0
#define SCR_COLOUR_FG_BLUE (SCR_ATTR_FG_BLUE)
#define SCR_COLOUR_FG_GREEN (SCR_ATTR_FG_GREEN)
#define SCR_COLOUR_FG_CYAN (SCR_ATTR_FG_BLUE | SCR_ATTR_FG_GREEN)
#define SCR_COLOUR_FG_RED (SCR_ATTR_FG_RED)
#define SCR_COLOUR_FG_MAGENTA (SCR_ATTR_FG_BLUE | SCR_ATTR_FG_RED)
#define SCR_COLOUR_FG_BROWN (SCR_ATTR_FG_GREEN | SCR_ATTR_FG_RED)
#define SCR_COLOUR_FG_LIGHT_GREY (SCR_ATTR_FG_BLUE | SCR_ATTR_FG_GREEN | SCR_ATTR_FG_RED)
#define SCR_COLOUR_FG_DARK_GREY (SCR_ATTR_FG_BRIGHT)
#define SCR_COLOUR_FG_LIGHT__BLUE (SCR_ATTR_FG_BLUE | SCR_ATTR_FG_BRIGHT)
#define SCR_COLOUR_FG_LIGHT__GREEN (SCR_ATTR_FG_GREEN | SCR_ATTR_FG_BRIGHT)
#define SCR_COLOUR_FG_LIGHT__CYAN (SCR_ATTR_FG_BLUE | SCR_ATTR_FG_GREEN | SCR_ATTR_FG_BRIGHT)
#define SCR_COLOUR_FG_LIGHT__RED (SCR_ATTR_FG_RED | SCR_ATTR_FG_BRIGHT)
#define SCR_COLOUR_FG_LIGHT__MAGENTA (SCR_ATTR_FG_BLUE | SCR_ATTR_FG_RED | SCR_ATTR_FG_BRIGHT)
#define SCR_COLOUR_FG_YELLOW (SCR_ATTR_FG_GREEN | SCR_ATTR_FG_RED | SCR_ATTR_FG_BRIGHT)
#define SCR_COLOUR_FG_WHITE (SCR_ATTR_FG_BLUE | SCR_ATTR_FG_GREEN | SCR_ATTR_FG_RED | SCR_ATTR_FG_BRIGHT)

#define SCR_COLOUR_BG_BLACK 0
#define SCR_COLOUR_BG_BLUE (SCR_ATTR_BG_BLUE)
#define SCR_COLOUR_BG_GREEN (SCR_ATTR_BG_GREEN)
#define SCR_COLOUR_BG_CYAN (SCR_ATTR_BG_BLUE | SCR_ATTR_BG_GREEN)
#define SCR_COLOUR_BG_RED (SCR_ATTR_BG_RED)
#define SCR_COLOUR_BG_MAGENTA (SCR_ATTR_BG_BLUE | SCR_ATTR_BG_RED)
#define SCR_COLOUR_BG_BROWN (SCR_ATTR_BG_GREEN | SCR_ATTR_BG_RED)
#define SCR_COLOUR_BG_LIGHT_GREY (SCR_ATTR_BG_BLUE | SCR_ATTR_BG_GREEN | SCR_ATTR_BG_RED)

#define SCR_COLOUR_DEFAULT (SCR_COLOUR_FG_LIGHT_GREY | SCR_COLOUR_BG_BLACK)

typedef struct {
  wchar_t text;
  unsigned char attributes;
} ScreenCharacter;

typedef struct {
  short rows, cols;	/* screen dimensions */
  short posx, posy;	/* cursor position */
  int number;		      /* screen number */
  unsigned cursor:1;
  const char *unreadable;
} ScreenDescription;

typedef struct {
  short left, top;	/* top-left corner (offset from 0) */
  short width, height;	/* dimensions */
} ScreenBox;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SCRDEFS */
