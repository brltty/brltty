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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "log.h"
#include "sys_linux.h"

#include "kbd.h"
#include "kbd_internal.h"

#ifdef HAVE_LINUX_INPUT_H
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <linux/input.h>

#include "async.h"

typedef struct {
  KeyboardInstanceData *kid;
  int fileDescriptor;
} KeyboardPlatformData;

BEGIN_KEY_CODE_MAP
  [KEY_ESC] = KBD_KEY_FUNCTION(Escape),
  [KEY_1] = KBD_KEY_SYMBOL(One_Exclamation),
  [KEY_2] = KBD_KEY_SYMBOL(Two_At),
  [KEY_3] = KBD_KEY_SYMBOL(Three_Number),
  [KEY_4] = KBD_KEY_SYMBOL(Four_Dollar),
  [KEY_5] = KBD_KEY_SYMBOL(Five_Percent),
  [KEY_6] = KBD_KEY_SYMBOL(Six_Circumflex),
  [KEY_7] = KBD_KEY_SYMBOL(Seven_Ampersand),
  [KEY_8] = KBD_KEY_SYMBOL(Eight_Asterisk),
  [KEY_9] = KBD_KEY_SYMBOL(Nine_LeftParenthesis),
  [KEY_0] = KBD_KEY_SYMBOL(Zero_RightParenthesis),
  [KEY_MINUS] = KBD_KEY_SYMBOL(Minus_Underscore),
  [KEY_EQUAL] = KBD_KEY_SYMBOL(Equals_Plus),
  [KEY_BACKSPACE] = KBD_KEY_FUNCTION(DeleteBackward),
  [KEY_TAB] = KBD_KEY_FUNCTION(Tab),
  [KEY_Q] = KBD_KEY_LETTER(Q),
  [KEY_W] = KBD_KEY_LETTER(W),
  [KEY_E] = KBD_KEY_LETTER(E),
  [KEY_R] = KBD_KEY_LETTER(R),
  [KEY_T] = KBD_KEY_LETTER(T),
  [KEY_Y] = KBD_KEY_LETTER(Y),
  [KEY_U] = KBD_KEY_LETTER(U),
  [KEY_I] = KBD_KEY_LETTER(I),
  [KEY_O] = KBD_KEY_LETTER(O),
  [KEY_P] = KBD_KEY_LETTER(P),
  [KEY_LEFTBRACE] = KBD_KEY_SYMBOL(LeftBracket_LeftBrace),
  [KEY_RIGHTBRACE] = KBD_KEY_SYMBOL(RightBracket_RightBrace),
  [KEY_ENTER] = KBD_KEY_FUNCTION(Enter),
  [KEY_LEFTCTRL] = KBD_KEY_FUNCTION(ControlLeft),
  [KEY_A] = KBD_KEY_LETTER(A),
  [KEY_S] = KBD_KEY_LETTER(S),
  [KEY_D] = KBD_KEY_LETTER(D),
  [KEY_F] = KBD_KEY_LETTER(F),
  [KEY_G] = KBD_KEY_LETTER(G),
  [KEY_H] = KBD_KEY_LETTER(H),
  [KEY_J] = KBD_KEY_LETTER(J),
  [KEY_K] = KBD_KEY_LETTER(K),
  [KEY_L] = KBD_KEY_LETTER(L),
  [KEY_SEMICOLON] = KBD_KEY_SYMBOL(Semicolon_Colon),
  [KEY_APOSTROPHE] = KBD_KEY_SYMBOL(Apostrophe_Quote),
  [KEY_GRAVE] = KBD_KEY_SYMBOL(Grave_Tilde),
  [KEY_LEFTSHIFT] = KBD_KEY_FUNCTION(ShiftLeft),
  [KEY_BACKSLASH] = KBD_KEY_SYMBOL(Backslash_Bar),
  [KEY_Z] = KBD_KEY_LETTER(Z),
  [KEY_X] = KBD_KEY_LETTER(X),
  [KEY_C] = KBD_KEY_LETTER(C),
  [KEY_V] = KBD_KEY_LETTER(V),
  [KEY_B] = KBD_KEY_LETTER(B),
  [KEY_N] = KBD_KEY_LETTER(N),
  [KEY_M] = KBD_KEY_LETTER(M),
  [KEY_COMMA] = KBD_KEY_SYMBOL(Comma_Less),
  [KEY_DOT] = KBD_KEY_SYMBOL(Period_Greater),
  [KEY_SLASH] = KBD_KEY_SYMBOL(Slash_Question),
  [KEY_RIGHTSHIFT] = KBD_KEY_FUNCTION(ShiftRight),
  [KEY_KPASTERISK] = KBD_KEY_KEYPAD(Asterisk),
  [KEY_LEFTALT] = KBD_KEY_FUNCTION(AltLeft),
  [KEY_SPACE] = KBD_KEY_FUNCTION(Space),
  [KEY_CAPSLOCK] = KBD_KEY_LOCK(Capitals),
  [KEY_F1] = KBD_KEY_FUNCTION(F1),
  [KEY_F2] = KBD_KEY_FUNCTION(F2),
  [KEY_F3] = KBD_KEY_FUNCTION(F3),
  [KEY_F4] = KBD_KEY_FUNCTION(F4),
  [KEY_F5] = KBD_KEY_FUNCTION(F5),
  [KEY_F6] = KBD_KEY_FUNCTION(F6),
  [KEY_F7] = KBD_KEY_FUNCTION(F7),
  [KEY_F8] = KBD_KEY_FUNCTION(F8),
  [KEY_F9] = KBD_KEY_FUNCTION(F9),
  [KEY_F10] = KBD_KEY_FUNCTION(F10),
  [KEY_NUMLOCK] = KBD_KEY_LOCKING(Numbers),
  [KEY_SCROLLLOCK] = KBD_KEY_LOCKING(Scroll),
  [KEY_KP7] = KBD_KEY_KEYPAD(Seven_Home),
  [KEY_KP8] = KBD_KEY_KEYPAD(Eight_ArrowUp),
  [KEY_KP9] = KBD_KEY_KEYPAD(Nine_PageUp),
  [KEY_KPMINUS] = KBD_KEY_KEYPAD(Minus),
  [KEY_KP4] = KBD_KEY_KEYPAD(Four_ArrowLeft),
  [KEY_KP5] = KBD_KEY_KEYPAD(Five),
  [KEY_KP6] = KBD_KEY_KEYPAD(Six_ArrowRight),
  [KEY_KPPLUS] = KBD_KEY_KEYPAD(Plus),
  [KEY_KP1] = KBD_KEY_KEYPAD(One_End),
  [KEY_KP2] = KBD_KEY_KEYPAD(Two_ArrowDown),
  [KEY_KP3] = KBD_KEY_KEYPAD(Three_PageDown),
  [KEY_KP0] = KBD_KEY_KEYPAD(Zero_Insert),
  [KEY_KPDOT] = KBD_KEY_KEYPAD(Period_Delete),
  [KEY_ZENKAKUHANKAKU] = KBD_KEY_UNMAPPED,
  [KEY_102ND] = KBD_KEY_SYMBOL(Europe2),
  [KEY_F11] = KBD_KEY_FUNCTION(F11),
  [KEY_F12] = KBD_KEY_FUNCTION(F12),
  [KEY_RO] = KBD_KEY_UNMAPPED,
  [KEY_KATAKANA] = KBD_KEY_UNMAPPED,
  [KEY_HIRAGANA] = KBD_KEY_UNMAPPED,
  [KEY_HENKAN] = KBD_KEY_UNMAPPED,
  [KEY_KATAKANAHIRAGANA] = KBD_KEY_UNMAPPED,
  [KEY_MUHENKAN] = KBD_KEY_UNMAPPED,
  [KEY_KPJPCOMMA] = KBD_KEY_UNMAPPED,
  [KEY_KPENTER] = KBD_KEY_KEYPAD(Enter),
  [KEY_RIGHTCTRL] = KBD_KEY_FUNCTION(ControlRight),
  [KEY_KPSLASH] = KBD_KEY_KEYPAD(Slash),
  [KEY_SYSRQ] = KBD_KEY_FUNCTION(SystemRequest),
  [KEY_RIGHTALT] = KBD_KEY_FUNCTION(AltRight),
  [KEY_LINEFEED] = KBD_KEY_UNMAPPED,
  [KEY_HOME] = KBD_KEY_FUNCTION(Home),
  [KEY_UP] = KBD_KEY_FUNCTION(ArrowUp),
  [KEY_PAGEUP] = KBD_KEY_FUNCTION(PageUp),
  [KEY_LEFT] = KBD_KEY_FUNCTION(ArrowLeft),
  [KEY_RIGHT] = KBD_KEY_FUNCTION(ArrowRight),
  [KEY_END] = KBD_KEY_FUNCTION(End),
  [KEY_DOWN] = KBD_KEY_FUNCTION(ArrowDown),
  [KEY_PAGEDOWN] = KBD_KEY_FUNCTION(PageDown),
  [KEY_INSERT] = KBD_KEY_FUNCTION(Insert),
  [KEY_DELETE] = KBD_KEY_FUNCTION(DeleteForward),
  [KEY_MACRO] = KBD_KEY_UNMAPPED,
  [KEY_MUTE] = KBD_KEY_MEDIA(Mute),
  [KEY_VOLUMEDOWN] = KBD_KEY_MEDIA(VolumeDown),
  [KEY_VOLUMEUP] = KBD_KEY_MEDIA(VolumeUp),
  [KEY_POWER] = KBD_KEY_FUNCTION(Power),
  [KEY_KPEQUAL] = KBD_KEY_KEYPAD(Equals),
  [KEY_KPPLUSMINUS] = KBD_KEY_KEYPAD(PlusMinus),
  [KEY_PAUSE] = KBD_KEY_FUNCTION(Pause),
  [KEY_KPCOMMA] = KBD_KEY_KEYPAD(Comma),
  [KEY_HANGEUL] = KBD_KEY_UNMAPPED,
  [KEY_HANGUEL] = KBD_KEY_UNMAPPED,
  [KEY_HANJA] = KBD_KEY_UNMAPPED,
  [KEY_YEN] = KBD_KEY_UNMAPPED,
  [KEY_LEFTMETA] = KBD_KEY_UNMAPPED,
  [KEY_RIGHTMETA] = KBD_KEY_UNMAPPED,
  [KEY_COMPOSE] = KBD_KEY_UNMAPPED,
  [KEY_STOP] = KBD_KEY_FUNCTION(Stop),
  [KEY_AGAIN] = KBD_KEY_FUNCTION(Again),
  [KEY_PROPS] = KBD_KEY_UNMAPPED,
  [KEY_UNDO] = KBD_KEY_FUNCTION(Undo),
  [KEY_FRONT] = KBD_KEY_UNMAPPED,
  [KEY_COPY] = KBD_KEY_FUNCTION(Copy),
  [KEY_OPEN] = KBD_KEY_FUNCTION(Open),
  [KEY_PASTE] = KBD_KEY_FUNCTION(Paste),
  [KEY_FIND] = KBD_KEY_FUNCTION(Find),
  [KEY_CUT] = KBD_KEY_FUNCTION(Cut),
  [KEY_HELP] = KBD_KEY_FUNCTION(Help),
  [KEY_MENU] = KBD_KEY_FUNCTION(Menu),
  [KEY_CALC] = KBD_KEY_UNMAPPED,
  [KEY_SETUP] = KBD_KEY_UNMAPPED,
  [KEY_SLEEP] = KBD_KEY_UNMAPPED,
  [KEY_WAKEUP] = KBD_KEY_UNMAPPED,
  [KEY_FILE] = KBD_KEY_UNMAPPED,
  [KEY_SENDFILE] = KBD_KEY_UNMAPPED,
  [KEY_DELETEFILE] = KBD_KEY_UNMAPPED,
  [KEY_XFER] = KBD_KEY_UNMAPPED,
  [KEY_PROG1] = KBD_KEY_UNMAPPED,
  [KEY_PROG2] = KBD_KEY_UNMAPPED,
  [KEY_WWW] = KBD_KEY_UNMAPPED,
  [KEY_MSDOS] = KBD_KEY_UNMAPPED,
  [KEY_COFFEE] = KBD_KEY_UNMAPPED,
  [KEY_SCREENLOCK] = KBD_KEY_UNMAPPED,
  [KEY_DIRECTION] = KBD_KEY_UNMAPPED,
  [KEY_CYCLEWINDOWS] = KBD_KEY_UNMAPPED,
  [KEY_MAIL] = KBD_KEY_UNMAPPED,
  [KEY_BOOKMARKS] = KBD_KEY_UNMAPPED,
  [KEY_COMPUTER] = KBD_KEY_UNMAPPED,
  [KEY_BACK] = KBD_KEY_UNMAPPED,
  [KEY_FORWARD] = KBD_KEY_UNMAPPED,
  [KEY_CLOSECD] = KBD_KEY_MEDIA(Close),
  [KEY_EJECTCD] = KBD_KEY_MEDIA(Eject),
  [KEY_EJECTCLOSECD] = KBD_KEY_MEDIA(EjectClose),
  [KEY_NEXTSONG] = KBD_KEY_MEDIA(Next),
  [KEY_PLAYPAUSE] = KBD_KEY_MEDIA(PlayPause),
  [KEY_PREVIOUSSONG] = KBD_KEY_MEDIA(Previous),
  [KEY_STOPCD] = KBD_KEY_MEDIA(Stop),
  [KEY_RECORD] = KBD_KEY_MEDIA(Record),
  [KEY_REWIND] = KBD_KEY_MEDIA(Backward),
  [KEY_PHONE] = KBD_KEY_UNMAPPED,
  [KEY_ISO] = KBD_KEY_UNMAPPED,
  [KEY_CONFIG] = KBD_KEY_UNMAPPED,
  [KEY_HOMEPAGE] = KBD_KEY_UNMAPPED,
  [KEY_REFRESH] = KBD_KEY_UNMAPPED,
  [KEY_EXIT] = KBD_KEY_UNMAPPED,
  [KEY_MOVE] = KBD_KEY_UNMAPPED,
  [KEY_EDIT] = KBD_KEY_UNMAPPED,
  [KEY_SCROLLUP] = KBD_KEY_UNMAPPED,
  [KEY_SCROLLDOWN] = KBD_KEY_UNMAPPED,
  [KEY_KPLEFTPAREN] = KBD_KEY_KEYPAD(LeftParenthesis),
  [KEY_KPRIGHTPAREN] = KBD_KEY_KEYPAD(RightParenthesis),
  [KEY_NEW] = KBD_KEY_UNMAPPED,
  [KEY_REDO] = KBD_KEY_UNMAPPED,
  [KEY_F13] = KBD_KEY_FUNCTION(F13),
  [KEY_F14] = KBD_KEY_FUNCTION(F14),
  [KEY_F15] = KBD_KEY_FUNCTION(F15),
  [KEY_F16] = KBD_KEY_FUNCTION(F16),
  [KEY_F17] = KBD_KEY_FUNCTION(F17),
  [KEY_F18] = KBD_KEY_FUNCTION(F18),
  [KEY_F19] = KBD_KEY_FUNCTION(F19),
  [KEY_F20] = KBD_KEY_FUNCTION(F20),
  [KEY_F21] = KBD_KEY_FUNCTION(F21),
  [KEY_F22] = KBD_KEY_FUNCTION(F22),
  [KEY_F23] = KBD_KEY_FUNCTION(F23),
  [KEY_F24] = KBD_KEY_FUNCTION(F24),
  [KEY_PLAYCD] = KBD_KEY_MEDIA(Play),
  [KEY_PAUSECD] = KBD_KEY_MEDIA(Pause),
  [KEY_PROG3] = KBD_KEY_UNMAPPED,
  [KEY_PROG4] = KBD_KEY_UNMAPPED,
  [KEY_SUSPEND] = KBD_KEY_UNMAPPED,
  [KEY_CLOSE] = KBD_KEY_UNMAPPED,
  [KEY_PLAY] = KBD_KEY_UNMAPPED,
  [KEY_FASTFORWARD] = KBD_KEY_MEDIA(Forward),
  [KEY_BASSBOOST] = KBD_KEY_UNMAPPED,
  [KEY_PRINT] = KBD_KEY_UNMAPPED,
  [KEY_HP] = KBD_KEY_UNMAPPED,
  [KEY_CAMERA] = KBD_KEY_UNMAPPED,
  [KEY_SOUND] = KBD_KEY_UNMAPPED,
  [KEY_QUESTION] = KBD_KEY_UNMAPPED,
  [KEY_EMAIL] = KBD_KEY_UNMAPPED,
  [KEY_CHAT] = KBD_KEY_UNMAPPED,
  [KEY_SEARCH] = KBD_KEY_UNMAPPED,
  [KEY_CONNECT] = KBD_KEY_UNMAPPED,
  [KEY_FINANCE] = KBD_KEY_UNMAPPED,
  [KEY_SPORT] = KBD_KEY_UNMAPPED,
  [KEY_SHOP] = KBD_KEY_UNMAPPED,
  [KEY_ALTERASE] = KBD_KEY_UNMAPPED,
  [KEY_CANCEL] = KBD_KEY_UNMAPPED,
  [KEY_BRIGHTNESSDOWN] = KBD_KEY_UNMAPPED,
  [KEY_BRIGHTNESSUP] = KBD_KEY_UNMAPPED,
  [KEY_MEDIA] = KBD_KEY_UNMAPPED,
  [KEY_SWITCHVIDEOMODE] = KBD_KEY_UNMAPPED,
  [KEY_KBDILLUMTOGGLE] = KBD_KEY_UNMAPPED,
  [KEY_KBDILLUMDOWN] = KBD_KEY_UNMAPPED,
  [KEY_KBDILLUMUP] = KBD_KEY_UNMAPPED,
  [KEY_SEND] = KBD_KEY_UNMAPPED,
  [KEY_REPLY] = KBD_KEY_UNMAPPED,
  [KEY_FORWARDMAIL] = KBD_KEY_UNMAPPED,
  [KEY_SAVE] = KBD_KEY_UNMAPPED,
  [KEY_DOCUMENTS] = KBD_KEY_UNMAPPED,
  [KEY_BATTERY] = KBD_KEY_UNMAPPED,
  [KEY_BLUETOOTH] = KBD_KEY_UNMAPPED,
  [KEY_WLAN] = KBD_KEY_UNMAPPED,
  [KEY_UWB] = KBD_KEY_UNMAPPED,
  [KEY_UNKNOWN] = KBD_KEY_UNMAPPED,
  [KEY_VIDEO_NEXT] = KBD_KEY_UNMAPPED,
  [KEY_VIDEO_PREV] = KBD_KEY_UNMAPPED,
  [KEY_BRIGHTNESS_CYCLE] = KBD_KEY_UNMAPPED,
  [KEY_BRIGHTNESS_ZERO] = KBD_KEY_UNMAPPED,
  [KEY_DISPLAY_OFF] = KBD_KEY_UNMAPPED,
  [KEY_WIMAX] = KBD_KEY_UNMAPPED,
  [BTN_MISC] = KBD_KEY_UNMAPPED,
  [BTN_0] = KBD_KEY_UNMAPPED,
  [BTN_1] = KBD_KEY_UNMAPPED,
  [BTN_2] = KBD_KEY_UNMAPPED,
  [BTN_3] = KBD_KEY_UNMAPPED,
  [BTN_4] = KBD_KEY_UNMAPPED,
  [BTN_5] = KBD_KEY_UNMAPPED,
  [BTN_6] = KBD_KEY_UNMAPPED,
  [BTN_7] = KBD_KEY_UNMAPPED,
  [BTN_8] = KBD_KEY_UNMAPPED,
  [BTN_9] = KBD_KEY_UNMAPPED,
  [BTN_MOUSE] = KBD_KEY_UNMAPPED,
  [BTN_LEFT] = KBD_KEY_UNMAPPED,
  [BTN_RIGHT] = KBD_KEY_UNMAPPED,
  [BTN_MIDDLE] = KBD_KEY_UNMAPPED,
  [BTN_SIDE] = KBD_KEY_UNMAPPED,
  [BTN_EXTRA] = KBD_KEY_UNMAPPED,
  [BTN_FORWARD] = KBD_KEY_UNMAPPED,
  [BTN_BACK] = KBD_KEY_UNMAPPED,
  [BTN_TASK] = KBD_KEY_UNMAPPED,
  [BTN_JOYSTICK] = KBD_KEY_UNMAPPED,
  [BTN_TRIGGER] = KBD_KEY_UNMAPPED,
  [BTN_THUMB] = KBD_KEY_UNMAPPED,
  [BTN_THUMB2] = KBD_KEY_UNMAPPED,
  [BTN_TOP] = KBD_KEY_UNMAPPED,
  [BTN_TOP2] = KBD_KEY_UNMAPPED,
  [BTN_PINKIE] = KBD_KEY_UNMAPPED,
  [BTN_BASE] = KBD_KEY_UNMAPPED,
  [BTN_BASE2] = KBD_KEY_UNMAPPED,
  [BTN_BASE3] = KBD_KEY_UNMAPPED,
  [BTN_BASE4] = KBD_KEY_UNMAPPED,
  [BTN_BASE5] = KBD_KEY_UNMAPPED,
  [BTN_BASE6] = KBD_KEY_UNMAPPED,
  [BTN_DEAD] = KBD_KEY_UNMAPPED,
  [BTN_GAMEPAD] = KBD_KEY_UNMAPPED,
  [BTN_A] = KBD_KEY_UNMAPPED,
  [BTN_B] = KBD_KEY_UNMAPPED,
  [BTN_C] = KBD_KEY_UNMAPPED,
  [BTN_X] = KBD_KEY_UNMAPPED,
  [BTN_Y] = KBD_KEY_UNMAPPED,
  [BTN_Z] = KBD_KEY_UNMAPPED,
  [BTN_TL] = KBD_KEY_UNMAPPED,
  [BTN_TR] = KBD_KEY_UNMAPPED,
  [BTN_TL2] = KBD_KEY_UNMAPPED,
  [BTN_TR2] = KBD_KEY_UNMAPPED,
  [BTN_SELECT] = KBD_KEY_UNMAPPED,
  [BTN_START] = KBD_KEY_UNMAPPED,
  [BTN_MODE] = KBD_KEY_UNMAPPED,
  [BTN_THUMBL] = KBD_KEY_UNMAPPED,
  [BTN_THUMBR] = KBD_KEY_UNMAPPED,
  [BTN_DIGI] = KBD_KEY_UNMAPPED,
  [BTN_TOOL_PEN] = KBD_KEY_UNMAPPED,
  [BTN_TOOL_RUBBER] = KBD_KEY_UNMAPPED,
  [BTN_TOOL_BRUSH] = KBD_KEY_UNMAPPED,
  [BTN_TOOL_PENCIL] = KBD_KEY_UNMAPPED,
  [BTN_TOOL_AIRBRUSH] = KBD_KEY_UNMAPPED,
  [BTN_TOOL_FINGER] = KBD_KEY_UNMAPPED,
  [BTN_TOOL_MOUSE] = KBD_KEY_UNMAPPED,
  [BTN_TOOL_LENS] = KBD_KEY_UNMAPPED,
  [BTN_TOUCH] = KBD_KEY_UNMAPPED,
  [BTN_STYLUS] = KBD_KEY_UNMAPPED,
  [BTN_STYLUS2] = KBD_KEY_UNMAPPED,
  [BTN_TOOL_DOUBLETAP] = KBD_KEY_UNMAPPED,
  [BTN_TOOL_TRIPLETAP] = KBD_KEY_UNMAPPED,
  [BTN_WHEEL] = KBD_KEY_UNMAPPED,
  [BTN_GEAR_DOWN] = KBD_KEY_UNMAPPED,
  [BTN_GEAR_UP] = KBD_KEY_UNMAPPED,
  [KEY_OK] = KBD_KEY_UNMAPPED,
  [KEY_SELECT] = KBD_KEY_FUNCTION(Select),
  [KEY_GOTO] = KBD_KEY_UNMAPPED,
  [KEY_CLEAR] = KBD_KEY_FUNCTION(Clear),
  [KEY_POWER2] = KBD_KEY_UNMAPPED,
  [KEY_OPTION] = KBD_KEY_UNMAPPED,
  [KEY_INFO] = KBD_KEY_UNMAPPED,
  [KEY_TIME] = KBD_KEY_UNMAPPED,
  [KEY_VENDOR] = KBD_KEY_UNMAPPED,
  [KEY_ARCHIVE] = KBD_KEY_UNMAPPED,
  [KEY_PROGRAM] = KBD_KEY_UNMAPPED,
  [KEY_CHANNEL] = KBD_KEY_UNMAPPED,
  [KEY_FAVORITES] = KBD_KEY_UNMAPPED,
  [KEY_EPG] = KBD_KEY_UNMAPPED,
  [KEY_PVR] = KBD_KEY_UNMAPPED,
  [KEY_MHP] = KBD_KEY_UNMAPPED,
  [KEY_LANGUAGE] = KBD_KEY_UNMAPPED,
  [KEY_TITLE] = KBD_KEY_UNMAPPED,
  [KEY_SUBTITLE] = KBD_KEY_UNMAPPED,
  [KEY_ANGLE] = KBD_KEY_UNMAPPED,
  [KEY_ZOOM] = KBD_KEY_UNMAPPED,
  [KEY_MODE] = KBD_KEY_UNMAPPED,
  [KEY_KEYBOARD] = KBD_KEY_UNMAPPED,
  [KEY_SCREEN] = KBD_KEY_UNMAPPED,
  [KEY_PC] = KBD_KEY_UNMAPPED,
  [KEY_TV] = KBD_KEY_UNMAPPED,
  [KEY_TV2] = KBD_KEY_UNMAPPED,
  [KEY_VCR] = KBD_KEY_UNMAPPED,
  [KEY_VCR2] = KBD_KEY_UNMAPPED,
  [KEY_SAT] = KBD_KEY_UNMAPPED,
  [KEY_SAT2] = KBD_KEY_UNMAPPED,
  [KEY_CD] = KBD_KEY_UNMAPPED,
  [KEY_TAPE] = KBD_KEY_UNMAPPED,
  [KEY_RADIO] = KBD_KEY_UNMAPPED,
  [KEY_TUNER] = KBD_KEY_UNMAPPED,
  [KEY_PLAYER] = KBD_KEY_UNMAPPED,
  [KEY_TEXT] = KBD_KEY_UNMAPPED,
  [KEY_DVD] = KBD_KEY_UNMAPPED,
  [KEY_AUX] = KBD_KEY_UNMAPPED,
  [KEY_MP3] = KBD_KEY_UNMAPPED,
  [KEY_AUDIO] = KBD_KEY_UNMAPPED,
  [KEY_VIDEO] = KBD_KEY_UNMAPPED,
  [KEY_DIRECTORY] = KBD_KEY_UNMAPPED,
  [KEY_LIST] = KBD_KEY_UNMAPPED,
  [KEY_MEMO] = KBD_KEY_UNMAPPED,
  [KEY_CALENDAR] = KBD_KEY_UNMAPPED,
  [KEY_RED] = KBD_KEY_UNMAPPED,
  [KEY_GREEN] = KBD_KEY_UNMAPPED,
  [KEY_YELLOW] = KBD_KEY_UNMAPPED,
  [KEY_BLUE] = KBD_KEY_UNMAPPED,
  [KEY_CHANNELUP] = KBD_KEY_UNMAPPED,
  [KEY_CHANNELDOWN] = KBD_KEY_UNMAPPED,
  [KEY_FIRST] = KBD_KEY_UNMAPPED,
  [KEY_LAST] = KBD_KEY_UNMAPPED,
  [KEY_AB] = KBD_KEY_UNMAPPED,
  [KEY_NEXT] = KBD_KEY_UNMAPPED,
  [KEY_RESTART] = KBD_KEY_UNMAPPED,
  [KEY_SLOW] = KBD_KEY_UNMAPPED,
  [KEY_SHUFFLE] = KBD_KEY_UNMAPPED,
  [KEY_BREAK] = KBD_KEY_UNMAPPED,
  [KEY_PREVIOUS] = KBD_KEY_UNMAPPED,
  [KEY_DIGITS] = KBD_KEY_UNMAPPED,
  [KEY_TEEN] = KBD_KEY_UNMAPPED,
  [KEY_TWEN] = KBD_KEY_UNMAPPED,
  [KEY_VIDEOPHONE] = KBD_KEY_UNMAPPED,
  [KEY_GAMES] = KBD_KEY_UNMAPPED,
  [KEY_ZOOMIN] = KBD_KEY_UNMAPPED,
  [KEY_ZOOMOUT] = KBD_KEY_UNMAPPED,
  [KEY_ZOOMRESET] = KBD_KEY_UNMAPPED,
  [KEY_WORDPROCESSOR] = KBD_KEY_UNMAPPED,
  [KEY_EDITOR] = KBD_KEY_UNMAPPED,
  [KEY_SPREADSHEET] = KBD_KEY_UNMAPPED,
  [KEY_GRAPHICSEDITOR] = KBD_KEY_UNMAPPED,
  [KEY_PRESENTATION] = KBD_KEY_UNMAPPED,
  [KEY_DATABASE] = KBD_KEY_UNMAPPED,
  [KEY_NEWS] = KBD_KEY_UNMAPPED,
  [KEY_VOICEMAIL] = KBD_KEY_UNMAPPED,
  [KEY_ADDRESSBOOK] = KBD_KEY_UNMAPPED,
  [KEY_MESSENGER] = KBD_KEY_UNMAPPED,
  [KEY_DISPLAYTOGGLE] = KBD_KEY_UNMAPPED,
  [KEY_SPELLCHECK] = KBD_KEY_UNMAPPED,
  [KEY_LOGOFF] = KBD_KEY_UNMAPPED,
  [KEY_DOLLAR] = KBD_KEY_UNMAPPED,
  [KEY_EURO] = KBD_KEY_UNMAPPED,
  [KEY_FRAMEBACK] = KBD_KEY_UNMAPPED,
  [KEY_FRAMEFORWARD] = KBD_KEY_UNMAPPED,
  [KEY_CONTEXT_MENU] = KBD_KEY_UNMAPPED,
  [KEY_MEDIA_REPEAT] = KBD_KEY_UNMAPPED,
  [KEY_DEL_EOL] = KBD_KEY_UNMAPPED,
  [KEY_DEL_EOS] = KBD_KEY_UNMAPPED,
  [KEY_INS_LINE] = KBD_KEY_UNMAPPED,
  [KEY_DEL_LINE] = KBD_KEY_UNMAPPED,
  [KEY_FN] = KBD_KEY_UNMAPPED,
  [KEY_FN_ESC] = KBD_KEY_UNMAPPED,
  [KEY_FN_F1] = KBD_KEY_UNMAPPED,
  [KEY_FN_F2] = KBD_KEY_UNMAPPED,
  [KEY_FN_F3] = KBD_KEY_UNMAPPED,
  [KEY_FN_F4] = KBD_KEY_UNMAPPED,
  [KEY_FN_F5] = KBD_KEY_UNMAPPED,
  [KEY_FN_F6] = KBD_KEY_UNMAPPED,
  [KEY_FN_F7] = KBD_KEY_UNMAPPED,
  [KEY_FN_F8] = KBD_KEY_UNMAPPED,
  [KEY_FN_F9] = KBD_KEY_UNMAPPED,
  [KEY_FN_F10] = KBD_KEY_UNMAPPED,
  [KEY_FN_F11] = KBD_KEY_UNMAPPED,
  [KEY_FN_F12] = KBD_KEY_UNMAPPED,
  [KEY_FN_1] = KBD_KEY_UNMAPPED,
  [KEY_FN_2] = KBD_KEY_UNMAPPED,
  [KEY_FN_D] = KBD_KEY_UNMAPPED,
  [KEY_FN_E] = KBD_KEY_UNMAPPED,
  [KEY_FN_F] = KBD_KEY_UNMAPPED,
  [KEY_FN_S] = KBD_KEY_UNMAPPED,
  [KEY_FN_B] = KBD_KEY_UNMAPPED,
  [KEY_BRL_DOT1] = KBD_KEY_UNMAPPED,
  [KEY_BRL_DOT2] = KBD_KEY_UNMAPPED,
  [KEY_BRL_DOT3] = KBD_KEY_UNMAPPED,
  [KEY_BRL_DOT4] = KBD_KEY_UNMAPPED,
  [KEY_BRL_DOT5] = KBD_KEY_UNMAPPED,
  [KEY_BRL_DOT6] = KBD_KEY_UNMAPPED,
  [KEY_BRL_DOT7] = KBD_KEY_UNMAPPED,
  [KEY_BRL_DOT8] = KBD_KEY_UNMAPPED,
  [KEY_BRL_DOT9] = KBD_KEY_UNMAPPED,
  [KEY_BRL_DOT10] = KBD_KEY_UNMAPPED,
  [KEY_MIN_INTERESTING] = KBD_KEY_UNMAPPED,
