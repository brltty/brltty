/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
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

import org.a11y.brltty.core.*;

import android.os.Build;
import android.os.SystemClock;

import android.accessibilityservice.AccessibilityService;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

import android.graphics.Rect;
import android.graphics.Point;

public final class ScreenDriver {
  private final static Object eventLock = new Object();
  private volatile static AccessibilityNodeInfo eventNode = null;

  private static ScreenLogger currentLogger = new ScreenLogger(ScreenDriver.class.getName());
  private static ScreenWindow currentWindow = new ScreenWindow(0);
  private static RenderedScreen currentScreen = null;

  public static ScreenLogger getLogger () {
    return currentLogger;
  }

  public static ScreenWindow getWindow () {
    return currentWindow;
  }

  public static RenderedScreen getScreen () {
    return currentScreen;
  }

  private static AccessibilityNodeInfo findFirstClickableSubnode (AccessibilityNodeInfo node) {
    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      final int actions = AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS
                        | AccessibilityNodeInfo.ACTION_CLICK;

      if (node != null) {
        if (node.isVisibleToUser()) {
          if (node.isEnabled()) {
            if ((node.getActions() & actions) == actions) {
              return AccessibilityNodeInfo.obtain(node);
            }
          }
        }

        {
          int childCount = node.getChildCount();

          for (int childIndex=0; childIndex<childCount; childIndex+=1) {
            AccessibilityNodeInfo child = node.getChild(childIndex);

            if (child != null) {
              AccessibilityNodeInfo subnode = findFirstClickableSubnode(child);

              child.recycle();
              child = null;

              if (subnode != null) return subnode;
            }
          }
        }
      }
    }

    return null;
  }

  private static boolean goToFirstClickableSubnode (AccessibilityNodeInfo root) {
    boolean done = false;

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      AccessibilityNodeInfo node = findFirstClickableSubnode(root);

      if (node != null) {
        boolean hasFocus;

        {
          AccessibilityNodeInfo subnode = node.findFocus(AccessibilityNodeInfo.FOCUS_ACCESSIBILITY);
          hasFocus = subnode != null;

          if (subnode != null) {
            subnode.recycle();
            subnode = null;
          }
        }

        if (!hasFocus) {
          if (node.performAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS)) done = true;
        }

        node.recycle();
        node = null;
      }
    }

    return done;
  }

  public static native void screenUpdated ();

  public static void onAccessibilityEvent (AccessibilityEvent event) {
    if (ApplicationParameters.LOG_ACCESSIBILITY_EVENTS) {
      currentLogger.logEvent(event);
    }

    switch (event.getEventType()) {
      case AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED:
        break;

      case AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED: {
        AccessibilityNodeInfo node = event.getSource();

        if (node != null) {
          goToFirstClickableSubnode(node);
          node.recycle();
          node = null;
        }

        return;
      }

      case AccessibilityEvent.TYPE_VIEW_SCROLLED:
        break;

      case AccessibilityEvent.TYPE_VIEW_SELECTED:
        break;

      case AccessibilityEvent.TYPE_VIEW_FOCUSED:
        break;

      case AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED:
        break;

      case AccessibilityEvent.TYPE_VIEW_TEXT_CHANGED: {
        AccessibilityNodeInfo node = event.getSource();

        if (node != null) {
          ScreenTextEditor.get(node, true).setCursorLocation(event.getFromIndex() + event.getAddedCount());
          node.recycle();
          node = null;
        }

        break;
      }

      case AccessibilityEvent.TYPE_VIEW_TEXT_SELECTION_CHANGED: {
        AccessibilityNodeInfo node = event.getSource();

        if (node != null) {
          ScreenTextEditor editor = ScreenTextEditor.get(node, true);

          editor.setSelectedRegion(event.getFromIndex(), event.getToIndex());
          editor.setCursorLocation(event.getToIndex());

          node.recycle();
          node = null;
        }

        break;
      }

      default:
        return;
    }

    {
      AccessibilityNodeInfo oldNode;

      synchronized (eventLock) {
        oldNode = eventNode;
        eventNode = event.getSource();
      }

      if (oldNode != null) {
        oldNode.recycle();
        oldNode = null;
      }
    }

    CoreWrapper.runOnCoreThread(new Runnable() {
      @Override
      public void run () {
        screenUpdated();
      }
    });
  }

  public static native void exportScreenProperties (
    int number,
    int columns, int rows,
    int column, int row,
    int from, int to,
    int left, int top,
    int right, int bottom
  );

  private static void exportScreenProperties () {
    int cursorColumn = 0;
    int cursorRow = 0;

    int selectedFrom = 0;
    int selectedTo = 0;
    int selectedLeft = 0;
    int selectedTop = 0;
    int selectedRight = 0;
    int selectedBottom = 0;

    AccessibilityNodeInfo node = currentScreen.getCursorNode();

    if (node != null) {
      ScreenElement element = currentScreen.findRenderedScreenElement(node);

      if (element != null) {
        Rect location = element.getBrailleLocation();

        cursorColumn = location.left;
        cursorRow = location.top;

        {
          ScreenTextEditor editor = ScreenTextEditor.getIfFocused(node);

          if (editor != null) {
            {
              int from = editor.getSelectedFrom();
              int to = editor.getSelectedTo();

              if (to > from) {
                Point topLeft = element.getBrailleCoordinate(from);

                if (topLeft != null) {
                  Point bottomRight = element.getBrailleCoordinate(to-1);

                  if (bottomRight != null) {
                    selectedFrom = cursorColumn + topLeft.x;
                    selectedTo = cursorColumn + bottomRight.x;
                    selectedLeft = location.left;
                    selectedTop = cursorRow + topLeft.y;
                    selectedRight = location.right + 1;
                    selectedBottom = cursorRow + bottomRight.y + 1;
                  }
                }
              }
            }

            {
              Point cursor = element.getBrailleCoordinate(editor.getCursorOffset());

              if (cursor != null) {
                cursorColumn += cursor.x;
                cursorRow += cursor.y;
              }
            }
          }
        }
      }

      node.recycle();
      node = null;
    }

    exportScreenProperties(
      currentWindow.getWindowIdentifier(),
      currentScreen.getScreenWidth(),
      currentScreen.getScreenHeight(),
      cursorColumn, cursorRow,
      selectedFrom, selectedTo,
      selectedLeft, selectedTop,
      selectedRight, selectedBottom
    );
  }

  public static void refreshScreen (AccessibilityNodeInfo node) {
    {
      int identifier = node.getWindowId();

      if (identifier != currentWindow.getWindowIdentifier()) {
        currentWindow = new ScreenWindow(identifier);
      }
    }

    currentScreen = new RenderedScreen(node);
    exportScreenProperties();
  }

  public static boolean refreshScreen () {
    if (LockUtilities.isLocked()) return false;

    AccessibilityNodeInfo node;
    synchronized (eventLock) {
      if ((node = eventNode) != null) {
        eventNode = null;
      }
    }

    if (node != null) {
      long start = System.currentTimeMillis();
      refreshScreen(node);
      long duration = System.currentTimeMillis() - start;
      currentLogger.log("screen refresh time: " + duration + "ms");
    } else if (currentScreen == null) {
      currentScreen = new RenderedScreen(null);
      exportScreenProperties();
    }

    return true;
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
    if (row == -1) return false;
    return currentScreen.performAction(column, row);
  }

  private ScreenDriver () {
  }

  static {
    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      eventNode = BrailleService.getBrailleService().getRootInActiveWindow();
    }
  }
}
