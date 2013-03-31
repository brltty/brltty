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

import android.content.Context;
import android.content.ComponentName;

import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

import android.view.inputmethod.InputMethodManager;
import android.view.inputmethod.InputMethodInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.KeyEvent;

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

  public static ScreenLogger getLogger () {
    return currentLogger;
  }

  public static ScreenWindow getWindow () {
    return currentWindow;
  }

  public static RenderedScreen getScreen () {
    return currentScreen;
  }

  private static void setEditCursor (AccessibilityNodeInfo node, int offset) {
    if (node != null) {
      ScreenTextEditor.get(node, true).setCursorOffset(offset);
    }
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
        return;

      case AccessibilityEvent.TYPE_VIEW_FOCUSED:
        break;

      case AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED:
        break;

      case AccessibilityEvent.TYPE_VIEW_TEXT_CHANGED:
        setEditCursor(event.getSource(), event.getFromIndex() + event.getAddedCount());
        break;

      case AccessibilityEvent.TYPE_VIEW_TEXT_SELECTION_CHANGED:
        setEditCursor(event.getSource(), event.getToIndex());
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
        ScreenTextEditor editor = ScreenTextEditor.getIfFocused(node);

        if (editor != null) {
          column += editor.getCursorOffset();
        }
      }

      setCursorLocation(column, row);
    } else {
      setCursorLocation(0, 0);
    }
  }

  public static void refreshScreen (AccessibilityNodeInfo node) {
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

  public static void refreshScreen () {
    AccessibilityNodeInfo node;

    synchronized (eventLock) {
      if ((node = eventNode) != null) {
        eventNode = null;
      }
    }

    if (node != null) {
      refreshScreen(node);
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

  public static InputMethodManager getInputMethodManager () {
    return (InputMethodManager)BrailleService.getBrailleService().getSystemService(Context.INPUT_METHOD_SERVICE);
  }

  public static String getInputServiceClassName () {
    return InputService.class.getName();
  }

  public static boolean inputServiceEnabled () {
    String inputServiceClass = getInputServiceClassName();

    for (InputMethodInfo info : getInputMethodManager().getEnabledInputMethodList()) {
      if (info.getComponent().getClassName().equals(inputServiceClass)) {
        return true;
      }
    }

    return false;
  }

  public static InputConnection getInputConnection () {
    InputService service = InputService.getInputService();
    if (service != null) return service.getCurrentInputConnection();

    getInputMethodManager().showInputMethodPicker();
    return null;
  }

  public static boolean inputCharacter (char character) {
    InputConnection connection =  getInputConnection();

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
    InputConnection connection =  getInputConnection();

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
    InputConnection connection =  getInputConnection();

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
    InputConnection connection =  getInputConnection();

    if (connection != null) {
      if (connection.deleteSurroundingText(0, 1)) {
        return true;
      }
    }

    return false;
  }

  public static boolean inputKeyFunction (int key) {
    switch (key) {
      case  1: return inputKey(KeyEvent.KEYCODE_F1);
      case  2: return inputKey(KeyEvent.KEYCODE_F2);
      case  3: return inputKey(KeyEvent.KEYCODE_F3);
      case  4: return inputKey(KeyEvent.KEYCODE_F4);
      case  5: return inputKey(KeyEvent.KEYCODE_F5);
      case  6: return inputKey(KeyEvent.KEYCODE_F6);
      case  7: return inputKey(KeyEvent.KEYCODE_F7);
      case  8: return inputKey(KeyEvent.KEYCODE_F8);
      case  9: return inputKey(KeyEvent.KEYCODE_F9);
      case 10: return inputKey(KeyEvent.KEYCODE_F10);
      case 11: return inputKey(KeyEvent.KEYCODE_F11);
      case 12: return inputKey(KeyEvent.KEYCODE_F12);
      default: return false;
    }
  }

  static {
    refreshScreen(BrailleService.getBrailleService().getRootInActiveWindow());
  }
}
