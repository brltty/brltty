/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#ifdef HAVE_LINUX_INPUT_H
#include <linux/input.h>
#endif /* HAVE_LINUX_INPUT_H */

#include "async.h"

static int
getKeyboardDevice (const KeyboardProperties *requiredProperties) {
  static int keyboardDevice = -1;

#ifdef HAVE_LINUX_INPUT_H
  if (keyboardDevice == -1) {
    const char root[] = "/dev/input";
    const size_t rootLength = strlen(root);
    DIR *directory;

    if ((directory = opendir(root))) {
      struct dirent *entry;

      while ((entry = readdir(directory))) {
        const size_t nameLength = strlen(entry->d_name);
        char path[rootLength + 1 + nameLength + 1];
        snprintf(path, sizeof(path), "%s/%s", root, entry->d_name);

        {
          struct stat status;
          if (stat(path, &status) == -1) continue;
          if (!S_ISCHR(status.st_mode)) continue;
        }

        {
          int device;

          LogPrint(LOG_DEBUG, "checking device: %s", path);
          if ((device = open(path, O_RDONLY)) != -1) {
            KeyboardProperties actualProperties = anyKeyboard;
            actualProperties.device = path;

            {
              struct input_id identity;
              if (ioctl(device, EVIOCGID, &identity) != -1) {
                LogPrint(LOG_DEBUG, "device identity: %s type=%04X vendor=%04X product=%04X version=%04X",
                         path, identity.bustype,
                         identity.vendor, identity.product, identity.version);

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
                    actualProperties.type = typeTable[identity.bustype];
                }

                actualProperties.vendor = identity.vendor;
                actualProperties.product = identity.product;
              } else {
                LogPrint(LOG_DEBUG, "cannot get device identity: %s: %s", path, strerror(errno));
              }
            }
            
            if (checkKeyboardProperties(&actualProperties, requiredProperties)) {
              LogPrint(LOG_DEBUG, "testing device: %s", path);

              if (hasInputEvent(device, EV_KEY, KEY_ENTER, KEY_MAX)) {
                LogPrint(LOG_DEBUG, "keyboard opened: %s fd=%d", path, device);
                keyboardDevice = device;
                break;
              }
            }

            close(device);
          } else {
            LogPrint(LOG_DEBUG, "cannot open device: %s: %s", path, strerror(errno));
          }
        }
      }

      closedir(directory);
    } else {
      LogPrint(LOG_DEBUG, "cannot open directory: %s", root);
    }
  }
#endif /* HAVE_LINUX_INPUT_H */

  return keyboardDevice;
}