END_KEY_CODE_MAP

static void
closeKeyboard (KeyboardPlatformData *kpd) {
  close(kpd->fileDescriptor);
  deallocateKeyboardInstanceData(kpd->kid);
  free(kpd);
}

int
forwardKeyEvent (int code, int press) {
  return writeKeyEvent(code, (press? 1: 0));
}

static size_t
handleKeyboardEvent (const AsyncInputResult *result) {
  KeyboardPlatformData *kpd = result->data;

  if (result->error) {
    logMessage(LOG_DEBUG, "keyboard read error: fd=%d: %s",
               kpd->fileDescriptor, strerror(result->error));
    closeKeyboard(kpd);
  } else if (result->end) {
    logMessage(LOG_DEBUG, "keyboard end-of-file: fd=%d", kpd->fileDescriptor);
    closeKeyboard(kpd);
  } else {
    const struct input_event *event = result->buffer;

    if (result->length >= sizeof(*event)) {
      if (event->type == EV_KEY) {
        int release = event->value == 0;
        int press   = event->value == 1;

        if (release || press) handleKeyEvent(kpd->kid, event->code, press);
      } else {
        writeInputEvent(event->type, event->code, event->value);
      }

      return sizeof(*event);
    }
  }

  return 0;
}

static int
monitorKeyboard (int device, KeyboardCommonData *kcd) {
  struct stat status;

  if (fstat(device, &status) != -1) {
    if (S_ISCHR(status.st_mode)) {
      KeyboardPlatformData *kpd;

      if ((kpd = malloc(sizeof(*kpd)))) {
        memset(kpd, 0, sizeof(*kpd));
        kpd->fileDescriptor = device;

        if ((kpd->kid = newKeyboardInstanceData(kcd))) {
          {
            struct input_id identity;
            if (ioctl(device, EVIOCGID, &identity) != -1) {
              logMessage(LOG_DEBUG, "input device identity: fd=%d: type=%04X vendor=%04X product=%04X version=%04X",
                         device, identity.bustype, identity.vendor, identity.product, identity.version);

              {
                static const KeyboardType typeTable[] = {
  #ifdef BUS_I8042
                  [BUS_I8042] = KBD_TYPE_PS2,
  #endif /* BUS_I8042 */

  #ifdef BUS_USB
                  [BUS_USB] = KBD_TYPE_USB,
  #endif /* BUS_USB */

  #ifdef BUS_BLUETOOTH
                  [BUS_BLUETOOTH] = KBD_TYPE_Bluetooth,
  #endif /* BUS_BLUETOOTH */
                };

                if (identity.bustype < ARRAY_COUNT(typeTable))
                  kpd->kid->actualProperties.type = typeTable[identity.bustype];
              }

              kpd->kid->actualProperties.vendor = identity.vendor;
              kpd->kid->actualProperties.product = identity.product;
            } else {
              logMessage(LOG_DEBUG, "cannot get input device identity: fd=%d: %s",
                         device, strerror(errno));
            }
          }
        
          if (kpd->kid->actualProperties.type) {
            if (checkKeyboardProperties(&kpd->kid->actualProperties, &kcd->requiredProperties)) {
              if (hasInputEvent(device, EV_KEY, KEY_ENTER, KEY_MAX)) {
                if (asyncRead(NULL, device, sizeof(struct input_event), handleKeyboardEvent, kpd)) {
  #ifdef EVIOCGRAB
                  ioctl(device, EVIOCGRAB, 1);
  #endif /* EVIOCGRAB */

                  logMessage(LOG_DEBUG, "keyboard found: fd=%d", device);
                  return 1;
                }
              }
            }
          }

          deallocateKeyboardInstanceData(kpd->kid);
        }

        free(kpd);
      }
    }
  } else {
    logMessage(LOG_DEBUG, "cannot stat input device: fd=%d: %s", device, strerror(errno));
  }

  return 0;
}

