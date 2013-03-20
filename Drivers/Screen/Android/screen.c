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

static jint screenNumber;
static jint screenColumns;
static jint screenRows;
static jint cursorColumn;
static jint cursorRow;

static int
findScreenDriverClass (void) {
  return findJavaClass(env, &screenDriverClass, "org/a11y/brltty/android/ScreenDriver");
}

JNIEXPORT void JNICALL
Java_org_a11y_brltty_android_ScreenDriver_exportScreenProperties (
  JNIEnv *env, jobject this,
  jint number,
  jint columns, jint rows,
  jint column, jint row
) {
  screenNumber = number;
  screenColumns = columns;
  screenRows = rows;
  cursorColumn = column;
  cursorRow = row;
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
        description->posx = cursorColumn;
        description->posy = cursorRow;
        description->number = screenNumber;
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

static int
routeCursor_AndroidScreen (int column, int row, int screen) {
  if (findScreenDriverClass()) {
    static jmethodID method = 0;

    if (findJavaStaticMethod(env, &method, screenDriverClass, "routeCursor",
                             JAVA_SIG_METHOD(JAVA_SIG_BOOLEAN,
                                             JAVA_SIG_INT // column
                                             JAVA_SIG_INT // row
                                            ))) {
      int result = (*env)->CallStaticObjectMethod(env, screenDriverClass, method, column, row) != JNI_FALSE;

      if (!clearJavaException(env, 1)) return result;
      errno = EIO;
    }
  }

  return 0;
}

static void
scr_initialize (MainScreen *main) {
  initializeRealScreen(main);
  main->base.describe = describe_AndroidScreen;
  main->base.readCharacters = readCharacters_AndroidScreen;
  main->base.routeCursor = routeCursor_AndroidScreen;
  env = getJavaNativeInterface();
}
