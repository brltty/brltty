/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

/* LogText/brlconf.h - Configurable definitions for the LogText driver
 * Dave Mielke <dave@mielke.cc> (October 2001)
 *
 * Edit as necessary for your system.
 */

#define KEY_ESCAPE 0x1B /* 27 Esc key on qwerty keyboard */
#define KEY_DELETE 0x7F /* 127 Delete key */

/* KEY_FUNCTION commands */
#define KEY_FUNCTION 0x00
#define KEY_FUNCTION_CURSOR_LEFT_JUMP 0x47
#define KEY_FUNCTION_CURSOR_UP 0x48
#define KEY_FUNCTION_CURSOR_UP_JUMP 0x49
#define KEY_FUNCTION_CURSOR_LEFT 0x4B
#define KEY_FUNCTION_CURSOR_RIGHT 0x4D
#define KEY_FUNCTION_CURSOR_RIGHT_JUMP 0x4F
#define KEY_FUNCTION_CURSOR_DOWN 0x50
#define KEY_FUNCTION_CURSOR_DOWN_JUMP 0x51

/* KEY_COMMAND commands */
#define KEY_COMMAND 0x9F /* dots-37 */
#define KEY_COMMAND_SWITCHVT_PREV 0x2E /* '.' dots-3 */
#define KEY_COMMAND_SWITCHVT_NEXT 0x2C /* ',' dots-2 */
#define KEY_COMMAND_SWITCHVT_1 0x31 /* '1' dots-18 */
#define KEY_COMMAND_SWITCHVT_2 0x32 /* '2' dots-128 */
#define KEY_COMMAND_SWITCHVT_3 0x33 /* '3' dots-148 */
#define KEY_COMMAND_SWITCHVT_4 0x34 /* '4' dots-1458 */
#define KEY_COMMAND_SWITCHVT_5 0x35 /* '5' dots-158 */
#define KEY_COMMAND_SWITCHVT_6 0x36 /* '6' dots-1248 */
#define KEY_COMMAND_PAGE_UP 0x75 /* 'u' dots-136 */
#define KEY_COMMAND_PAGE_DOWN 0x64 /* 'd' dots-145 */
#define KEY_COMMAND_CUT_BEG 0x61 /* redefine */
#define KEY_COMMAND_CUT_END 0x62 /* redefine */
#define KEY_COMMAND_PASTE 0x63 /* redefine */
#define KEY_COMMAND_FREEZE_ON 0x66 /* 'f' dots-124 */
#define KEY_COMMAND_FREEZE_OFF 0x46 /* 'F' dots-1247 */
#define KEY_COMMAND_INFO 0x69 /* 'i' dots-24 */
#define KEY_COMMAND_PREFMENU 0x50 /* 'P' dots-12347 */
#define KEY_COMMAND_PREFSAVE 0x53 /* 'S' dots-2347 */
#define KEY_COMMAND_PREFLOAD 0x4C /* 'L' dots-1237 */
#define KEY_COMMAND_RESTARTBRL 0x52 /* 'R' dots-12357 */

/* Update screen, not a key */
#define KEY_UPDATE 0xFF