static void
monitorCurrentKeyboards (KeyboardCommonData *kcd) {
  const char *root = "/dev/input";
  const size_t rootLength = strlen(root);
  DIR *directory;

  logMessage(LOG_DEBUG, "searching for keyboards");
  if ((directory = opendir(root))) {
    struct dirent *entry;

    while ((entry = readdir(directory))) {
      const size_t nameLength = strlen(entry->d_name);
      char path[rootLength + 1 + nameLength + 1];
      int device;

      snprintf(path, sizeof(path), "%s/%s", root, entry->d_name);
      if ((device = open(path, O_RDONLY)) != -1) {
        logMessage(LOG_DEBUG, "input device opened: %s: fd=%d", path, device);
        if (monitorKeyboard(device, kcd)) continue;

        close(device);
        logMessage(LOG_DEBUG, "input device closed: %s: fd=%d", path, device);
      } else {
        logMessage(LOG_DEBUG, "cannot open input device: %s: %s", path, strerror(errno));
      }
    }

    closedir(directory);
  } else {
    logMessage(LOG_DEBUG, "cannot open directory: %s: %s", root, strerror(errno));
  }
  logMessage(LOG_DEBUG, "keyboard search complete");
}

#ifdef NETLINK_KOBJECT_UEVENT
typedef struct {
  char *name;
  int major;
  int minor;
  KeyboardCommonData *kcd;
} InputDeviceData;

