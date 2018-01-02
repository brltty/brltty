/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <errno.h>

#include "log.h"
#include "brl_cmds.h"
#include "unicode.h"

#include "scr_driver.h"
#include "system_java.h"

static JNIEnv *env = NULL;
static jclass screenDriverClass = NULL;
static jclass inputServiceClass = NULL;
static jclass lockUtilitiesClass = NULL;

static jint screenNumber;
static jint screenColumns;
static jint screenRows;
static jint cursorColumn;
static jint cursorRow;
static jint selectedFrom;
static jint selectedTo;
static jint selectedLeft;
static jint selectedTop;
static jint selectedRight;
static jint selectedBottom;

static const char *problemText;

static int
findScreenDriverClass (void) {
  return findJavaClass(env, &screenDriverClass, "org/a11y/brltty/android/ScreenDriver");
}

static int
findInputServiceClass (void) {
  return findJavaClass(env, &inputServiceClass, "org/a11y/brltty/android/InputService");
}

static int
findLockUtilitiesClass (void) {
  return findJavaClass(env, &lockUtilitiesClass, "org/a11y/brltty/android/LockUtilities");
}

static int
poll_AndroidScreen (void) {
  return 0;
}

JAVA_METHOD (
  org_a11y_brltty_android_ScreenDriver, screenUpdated, void
) {
  mainScreenUpdated();
}

JAVA_METHOD (
  org_a11y_brltty_android_ScreenDriver, exportScreenProperties, void,
  jint number,
  jint columns, jint rows,
  jint column, jint row,
  jint from, jint to,
  jint left, jint top,
  int right, int bottom
) {
  screenNumber = number;
  screenColumns = columns;
  screenRows = rows;
  cursorColumn = column;
  cursorRow = row;
  selectedFrom = from;
  selectedTo = to;
  selectedLeft = left;
  selectedTop = top;
  selectedRight = right;
  selectedBottom = bottom;
}

static int
refresh_AndroidScreen (void) {
  if (findScreenDriverClass()) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, screenDriverClass, "refreshScreen",
                             JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                            ))) {
      jboolean result = (*env)->CallStaticBooleanMethod(env, screenDriverClass, method);

      if (clearJavaException(env, 1)) {
        problemText = "java exception";
      } else if (result == JNI_FALSE) {
        problemText = "device locked";
      } else {
        problemText = NULL;
      }
    } else {
      problemText = "method not found";
    }
  } else {
    problemText = "class not found";
  }

  return 1;
}

static void
describe_AndroidScreen (ScreenDescription *description) {
  if ((description->unreadable = problemText)) {
    description->cols = strlen(problemText);
    description->rows = 1;
    description->posx = 0;
    description->posy = 0;
    description->number = 0;
  } else {
    description->cols = screenColumns;
    description->rows = screenRows;
    description->posx = cursorColumn;
    description->posy = cursorRow;
    description->number = screenNumber;
  }
}

static int
getRowCharacters (ScreenCharacter *characters, jint rowNumber, jint columnNumber, jint columnCount) {
  if (findScreenDriverClass()) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, screenDriverClass, "getRowText",
                             JAVA_SIG_METHOD(JAVA_SIG_VOID, 
                                             JAVA_SIG_ARRAY(JAVA_SIG_CHAR) // textBuffer
                                             JAVA_SIG_INT // rowNumber
                                             JAVA_SIG_INT // columnNumber
                                            ))) {
      jcharArray textBuffer = (*env)->NewCharArray(env, columnCount);

      if (textBuffer) {
        (*env)->CallStaticVoidMethod(env, screenDriverClass, method, textBuffer, rowNumber, columnNumber);

        if (!clearJavaException(env, 1)) {
          jchar buffer[columnCount];

          (*env)->GetCharArrayRegion(env, textBuffer, 0, columnCount, buffer);
          (*env)->DeleteLocalRef(env, textBuffer);
          textBuffer = NULL;

          {
            const jchar *source = buffer;
            const jchar *end = source + columnCount;
            ScreenCharacter *target = characters;

            while (source < end) {
              target->text = *source++;
              target->attributes = SCR_COLOUR_DEFAULT;
              target += 1;
            }
          }

          if ((rowNumber >= selectedTop) && (rowNumber < selectedBottom)) {
            int from = MAX(selectedLeft, columnNumber);
            int to = MIN(selectedRight, columnNumber+columnCount);

            if (rowNumber == selectedTop) from += selectedFrom - selectedLeft;
            if (rowNumber == (selectedBottom - 1)) to -= selectedRight - selectedTo - 1;

            if (from < to) {
              ScreenCharacter *target = characters + (from - columnNumber);
              const ScreenCharacter *end = target + (to - from);

              while (target < end) {
                target->attributes = SCR_COLOUR_FG_BLACK | SCR_COLOUR_BG_LIGHT_GREY;
                target += 1;
              }
            }
          }

          return 1;
        }
      } else {
        logMallocError();
        clearJavaException(env, 0);
      }
    }
  }

  return 0;
}

