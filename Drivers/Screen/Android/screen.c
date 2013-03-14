/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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

#include <errno.h>

#include "log.h"
#include "brldefs.h"

#include "scr_driver.h"
#include "sys_android.h"

static JNIEnv *env = NULL;
static jclass screenDriverClass = NULL;

static jint screenRows;
static jint screenColumns;

static int
findScreenDriverClass (void) {
  return findJavaClass(env, &screenDriverClass, "org/a11y/brltty/android/ScreenDriver");
}

JNIEXPORT void JNICALL
Java_org_a11y_brltty_android_ScreenDriver_setScreenProperties (
  JNIEnv *env, jobject this,
  jint rows, jint columns
) {
  screenRows = rows;
  screenColumns = columns;
}

static void
describe_AndroidScreen (ScreenDescription *description) {
  if (findScreenDriverClass()) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, screenDriverClass, "updateCurrentView",
                             JAVA_SIG_METHOD(JAVA_SIG_VOID, ))) {
      (*env)->CallStaticObjectMethod(env, screenDriverClass, method);

      if (!clearJavaException(env, 1)) {
        description->cols = screenColumns;
        description->rows = screenRows;
        description->posx = 0;
        description->posy = 0;
        description->number = 0;
      } else {
        errno = EIO;
      }
    }
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
        (*env)->CallStaticObjectMethod(env, screenDriverClass, method, textBuffer, rowNumber, columnNumber);

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
    int rowIndex;

    for (rowIndex=0; rowIndex<box->height; rowIndex+=1) {
      if (!getRowCharacters(&buffer[rowIndex * box->width], (rowIndex + box->top), box->left, box->width)) return 0;
    }

    return 1;
  }

  return 0;
}

static void
doScreenDriverCommand (jmethodID *methodIdentifier, const char *methodName) {
  if (findScreenDriverClass()) {
    if (findJavaStaticMethod(env, methodIdentifier, screenDriverClass, methodName,
                             JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                            ))) {
      int result = (*env)->CallStaticObjectMethod(env, screenDriverClass, *methodIdentifier) != JNI_FALSE;

      if (clearJavaException(env, 1)) {
        errno = EIO;
      } else if (!result) {
      }
    }
  }
}

static int
executeCommand_AndroidScreen (int *command) {
  int blk = *command & BRL_MSK_BLK;
  int arg UNUSED = *command & BRL_MSK_ARG;

  switch (blk) {
    case -1:
      switch (arg) {
        case BRL_CMD_TOP: {
          static jmethodID method = 0;

          doScreenDriverCommand(&method, "moveUp");
          return 1;
        }

        case BRL_CMD_BOT: {
          static jmethodID method = 0;

          doScreenDriverCommand(&method, "moveDown");
          return 1;
        }

        case BRL_CMD_LNBEG: {
          static jmethodID method = 0;

          doScreenDriverCommand(&method, "moveLeft");
          return 1;
        }

        case BRL_CMD_LNEND: {
          static jmethodID method = 0;

          doScreenDriverCommand(&method, "moveRight");
          return 1;
        }

        default:
          break;
      }
      break;

    default:
      break;
  }

  return 0;
}

static void
scr_initialize (MainScreen *main) {
  initializeRealScreen(main);
  main->base.describe = describe_AndroidScreen;
  main->base.readCharacters = readCharacters_AndroidScreen;
  main->base.executeCommand = executeCommand_AndroidScreen;
  env = getJavaNativeInterface();
}