static size_t
handleKeyboardEvent (const AsyncInputResult *result) {
  if (result->error) {
    LogPrint(LOG_DEBUG, "keyboard read error: %s", strerror(result->error));
  } else if (result->end) {
    LogPrint(LOG_DEBUG, "keyboard end-of-file");
  } else {
#ifdef HAVE_LINUX_INPUT_H
    const struct input_event *event = result->buffer;

    if (result->length >= sizeof(*event)) {
      if (event->type == EV_KEY) {
        static const KeyCode map[KEY_MAX] = {
          [KEY_ESC] = KEY_FUNCTION_Escape,
          [KEY_1] = KEY_SYMBOL_One_Exclamation,
          [KEY_2] = KEY_SYMBOL_Two_At,
          [KEY_3] = KEY_SYMBOL_Three_Number,
          [KEY_4] = KEY_SYMBOL_Four_Dollar,
          [KEY_5] = KEY_SYMBOL_Five_Percent,
          [KEY_6] = KEY_SYMBOL_Six_Circumflex,
          [KEY_7] = KEY_SYMBOL_Seven_Ampersand,
          [KEY_8] = KEY_SYMBOL_Eight_Asterisk,
          [KEY_9] = KEY_SYMBOL_Nine_LeftParenthesis,
          [KEY_0] = KEY_SYMBOL_Zero_RightParenthesis,
          [KEY_MINUS] = KEY_SYMBOL_Minus_Underscore,
          [KEY_EQUAL] = KEY_SYMBOL_Equals_Plus,
          [KEY_BACKSPACE] = KEY_FUNCTION_DeleteBackward,
          [KEY_TAB] = KEY_FUNCTION_Tab,
          [KEY_Q] = KEY_LETTER_Q,
          [KEY_W] = KEY_LETTER_W,
          [KEY_E] = KEY_LETTER_E,
          [KEY_R] = KEY_LETTER_R,
          [KEY_T] = KEY_LETTER_T,
          [KEY_Y] = KEY_LETTER_Y,
          [KEY_U] = KEY_LETTER_U,
          [KEY_I] = KEY_LETTER_I,
          [KEY_O] = KEY_LETTER_O,
          [KEY_P] = KEY_LETTER_P,
          [KEY_LEFTBRACE] = KEY_SYMBOL_LeftBracket_LeftBrace,
          [KEY_RIGHTBRACE] = KEY_SYMBOL_RightBracket_RightBrace,
          [KEY_ENTER] = KEY_FUNCTION_Enter,
          [KEY_LEFTCTRL] = KEY_FUNCTION_ControlLeft,
          [KEY_A] = KEY_LETTER_A,
          [KEY_S] = KEY_LETTER_S,
          [KEY_D] = KEY_LETTER_D,
          [KEY_F] = KEY_LETTER_F,
          [KEY_G] = KEY_LETTER_G,
          [KEY_H] = KEY_LETTER_H,
          [KEY_J] = KEY_LETTER_J,
          [KEY_K] = KEY_LETTER_K,
          [KEY_L] = KEY_LETTER_L,
          [KEY_SEMICOLON] = KEY_SYMBOL_Semicolon_Colon,
          [KEY_APOSTROPHE] = KEY_SYMBOL_Apostrophe_Quote,
          [KEY_GRAVE] = KEY_SYMBOL_Grave_Tilde,
          [KEY_LEFTSHIFT] = KEY_FUNCTION_ShiftLeft,
          [KEY_BACKSLASH] = KEY_SYMBOL_Backslash_Bar,
          [KEY_Z] = KEY_LETTER_Z,
          [KEY_X] = KEY_LETTER_X,
          [KEY_C] = KEY_LETTER_C,
          [KEY_V] = KEY_LETTER_V,
          [KEY_B] = KEY_LETTER_B,
          [KEY_N] = KEY_LETTER_N,
          [KEY_M] = KEY_LETTER_M,
          [KEY_COMMA] = KEY_SYMBOL_Comma_Less,
          [KEY_DOT] = KEY_SYMBOL_Period_Greater,
          [KEY_SLASH] = KEY_SYMBOL_Slash_Question,
          [KEY_RIGHTSHIFT] = KEY_FUNCTION_ShiftRight,
          [KEY_KPASTERISK] = KEY_KEYPAD_Asterisk,
          [KEY_LEFTALT] = KEY_FUNCTION_AltLeft,
          [KEY_SPACE] = KEY_FUNCTION_Space,
          [KEY_CAPSLOCK] = KEY_LOCK_Capitals,
          [KEY_F1] = KEY_FUNCTION_F1,
          [KEY_F2] = KEY_FUNCTION_F2,
          [KEY_F3] = KEY_FUNCTION_F3,
          [KEY_F4] = KEY_FUNCTION_F4,
          [KEY_F5] = KEY_FUNCTION_F5,
          [KEY_F6] = KEY_FUNCTION_F6,
          [KEY_F7] = KEY_FUNCTION_F7,
          [KEY_F8] = KEY_FUNCTION_F8,
          [KEY_F9] = KEY_FUNCTION_F9,
          [KEY_F10] = KEY_FUNCTION_F10,
          [KEY_NUMLOCK] = KEY_LOCKING_Numbers,
          [KEY_SCROLLLOCK] = KEY_LOCKING_Scroll,
          [KEY_KP7] = KEY_KEYPAD_Seven_Home,
          [KEY_KP8] = KEY_KEYPAD_Eight_ArrowUp,
          [KEY_KP9] = KEY_KEYPAD_Nine_PageUp,
          [KEY_KPMINUS] = KEY_KEYPAD_Minus,
          [KEY_KP4] = KEY_KEYPAD_Four_ArrowLeft,
          [KEY_KP5] = KEY_KEYPAD_Five,
          [KEY_KP6] = KEY_KEYPAD_Six_ArrowRight,
          [KEY_KPPLUS] = KEY_KEYPAD_Plus,
          [KEY_KP1] = KEY_KEYPAD_One_End,
          [KEY_KP2] = KEY_KEYPAD_Two_ArrowDown,
          [KEY_KP3] = KEY_KEYPAD_Three_PageDown,
          [KEY_KP0] = KEY_KEYPAD_Zero_Insert,
          [KEY_KPDOT] = KEY_KEYPAD_Period_Delete,
          /* [KEY_ZENKAKUHANKAKU] = KEY_..., */
          /* [KEY_102ND] = KEY_..., */
          [KEY_F11] = KEY_FUNCTION_F11,
          [KEY_F12] = KEY_FUNCTION_F12,
          /* [KEY_RO] = KEY_...,
             [KEY_KATAKANA] = KEY_...,
             [KEY_HIRAGANA] = KEY_...,
             [KEY_HENKAN] = KEY_...,
             [KEY_KATAKANAHIRAGANA] = KEY_...,
             [KEY_MUHENKAN] = KEY_...,
             [KEY_KPJPCOMMA] = KEY_..., */
          [KEY_KPENTER] = KEY_KEYPAD_Enter,
          [KEY_RIGHTCTRL] = KEY_FUNCTION_ControlRight,
          [KEY_KPSLASH] = KEY_KEYPAD_Slash,
          [KEY_SYSRQ] = KEY_FUNCTION_SystemRequest,
          [KEY_RIGHTALT] = KEY_FUNCTION_AltRight,
          /* [KEY_LINEFEED] = KEY_..., */
          [KEY_HOME] = KEY_FUNCTION_Home,
          [KEY_UP] = KEY_FUNCTION_ArrowUp,
          [KEY_PAGEUP] = KEY_FUNCTION_PageUp,
          [KEY_LEFT] = KEY_FUNCTION_ArrowLeft,
          [KEY_RIGHT] = KEY_FUNCTION_ArrowRight,
          [KEY_END] = KEY_FUNCTION_End,
          [KEY_DOWN] = KEY_FUNCTION_ArrowDown,
          [KEY_PAGEDOWN] = KEY_FUNCTION_PageDown,
          [KEY_INSERT] = KEY_FUNCTION_Insert,
          [KEY_DELETE] = KEY_FUNCTION_DeleteForward,
          /* [KEY_MACRO] = KEY_..., */
          [KEY_MUTE] = KEY_FUNCTION_Mute,
          [KEY_VOLUMEDOWN] = KEY_FUNCTION_VolumeDown,
          [KEY_VOLUMEUP] = KEY_FUNCTION_VolumeUp,
          [KEY_POWER] = KEY_FUNCTION_Power,
          [KEY_KPEQUAL] = KEY_KEYPAD_Equals,
          [KEY_KPPLUSMINUS] = KEY_KEYPAD_PlusMinus,
          [KEY_PAUSE] = KEY_FUNCTION_Pause,
          [KEY_KPCOMMA] = KEY_KEYPAD_Comma,
          /* [KEY_HANGEUL] = KEY_...,
             [KEY_HANGUEL] = KEY_...,
             [KEY_HANJA] = KEY_...,
             [KEY_YEN] = KEY_...,
             [KEY_LEFTMETA] = KEY_...,
             [KEY_RIGHTMETA] = KEY_...,
             [KEY_COMPOSE] = KEY_..., */
          [KEY_STOP] = KEY_FUNCTION_Stop,
          [KEY_AGAIN] = KEY_FUNCTION_Again,
          /* [KEY_PROPS] = KEY_, */
          [KEY_UNDO] = KEY_FUNCTION_Undo,
          /* [KEY_FRONT] = KEY_..., */
          [KEY_COPY] = KEY_FUNCTION_Copy,
          [KEY_OPEN] = KEY_FUNCTION_Execute,
          [KEY_PASTE] = KEY_FUNCTION_Paste,
          [KEY_FIND] = KEY_FUNCTION_Find,
          [KEY_CUT] = KEY_FUNCTION_Cut,
          [KEY_HELP] = KEY_FUNCTION_Help,
          [KEY_MENU] = KEY_FUNCTION_Menu,
          /* [KEY_CALC] = KEY_...,
             [KEY_SETUP] = KEY_...,
             [KEY_SLEEP] = KEY_...,
             [KEY_WAKEUP] = KEY_...,
             [KEY_FILE] = KEY_...,
             [KEY_SENDFILE] = KEY_...,
             [KEY_DELETEFILE] = KEY_...,
             [KEY_XFER] = KEY_...,
             [KEY_PROG1] = KEY_...,
             [KEY_PROG2] = KEY_...,
             [KEY_WWW] = KEY_...,
             [KEY_MSDOS] = KEY_...,
             [KEY_COFFEE] = KEY_...,
             [KEY_SCREENLOCK] = KEY_...,
             [KEY_DIRECTION] = KEY_...,
             [KEY_CYCLEWINDOWS] = KEY_...,
             [KEY_MAIL] = KEY_...,
             [KEY_BOOKMARKS] = KEY_...,
             [KEY_COMPUTER] = KEY_...,
             [KEY_BACK] = KEY_...,
             [KEY_FORWARD] = KEY_...,
             [KEY_CLOSECD] = KEY_...,
             [KEY_EJECTCD] = KEY_...,
             [KEY_EJECTCLOSECD] = KEY_...,
             [KEY_NEXTSONG] = KEY_...,
             [KEY_PLAYPAUSE] = KEY_...,
             [KEY_PREVIOUSSONG] = KEY_...,
             [KEY_STOPCD] = KEY_...,
             [KEY_RECORD] = KEY_...,
             [KEY_REWIND] = KEY_...,
             [KEY_PHONE] = KEY_...,
             [KEY_ISO] = KEY_...,
             [KEY_CONFIG] = KEY_...,
             [KEY_HOMEPAGE] = KEY_...,
             [KEY_REFRESH] = KEY_...,
             [KEY_EXIT] = KEY_...,
             [KEY_MOVE] = KEY_...,
             [KEY_EDIT] = KEY_...,
             [KEY_SCROLLUP] = KEY_...,
             [KEY_SCROLLDOWN] = KEY_..., */
          [KEY_KPLEFTPAREN] = KEY_KEYPAD_LeftParenthesis,
          [KEY_KPRIGHTPAREN] = KEY_KEYPAD_RightParenthesis,
          /* [KEY_NEW] = KEY_...,
             [KEY_REDO] = KEY_..., */
          [KEY_F13] = KEY_FUNCTION_F13,
          [KEY_F14] = KEY_FUNCTION_F14,
          [KEY_F15] = KEY_FUNCTION_F15,
          [KEY_F16] = KEY_FUNCTION_F16,
          [KEY_F17] = KEY_FUNCTION_F17,
          [KEY_F18] = KEY_FUNCTION_F18,
          [KEY_F19] = KEY_FUNCTION_F19,
          [KEY_F20] = KEY_FUNCTION_F20,
          [KEY_F21] = KEY_FUNCTION_F21,
          [KEY_F22] = KEY_FUNCTION_F22,
          [KEY_F23] = KEY_FUNCTION_F23,
          [KEY_F24] = KEY_FUNCTION_F24,
          /* [KEY_PLAYCD] = KEY_...,
             [KEY_PAUSECD] = KEY_...,
             [KEY_PROG3] = KEY_...,
             [KEY_PROG4] = KEY_...,
             [KEY_SUSPEND] = KEY_...,
             [KEY_CLOSE] = KEY_...,
             [KEY_PLAY] = KEY_...,
             [KEY_FASTFORWARD] = KEY_...,
             [KEY_BASSBOOST] = KEY_...,
             [KEY_PRINT] = KEY_...,
             [KEY_HP] = KEY_...,
             [KEY_CAMERA] = KEY_...,
             [KEY_SOUND] = KEY_...,
             [KEY_QUESTION] = KEY_...,
             [KEY_EMAIL] = KEY_...,
             [KEY_CHAT] = KEY_...,
             [KEY_SEARCH] = KEY_...,
             [KEY_CONNECT] = KEY_...,
             [KEY_FINANCE] = KEY_...,
             [KEY_SPORT] = KEY_...,
             [KEY_SHOP] = KEY_...,
             [KEY_ALTERASE] = KEY_...,
             [KEY_CANCEL] = KEY_...,
             [KEY_BRIGHTNESSDOWN] = KEY_...,
             [KEY_BRIGHTNESSUP] = KEY_...,
             [KEY_MEDIA] = KEY_...,
             [KEY_SWITCHVIDEOMODE] = KEY_...,
             [KEY_KBDILLUMTOGGLE] = KEY_...,
             [KEY_KBDILLUMDOWN] = KEY_...,
             [KEY_KBDILLUMUP] = KEY_...,
             [KEY_SEND] = KEY_...,
             [KEY_REPLY] = KEY_...,
             [KEY_FORWARDMAIL] = KEY_...,
             [KEY_SAVE] = KEY_...,
             [KEY_DOCUMENTS] = KEY_...,
             [KEY_BATTERY] = KEY_...,
             [KEY_BLUETOOTH] = KEY_...,
             [KEY_WLAN] = KEY_...,
             [KEY_UWB] = KEY_...,
             [KEY_UNKNOWN] = KEY_...,
             [KEY_VIDEO_NEXT] = KEY_...,
             [KEY_VIDEO_PREV] = KEY_...,
             [KEY_BRIGHTNESS_CYCLE] = KEY_...,
             [KEY_BRIGHTNESS_ZERO] = KEY_...,
             [KEY_DISPLAY_OFF] = KEY_...,
             [KEY_WIMAX] = KEY_...,
             [BTN_MISC] = KEY_...,
             [BTN_0] = KEY_...,
             [BTN_1] = KEY_...,
             [BTN_2] = KEY_...,
             [BTN_3] = KEY_...,
             [BTN_4] = KEY_...,
             [BTN_5] = KEY_...,
             [BTN_6] = KEY_...,
             [BTN_7] = KEY_...,
             [BTN_8] = KEY_...,
             [BTN_9] = KEY_...,
             [BTN_MOUSE] = KEY_...,
             [BTN_LEFT] = KEY_...,
             [BTN_RIGHT] = KEY_...,
             [BTN_MIDDLE] = KEY_...,
             [BTN_SIDE] = KEY_...,
             [BTN_EXTRA] = KEY_...,
             [BTN_FORWARD] = KEY_...,
             [BTN_BACK] = KEY_...,
             [BTN_TASK] = KEY_...,
             [BTN_JOYSTICK] = KEY_...,
             [BTN_TRIGGER] = KEY_...,
             [BTN_THUMB] = KEY_...,
             [BTN_THUMB2] = KEY_...,
             [BTN_TOP] = KEY_...,
             [BTN_TOP2] = KEY_...,
             [BTN_PINKIE] = KEY_...,
             [BTN_BASE] = KEY_...,
             [BTN_BASE2] = KEY_...,
             [BTN_BASE3] = KEY_...,
             [BTN_BASE4] = KEY_...,
             [BTN_BASE5] = KEY_...,
             [BTN_BASE6] = KEY_...,
             [BTN_DEAD] = KEY_...,
             [BTN_GAMEPAD] = KEY_...,
             [BTN_A] = KEY_...,
             [BTN_B] = KEY_...,
             [BTN_C] = KEY_...,
             [BTN_X] = KEY_...,
             [BTN_Y] = KEY_...,
             [BTN_Z] = KEY_...,
             [BTN_TL] = KEY_...,
             [BTN_TR] = KEY_...,
             [BTN_TL2] = KEY_...,
             [BTN_TR2] = KEY_...,
             [BTN_SELECT] = KEY_...,
             [BTN_START] = KEY_...,
             [BTN_MODE] = KEY_...,
             [BTN_THUMBL] = KEY_...,
             [BTN_THUMBR] = KEY_...,
             [BTN_DIGI] = KEY_...,
             [BTN_TOOL_PEN] = KEY_...,
             [BTN_TOOL_RUBBER] = KEY_...,
             [BTN_TOOL_BRUSH] = KEY_...,
             [BTN_TOOL_PENCIL] = KEY_...,
             [BTN_TOOL_AIRBRUSH] = KEY_...,
             [BTN_TOOL_FINGER] = KEY_...,
             [BTN_TOOL_MOUSE] = KEY_...,
             [BTN_TOOL_LENS] = KEY_...,
             [BTN_TOUCH] = KEY_...,
             [BTN_STYLUS] = KEY_...,
             [BTN_STYLUS2] = KEY_...,
             [BTN_TOOL_DOUBLETAP] = KEY_...,
             [BTN_TOOL_TRIPLETAP] = KEY_...,
             [BTN_WHEEL] = KEY_...,
             [BTN_GEAR_DOWN] = KEY_...,
             [BTN_GEAR_UP] = KEY_...,
             [KEY_OK] = KEY_..., */
          [KEY_SELECT] = KEY_FUNCTION_Select,
          /* [KEY_GOTO] = KEY_..., */
          [KEY_CLEAR] = KEY_FUNCTION_Clear,
          /* [KEY_POWER2] = KEY_...,
             [KEY_OPTION] = KEY_...,
             [KEY_INFO] = KEY_...,
             [KEY_TIME] = KEY_...,
             [KEY_VENDOR] = KEY_...,
             [KEY_ARCHIVE] = KEY_...,
             [KEY_PROGRAM] = KEY_...,
             [KEY_CHANNEL] = KEY_...,
             [KEY_FAVORITES] = KEY_...,
             [KEY_EPG] = KEY_...,
             [KEY_PVR] = KEY_...,
             [KEY_MHP] = KEY_...,
             [KEY_LANGUAGE] = KEY_...,
             [KEY_TITLE] = KEY_...,
             [KEY_SUBTITLE] = KEY_...,
             [KEY_ANGLE] = KEY_...,
             [KEY_ZOOM] = KEY_...,
             [KEY_MODE] = KEY_...,
             [KEY_KEYBOARD] = KEY_...,
             [KEY_SCREEN] = KEY_...,
             [KEY_PC] = KEY_...,
             [KEY_TV] = KEY_...,
             [KEY_TV2] = KEY_...,
             [KEY_VCR] = KEY_...,
             [KEY_VCR2] = KEY_...,
             [KEY_SAT] = KEY_...,
             [KEY_SAT2] = KEY_...,
             [KEY_CD] = KEY_...,
             [KEY_TAPE] = KEY_...,
             [KEY_RADIO] = KEY_...,
             [KEY_TUNER] = KEY_...,
             [KEY_PLAYER] = KEY_...,
             [KEY_TEXT] = KEY_...,
             [KEY_DVD] = KEY_...,
             [KEY_AUX] = KEY_...,
             [KEY_MP3] = KEY_...,
             [KEY_AUDIO] = KEY_...,
             [KEY_VIDEO] = KEY_...,
             [KEY_DIRECTORY] = KEY_...,
             [KEY_LIST] = KEY_...,
             [KEY_MEMO] = KEY_...,
             [KEY_CALENDAR] = KEY_...,
             [KEY_RED] = KEY_...,
             [KEY_GREEN] = KEY_...,
             [KEY_YELLOW] = KEY_...,
             [KEY_BLUE] = KEY_...,
             [KEY_CHANNELUP] = KEY_...,
             [KEY_CHANNELDOWN] = KEY_...,
             [KEY_FIRST] = KEY_...,
             [KEY_LAST] = KEY_...,
             [KEY_AB] = KEY_...,
             [KEY_NEXT] = KEY_...,
             [KEY_RESTART] = KEY_...,
             [KEY_SLOW] = KEY_...,
             [KEY_SHUFFLE] = KEY_...,
             [KEY_BREAK] = KEY_...,
             [KEY_PREVIOUS] = KEY_...,
             [KEY_DIGITS] = KEY_...,
             [KEY_TEEN] = KEY_...,
             [KEY_TWEN] = KEY_...,
             [KEY_VIDEOPHONE] = KEY_...,
             [KEY_GAMES] = KEY_...,
             [KEY_ZOOMIN] = KEY_...,
             [KEY_ZOOMOUT] = KEY_...,
             [KEY_ZOOMRESET] = KEY_...,
             [KEY_WORDPROCESSOR] = KEY_...,
             [KEY_EDITOR] = KEY_...,
             [KEY_SPREADSHEET] = KEY_...,
             [KEY_GRAPHICSEDITOR] = KEY_...,
             [KEY_PRESENTATION] = KEY_...,
             [KEY_DATABASE] = KEY_...,
             [KEY_NEWS] = KEY_...,
             [KEY_VOICEMAIL] = KEY_...,
             [KEY_ADDRESSBOOK] = KEY_...,
             [KEY_MESSENGER] = KEY_...,
             [KEY_DISPLAYTOGGLE] = KEY_...,
             [KEY_SPELLCHECK] = KEY_...,
             [KEY_LOGOFF] = KEY_...,
             [KEY_DOLLAR] = KEY_...,
             [KEY_EURO] = KEY_...,
             [KEY_FRAMEBACK] = KEY_...,
             [KEY_FRAMEFORWARD] = KEY_...,
             [KEY_CONTEXT_MENU] = KEY_...,
             [KEY_MEDIA_REPEAT] = KEY_...,
             [KEY_DEL_EOL] = KEY_...,
             [KEY_DEL_EOS] = KEY_...,
             [KEY_INS_LINE] = KEY_...,
             [KEY_DEL_LINE] = KEY_...,
             [KEY_FN] = KEY_...,
             [KEY_FN_ESC] = KEY_...,
             [KEY_FN_F1] = KEY_...,
             [KEY_FN_F2] = KEY_...,
             [KEY_FN_F3] = KEY_...,
             [KEY_FN_F4] = KEY_...,
             [KEY_FN_F5] = KEY_...,
             [KEY_FN_F6] = KEY_...,
             [KEY_FN_F7] = KEY_...,
             [KEY_FN_F8] = KEY_...,
             [KEY_FN_F9] = KEY_...,
             [KEY_FN_F10] = KEY_...,
             [KEY_FN_F11] = KEY_...,
             [KEY_FN_F12] = KEY_...,
             [KEY_FN_1] = KEY_...,
             [KEY_FN_2] = KEY_...,
             [KEY_FN_D] = KEY_...,
             [KEY_FN_E] = KEY_...,
             [KEY_FN_F] = KEY_...,
             [KEY_FN_S] = KEY_...,
             [KEY_FN_B] = KEY_...,
             [KEY_BRL_DOT1] = KEY_...,
             [KEY_BRL_DOT2] = KEY_...,
             [KEY_BRL_DOT3] = KEY_...,
             [KEY_BRL_DOT4] = KEY_...,
             [KEY_BRL_DOT5] = KEY_...,
             [KEY_BRL_DOT6] = KEY_...,
             [KEY_BRL_DOT7] = KEY_...,
             [KEY_BRL_DOT8] = KEY_...,
             [KEY_BRL_DOT9] = KEY_...,
             [KEY_BRL_DOT10] = KEY_...,
             [KEY_MIN_INTERESTING] = KEY_... */
        };
        KeyCode code = map[event->code];

        if (code) {
          KeyEventHandler *handleKeyEvent = result->data;

          if (handleKeyEvent(code, event->value)) {
            return sizeof(*event);
          }
        } else {
          LogPrint(LOG_INFO, "unmapped Linux keycode %d", event->code);
        }

        writeKeyEvent(event->code, event->value);
      } else {
        writeInputEvent(event);
      }

      return sizeof(*event);
    }
#else /* HAVE_LINUX_INPUT_H */
    return result->length;
#endif /* HAVE_LINUX_INPUT_H */
  }

  return 0;
}

int
startKeyboardMonitor (const KeyboardProperties *keyboardProperties, KeyEventHandler handleKeyEvent) {
  int uinput = getUinputDevice();

  if (uinput != -1) {
    int keyboard = getKeyboardDevice(keyboardProperties);

    if (keyboard != -1) {
      if (asyncRead(keyboard, sizeof(struct input_event), handleKeyboardEvent, handleKeyEvent)) {
#ifdef EVIOCGRAB
        ioctl(keyboard, EVIOCGRAB, 1);
#endif /* EVIOCGRAB */

        return 1;
      }
    }
  }

  return 0;
}