static int
readCharacters_AndroidScreen (const ScreenBox *box, ScreenCharacter *buffer) {
  if (validateScreenBox(box, screenColumns, screenRows)) {
    if (problemText) {
      setScreenMessage(box, buffer, problemText);
    } else {
      int rowIndex;

      for (rowIndex=0; rowIndex<box->height; rowIndex+=1) {
        if (!getRowCharacters(&buffer[rowIndex * box->width], (rowIndex + box->top), box->left, box->width)) return 0;
      }
    }

    return 1;
  }

  return 0;
}

static int
resetLockTimer (void) {
  if (findLockUtilitiesClass()) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, lockUtilitiesClass, "resetTimer",
                             JAVA_SIG_METHOD(JAVA_SIG_VOID,
                                            ))) {
      (*env)->CallStaticVoidMethod(env, lockUtilitiesClass, method);
      if (!clearJavaException(env, 1)) return 1;
      errno = EIO;
    }
  }

  return 0;
}

static int
handleCommand_AndroidScreen (int command) {
  resetLockTimer();
  return 0;
}

static int
routeCursor_AndroidScreen (int column, int row, int screen) {
  if (findScreenDriverClass()) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, screenDriverClass, "routeCursor",
                             JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                             JAVA_SIG_INT // column
                                             JAVA_SIG_INT // row
                                            ))) {
      jboolean result = (*env)->CallStaticBooleanMethod(env, screenDriverClass, method, column, row);

      if (!clearJavaException(env, 1)) {
        if (result != JNI_FALSE) {
          return 1;
        }
      }
    }
  }

  return 0;
}

static int
insertKey_AndroidScreen (ScreenKey key) {
  if (findInputServiceClass()) {
    wchar_t character = key & SCR_KEY_CHAR_MASK;

    setScreenKeyModifiers(&key, 0);

    if (!isSpecialKey(key)) {
      static jmethodID method = 0;

      if (findJavaStaticMethod(env, &method, inputServiceClass, "inputCharacter",
                               JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                               JAVA_SIG_CHAR // character
                                              ))) {
        jboolean result = (*env)->CallStaticBooleanMethod(env, inputServiceClass, method, character);

        if (!clearJavaException(env, 1)) {
          if (result != JNI_FALSE) {
            return 1;
          }
        }
      }
    } else if (character < SCR_KEY_FUNCTION) {
#define SIZE UNICODE_CELL_NUMBER(SCR_KEY_FUNCTION)
#define KEY(key,method) [UNICODE_CELL_NUMBER(SCR_KEY_##key)] = method

      static const char *const methodNames[SIZE] = {
        KEY(ENTER, "inputKeyEnter"),
        KEY(TAB, "inputKeyTab"),
        KEY(BACKSPACE, "inputKeyBackspace"),
        KEY(ESCAPE, "inputKeyEscape"),
        KEY(CURSOR_LEFT, "inputKeyCursorLeft"),
        KEY(CURSOR_RIGHT, "inputKeyCursorRight"),
        KEY(CURSOR_UP, "inputKeyCursorUp"),
        KEY(CURSOR_DOWN, "inputKeyCursorDown"),
        KEY(PAGE_UP, "inputKeyPageUp"),
        KEY(PAGE_DOWN, "inputKeyPageDown"),
        KEY(HOME, "inputKeyHome"),
        KEY(END, "inputKeyEnd"),
        KEY(INSERT, "inputKeyInsert"),
        KEY(DELETE, "inputKeyDelete"),
      };

      const unsigned int key = UNICODE_CELL_NUMBER(character);
      const char *methodName = methodNames[key];
      if (!methodName) return 0;

      static jmethodID methodIdentifiers[SIZE];
      jmethodID *methodIdentifier = &methodIdentifiers[key];

#undef SIZE
#undef KEY

      if (findJavaStaticMethod(env, methodIdentifier, inputServiceClass, methodName,
                               JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                              ))) {
        jboolean result = (*env)->CallStaticBooleanMethod(env, inputServiceClass, *methodIdentifier);

        if (!clearJavaException(env, 1)) {
          if (result != JNI_FALSE) {
            return 1;
          }
        }
      }
    } else {
      static jmethodID method = 0;

      if (findJavaStaticMethod(env, &method, inputServiceClass, "inputKeyFunction",
                               JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                               JAVA_SIG_INT // key
                                              ))) {
        jboolean result = (*env)->CallStaticBooleanMethod(env, inputServiceClass, method, character-SCR_KEY_FUNCTION);

        if (!clearJavaException(env, 1)) {
          if (result != JNI_FALSE) {
            return 1;
          }
        }
      }
    }
  }

  logMessage(LOG_WARNING, "unsuported key: %04X", key);
  return 0;
}

static void
scr_initialize (MainScreen *main) {
  initializeRealScreen(main);
  main->base.poll = poll_AndroidScreen;
  main->base.refresh = refresh_AndroidScreen;
  main->base.describe = describe_AndroidScreen;
  main->base.readCharacters = readCharacters_AndroidScreen;
  main->base.handleCommand = handleCommand_AndroidScreen;
  main->base.routeCursor = routeCursor_AndroidScreen;
  main->base.insertKey = insertKey_AndroidScreen;
  env = getJavaNativeInterface();
}