static void
doOpenInputDevice (const AsyncAlarmResult *result) {
  InputDeviceData *idd = result->data;
  int device = openCharacterDevice(idd->name, O_RDONLY, idd->major, idd->minor);

  if (device != -1) {
    logMessage(LOG_DEBUG, "input device opened: %s: fd=%d", idd->name, device);
    if (!monitorKeyboard(device, idd->kcd)) {
      close(device);
      logMessage(LOG_DEBUG, "input device closed: %s: fd=%d", idd->name, device);
    }
  } else {
    logMessage(LOG_DEBUG, "cannot open input device: %s: %s", idd->name, strerror(errno));
  }

  free(idd->name);
  releaseKeyboardCommonData(idd->kcd);
  free(idd);
}

static size_t
handleKobjectUeventString (const AsyncInputResult *result) {
  if (result->error) {
    logMessage(LOG_DEBUG, "netlink read error: %s", strerror(result->error));
  } else if (result->end) {
    logMessage(LOG_DEBUG, "netlink end-of-file");
    releaseKeyboardCommonData(result->data);
  } else {
    const char *buffer = result->buffer;
    const char *end = memchr(buffer, 0, result->length);

    if (end) {
      const char *path = strchr(buffer, '@');

      if (path) {
        const char *action = buffer;
        int actionLength = path++ - action;

        logMessage(LOG_DEBUG, "OBJECT_UEVENT: %.*s %s", actionLength, action, path);
        if (strncmp(action, "add", actionLength) == 0) {
          const char *suffix = path;

          while ((suffix = strstr(suffix, "/input"))) {
            int inputNumber, eventNumber;

            if (sscanf(++suffix, "input%d/event%d", &inputNumber, &eventNumber) == 2) {
              static const char sysfsRoot[] = "/sys";
              static const char devName[] = "/dev";
              char sysfsPath[strlen(sysfsRoot) + strlen(path) + sizeof(devName)];
              int descriptor;

              snprintf(sysfsPath, sizeof(sysfsPath), "%s%s%s", sysfsRoot, path, devName);
              if ((descriptor = open(sysfsPath, O_RDONLY)) != -1) {
                char stringBuffer[0X10];
                int stringLength;

                if ((stringLength = read(descriptor, stringBuffer, sizeof(stringBuffer))) > 0) {
                  InputDeviceData *idd;

                  if ((idd = malloc(sizeof(*idd)))) {
                    if (sscanf(stringBuffer, "%d:%d", &idd->major, &idd->minor) == 2) {
                      char eventDevice[0X40];
                      snprintf(eventDevice, sizeof(eventDevice), "input/event%d", eventNumber);

                      if ((idd->name = strdup(eventDevice))) {
                        idd->kcd = result->data;

                        if (asyncRelativeAlarm(NULL, 1000, doOpenInputDevice, idd)) {
                          claimKeyboardCommonData(idd->kcd);
                          close(descriptor);
                          break;
                        }

                        free(idd->name);
                      } else {
                        logMallocError();
                      }
                    }

                    free(idd);
                  } else {
                    logMallocError();
                  }
                }

                close(descriptor);
              }
            }
          }
        }
      }

      return end - buffer + 1;
    }
  }

  return 0;
}

