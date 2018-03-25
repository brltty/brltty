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

package org.a11y.brltty.android;
import org.a11y.brltty.core.*;

import android.util.Log;

import android.accessibilityservice.AccessibilityService;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityWindowInfo;

import android.graphics.Rect;
import android.graphics.Point;

public abstract class ScreenDriver {
  private static final String LOG_TAG = ScreenDriver.class.getName();

  private ScreenDriver () {
  }

  private static ScreenWindow currentWindow = new ScreenWindow(0);
  private static RenderedScreen currentScreen = null;

  public static ScreenWindow getWindow () {
    return currentWindow;
  }

  public static RenderedScreen getScreen () {
    return currentScreen;
  }

  private static AccessibilityNodeInfo findFirstClickableSubnode (AccessibilityNodeInfo node) {
    if (ApplicationUtilities.haveJellyBean) {
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

    if (ApplicationUtilities.haveJellyBean) {
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

  private static native void screenUpdated ();
  private final static Object eventLock = new Object();
  private volatile static AccessibilityNodeInfo currentNode = null;

  static {
    if (ApplicationUtilities.haveJellyBean) {
      currentNode = BrailleService.getBrailleService().getRootInActiveWindow();
    }
  }

  public static void onAccessibilityEvent (AccessibilityEvent event) {
    AccessibilityNodeInfo newNode = event.getSource();

    if (ApplicationSettings.LOG_ACCESSIBILITY_EVENTS) {
      ScreenLogger.log(event);
    }

    if (ApplicationUtilities.haveLollipop) {
      if (newNode != null) {
        AccessibilityWindowInfo window = newNode.getWindow();

        if (window != null) {
          boolean ignore = !window.isActive();
          window.recycle();
          window = null;

          if (ignore) {
            newNode.recycle();
            newNode = null;
            return;
          }
        }
      }
    }

    switch (event.getEventType()) {
      case AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED:
        break;

      case AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED: {
        if (newNode != null) {
          goToFirstClickableSubnode(newNode);
        }

        break;
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
        if (newNode != null) {
          TextField field = TextField.get(newNode, true);
          field.setCursor(event.getFromIndex() + event.getAddedCount());
        }

        break;
      }

      case AccessibilityEvent.TYPE_VIEW_TEXT_SELECTION_CHANGED: {
        if (newNode != null) {
          TextField field = TextField.get(newNode, true);
          field.setSelection(event.getFromIndex(), event.getToIndex());
        }

        break;
      }

      default: {
        if (newNode != null) {
          newNode.recycle();
          newNode = null;
        }

        return;
      }
    }

    {
      AccessibilityNodeInfo oldNode;

      synchronized (eventLock) {
        oldNode = currentNode;
        currentNode = newNode;
      }

      if (oldNode != null) {
        oldNode.recycle();
        oldNode = null;
      }
    }

    CoreWrapper.runOnCoreThread(
      new Runnable() {
        @Override
        public void run () {
          screenUpdated();
        }
      }
    );
  }

  private static native void exportScreenProperties (
    int number,
    int columns, int rows,
    int left, int top,
    int right, int bottom
  );

  private static void exportScreenProperties () {
    AccessibilityNodeInfo node = currentScreen.getCursorNode();

    int selectionLeft = 0;
    int selectionTop = 0;
    int selectionRight = 0;
    int selectionBottom = 0;

    if (node != null) {
      try {
        ScreenElement element = currentScreen.findRenderedScreenElement(node);

        if (element != null) {
          Rect location = element.getBrailleLocation();
          selectionLeft = selectionRight = location.left;
          selectionTop = selectionBottom = location.top;

          if (ScreenUtilities.isEditable(node)) {
            TextField field = TextField.get(node);

            if (field != null) {
              int start;
              int end;

              synchronized (field) {
                start = field.getSelectionStart();
                end = field.getSelectionEnd();
              }

              {
                Point topLeft = element.getBrailleCoordinate(start);

                if (topLeft != null) {
                  if (start == end) {
                    selectionLeft += topLeft.x;
                    selectionTop += topLeft.y;
                    selectionRight += topLeft.x;
                    selectionBottom += topLeft.y;
                  } else {
                    Point bottomRight = element.getBrailleCoordinate(end-1);

                    if (bottomRight != null) {
                      selectionLeft += topLeft.x;
                      selectionTop += topLeft.y;
                      selectionRight += bottomRight.x + 1;
                      selectionBottom += bottomRight.y + 1;
                    }
                  }
                }
              }
            }
          }
        }
      } finally {
        node.recycle();
        node = null;
      }
    }

    exportScreenProperties(
      currentWindow.getWindowIdentifier(),
      currentScreen.getScreenWidth(),
      currentScreen.getScreenHeight(),
      selectionLeft, selectionTop,
      selectionRight, selectionBottom
    );
  }

  private static void refreshScreen (AccessibilityNodeInfo node) {
    {
      int identifier = node.getWindowId();

      if (identifier != currentWindow.getWindowIdentifier()) {
        currentWindow = new ScreenWindow(identifier);
      }
    }

    currentScreen = new RenderedScreen(node);
    exportScreenProperties();
  }

  public static char refreshScreen () {
    if (ApplicationSettings.RELEASE_BRAILLE_DEVICE) return 'r';
    if (LockUtilities.isLocked()) return 'l';
    AccessibilityNodeInfo node;

    synchronized (eventLock) {
      if ((node = currentNode) != null) {
        currentNode = null;
      }
    }

    if (node != null) {
      refreshScreen(node);
      node.recycle();
      node = null;
    } else if (currentScreen == null) {
      currentScreen = new RenderedScreen(null);
      exportScreenProperties();
    }

    return 0;
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

  public static void reportEvent (char event) {
    switch (event) {
      case 'b': // braille device online
        ApplicationSettings.BRAILLE_DEVICE_ONLINE = true;
        BrailleNotification.setState();
        break;

      case 'B': // braille device offline
        ApplicationSettings.BRAILLE_DEVICE_ONLINE = false;
        BrailleNotification.setState();
        break;

      case 'k': // braille key event
        LockUtilities.resetTimer();
        break;
    }
  }
}
