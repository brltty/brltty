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
import android.view.accessibility.AccessibilityNodeInfo;

import android.graphics.Rect;

public class ScreenDriver {
  private static final String LOG_TAG = ScreenDriver.class.getName();

  private static final String ROOT_NODE_NAME = "root";
  private static final boolean LOG_ACCESSIBILITY_EVENTS = false;

  private final static Object eventLock = new Object();
  private volatile static AccessibilityNodeInfo eventNode = null;

  private static ScreenWindow currentWindow = new ScreenWindow(0);
  private static RenderedScreen currentScreen = new RenderedScreen(null);
  private static int cursorColumn = 0;
  private static int cursorRow = 0;

  private static void addPropertyValue (
    List<CharSequence> values,
    boolean condition,
    CharSequence onValue,
    CharSequence offValue
  ) {
    CharSequence value = condition? onValue: offValue;

    if (value != null) {
      values.add(value);
    }
  }

  private static void addPropertyValue (
    List<CharSequence> values,
    boolean condition,
    CharSequence onValue
  ) {
    addPropertyValue(values, condition, onValue, null);
  }

  private static void addPropertyValue (
    List<CharSequence> values,
    CharSequence value
  ) {
    addPropertyValue(values, true, value);
  }

  private static void addNodeProperty (
    List<CharSequence> rows,
    CharSequence name,
    CharSequence ... values
  ) {
    if (values.length > 0) {
      StringBuilder sb = new StringBuilder();

      sb.append(name);
      sb.append(':');

      for (CharSequence value : values) {
        sb.append(' ');

        if (value == null) {
          sb.append("null");
        } else if (value.length() == 0) {
          sb.append("nil");
        } else {
          sb.append(value);
        }
      }

      rows.add(sb.toString());
    }
  }

  private static void addNodeProperties (
    List<CharSequence> rows,
    AccessibilityNodeInfo node
  ) {
    {
      List<CharSequence> values = new ArrayList<CharSequence>();
      addPropertyValue(values, node.getParent()==null, ROOT_NODE_NAME);

      {
        int count = node.getChildCount();
        addPropertyValue(values, count>0, "cld=" + count);
      }

      addPropertyValue(values, node.isVisibleToUser(), "vis", "inv");
      addPropertyValue(values, node.isEnabled(), "enb", "dsb");
      addPropertyValue(values, node.isSelected(), "sel");
      addPropertyValue(values, node.isScrollable(), "scl");
      addPropertyValue(values, node.isFocusable(), "fcb");
      addPropertyValue(values, node.isFocused(), "fcd");
      addPropertyValue(values, node.isAccessibilityFocused(), "fca");
      addPropertyValue(values, node.isClickable(), "clk");
      addPropertyValue(values, node.isLongClickable(), "lng");
      addPropertyValue(values, node.isCheckable(), "ckb");
      addPropertyValue(values, node.isChecked(), "ckd");
      addPropertyValue(values, node.isPassword(), "pwd");
      addNodeProperty(rows, "flgs", values.toArray(new CharSequence[values.size()]));
    }

    addNodeProperty(rows, "desc", node.getContentDescription());
    addNodeProperty(rows, "text", node.getText());
    addNodeProperty(rows, "obj", node.getClassName());
    addNodeProperty(rows, "app", node.getPackageName());

    {
      AccessibilityNodeInfo subnode = node.getLabelFor();

      if (subnode != null) {
        addNodeProperty(rows, "lbl", currentScreen.getNodeText(subnode));
      }
    }

    {
      AccessibilityNodeInfo subnode = node.getLabeledBy();

      if (subnode != null) {
        addNodeProperty(rows, "lbd", currentScreen.getNodeText(subnode));
      }
    }

    {
      Rect location = new Rect();
      node.getBoundsInScreen(location);
      addNodeProperty(rows, "locn", location.toShortString());
    }
  }

  private static void addSubtreeProperties (
    List<CharSequence> rows,
    AccessibilityNodeInfo root,
    CharSequence name
  ) {
    addNodeProperty(rows, "name", name);

    if (root != null) {
      addNodeProperties(rows, root);

      {
        int childCount = root.getChildCount();

        for (int childIndex=0; childIndex<childCount; childIndex+=1) {
          addSubtreeProperties(rows, root.getChild(childIndex), name + "." + childIndex);
        }
      }
    }
  }

  private static void logSubtreeProperties (AccessibilityNodeInfo root) {
    List<CharSequence> rows = new ArrayList<CharSequence>();
    addSubtreeProperties(rows, root, ROOT_NODE_NAME);

    for (CharSequence row : rows) {
      Log.d(LOG_TAG, row.toString());
    }
  }

  private static void logAccessibilityNode (AccessibilityNodeInfo node, String description) {
    StringBuilder sb = new StringBuilder();

    sb.append(description);
    sb.append(": ");

    if (node == null) {
      sb.append("null");
    } else {
      String text = currentScreen.getNodeText(node);

      if ((text != null) && (text.length() > 0)) {
        sb.append(text);
      } else {
        sb.append(node.getClassName());
      }

      Rect location = new Rect();
      node.getBoundsInScreen(location);
      sb.append(' ');
      sb.append(location.toShortString());
    }

    Log.d(LOG_TAG, sb.toString());
  }

  private static void logAccessibilityEvent (AccessibilityEvent event) {
    Log.d(LOG_TAG, "accessibility event: " + event.getEventType() + "(" + event.toString() + ")");

    AccessibilityNodeInfo node = event.getSource();

    if (node != null) {
      Log.d(LOG_TAG, "current window: " + node.getWindowId());
    }

    logAccessibilityNode(node, "event node");
    logSubtreeProperties(currentScreen.findRootNode(node));
  }

  private static void setTextCursor (AccessibilityNodeInfo node, int offset) {
    currentWindow.getTextEntry(node, true).setCursorOffset(offset);
  }

  public static void onAccessibilityEvent (AccessibilityEvent event) {
    if (LOG_ACCESSIBILITY_EVENTS) {
      logAccessibilityEvent(event);
    }

    switch (event.getEventType()) {
      case AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED:
        break;

      case AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED:
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

  private static void setCursorLocation (int column, int row) {
    cursorColumn = column;
    cursorRow = row;
  }

  private static void setCursorLocation () {
    AccessibilityNodeInfo root = currentScreen.getRootNode();
    AccessibilityNodeInfo node;

    if ((node = root.findFocus(AccessibilityNodeInfo.FOCUS_ACCESSIBILITY)) != null) {
    } else if ((node = root.findFocus(AccessibilityNodeInfo.FOCUS_INPUT)) != null) {
    } else {
      node = root;
    }

    ScreenElement element = currentScreen.findRenderedScreenElement(node);

    if (element != null) {
      Rect location = element.getBrailleLocation();
      int column = location.left;
      int row = location.top;

      {
        ScreenWindow.TextEntry textEntry = currentWindow.getTextEntry(node, false);

        if (textEntry != null) {
          column += textEntry.getCursorOffset();
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
}
