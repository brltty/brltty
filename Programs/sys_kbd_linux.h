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

#ifdef HAVE_LINUX_INPUT_H
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/input.h>

#include "async.h"

typedef struct {
  KeyEventHandler *handleKeyEvent;
  KeyboardProperties requiredProperties;
} KeyboardCommonData;

typedef struct {
  KeyboardCommonData *kcd;
  int fileDescriptor;
  KeyboardProperties actualProperties;

  BITMASK(handledKeys, KEY_MAX+1, char);
  unsigned justModifiers:1;

  struct input_event *keyEventBuffer;
  unsigned int keyEventLimit;
  unsigned int keyEventCount;
} KeyboardPrivateData;

static void
closeKeyboard (KeyboardPrivateData *kpd) {
  close(kpd->fileDescriptor);
  free(kpd);
}

static size_t
handleKeyboardEvent (const AsyncInputResult *result) {
  KeyboardPrivateData *kpd = result->data;

  if (result->error) {
    LogPrint(LOG_DEBUG, "keyboard read error: fd=%d: %s",
             kpd->fileDescriptor, strerror(result->error));
    closeKeyboard(kpd);
  } else if (result->end) {
    LogPrint(LOG_DEBUG, "keyboard end-of-file: fd=%d", kpd->fileDescriptor);
    closeKeyboard(kpd);
  } else {
    const struct input_event *event = result->buffer;

    if (result->length >= sizeof(*event)) {
      if (event->type == EV_KEY) {
        int press = event->value != 0;
        KeyTableState state;

        {
          static const unsigned char map[KEY_MAX] = {
            [KEY_ESC] = KBD_KEY_FUNCTION_Escape,
            [KEY_1] = KBD_KEY_SYMBOL_One_Exclamation,
            [KEY_2] = KBD_KEY_SYMBOL_Two_At,
            [KEY_3] = KBD_KEY_SYMBOL_Three_Number,
            [KEY_4] = KBD_KEY_SYMBOL_Four_Dollar,
            [KEY_5] = KBD_KEY_SYMBOL_Five_Percent,
            [KEY_6] = KBD_KEY_SYMBOL_Six_Circumflex,
            [KEY_7] = KBD_KEY_SYMBOL_Seven_Ampersand,
            [KEY_8] = KBD_KEY_SYMBOL_Eight_Asterisk,
            [KEY_9] = KBD_KEY_SYMBOL_Nine_LeftParenthesis,
            [KEY_0] = KBD_KEY_SYMBOL_Zero_RightParenthesis,
            [KEY_MINUS] = KBD_KEY_SYMBOL_Minus_Underscore,
            [KEY_EQUAL] = KBD_KEY_SYMBOL_Equals_Plus,
            [KEY_BACKSPACE] = KBD_KEY_FUNCTION_DeleteBackward,
            [KEY_TAB] = KBD_KEY_FUNCTION_Tab,
            [KEY_Q] = KBD_KEY_LETTER_Q,
            [KEY_W] = KBD_KEY_LETTER_W,
            [KEY_E] = KBD_KEY_LETTER_E,
            [KEY_R] = KBD_KEY_LETTER_R,
            [KEY_T] = KBD_KEY_LETTER_T,
            [KEY_Y] = KBD_KEY_LETTER_Y,
            [KEY_U] = KBD_KEY_LETTER_U,
            [KEY_I] = KBD_KEY_LETTER_I,
            [KEY_O] = KBD_KEY_LETTER_O,
            [KEY_P] = KBD_KEY_LETTER_P,
            [KEY_LEFTBRACE] = KBD_KEY_SYMBOL_LeftBracket_LeftBrace,
            [KEY_RIGHTBRACE] = KBD_KEY_SYMBOL_RightBracket_RightBrace,
            [KEY_ENTER] = KBD_KEY_FUNCTION_Enter,
            [KEY_LEFTCTRL] = KBD_KEY_FUNCTION_ControlLeft,
            [KEY_A] = KBD_KEY_LETTER_A,
            [KEY_S] = KBD_KEY_LETTER_S,
            [KEY_D] = KBD_KEY_LETTER_D,
            [KEY_F] = KBD_KEY_LETTER_F,
            [KEY_G] = KBD_KEY_LETTER_G,
            [KEY_H] = KBD_KEY_LETTER_H,
            [KEY_J] = KBD_KEY_LETTER_J,
            [KEY_K] = KBD_KEY_LETTER_K,
            [KEY_L] = KBD_KEY_LETTER_L,
            [KEY_SEMICOLON] = KBD_KEY_SYMBOL_Semicolon_Colon,
            [KEY_APOSTROPHE] = KBD_KEY_SYMBOL_Apostrophe_Quote,
            [KEY_GRAVE] = KBD_KEY_SYMBOL_Grave_Tilde,
            [KEY_LEFTSHIFT] = KBD_KEY_FUNCTION_ShiftLeft,
            [KEY_BACKSLASH] = KBD_KEY_SYMBOL_Backslash_Bar,
            [KEY_Z] = KBD_KEY_LETTER_Z,
            [KEY_X] = KBD_KEY_LETTER_X,
            [KEY_C] = KBD_KEY_LETTER_C,
            [KEY_V] = KBD_KEY_LETTER_V,
            [KEY_B] = KBD_KEY_LETTER_B,
            [KEY_N] = KBD_KEY_LETTER_N,
            [KEY_M] = KBD_KEY_LETTER_M,
            [KEY_COMMA] = KBD_KEY_SYMBOL_Comma_Less,
            [KEY_DOT] = KBD_KEY_SYMBOL_Period_Greater,
            [KEY_SLASH] = KBD_KEY_SYMBOL_Slash_Question,
            [KEY_RIGHTSHIFT] = KBD_KEY_FUNCTION_ShiftRight,
            [KEY_KPASTERISK] = KBD_KEY_KEYPAD_Asterisk,
            [KEY_LEFTALT] = KBD_KEY_FUNCTION_AltLeft,
            [KEY_SPACE] = KBD_KEY_FUNCTION_Space,
            [KEY_CAPSLOCK] = KBD_KEY_LOCK_Capitals,
            [KEY_F1] = KBD_KEY_FUNCTION_F1,
            [KEY_F2] = KBD_KEY_FUNCTION_F2,
            [KEY_F3] = KBD_KEY_FUNCTION_F3,
            [KEY_F4] = KBD_KEY_FUNCTION_F4,
            [KEY_F5] = KBD_KEY_FUNCTION_F5,
            [KEY_F6] = KBD_KEY_FUNCTION_F6,
            [KEY_F7] = KBD_KEY_FUNCTION_F7,
            [KEY_F8] = KBD_KEY_FUNCTION_F8,
            [KEY_F9] = KBD_KEY_FUNCTION_F9,
            [KEY_F10] = KBD_KEY_FUNCTION_F10,
            [KEY_NUMLOCK] = KBD_KEY_LOCKING_Numbers,
            [KEY_SCROLLLOCK] = KBD_KEY_LOCKING_Scroll,
            [KEY_KP7] = KBD_KEY_KEYPAD_Seven_Home,
            [KEY_KP8] = KBD_KEY_KEYPAD_Eight_ArrowUp,
            [KEY_KP9] = KBD_KEY_KEYPAD_Nine_PageUp,
            [KEY_KPMINUS] = KBD_KEY_KEYPAD_Minus,
            [KEY_KP4] = KBD_KEY_KEYPAD_Four_ArrowLeft,
            [KEY_KP5] = KBD_KEY_KEYPAD_Five,
            [KEY_KP6] = KBD_KEY_KEYPAD_Six_ArrowRight,
            [KEY_KPPLUS] = KBD_KEY_KEYPAD_Plus,
            [KEY_KP1] = KBD_KEY_KEYPAD_One_End,
            [KEY_KP2] = KBD_KEY_KEYPAD_Two_ArrowDown,
            [KEY_KP3] = KBD_KEY_KEYPAD_Three_PageDown,
            [KEY_KP0] = KBD_KEY_KEYPAD_Zero_Insert,
            [KEY_KPDOT] = KBD_KEY_KEYPAD_Period_Delete,
            /* [KEY_ZENKAKUHANKAKU] = KBD_KEY_..., */
            /* [KEY_102ND] = KBD_KEY_..., */
            [KEY_F11] = KBD_KEY_FUNCTION_F11,
            [KEY_F12] = KBD_KEY_FUNCTION_F12,
            /* [KEY_RO] = KBD_KEY_...,
               [KEY_KATAKANA] = KBD_KEY_...,
               [KEY_HIRAGANA] = KBD_KEY_...,
               [KEY_HENKAN] = KBD_KEY_...,
               [KEY_KATAKANAHIRAGANA] = KBD_KEY_...,
               [KEY_MUHENKAN] = KBD_KEY_...,
               [KEY_KPJPCOMMA] = KBD_KEY_..., */
            [KEY_KPENTER] = KBD_KEY_KEYPAD_Enter,
            [KEY_RIGHTCTRL] = KBD_KEY_FUNCTION_ControlRight,
            [KEY_KPSLASH] = KBD_KEY_KEYPAD_Slash,
            [KEY_SYSRQ] = KBD_KEY_FUNCTION_SystemRequest,
            [KEY_RIGHTALT] = KBD_KEY_FUNCTION_AltRight,
            /* [KEY_LINEFEED] = KBD_KEY_..., */
            [KEY_HOME] = KBD_KEY_FUNCTION_Home,
            [KEY_UP] = KBD_KEY_FUNCTION_ArrowUp,
            [KEY_PAGEUP] = KBD_KEY_FUNCTION_PageUp,
            [KEY_LEFT] = KBD_KEY_FUNCTION_ArrowLeft,
            [KEY_RIGHT] = KBD_KEY_FUNCTION_ArrowRight,
            [KEY_END] = KBD_KEY_FUNCTION_End,
            [KEY_DOWN] = KBD_KEY_FUNCTION_ArrowDown,
            [KEY_PAGEDOWN] = KBD_KEY_FUNCTION_PageDown,
            [KEY_INSERT] = KBD_KEY_FUNCTION_Insert,
            [KEY_DELETE] = KBD_KEY_FUNCTION_DeleteForward,
            /* [KEY_MACRO] = KBD_KEY_..., */
            [KEY_MUTE] = KBD_KEY_FUNCTION_Mute,
            [KEY_VOLUMEDOWN] = KBD_KEY_FUNCTION_VolumeDown,
            [KEY_VOLUMEUP] = KBD_KEY_FUNCTION_VolumeUp,
            [KEY_POWER] = KBD_KEY_FUNCTION_Power,
            [KEY_KPEQUAL] = KBD_KEY_KEYPAD_Equals,
            [KEY_KPPLUSMINUS] = KBD_KEY_KEYPAD_PlusMinus,
            [KEY_PAUSE] = KBD_KEY_FUNCTION_Pause,
            [KEY_KPCOMMA] = KBD_KEY_KEYPAD_Comma,
            /* [KEY_HANGEUL] = KBD_KEY_...,
               [KEY_HANGUEL] = KBD_KEY_...,
               [KEY_HANJA] = KBD_KEY_...,
               [KEY_YEN] = KBD_KEY_...,
               [KEY_LEFTMETA] = KBD_KEY_...,
               [KEY_RIGHTMETA] = KBD_KEY_...,
               [KEY_COMPOSE] = KBD_KEY_..., */
            [KEY_STOP] = KBD_KEY_FUNCTION_Stop,
            [KEY_AGAIN] = KBD_KEY_FUNCTION_Again,
            /* [KEY_PROPS] = KBD_KEY_, */
            [KEY_UNDO] = KBD_KEY_FUNCTION_Undo,
            /* [KEY_FRONT] = KBD_KEY_..., */
            [KEY_COPY] = KBD_KEY_FUNCTION_Copy,
            [KEY_OPEN] = KBD_KEY_FUNCTION_Execute,
            [KEY_PASTE] = KBD_KEY_FUNCTION_Paste,
            [KEY_FIND] = KBD_KEY_FUNCTION_Find,
            [KEY_CUT] = KBD_KEY_FUNCTION_Cut,
            [KEY_HELP] = KBD_KEY_FUNCTION_Help,
            [KEY_MENU] = KBD_KEY_FUNCTION_Menu,
            /* [KEY_CALC] = KBD_KEY_...,
               [KEY_SETUP] = KBD_KEY_...,
               [KEY_SLEEP] = KBD_KEY_...,
               [KEY_WAKEUP] = KBD_KEY_...,
               [KEY_FILE] = KBD_KEY_...,
               [KEY_SENDFILE] = KBD_KEY_...,
               [KEY_DELETEFILE] = KBD_KEY_...,
               [KEY_XFER] = KBD_KEY_...,
               [KEY_PROG1] = KBD_KEY_...,
               [KEY_PROG2] = KBD_KEY_...,
               [KEY_WWW] = KBD_KEY_...,
               [KEY_MSDOS] = KBD_KEY_...,
               [KEY_COFFEE] = KBD_KEY_...,
               [KEY_SCREENLOCK] = KBD_KEY_...,
               [KEY_DIRECTION] = KBD_KEY_...,
               [KEY_CYCLEWINDOWS] = KBD_KEY_...,
               [KEY_MAIL] = KBD_KEY_...,
               [KEY_BOOKMARKS] = KBD_KEY_...,
               [KEY_COMPUTER] = KBD_KEY_...,
               [KEY_BACK] = KBD_KEY_...,
               [KEY_FORWARD] = KBD_KEY_...,
               [KEY_CLOSECD] = KBD_KEY_...,
               [KEY_EJECTCD] = KBD_KEY_...,
               [KEY_EJECTCLOSECD] = KBD_KEY_...,
               [KEY_NEXTSONG] = KBD_KEY_...,
               [KEY_PLAYPAUSE] = KBD_KEY_...,
               [KEY_PREVIOUSSONG] = KBD_KEY_...,
               [KEY_STOPCD] = KBD_KEY_...,
               [KEY_RECORD] = KBD_KEY_...,
               [KEY_REWIND] = KBD_KEY_...,
               [KEY_PHONE] = KBD_KEY_...,
               [KEY_ISO] = KBD_KEY_...,
               [KEY_CONFIG] = KBD_KEY_...,
               [KEY_HOMEPAGE] = KBD_KEY_...,
               [KEY_REFRESH] = KBD_KEY_...,
               [KEY_EXIT] = KBD_KEY_...,
               [KEY_MOVE] = KBD_KEY_...,
               [KEY_EDIT] = KBD_KEY_...,
               [KEY_SCROLLUP] = KBD_KEY_...,
               [KEY_SCROLLDOWN] = KBD_KEY_..., */
            [KEY_KPLEFTPAREN] = KBD_KEY_KEYPAD_LeftParenthesis,
            [KEY_KPRIGHTPAREN] = KBD_KEY_KEYPAD_RightParenthesis,
            /* [KEY_NEW] = KBD_KEY_...,
               [KEY_REDO] = KBD_KEY_..., */
            [KEY_F13] = KBD_KEY_FUNCTION_F13,
            [KEY_F14] = KBD_KEY_FUNCTION_F14,
            [KEY_F15] = KBD_KEY_FUNCTION_F15,
            [KEY_F16] = KBD_KEY_FUNCTION_F16,
            [KEY_F17] = KBD_KEY_FUNCTION_F17,
            [KEY_F18] = KBD_KEY_FUNCTION_F18,
            [KEY_F19] = KBD_KEY_FUNCTION_F19,
            [KEY_F20] = KBD_KEY_FUNCTION_F20,
            [KEY_F21] = KBD_KEY_FUNCTION_F21,
            [KEY_F22] = KBD_KEY_FUNCTION_F22,
            [KEY_F23] = KBD_KEY_FUNCTION_F23,
            [KEY_F24] = KBD_KEY_FUNCTION_F24,
            /* [KEY_PLAYCD] = KBD_KEY_...,
               [KEY_PAUSECD] = KBD_KEY_...,
               [KEY_PROG3] = KBD_KEY_...,
               [KEY_PROG4] = KBD_KEY_...,
               [KEY_SUSPEND] = KBD_KEY_...,
               [KEY_CLOSE] = KBD_KEY_...,
               [KEY_PLAY] = KBD_KEY_...,
               [KEY_FASTFORWARD] = KBD_KEY_...,
               [KEY_BASSBOOST] = KBD_KEY_...,
               [KEY_PRINT] = KBD_KEY_...,
               [KEY_HP] = KBD_KEY_...,
               [KEY_CAMERA] = KBD_KEY_...,
               [KEY_SOUND] = KBD_KEY_...,
               [KEY_QUESTION] = KBD_KEY_...,
               [KEY_EMAIL] = KBD_KEY_...,
               [KEY_CHAT] = KBD_KEY_...,
               [KEY_SEARCH] = KBD_KEY_...,
               [KEY_CONNECT] = KBD_KEY_...,
               [KEY_FINANCE] = KBD_KEY_...,
               [KEY_SPORT] = KBD_KEY_...,
               [KEY_SHOP] = KBD_KEY_...,
               [KEY_ALTERASE] = KBD_KEY_...,
               [KEY_CANCEL] = KBD_KEY_...,
               [KEY_BRIGHTNESSDOWN] = KBD_KEY_...,
               [KEY_BRIGHTNESSUP] = KBD_KEY_...,
               [KEY_MEDIA] = KBD_KEY_...,
               [KEY_SWITCHVIDEOMODE] = KBD_KEY_...,
               [KEY_KBDILLUMTOGGLE] = KBD_KEY_...,
               [KEY_KBDILLUMDOWN] = KBD_KEY_...,
               [KEY_KBDILLUMUP] = KBD_KEY_...,
               [KEY_SEND] = KBD_KEY_...,
               [KEY_REPLY] = KBD_KEY_...,
               [KEY_FORWARDMAIL] = KBD_KEY_...,
               [KEY_SAVE] = KBD_KEY_...,
               [KEY_DOCUMENTS] = KBD_KEY_...,
               [KEY_BATTERY] = KBD_KEY_...,
               [KEY_BLUETOOTH] = KBD_KEY_...,
               [KEY_WLAN] = KBD_KEY_...,
               [KEY_UWB] = KBD_KEY_...,
               [KEY_UNKNOWN] = KBD_KEY_...,
               [KEY_VIDEO_NEXT] = KBD_KEY_...,
               [KEY_VIDEO_PREV] = KBD_KEY_...,
               [KEY_BRIGHTNESS_CYCLE] = KBD_KEY_...,
               [KEY_BRIGHTNESS_ZERO] = KBD_KEY_...,
               [KEY_DISPLAY_OFF] = KBD_KEY_...,
               [KEY_WIMAX] = KBD_KEY_...,
               [BTN_MISC] = KBD_KEY_...,
               [BTN_0] = KBD_KEY_...,
               [BTN_1] = KBD_KEY_...,
               [BTN_2] = KBD_KEY_...,
               [BTN_3] = KBD_KEY_...,
               [BTN_4] = KBD_KEY_...,
               [BTN_5] = KBD_KEY_...,
               [BTN_6] = KBD_KEY_...,
               [BTN_7] = KBD_KEY_...,
               [BTN_8] = KBD_KEY_...,
               [BTN_9] = KBD_KEY_...,
               [BTN_MOUSE] = KBD_KEY_...,
               [BTN_LEFT] = KBD_KEY_...,
               [BTN_RIGHT] = KBD_KEY_...,
               [BTN_MIDDLE] = KBD_KEY_...,
               [BTN_SIDE] = KBD_KEY_...,
               [BTN_EXTRA] = KBD_KEY_...,
               [BTN_FORWARD] = KBD_KEY_...,
               [BTN_BACK] = KBD_KEY_...,
               [BTN_TASK] = KBD_KEY_...,
               [BTN_JOYSTICK] = KBD_KEY_...,
               [BTN_TRIGGER] = KBD_KEY_...,
               [BTN_THUMB] = KBD_KEY_...,
               [BTN_THUMB2] = KBD_KEY_...,
               [BTN_TOP] = KBD_KEY_...,
               [BTN_TOP2] = KBD_KEY_...,
               [BTN_PINKIE] = KBD_KEY_...,
               [BTN_BASE] = KBD_KEY_...,
               [BTN_BASE2] = KBD_KEY_...,
               [BTN_BASE3] = KBD_KEY_...,
               [BTN_BASE4] = KBD_KEY_...,
               [BTN_BASE5] = KBD_KEY_...,
               [BTN_BASE6] = KBD_KEY_...,
               [BTN_DEAD] = KBD_KEY_...,
               [BTN_GAMEPAD] = KBD_KEY_...,
               [BTN_A] = KBD_KEY_...,
               [BTN_B] = KBD_KEY_...,
               [BTN_C] = KBD_KEY_...,
               [BTN_X] = KBD_KEY_...,
               [BTN_Y] = KBD_KEY_...,
               [BTN_Z] = KBD_KEY_...,
               [BTN_TL] = KBD_KEY_...,
               [BTN_TR] = KBD_KEY_...,
               [BTN_TL2] = KBD_KEY_...,
               [BTN_TR2] = KBD_KEY_...,
               [BTN_SELECT] = KBD_KEY_...,
               [BTN_START] = KBD_KEY_...,
               [BTN_MODE] = KBD_KEY_...,
               [BTN_THUMBL] = KBD_KEY_...,
               [BTN_THUMBR] = KBD_KEY_...,
               [BTN_DIGI] = KBD_KEY_...,
               [BTN_TOOL_PEN] = KBD_KEY_...,
               [BTN_TOOL_RUBBER] = KBD_KEY_...,
               [BTN_TOOL_BRUSH] = KBD_KEY_...,
               [BTN_TOOL_PENCIL] = KBD_KEY_...,
               [BTN_TOOL_AIRBRUSH] = KBD_KEY_...,
               [BTN_TOOL_FINGER] = KBD_KEY_...,
               [BTN_TOOL_MOUSE] = KBD_KEY_...,
               [BTN_TOOL_LENS] = KBD_KEY_...,
               [BTN_TOUCH] = KBD_KEY_...,
               [BTN_STYLUS] = KBD_KEY_...,
               [BTN_STYLUS2] = KBD_KEY_...,
               [BTN_TOOL_DOUBLETAP] = KBD_KEY_...,
               [BTN_TOOL_TRIPLETAP] = KBD_KEY_...,
               [BTN_WHEEL] = KBD_KEY_...,
               [BTN_GEAR_DOWN] = KBD_KEY_...,
               [BTN_GEAR_UP] = KBD_KEY_...,
               [KEY_OK] = KBD_KEY_..., */
            [KEY_SELECT] = KBD_KEY_FUNCTION_Select,
            /* [KEY_GOTO] = KBD_KEY_..., */
            [KEY_CLEAR] = KBD_KEY_FUNCTION_Clear,
            /* [KEY_POWER2] = KBD_KEY_...,
               [KEY_OPTION] = KBD_KEY_...,
               [KEY_INFO] = KBD_KEY_...,
               [KEY_TIME] = KBD_KEY_...,
               [KEY_VENDOR] = KBD_KEY_...,
               [KEY_ARCHIVE] = KBD_KEY_...,
               [KEY_PROGRAM] = KBD_KEY_...,
               [KEY_CHANNEL] = KBD_KEY_...,
               [KEY_FAVORITES] = KBD_KEY_...,
               [KEY_EPG] = KBD_KEY_...,
               [KEY_PVR] = KBD_KEY_...,
               [KEY_MHP] = KBD_KEY_...,
               [KEY_LANGUAGE] = KBD_KEY_...,
               [KEY_TITLE] = KBD_KEY_...,
               [KEY_SUBTITLE] = KBD_KEY_...,
               [KEY_ANGLE] = KBD_KEY_...,
               [KEY_ZOOM] = KBD_KEY_...,
               [KEY_MODE] = KBD_KEY_...,
               [KEY_KEYBOARD] = KBD_KEY_...,
               [KEY_SCREEN] = KBD_KEY_...,
               [KEY_PC] = KBD_KEY_...,
               [KEY_TV] = KBD_KEY_...,
               [KEY_TV2] = KBD_KEY_...,
               [KEY_VCR] = KBD_KEY_...,
               [KEY_VCR2] = KBD_KEY_...,
               [KEY_SAT] = KBD_KEY_...,
               [KEY_SAT2] = KBD_KEY_...,
               [KEY_CD] = KBD_KEY_...,
               [KEY_TAPE] = KBD_KEY_...,
               [KEY_RADIO] = KBD_KEY_...,
               [KEY_TUNER] = KBD_KEY_...,
               [KEY_PLAYER] = KBD_KEY_...,
               [KEY_TEXT] = KBD_KEY_...,
               [KEY_DVD] = KBD_KEY_...,
               [KEY_AUX] = KBD_KEY_...,
               [KEY_MP3] = KBD_KEY_...,
               [KEY_AUDIO] = KBD_KEY_...,
               [KEY_VIDEO] = KBD_KEY_...,
               [KEY_DIRECTORY] = KBD_KEY_...,
               [KEY_LIST] = KBD_KEY_...,
               [KEY_MEMO] = KBD_KEY_...,
               [KEY_CALENDAR] = KBD_KEY_...,
               [KEY_RED] = KBD_KEY_...,
               [KEY_GREEN] = KBD_KEY_...,
               [KEY_YELLOW] = KBD_KEY_...,
               [KEY_BLUE] = KBD_KEY_...,
               [KEY_CHANNELUP] = KBD_KEY_...,
               [KEY_CHANNELDOWN] = KBD_KEY_...,
               [KEY_FIRST] = KBD_KEY_...,
               [KEY_LAST] = KBD_KEY_...,
               [KEY_AB] = KBD_KEY_...,
               [KEY_NEXT] = KBD_KEY_...,
               [KEY_RESTART] = KBD_KEY_...,
               [KEY_SLOW] = KBD_KEY_...,
               [KEY_SHUFFLE] = KBD_KEY_...,
               [KEY_BREAK] = KBD_KEY_...,
               [KEY_PREVIOUS] = KBD_KEY_...,
               [KEY_DIGITS] = KBD_KEY_...,
               [KEY_TEEN] = KBD_KEY_...,
               [KEY_TWEN] = KBD_KEY_...,
               [KEY_VIDEOPHONE] = KBD_KEY_...,
               [KEY_GAMES] = KBD_KEY_...,
               [KEY_ZOOMIN] = KBD_KEY_...,
               [KEY_ZOOMOUT] = KBD_KEY_...,
               [KEY_ZOOMRESET] = KBD_KEY_...,
               [KEY_WORDPROCESSOR] = KBD_KEY_...,
               [KEY_EDITOR] = KBD_KEY_...,
               [KEY_SPREADSHEET] = KBD_KEY_...,
               [KEY_GRAPHICSEDITOR] = KBD_KEY_...,
               [KEY_PRESENTATION] = KBD_KEY_...,
               [KEY_DATABASE] = KBD_KEY_...,
               [KEY_NEWS] = KBD_KEY_...,
               [KEY_VOICEMAIL] = KBD_KEY_...,
               [KEY_ADDRESSBOOK] = KBD_KEY_...,
               [KEY_MESSENGER] = KBD_KEY_...,
               [KEY_DISPLAYTOGGLE] = KBD_KEY_...,
               [KEY_SPELLCHECK] = KBD_KEY_...,
               [KEY_LOGOFF] = KBD_KEY_...,
               [KEY_DOLLAR] = KBD_KEY_...,
               [KEY_EURO] = KBD_KEY_...,
               [KEY_FRAMEBACK] = KBD_KEY_...,
               [KEY_FRAMEFORWARD] = KBD_KEY_...,
               [KEY_CONTEXT_MENU] = KBD_KEY_...,
               [KEY_MEDIA_REPEAT] = KBD_KEY_...,
               [KEY_DEL_EOL] = KBD_KEY_...,
               [KEY_DEL_EOS] = KBD_KEY_...,
               [KEY_INS_LINE] = KBD_KEY_...,
               [KEY_DEL_LINE] = KBD_KEY_...,
               [KEY_FN] = KBD_KEY_...,
               [KEY_FN_ESC] = KBD_KEY_...,
               [KEY_FN_F1] = KBD_KEY_...,
               [KEY_FN_F2] = KBD_KEY_...,
               [KEY_FN_F3] = KBD_KEY_...,
               [KEY_FN_F4] = KBD_KEY_...,
               [KEY_FN_F5] = KBD_KEY_...,
               [KEY_FN_F6] = KBD_KEY_...,
               [KEY_FN_F7] = KBD_KEY_...,
               [KEY_FN_F8] = KBD_KEY_...,
               [KEY_FN_F9] = KBD_KEY_...,
               [KEY_FN_F10] = KBD_KEY_...,
               [KEY_FN_F11] = KBD_KEY_...,
               [KEY_FN_F12] = KBD_KEY_...,
               [KEY_FN_1] = KBD_KEY_...,
               [KEY_FN_2] = KBD_KEY_...,
               [KEY_FN_D] = KBD_KEY_...,
               [KEY_FN_E] = KBD_KEY_...,
               [KEY_FN_F] = KBD_KEY_...,
               [KEY_FN_S] = KBD_KEY_...,
               [KEY_FN_B] = KBD_KEY_...,
               [KEY_BRL_DOT1] = KBD_KEY_...,
               [KEY_BRL_DOT2] = KBD_KEY_...,
               [KEY_BRL_DOT3] = KBD_KEY_...,
               [KEY_BRL_DOT4] = KBD_KEY_...,
               [KEY_BRL_DOT5] = KBD_KEY_...,
               [KEY_BRL_DOT6] = KBD_KEY_...,
               [KEY_BRL_DOT7] = KBD_KEY_...,
               [KEY_BRL_DOT8] = KBD_KEY_...,
               [KEY_BRL_DOT9] = KBD_KEY_...,
               [KEY_BRL_DOT10] = KBD_KEY_...,
               [KEY_MIN_INTERESTING] = KBD_KEY_... */
          };
          unsigned char key = map[event->code];

          if (key) {
            state = kpd->kcd->handleKeyEvent(0, key, press);
          } else {
            LogPrint(LOG_INFO, "unmapped Linux keycode: %d", event->code);
            state = KTS_UNBOUND;
          }
        }

        {
          typedef enum {
            WKA_NONE,
            WKA_CURRENT,
            WKA_ALL
          } WriteKeysAction;
          WriteKeysAction action = WKA_NONE;

          if (press) {
            kpd->justModifiers = state == KTS_MODIFIERS;

            if (state == KTS_UNBOUND) {
              action = WKA_ALL;
            } else {
              if (kpd->keyEventCount == kpd->keyEventLimit) {
                unsigned int newLimit = kpd->keyEventLimit? kpd->keyEventLimit<<1: 0X1;
                struct input_event *newBuffer = realloc(kpd->keyEventBuffer, (newLimit * sizeof(*newBuffer)));

                if (newBuffer) {
                  kpd->keyEventBuffer = newBuffer;
                  kpd->keyEventLimit = newLimit;
                }
              }

              if (kpd->keyEventCount < kpd->keyEventLimit) {
                kpd->keyEventBuffer[kpd->keyEventCount++] = *event;
                BITMASK_SET(kpd->handledKeys, event->code);
              }
            }
          } else if (kpd->justModifiers) {
            kpd->justModifiers = 0;
            action = WKA_ALL;
          } else if (BITMASK_TEST(kpd->handledKeys, event->code)) {
            struct input_event *to = kpd->keyEventBuffer;
            const struct input_event *from = to;
            unsigned int count = kpd->keyEventCount;

            while (count) {
              if (from->code != event->code)
                if (to != from)
                  *to++ = *from;

              from += 1, count -= 1;
            }

            kpd->keyEventCount = to - kpd->keyEventBuffer;
            BITMASK_CLEAR(kpd->handledKeys, event->code);
          } else {
            action = WKA_CURRENT;
          }

          switch (action) {
            case WKA_ALL: {
              const struct input_event *keyEvent = kpd->keyEventBuffer;

              while (kpd->keyEventCount) {
                writeKeyEvent(keyEvent->code, keyEvent->value);
                keyEvent += 1, kpd->keyEventCount -= 1;
              }

              memset(kpd->handledKeys, 0, sizeof(kpd->handledKeys));
            }

            case WKA_CURRENT:
              writeKeyEvent(event->code, event->value);

            case WKA_NONE:
              break;
          }
        }
      } else {
        writeInputEvent(event);
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
      KeyboardPrivateData *kpd;

      if ((kpd = malloc(sizeof(*kpd)))) {
        memset(kpd, 0, sizeof(*kpd));
        kpd->kcd = kcd;
        kpd->fileDescriptor = device;

        kpd->keyEventBuffer = NULL;
        kpd->keyEventLimit = 0;
        kpd->keyEventCount = 0;

        kpd->actualProperties = anyKeyboard;
        {
          struct input_id identity;
          if (ioctl(device, EVIOCGID, &identity) != -1) {
            LogPrint(LOG_DEBUG, "input device identity: fd=%d: type=%04X vendor=%04X product=%04X version=%04X",
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
                kpd->actualProperties.type = typeTable[identity.bustype];
            }

            kpd->actualProperties.vendor = identity.vendor;
            kpd->actualProperties.product = identity.product;
          } else {
            LogPrint(LOG_DEBUG, "cannot get input device identity: fd=%d: %s",
                     device, strerror(errno));
          }
        }
      
        if (checkKeyboardProperties(&kpd->actualProperties, &kcd->requiredProperties)) {
          if (hasInputEvent(device, EV_KEY, KEY_ENTER, KEY_MAX)) {
            if (asyncRead(device, sizeof(struct input_event), handleKeyboardEvent, kpd)) {
#ifdef EVIOCGRAB
              ioctl(device, EVIOCGRAB, 1);
#endif /* EVIOCGRAB */

              LogPrint(LOG_DEBUG, "keyboard found: fd=%d", device);
              return 1;
            }
          }
        }

        free(kpd);
      }
    }
  } else {
    LogPrint(LOG_DEBUG, "cannot stat input device: fd=%d: %s", device, strerror(errno));
  }

  return 0;
}

static void
monitorCurrentKeyboards (KeyboardCommonData *kcd) {
  const char *root = "/dev/input";
  const size_t rootLength = strlen(root);
  DIR *directory;

  LogPrint(LOG_DEBUG, "searching for keyboards");
  if ((directory = opendir(root))) {
    struct dirent *entry;

    while ((entry = readdir(directory))) {
      const size_t nameLength = strlen(entry->d_name);
      char path[rootLength + 1 + nameLength + 1];
      int device;

      snprintf(path, sizeof(path), "%s/%s", root, entry->d_name);
      if ((device = open(path, O_RDONLY)) != -1) {
        LogPrint(LOG_DEBUG, "input device opened: %s: fd=%d", path, device);
        if (monitorKeyboard(device, kcd)) continue;

        close(device);
        LogPrint(LOG_DEBUG, "input device closed: %s: fd=%d", path, device);
      } else {
        LogPrint(LOG_DEBUG, "cannot open input device: %s: %s", path, strerror(errno));
      }
    }

    closedir(directory);
  } else {
    LogPrint(LOG_DEBUG, "cannot open directory: %s: %s", root, strerror(errno));
  }
  LogPrint(LOG_DEBUG, "keyboard search complete");
}

#ifdef NETLINK_KOBJECT_UEVENT
typedef struct {
  char *name;
  int major;
  int minor;
  KeyboardCommonData *kcd;
} InputDeviceData;

static void
doOpenInputDevice (void *data) {
  InputDeviceData *idd = data;
  int device = openCharacterDevice(idd->name, O_RDONLY, idd->major, idd->minor);

  if (device != -1) {
    LogPrint(LOG_DEBUG, "input device opened: %s: fd=%d", idd->name, device);
    if (!monitorKeyboard(device, idd->kcd)) {
      close(device);
      LogPrint(LOG_DEBUG, "input device closed: %s: fd=%d", idd->name, device);
    }
  } else {
    LogPrint(LOG_DEBUG, "cannot open input device: %s: %s", idd->name, strerror(errno));
  }

  free(idd->name);
  free(idd);
}

static size_t
handleKobjectUeventString (const AsyncInputResult *result) {
  if (result->error) {
    LogPrint(LOG_DEBUG, "netlink read error: %s", strerror(result->error));
  } else if (result->end) {
    LogPrint(LOG_DEBUG, "netlink end-of-file");
  } else {
    const char *buffer = result->buffer;
    const char *end = memchr(buffer, 0, result->length);

    if (end) {
      char *path = strchr(buffer, '@');

      if (path) {
        const char *action = buffer;
        int actionLength = path++ - action;

        LogPrint(LOG_DEBUG, "OBJECT_UEVENT: %.*s %s", actionLength, action, path);
        if (strncmp(action, "add", actionLength) == 0) {
          int inputNumber, eventNumber;

          if (sscanf(path, "/class/input/input%d/event%d", &inputNumber, &eventNumber) == 2) {
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
                int ok = 0;

                if ((idd = malloc(sizeof(*idd)))) {
                  if (sscanf(stringBuffer, "%d:%d", &idd->major, &idd->minor) == 2) {
                    char eventDevice[0X40];
                    snprintf(eventDevice, sizeof(eventDevice), "input/event%d", eventNumber);

                    if ((idd->name = strdup(eventDevice))) {
                      idd->kcd = result->data;
                      if (asyncRelativeAlarm(1000, doOpenInputDevice, idd)) ok = 1;

                      if (!ok) free(idd->name);
                    }
                  }

                  if (!ok) free(idd);
                }
              }

              close(descriptor);
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
        LogError("bind");
        close(netlinkSocket);
        netlinkSocket = -1;
      }
    } else {
      LogError("socket");
    }
  }

  return netlinkSocket;
}
#endif /* NETLINK_KOBJECT_UEVENT */

static int
monitorKeyboardAdditions (KeyboardCommonData *kcd) {
#ifdef NETLINK_KOBJECT_UEVENT
  int kobjectEventSocket = getKobjectUeventSocket();

  if (kobjectEventSocket != -1) {
    if (asyncRead(kobjectEventSocket,
                  6+1+PATH_MAX+1,
                  handleKobjectUeventString, kcd))
      return 1;

    close(kobjectEventSocket);
  }
#endif /* NETLINK_KOBJECT_UEVENT */

  return 0;
}
#endif /* HAVE_LINUX_INPUT_H */

int
startKeyboardMonitor (const KeyboardProperties *properties, KeyEventHandler handleKeyEvent) {
#ifdef HAVE_LINUX_INPUT_H
  if (getUinputDevice() != -1) {
    KeyboardCommonData *kcd;

    if ((kcd = malloc(sizeof(*kcd)))) {
      memset(kcd, 0, sizeof(*kcd));
      kcd->handleKeyEvent = handleKeyEvent;
      kcd->requiredProperties = *properties;

      monitorCurrentKeyboards(kcd);
      monitorKeyboardAdditions(kcd);
      return 1;
    }
  }
#endif /* HAVE_LINUX_INPUT_H */

  return 0;
}
