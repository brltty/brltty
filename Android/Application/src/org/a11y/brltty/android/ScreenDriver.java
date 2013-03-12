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

import android.util.Log;

import android.view.accessibility.AccessibilityEvent;

public class ScreenDriver {
  private static final String LOG_TAG = ScreenDriver.class.getName();

  private final static Object eventLock = new Object();
  private volatile static List<CharSequence> latestText = null;

  private static List<CharSequence> currentText;
  private static int currentWidth;

  public static void handleAccessibilityEvent (AccessibilityEvent event) {
    switch (event.getEventType()) {
      case AccessibilityEvent.TYPE_VIEW_FOCUSED:
      case AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED:
      case AccessibilityEvent.TYPE_VIEW_TEXT_CHANGED:
        List<CharSequence> text = new ArrayList(event.getText());

        if (text.isEmpty()) {
          CharSequence description = event.getContentDescription();

          if (description != null) {
            text.add(description);
          }
        }

        synchronized (eventLock) {
          latestText = text;
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
    List<CharSequence> text = null;

    synchronized (eventLock) {
      if (latestText != currentText) {
        text = latestText;
      }
    }

    if (text != null) {
      setCurrentText(text);
    }
  }

  public static void getRowText (char[] textBuffer, int rowNumber, int columnNumber) {
    CharSequence rowText = (rowNumber < currentText.size())? currentText.get(rowNumber): null;
    int rowLength = (rowText != null)? rowText.length(): 0;

    int textLength = textBuffer.length;
    int textIndex = 0;

    while (textIndex < textLength) {
      textBuffer[textIndex++] = (columnNumber < rowLength)? rowText.charAt(columnNumber++): ' ';
    }
  }

  static {
    List<CharSequence> text = new ArrayList<CharSequence>();
    text.add("BRLTTY on Android");
    setCurrentText(text);
  }
}
