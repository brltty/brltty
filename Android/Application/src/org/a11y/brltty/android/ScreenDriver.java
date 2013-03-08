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

package org.a11y.brltty.android;

import java.util.List;
import java.util.ArrayList;

import android.view.accessibility.AccessibilityEvent;

public class ScreenDriver {
  private static final String LOG_TAG = ScreenDriver.class.getName();

  private final static Object eventLock = new Object();
  private volatile static AccessibilityEvent latestEvent = null;

  private static AccessibilityEvent currentEvent = null;
  private static List<CharSequence> currentText;
  private static int currentWidth;

  public static void handleAccessibilityEvent (AccessibilityEvent event) {
    switch (event.getEventType()) {
      case AccessibilityEvent.TYPE_VIEW_FOCUSED:
        synchronized (eventLock) {
          latestEvent = event;
        }
        break;
    }
  }

  public static native void setScreenProperties (int rows, int columns);

  public static void setCurrentText (List<CharSequence> text) {
    int width = 1;

    if (text == null) {
      text = new ArrayList<CharSequence>();
    }

    if (text.isEmpty()) {
      text.add("empty screen");
    }

    for (CharSequence row : text) {
      int length = row.length();

      if (length > width) {
        width = length;
      }
    }

    currentText = text;
    currentWidth = width;
    setScreenProperties(currentText.size(), currentWidth);
  }

  public static void updateCurrentView () {
    boolean hasChanged;

    synchronized (eventLock) {
      if ((hasChanged = (latestEvent != currentEvent))) {
        currentEvent = latestEvent;
      }
    }

    if (hasChanged) {
      List<CharSequence> text = null;

      if (currentEvent != null) {
        text = currentEvent.getText();
      }

      setCurrentText(text);
    }
  }

  public static void getRowText (char[] textBuffer, int rowNumber, int columnNumber) {
    int columnCount = textBuffer.length;
    CharSequence rowText = currentText.get(rowNumber);
    int rowLength = rowText.length();
    int textIndex = 0;

    while (textIndex < columnCount) {
      textBuffer[textIndex++] = (columnNumber < rowLength)? rowText.charAt(columnNumber++): ' ';
    }
  }
}
