/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
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
  private final static String LOG_TAG = ScreenDriver.class.getName();

  private ScreenDriver () {
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

    if (APITests.haveKitkat) {
      Bundle extras = notification.extras;

      if (extras != null) {
        {
          CharSequence title = extras.getCharSequence(Notification.EXTRA_TITLE);
          if (!TextUtils.isEmpty(title)) lines.add(title);
        }

        {
          CharSequence text = extras.getCharSequence(Notification.EXTRA_TEXT);

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

  private static void showEventText (BrailleMessage message, AccessibilityEvent event) {
    message.show(toText(event));
  }

  private static void showNotification (AccessibilityEvent event) {
    BrailleMessage message;
    String text = null;
    Notification notification = (Notification)event.getParcelableData();

    if (notification != null) {
      if (!ApplicationSettings.SHOW_NOTIFICATIONS) return;
      if (!APITests.haveJellyBean) return;
      if (notification.priority < Notification.PRIORITY_DEFAULT) return;

      message = BrailleMessage.NOTIFICATION;
      text = toText(notification);
    } else {
      if (!ApplicationSettings.SHOW_TOASTS) return;
      message = BrailleMessage.TOAST;
    }

    if (text == null) text = toText(event);
    message.show(text);
  }

  private static void logUnhandledEvent (AccessibilityEvent event, AccessibilityNodeInfo node) {
    if (ApplicationSettings.LOG_UNHANDLED_EVENTS) {
      StringBuilder log = new StringBuilder();

      log.append("unhandled accessibility event: ");
      log.append(event.toString());

      if (node != null) {
        log.append("; Source: ");
        log.append(ScreenLogger.toString(node));
      }

      Log.w(LOG_TAG, log.toString());
    }
  }

  private static boolean setFocus (AccessibilityNodeInfo node) {
    if (APITests.haveJellyBean) {
      if (node.isAccessibilityFocused()) return true;
      node = AccessibilityNodeInfo.obtain(node);

      {
        AccessibilityNodeInfo subnode = node.findFocus(AccessibilityNodeInfo.FOCUS_ACCESSIBILITY);

        if (subnode != null) {
          node.recycle();
          node = subnode;
          subnode = null;
        }
      }

      {
        AccessibilityNodeInfo subnode = ScreenUtilities.findTextNode(node);
        if (subnode == null) subnode = ScreenUtilities.findDescribedNode(node);

        if (subnode != null) {
          node.recycle();
          node = subnode;
          subnode = null;
        }
      }

      try {
        if (node.isAccessibilityFocused()) return true;

        if (node.performAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS)) {
          return true;
        }
      } finally {
        node.recycle();
        node = null;
      }
    }

    return false;
  }

  private static class FocusSetter extends DeferredTask {
    private AccessibilityNodeInfo focusNode = null;

    public FocusSetter () {
      super(ApplicationParameters.FOCUS_SETTER_DELAY);
    }

    @Override
    protected final boolean isStartable () {
      return focusNode != null;
    }

    @Override
    protected final void releaseResources () {
      if (focusNode != null) {
        focusNode.recycle();
        focusNode = null;
      }
    }

    public final void start (AccessibilityNodeInfo node) {
      synchronized (this) {
        releaseResources();
        if (node != null) focusNode = AccessibilityNodeInfo.obtain(node);
        restart();
      }
    }

    @Override
    public void runTask () {
      if (focusNode != null) setFocus(focusNode);
    }
  }

  private final static FocusSetter focusSetter = new FocusSetter();

  private native static void screenUpdated ();
  private final static Object NODE_LOCK = new Object();
  private volatile static AccessibilityNodeInfo currentNode = null;

  public static void setCurrentNode (AccessibilityNodeInfo newNode) {
    if (newNode != null) {
      {
        AccessibilityNodeInfo oldNode;

        synchronized (NODE_LOCK) {
          oldNode = currentNode;
          currentNode = AccessibilityNodeInfo.obtain(newNode);
          newNode = null;
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
  }

  static {
    AccessibilityNodeInfo root = ScreenUtilities.getRootNode();

    if (root != null) {
      try {
        setFocus(root);
        setCurrentNode(root);
      } finally {
        root.recycle();
        root = null;
      }
    }
  }

  private static ScreenWindow lockedScreenWindow = null;

  public static void lockScreenWindow (ScreenWindow window) {
    lockedScreenWindow = window;
  }

  public static void unlockScreenWindow () {
    lockScreenWindow(null);
  }

  private static ScreenWindow currentScreenWindow =
    ScreenWindow.getScreenWindow(0)
                .setRenderedScreen(new RenderedScreen(null))
                ;

  public static ScreenWindow getCurrentScreenWindow () {
    if (lockedScreenWindow != null) return lockedScreenWindow;
    return currentScreenWindow;
  }

  public static RenderedScreen getCurrentRenderedScreen () {
    return getCurrentScreenWindow().getRenderedScreen();
  }

  public static void onAccessibilityEvent (AccessibilityEvent event) {
    focusSetter.restart();

    final boolean log = ApplicationSettings.LOG_ACCESSIBILITY_EVENTS;
    if (log) Log.d(LOG_TAG, ("accessibility event: " + event.toString()));

    int eventType = event.getEventType();
    AccessibilityNodeInfo node = event.getSource();

    if (node == null) {
      switch (eventType) {
        case AccessibilityEvent.TYPE_NOTIFICATION_STATE_CHANGED:
          showNotification(event);
          break;

        case AccessibilityEvent.TYPE_ANNOUNCEMENT:
          if (ApplicationSettings.SHOW_ANNOUNCEMENTS) showEventText(BrailleMessage.ANNOUNCEMENT, event);
          break;

        case AccessibilityEvent.TYPE_WINDOWS_CHANGED:
          setCurrentNode(ScreenUtilities.getRootNode());
          break;

        default:
          logUnhandledEvent(event, null);
        case AccessibilityEvent.TYPE_TOUCH_INTERACTION_START:
        case AccessibilityEvent.TYPE_TOUCH_INTERACTION_END:
        case AccessibilityEvent.TYPE_GESTURE_DETECTION_START:
        case AccessibilityEvent.TYPE_GESTURE_DETECTION_END:
        case AccessibilityEvent.TYPE_TOUCH_EXPLORATION_GESTURE_START:
        case AccessibilityEvent.TYPE_TOUCH_EXPLORATION_GESTURE_END:
        case AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED:
        case AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED:
        case AccessibilityEvent.TYPE_VIEW_FOCUSED:
        case AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUS_CLEARED:
        case AccessibilityEvent.TYPE_VIEW_CLICKED:
          break;
      }
    } else {
      try {
        if (log) ScreenLogger.log(node);

        if (APITests.haveLollipop) {
          AccessibilityWindowInfo window = node.getWindow();

          if (window != null) {
            try {
              boolean accept =
                (lockedScreenWindow != null)?
                (window.getId() == lockedScreenWindow.getWindowIdentifier()):
                window.isActive();

              if (!accept) return;
            } finally {
              window.recycle();
              window = null;
            }
          }
        }

        switch (eventType) {
          case AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED:
            focusSetter.start(node);
            break;

          case AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED:
            break;

          case AccessibilityEvent.TYPE_VIEW_SCROLLED:
            break;

          case AccessibilityEvent.TYPE_VIEW_SELECTED:
            break;

          case AccessibilityEvent.TYPE_VIEW_FOCUSED:
            break;

          case AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED:
            focusSetter.stop();
            break;

          case AccessibilityEvent.TYPE_VIEW_TEXT_CHANGED: {
            if (!APITests.haveJellyBeanMR2) {
              TextField field = TextField.get(node, true);
              field.setCursor(event.getFromIndex() + event.getAddedCount());
            }

            break;
          }

          case AccessibilityEvent.TYPE_VIEW_TEXT_SELECTION_CHANGED: {
            if (!APITests.haveJellyBeanMR2) {
              TextField field = TextField.get(node, true);
              field.setSelection(event.getFromIndex(), event.getToIndex());
            }

            break;
          }

          default:
            logUnhandledEvent(event, node);
          case AccessibilityEvent.TYPE_VIEW_HOVER_ENTER:
          case AccessibilityEvent.TYPE_VIEW_HOVER_EXIT:
          case AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUS_CLEARED:
          case AccessibilityEvent.TYPE_VIEW_CLICKED:
          case AccessibilityEvent.TYPE_VIEW_LONG_CLICKED:
          case AccessibilityEvent.TYPE_VIEW_TEXT_TRAVERSED_AT_MOVEMENT_GRANULARITY:
            return;
        }

        if (false && node.isPassword()) {
          TextField field = TextField.get(node, true);
          field.setAccessibilityText(toText(event));
        }

        setCurrentNode(node);
      } finally {
        node.recycle();
        node = null;
      }
    }
  }

  private native static void exportScreenProperties (
    int number, int columns, int rows,
    int locLeft, int locTop, int locRight, int locBottom,
    int selLeft, int selTop, int selRight, int selBottom
  );

  private static void exportScreenProperties () {
    ScreenWindow window = getCurrentScreenWindow();
    RenderedScreen screen = window.getRenderedScreen();
    AccessibilityNodeInfo node = screen.getCursorNode();

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
        ScreenElement element = screen.findScreenElement(node);

        if (element != null) {
          Rect location = element.getBrailleLocation();
          locationLeft = location.left;
          locationTop = location.top;
          locationRight = location.right;
          locationBottom = location.bottom;

          if (ScreenUtilities.isEditable(node)) {
            int start = -1;
            int end = -1;

            if (APITests.haveJellyBeanMR2) {
              start = node.getTextSelectionStart();
              end = node.getTextSelectionEnd();
            } else {
              TextField field = TextField.get(node);

              if (field != null) {
                synchronized (field) {
                  start = field.getSelectionStart();
                  end = field.getSelectionEnd();
                }
              }
            }

            if ((0 <= start) && (start <= end)) {
              Point topLeft = element.getBrailleCoordinates(start);

              if (topLeft != null) {
                if (start == end) {
                  selectionLeft = selectionRight = topLeft.x;
                  selectionTop = selectionBottom = topLeft.y;
                } else {
                  Point bottomRight = element.getBrailleCoordinates(end-1);

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
      } finally {
        node.recycle();
        node = null;
      }
    }

    exportScreenProperties(
      window.getWindowIdentifier(),
      screen.getScreenWidth(), screen.getScreenHeight(),
      locationLeft, locationTop, locationRight, locationBottom,
      selectionLeft, selectionTop, selectionRight, selectionBottom
    );
  }

  static {
    exportScreenProperties();
  }

  private static void refreshScreen (AccessibilityNodeInfo node) {
    currentScreenWindow = ScreenWindow.setRenderedScreen(node);
    exportScreenProperties();
  }

  public static char refreshScreen () {
    if (ApplicationSettings.RELEASE_BRAILLE_DEVICE) return 'r';
    if (LockUtilities.isLocked()) return 'l';

    AccessibilityNodeInfo node;

    synchronized (NODE_LOCK) {
      if ((node = currentNode) != null) {
        currentNode = null;
      }
    }

    if (node != null) {
      try {
        refreshScreen(node);
      } finally {
        node.recycle();
        node = null;
      }
    }

    return 0;
  }

  public static char[] getRowText (int row, int column) {
    RenderedScreen screen = getCurrentRenderedScreen();
    String text = (row < screen.getScreenHeight())? screen.getScreenRow(row): "";
    int length = text.length();

    if (column > length) column = length;
    int count = length - column;

    char[] characters = new char[count];
    text.getChars(column, length, characters, 0);
    return characters;
  }

  public static boolean routeCursor (int column, int row) {
    if (row == -1) return false;
    return getCurrentRenderedScreen().performAction(column, row);
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
