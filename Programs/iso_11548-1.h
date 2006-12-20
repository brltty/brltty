/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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

#ifndef BRL_INCLUDED_ISO_11548_1
#define BRL_INCLUDED_ISO_11548_1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* The bits for each braille dot as defined by the ISO 11548-1 standard.
 *
 * From least- to most-significant octal digit:
 * +  the first contains dots 1-3
 * +  the second contains dots 4-6
 * +  the third contains dots 7-8
 * 
 * Here are a few ways to illustrate a braille cell:
 *    By Dot   By Bit   As Octal
 *    Number   Number    Digits
 *     1  4     0  3    001  010
 *     2  5     1  4    002  020
 *     3  6     2  5    004  040
 *     7  8     6  7    100  200
 */

#define BRL_DOT(number) (1 << ((number) - 1))
#define BRL_DOT1 BRL_DOT(1) /* upper-left dot of standard braille cell */
#define BRL_DOT2 BRL_DOT(2) /* middle-left dot of standard braille cell */
#define BRL_DOT3 BRL_DOT(3) /* lower-left dot of standard braille cell */
#define BRL_DOT4 BRL_DOT(4) /* upper-right dot of standard braille cell */
#define BRL_DOT5 BRL_DOT(5) /* middle-right dot of standard braille cell */
#define BRL_DOT6 BRL_DOT(6) /* lower-right dot of standard braille cell */
#define BRL_DOT7 BRL_DOT(7) /* lower-left dot of computer braille cell */
#define BRL_DOT8 BRL_DOT(8) /* lower-right dot of computer braille cell */

typedef unsigned char BrlDots;

static inline BrlDots brlNumberToDot (char number) {
  return ((number >= '1') && (number <= '8'))? BRL_DOT(number - '0'): 0;
}

static inline char brlDotToNumber (BrlDots dot) {
  int shift = ffs(dot);
  return shift? (shift + '0'): 0;
}

typedef char BrlDotNumbersBuffer[9];

static inline unsigned int brlDotsToNumbers (BrlDots dots, BrlDotNumbersBuffer numbers) {
  char *number = numbers;
  while (dots) {
    int shift = ffs(dots) - 1;
    dots -= 1 << shift;
    *number++ = shift + '1';
  }
  *number = 0;
  return number - numbers;
}

/* The Unicode row used for literal braille dot representations. */
#define BRL_UNICODE_ROW 0X2800

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRL_INCLUDED_ISO_11548_1 */