static int
getKobjectUeventSocket (void) {
  static int netlinkSocket = -1;

  if (netlinkSocket == -1) {
    const struct sockaddr_nl socketAddress = {
      .nl_family = AF_NETLINK,
      .nl_pid = getpid(),
      .nl_groups = 0XFFFFFFFF
    };

    if ((netlinkSocket = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT)) != -1) {
      if (bind(netlinkSocket, (const struct sockaddr *)&socketAddress, sizeof(socketAddress)) == -1) {
        logSystemError("bind");
        close(netlinkSocket);
        netlinkSocket = -1;
      }
    } else {
      logSystemError("socket");
    }
  }

  return netlinkSocket;
}
#endif /* NETLINK_KOBJECT_UEVENT */

static int
monitorKeyboardAdditions (KeyboardCommonData *kcd) {
#ifdef NETLINK_KOBJECT_UEVENT
  int kobjectUeventSocket = getKobjectUeventSocket();

  if (kobjectUeventSocket != -1) {
    if (asyncRead(NULL, kobjectUeventSocket, 6+1+PATH_MAX+1,
                  handleKobjectUeventString, kcd)) {
      claimKeyboardCommonData(kcd);
      return 1;
    }

    close(kobjectUeventSocket);
  }
#endif /* NETLINK_KOBJECT_UEVENT */

  return 0;
}
#endif /* HAVE_LINUX_INPUT_H */

int
monitorKeyboards (KeyboardCommonData *kcd) {
#ifdef HAVE_LINUX_INPUT_H
  if (getUinputDevice() != -1) {
    monitorCurrentKeyboards(kcd);
    monitorKeyboardAdditions(kcd);
    return 1;
  }
#endif /* HAVE_LINUX_INPUT_H */

  return 0;
}
