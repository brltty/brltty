/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
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

/* BrailleNote/braille.h - Configurable definitions for the Braille Note driver
 * Dave Mielke <dave@mielke.cc> (June 2001)
 *
 * Edit as necessary for your system.
 */

/* The braille dots. */
#define BND_0 0000
#define BND_1 0001
#define BND_2 0002
#define BND_3 0004
#define BND_4 0010
#define BND_5 0020
#define BND_6 0040

/* The braille characters. */
#define BNC_A (BND_1)
#define BNC_B (BND_1 | BND_2)
#define BNC_C (BND_1 | BND_4)
#define BNC_D (BND_1 | BND_4 | BND_5)
#define BNC_E (BND_1 | BND_5)
#define BNC_F (BND_1 | BND_2 | BND_4)
#define BNC_G (BND_1 | BND_2 | BND_4 | BND_5)
#define BNC_H (BND_1 | BND_2 | BND_5)
#define BNC_I (BND_2 | BND_4)
#define BNC_J (BND_2 | BND_4 | BND_5)
#define BNC_K (BND_1 | BND_3)
#define BNC_L (BND_1 | BND_2 | BND_3)
#define BNC_M (BND_1 | BND_3 | BND_4)
#define BNC_N (BND_1 | BND_3 | BND_4 | BND_5)
#define BNC_O (BND_1 | BND_3 | BND_5)
#define BNC_P (BND_1 | BND_2 | BND_3 | BND_4)
#define BNC_Q (BND_1 | BND_2 | BND_3 | BND_4 | BND_5)
#define BNC_R (BND_1 | BND_2 | BND_3 | BND_5)
#define BNC_S (BND_2 | BND_3 | BND_4)
#define BNC_T (BND_2 | BND_3 | BND_4 | BND_5)
#define BNC_U (BND_1 | BND_3 | BND_6)
#define BNC_V (BND_1 | BND_2 | BND_3 | BND_6)
#define BNC_X (BND_1 | BND_3 | BND_4 | BND_6)
#define BNC_Y (BND_1 | BND_3 | BND_4 | BND_5 | BND_6)
#define BNC_Z (BND_1 | BND_3 | BND_5 | BND_6)
#define BNC_AND (BND_1 | BND_2 | BND_3 | BND_4 | BND_6)
#define BNC_EQUAL (BND_1 | BND_2 | BND_3 | BND_4 | BND_5 | BND_6)
#define BNC_LPAREN (BND_1 | BND_2 | BND_3 | BND_5 | BND_6)
#define BNC_EXCLAM (BND_2 | BND_3 | BND_4 | BND_6)
#define BNC_RPAREN (BND_2 | BND_3 | BND_4 | BND_5 | BND_6)
#define BNC_ASTERISK (BND_1 | BND_6)
#define BNC_LESS (BND_1 | BND_2 | BND_6)
#define BNC_PERCENT (BND_1 | BND_4 | BND_6)
#define BNC_QUESTION (BND_1 | BND_4 | BND_5 | BND_6)
#define BNC_COLON (BND_1 | BND_5 | BND_6)
#define BNC_DOLLAR (BND_1 | BND_2 | BND_4 | BND_6)
#define BNC_RBRACE (BND_1 | BND_2 | BND_4 | BND_5 | BND_6)
#define BNC_BAR (BND_1 | BND_2 | BND_5 | BND_6)
#define BNC_LBRACE (BND_2 | BND_4 | BND_6)
#define BNC_W (BND_2 | BND_4 | BND_5 | BND_6)
#define BNC_1 (BND_2)
#define BNC_2 (BND_2 | BND_3)
#define BNC_3 (BND_2 | BND_5)
#define BNC_4 (BND_2 | BND_5 | BND_6)
#define BNC_5 (BND_2 | BND_6)
#define BNC_6 (BND_2 | BND_3 | BND_5)
#define BNC_7 (BND_2 | BND_3 | BND_5 | BND_6)
#define BNC_8 (BND_2 | BND_3 | BND_6)
#define BNC_9 (BND_3 | BND_5)
#define BNC_0 (BND_3 | BND_5 | BND_6)
#define BNC_SLASH (BND_3 | BND_4)
#define BNC_GREATER (BND_3 | BND_4 | BND_5)
#define BNC_NUMBER (BND_3 | BND_4 | BND_5 | BND_6)
#define BNC_PLUS (BND_3 | BND_4 | BND_6)
#define BNC_ACUTE (BND_3)
#define BNC_MINUS (BND_3 | BND_6)
#define BNC_GRAVE (BND_4)
#define BNC_TILDE (BND_4 | BND_5)
#define BNC_UNDERSCORE (BND_4 | BND_5 | BND_6)
#define BNC_QUOTE (BND_5)
#define BNC_PERIOD (BND_4 | BND_6)
#define BNC_SEMICOLON (BND_5 | BND_6)
#define BNC_COMMA (BND_6)
#define BNC_SPACE 0

/* The thumb keys. */
#define BNT_PREVIOUS 0X01
#define BNT_BACK 0X02
#define BNT_ADVANCE 0X04
#define BNT_NEXT 0X08

/* Input commands. */
#define BNI_CHARACTER 0X80
#define BNI_SPACE 0X81
#define BNI_BACKSPACE 0X82
#define BNI_ENTER 0X83
#define BNI_THUMB 0X84
#define BNI_ROUTE 0X85
#define BNI_DESCRIBE 0X86
#define BNI_DISPLAY 0X1B

/* Output commands. */
#define BNO_BEGIN 0X1B
#define BNO_DESCRIBE '?'
#define BNO_WRITE 'B'
