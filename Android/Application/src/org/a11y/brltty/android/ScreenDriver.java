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

import android.os.Build;

import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

import android.graphics.Rect;

public class ScreenDriver {
  private static final boolean LOG_ACCESSIBILITY_EVENTS = false;

  private final static Object eventLock = new Object();
  private volatile static AccessibilityNodeInfo eventNode = null;

  private static ScreenLogger currentLogger = new ScreenLogger(ScreenDriver.class.getName());
  private static ScreenWindow currentWindow = new ScreenWindow(0);
  private static RenderedScreen currentScreen = new RenderedScreen(null);
  private static int cursorColumn = 0;
  private static int cursorRow = 0;

  private static void setTextCursor (AccessibilityNodeInfo node, int offset) {
    ScreenTextEditor.get(node, true).setCursorOffset(offset);
  }

  private static AccessibilityNodeInfo findFirstClickableSubnode (AccessibilityNodeInfo node) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
      final int actions = AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS | AccessibilityNodeInfo.ACTION_CLICK;

      if (node != null) {
        if (node.isVisibleToUser()) {
          if (node.isEnabled()) {
            if ((node.getActions() & actions) == actions) {
              return node;
            }
          }
        }

        {
          int childCount = node.getChildCount();

          for (int childIndex=0; childIndex<childCount; childIndex+=1) {
            AccessibilityNodeInfo subnode = findFirstClickableSubnode(node.getChild(childIndex));

            if (subnode != null) {
              return subnode;
            }
          }
        }
      }
    }

    return null;
  }

  private static boolean goToFirstClickableSubnode (AccessibilityNodeInfo node) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
      if ((node = findFirstClickableSubnode(node)) != null) {
        if (node.findFocus(AccessibilityNodeInfo.FOCUS_ACCESSIBILITY) == null) {
          if (node.performAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS)) {
            return true;
          }
        }
      }
    }

    return false;
  }

  public static void onAccessibilityEvent (AccessibilityEvent event) {
    if (LOG_ACCESSIBILITY_EVENTS) {
      currentLogger.logEvent(event);
    }

    switch (event.getEventType()) {
      case AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED:
        break;

      case AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED:
        goToFirstClickableSubnode(event.getSource());
        break;

      case AccessibilityEvent.TYPE_VIEW_FOCUSED:
        break;

      case AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED:
        break;

      case AccessibilityEvent.TYPE_VIEW_TEXT_CHANGED:
        setTextCursor(event.getSource(), event.getFromIndex() + event.getAddedCount());
        break;

      case AccessibilityEvent.TYPE_VIEW_TEXT_SELECTION_CHANGED:
        setTextCursor(event.getSource(), event.getToIndex());
        break;

      default:
        return;
    }

    synchronized (eventLock) {
      eventNode = event.getSource();
    }
  }

  public static native void exportScreenProperties (
    int number,
    int columns, int rows,
    int column, int row
  );

  private static void exportScreenProperties () {
    exportScreenProperties(
      currentWindow.getWindowIdentifier(),
      currentScreen.getScreenWidth(),
      currentScreen.getScreenHeight(),
      cursorColumn, cursorRow
    );
  }

  private static AccessibilityNodeInfo getCursorNode () {
    AccessibilityNodeInfo root = currentScreen.getRootNode();

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
      AccessibilityNodeInfo node = root.findFocus(AccessibilityNodeInfo.FOCUS_ACCESSIBILITY);

      if (node != null) {
        return node;
      }
    }

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
      AccessibilityNodeInfo node = root.findFocus(AccessibilityNodeInfo.FOCUS_INPUT);

      if (node != null) {
        return node;
      }
    }

    return root;
  }

  private static void setCursorLocation (int column, int row) {
    cursorColumn = column;
    cursorRow = row;
  }

  private static void setCursorLocation () {
    AccessibilityNodeInfo node = getCursorNode();
    ScreenElement element = currentScreen.findRenderedScreenElement(node);

    if (element != null) {
      Rect location = element.getBrailleLocation();
      int column = location.left;
      int row = location.top;

      {
        ScreenTextEditor editor = ScreenTextEditor.get(node);

        if (editor != null) {
          column += editor.getCursorOffset();
        }
      }

      setCursorLocation(column, row);
    } else {
      setCursorLocation(0, 0);
    }
  }

  public static void refreshScreen () {
    AccessibilityNodeInfo node;

    synchronized (eventLock) {
      if ((node = eventNode) != null) {
        eventNode = null;
      }
    }

    if (node != null) {
      {
        int identifier = node.getWindowId();

        if (identifier != currentWindow.getWindowIdentifier()) {
          currentWindow = new ScreenWindow(identifier);
        }
      }

      currentScreen = new RenderedScreen(node);
      setCursorLocation();
      exportScreenProperties();
    }
  }

  public static void getRowText (char[] textBuffer, int rowIndex, int columnIndex) {
    CharSequence rowText = (rowIndex < currentScreen.getScreenHeight())? currentScreen.getScreenRow(rowIndex): null;
    int rowLength = (rowText != null)? rowText.length(): 0;

    int textLength = textBuffer.length;
    int textIndex = 0;

    while (textIndex < textLength) {
      textBuffer[textIndex++] = (columnIndex < rowLength)? rowText.charAt(columnIndex++): ' ';
    }
  }

  public static boolean routeCursor (int column, int row) {
    if (row == -1) {
      return false;
    }

    return currentScreen.performAction(column, row);
  }

  public static boolean inputCharacter (char character) {
    return false;
  }

  public static boolean inputKeyEnter () {
    return false;
  }

  public static boolean inputKeyTab () {
    return false;
  }

  public static boolean inputKeyBackspace () {
    return false;
  }

  public static boolean inputKeyEscape () {
    return false;
  }

  public static boolean inputKeyCursorLeft () {
    return false;
  }

  public static boolean inputKeyCursorRight () {
    return false;
  }

  public static boolean inputKeyCursorUp () {
    return false;
  }

  public static boolean inputKeyCursorDown () {
    return false;
  }

  public static boolean inputKeyPageUp () {
    return false;
  }

  public static boolean inputKeyPageDown () {
    return false;
  }

  public static boolean inputKeyHome () {
    return false;
  }

  public static boolean inputKeyEnd () {
    return false;
  }

  public static boolean inputKeyInsert () {
    return false;
  }

  public static boolean inputKeyDelete () {
    return false;
  }

  public static boolean inputKeyFunction (int key) {
    return false;
  }

  static {
    exportScreenProperties();
  }
}
