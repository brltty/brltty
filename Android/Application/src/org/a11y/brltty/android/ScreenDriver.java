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
import android.os.SystemClock;
import android.os.PowerManager;

import android.accessibilityservice.AccessibilityService;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

import android.view.inputmethod.InputMethodInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.KeyEvent;

import android.graphics.Rect;

public final class ScreenDriver {
  private final static Object eventLock = new Object();
  private volatile static AccessibilityNodeInfo eventNode = null;

  private static ScreenLogger currentLogger = new ScreenLogger(ScreenDriver.class.getName());
  private static ScreenWindow currentWindow = new ScreenWindow(0);
  private static RenderedScreen currentScreen = new RenderedScreen(null);

  private static final PowerManager.WakeLock wakeLock =
    ApplicationUtilities.getPowerManager().newWakeLock(
      PowerManager.SCREEN_DIM_WAKE_LOCK | PowerManager.ON_AFTER_RELEASE,
      ApplicationHooks.getContext().getString(R.string.app_name)
    );

  public static ScreenLogger getLogger () {
    return currentLogger;
  }

  public static ScreenWindow getWindow () {
    return currentWindow;
  }

  public static RenderedScreen getScreen () {
    return currentScreen;
  }

  private static boolean isDeviceLocked () {
    if (!ApplicationUtilities.getPowerManager().isScreenOn()) {
      if (ApplicationUtilities.getKeyguardManager().inKeyguardRestrictedInputMode()) {
        return true;
      }
    }

    return false;
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
  }

  private static AccessibilityNodeInfo getCursorNode () {
    AccessibilityNodeInfo root = currentScreen.getRootNode();

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      AccessibilityNodeInfo node = root.findFocus(AccessibilityNodeInfo.FOCUS_ACCESSIBILITY);

      if (node != null) {
        root.recycle();
        root = null;
        return node;
      }
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      AccessibilityNodeInfo node = root.findFocus(AccessibilityNodeInfo.FOCUS_INPUT);

      if (node != null) {
        root.recycle();
        root = null;
        return node;
      }
    }

