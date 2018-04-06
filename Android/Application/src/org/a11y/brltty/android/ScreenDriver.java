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

import java.util.Collection;
import java.util.ArrayList;

import android.util.Log;
import android.os.Bundle;
import android.app.Notification;

import android.accessibilityservice.AccessibilityService;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityWindowInfo;

import android.graphics.Rect;
import android.graphics.Point;
import android.text.TextUtils;

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

  private static String toText (Collection<CharSequence> lines) {
    if (lines == null) return null;

    StringBuilder text = new StringBuilder();
    boolean first = true;

    for (CharSequence line : lines) {
      if (line == null) continue;

      if (first) {
        first = false;
      } else {
        text.append('\n');
      }

      text.append(line);
    }

    if (text.length() == 0) return null;
    return text.toString();
  }

  private static String toText (Notification notification) {
    if (notification == null) return null;

    Collection<CharSequence> lines = new ArrayList<CharSequence>();
    CharSequence tickerText = notification.tickerText;

    if (ApplicationUtilities.haveKitkat) {
      Bundle extras = notification.extras;

      if (extras != null) {
        {
          CharSequence title = extras.getCharSequence("android.title");
          if (!TextUtils.isEmpty(title)) lines.add(title);
        }

        {
          CharSequence text = extras.getCharSequence("android.text");

          if (!TextUtils.isEmpty(text)) {
            lines.add(text);
            tickerText = null;
          }
        }
      }
    }

    if (!TextUtils.isEmpty(tickerText)) lines.add(tickerText);
    if (lines.isEmpty()) return null;
    return toText(lines);
  }

  private static String toText (AccessibilityEvent event) {
    return toText(event.getText());
  }

  private static void showNotification (AccessibilityEvent event) {
    if (ApplicationSettings.SHOW_NOTIFICATIONS) {
      String text = toText((Notification)event.getParcelableData());
      if (text == null) text = toText(event);

      if (text != null) {
        final String message = text;

        CoreWrapper.runOnCoreThread(
          new Runnable() {
            @Override
            public void run () {
              CoreWrapper.showMessage(message);
            }
          }
        );
      }
    }
  }

  private static boolean goToFirstTextSubnode (AccessibilityNodeInfo root) {
    if (ApplicationUtilities.haveJellyBean) {
      AccessibilityNodeInfo node = ScreenUtilities.findTextNode(root);
      if (node == null) node = ScreenUtilities.findDescribedNode(root);

      if (node != null) {
        try {
          AccessibilityNodeInfo subnode = node.findFocus(AccessibilityNodeInfo.FOCUS_ACCESSIBILITY);

          if (subnode != null) {
            try {
              return true;
            } finally {
              subnode.recycle();
              subnode = null;
            }
          }

          if (node.performAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS)) {
            return true;
          }
        } finally {
          node.recycle();
          node = null;
        }
      }
    }

    return false;
  }

  private static native void screenUpdated ();
  private final static Object eventLock = new Object();
  private volatile static AccessibilityNodeInfo currentNode = ScreenUtilities.getRootNode();

  public static void onAccessibilityEvent (AccessibilityEvent event) {
    int eventType = event.getEventType();
    AccessibilityNodeInfo newNode = event.getSource();

    if (ApplicationSettings.LOG_ACCESSIBILITY_EVENTS) {
      ScreenLogger.log(event);
    }

    if (ApplicationUtilities.haveLollipop) {
      if (newNode != null) {
        AccessibilityWindowInfo window = newNode.getWindow();

        if (window != null) {
          try {
            if (!window.isActive()) {
              newNode.recycle();
              newNode = null;
              return;
            }
          } finally {
            window.recycle();
            window = null;
          }
        }
      }
    }

    switch (eventType) {
      case AccessibilityEvent.TYPE_WINDOWS_CHANGED:
        newNode = ScreenUtilities.getRootNode();
        break;

      case AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED:
        break;

      case AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED:
        goToFirstTextSubnode(newNode);
        break;

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

      case AccessibilityEvent.TYPE_NOTIFICATION_STATE_CHANGED:
        showNotification(event);
        break;

      default: {
        if (newNode != null) {
          newNode.recycle();
          newNode = null;
        }

        return;
      }
    }

    if (newNode != null) {
      if (newNode.isPassword()) {
        TextField field = TextField.get(newNode, true);
      //field.setAccessibilityText(toText(event));
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
    int locationLeft, int locationTop, int locationRight, int locationBottom,
    int selectionLeft, int selectionTop, int selectionRight, int selectionBottom
  );

  private static void exportScreenProperties () {
    AccessibilityNodeInfo node = currentScreen.getCursorNode();

    int locationLeft = 0;
    int locationTop = 0;
    int locationRight = 0;
    int locationBottom = 0;

    int selectionLeft = 0;
    int selectionTop = 0;
    int selectionRight = 0;
    int selectionBottom = 0;

    if (node != null) {
      try {
        ScreenElement element = currentScreen.findRenderedScreenElement(node);

        if (element != null) {
          Rect location = element.getBrailleLocation();
          locationLeft = location.left;
          locationTop = location.top;
          locationRight = location.right;
          locationBottom = location.bottom;

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
                    selectionLeft = selectionRight = topLeft.x;
                    selectionTop = selectionBottom = topLeft.y;
                  } else {
                    Point bottomRight = element.getBrailleCoordinate(end-1);

                    if (bottomRight != null) {
                      selectionLeft = topLeft.x;
                      selectionTop = topLeft.y;
                      selectionRight = bottomRight.x + 1;
                      selectionBottom = bottomRight.y + 1;
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
      locationLeft, locationTop, locationRight, locationBottom,
      selectionLeft, selectionTop, selectionRight, selectionBottom
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

  public static char[] getRowText (int row, int column) {
    String text = (row < currentScreen.getScreenHeight())? currentScreen.getScreenRow(row): "";
    int length = text.length();

    if (column > length) column = length;
    int count = length - column;

    char[] characters = new char[count];
    text.getChars(column, length, characters, 0);
    return characters;
  }

  public static boolean routeCursor (int column, int row) {
    if (row == -1) return false;
    return currentScreen.performAction(column, row);
  }

  public static void reportEvent (char event) {
    switch (event) {
      case 'b': // braille device online
        ApplicationSettings.BRAILLE_DEVICE_ONLINE = true;
        BrailleNotification.updateState();
        break;

      case 'B': // braille device offline
        ApplicationSettings.BRAILLE_DEVICE_ONLINE = false;
        BrailleNotification.updateState();
        break;

      case 'k': // braille key event
        LockUtilities.resetTimer();
        break;
    }
  }
}