    return root;
  }

  public static native void exportScreenProperties (
    int number,
    int columns, int rows,
    int column, int row,
    int from, int to
  );

  private static void exportScreenProperties () {
    int cursorColumn = 0;
    int cursorRow = 0;

    int selectedFrom = 0;
    int selectedTo = 0;

    AccessibilityNodeInfo node = getCursorNode();
    ScreenElement element = currentScreen.findRenderedScreenElement(node);

    if (element != null) {
      Rect location = element.getBrailleLocation();

      cursorColumn = location.left;
      cursorRow = location.top;

      {
        ScreenTextEditor editor = ScreenTextEditor.getIfFocused(node);

        if (editor != null) {
          selectedFrom = cursorColumn + editor.getSelectedFrom();
          selectedTo = cursorColumn + editor.getSelectedTo();
          cursorColumn += editor.getCursorOffset();
        }
      }
    }

    node.recycle();
    node = null;

    exportScreenProperties(
      currentWindow.getWindowIdentifier(),
      currentScreen.getScreenWidth(),
      currentScreen.getScreenHeight(),
      cursorColumn, cursorRow,
      selectedFrom, selectedTo
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
    if (isDeviceLocked()) return false;

    AccessibilityNodeInfo node;
    synchronized (eventLock) {
      if ((node = eventNode) != null) {
        eventNode = null;
      }
    }

    if (node != null) {
      refreshScreen(node);
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
    if (row == -1) {
      return false;
    }

    return currentScreen.performAction(column, row);
  }

  private static boolean performGlobalAction (int action) {
    return BrailleService.getBrailleService().performGlobalAction(action);
  }

  public static InputMethodInfo getInputMethodInfo (Class classObject) {
    String className = classObject.getName();

    for (InputMethodInfo info : ApplicationUtilities.getInputMethodManager().getEnabledInputMethodList()) {
      if (info.getComponent().getClassName().equals(className)) {
        return info;
      }
    }

    return null;
  }

  public static InputMethodInfo getInputMethodInfo () {
    return getInputMethodInfo(InputService.class);
  }

  public static boolean isInputServiceEnabled () {
    return getInputMethodInfo() != null;
  }

  public static boolean isInputServiceSelected () {
    InputMethodInfo info = getInputMethodInfo();

    if (info != null) {
      if (info.getId().equals(ApplicationUtilities.getSelectedInputMethodIdentifier())) {
        return true;
      }
    }

    return false;
  }

  public static InputConnection getInputConnection () {
    InputService service = InputService.getInputService();
    if (service != null) {
      InputConnection connection = service.getCurrentInputConnection();
      if (connection != null) return connection;
    }

    if (!isInputServiceSelected()) {
      ApplicationUtilities.getInputMethodManager().showInputMethodPicker();
    }

    return null;
  }

  public static boolean inputCharacter (char character) {
    InputConnection connection = getInputConnection();

    if (connection != null) {
      if (connection.commitText(Character.toString(character), 1)) {
        return true;
      }
    }

    return false;
  }

  public static KeyEvent newKeyEvent (int action, int code) {
    long time = SystemClock.uptimeMillis();
    return new KeyEvent(time, time, action, code, 0);
  }

  public static boolean inputKey (int code) {
    InputConnection connection = getInputConnection();

    if (connection != null) {
      if (connection.sendKeyEvent(newKeyEvent(KeyEvent.ACTION_DOWN, code))) {
        if (connection.sendKeyEvent(newKeyEvent(KeyEvent.ACTION_UP, code))) {
          return true;
        }
      }
    }

    return false;
  }

  public static boolean inputKeyEnter () {
    {
      InputService service = InputService.getInputService();

      if (service != null) {
        EditorInfo info = service.getCurrentInputEditorInfo();

        if (info != null) {
          if (info.actionId != 0) {
            InputConnection connection = service.getCurrentInputConnection();

            if (connection != null) {
              if (connection.performEditorAction(info.actionId)) {
                return true;
              }
            }
          } else if (service.sendDefaultEditorAction(false)) {
            return true;
          }
        }
      }
    }

    return false;
  }

  public static boolean inputKeyTab () {
    return inputKey(KeyEvent.KEYCODE_TAB);
  }

  public static boolean inputKeyBackspace () {
    InputConnection connection = getInputConnection();

    if (connection != null) {
      if (connection.deleteSurroundingText(1, 0)) {
        return true;
      }
    }

    return false;
  }

  public static boolean inputKeyEscape () {
    return inputKey(KeyEvent.KEYCODE_ESCAPE);
  }

  public static boolean inputKeyCursorLeft () {
    return inputKey(KeyEvent.KEYCODE_DPAD_LEFT);
  }

  public static boolean inputKeyCursorRight () {
    return inputKey(KeyEvent.KEYCODE_DPAD_RIGHT);
  }

  public static boolean inputKeyCursorUp () {
    return inputKey(KeyEvent.KEYCODE_DPAD_UP);
  }

  public static boolean inputKeyCursorDown () {
    return inputKey(KeyEvent.KEYCODE_DPAD_DOWN);
  }

  public static boolean inputKeyPageUp () {
    return inputKey(KeyEvent.KEYCODE_PAGE_UP);
  }

  public static boolean inputKeyPageDown () {
    return inputKey(KeyEvent.KEYCODE_PAGE_DOWN);
  }

  public static boolean inputKeyHome () {
    return inputKey(KeyEvent.KEYCODE_MOVE_HOME);
  }

  public static boolean inputKeyEnd () {
    return inputKey(KeyEvent.KEYCODE_MOVE_END);
  }

  public static boolean inputKeyInsert () {
    return inputKey(KeyEvent.KEYCODE_INSERT);
  }

  public static boolean inputKeyDelete () {
    InputConnection connection = getInputConnection();

    if (connection != null) {
      if (connection.deleteSurroundingText(0, 1)) {
        return true;
      }
    }

    return false;
  }

  private static final int[] functionKeyMap = new int[] {
    KeyEvent.KEYCODE_F1,
    KeyEvent.KEYCODE_F2,
    KeyEvent.KEYCODE_F3,
    KeyEvent.KEYCODE_F4,
    KeyEvent.KEYCODE_F5,
    KeyEvent.KEYCODE_F6,
    KeyEvent.KEYCODE_F7,
    KeyEvent.KEYCODE_F8,
    KeyEvent.KEYCODE_F9,
    KeyEvent.KEYCODE_F10,
    KeyEvent.KEYCODE_F11,
    KeyEvent.KEYCODE_F12,

    KeyEvent.KEYCODE_NOTIFICATION,
    KeyEvent.KEYCODE_BACK,
    KeyEvent.KEYCODE_HOME,
    KeyEvent.KEYCODE_APP_SWITCH,
    KeyEvent.KEYCODE_SETTINGS
  };

  public static boolean inputKeyFunction (int key) {
    if (key < 0) return false;
    if (key >= functionKeyMap.length) return false;

    int code = functionKeyMap[key];
    if (code == KeyEvent.KEYCODE_UNKNOWN) return false;

    switch (code) {
      case KeyEvent.KEYCODE_HOME:
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          return performGlobalAction(AccessibilityService.GLOBAL_ACTION_HOME);
        }
        break;

      case KeyEvent.KEYCODE_BACK:
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          return performGlobalAction(AccessibilityService.GLOBAL_ACTION_BACK);
        }
        break;

      case KeyEvent.KEYCODE_APP_SWITCH:
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          return performGlobalAction(AccessibilityService.GLOBAL_ACTION_RECENTS);
        }
        break;

      case KeyEvent.KEYCODE_NOTIFICATION:
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          return performGlobalAction(AccessibilityService.GLOBAL_ACTION_NOTIFICATIONS);
        }
        break;

      case KeyEvent.KEYCODE_SETTINGS:
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          return performGlobalAction(AccessibilityService.GLOBAL_ACTION_QUICK_SETTINGS);
        }
        break;

      default:
        break;
    }

    return inputKey(code);
  }

  public static void resetLockTimer () {
    wakeLock.acquire();
    wakeLock.release();
  }

  private ScreenDriver () {
  }

  static {
    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      eventNode = BrailleService.getBrailleService().getRootInActiveWindow();
    }
  }
}
