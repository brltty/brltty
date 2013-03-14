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

import android.graphics.Rect;

import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

public class ScreenDriver {
  private static final String LOG_TAG = ScreenDriver.class.getName();

  private static class RenderedNode {
    private final AccessibilityNodeInfo node;
    private final CharSequence text;
    private final Rect rectangle = new Rect();

    public CharSequence getText () {
      return text;
    }

    RenderedNode (AccessibilityNodeInfo node, CharSequence text) {
      this.node = node;
      this.text = text;
      node.getBoundsInScreen(this.rectangle);
    }
  }

  private static final String ROOT_NODE_NAME = "root";

  private final static Object eventLock = new Object();
  private volatile static AccessibilityNodeInfo latestNode = null;

  private static AccessibilityNodeInfo homeNode;
  private static AccessibilityNodeInfo currentNode;

  private static List<CharSequence> screenRows;
  private static int screenWidth;

  public static void handleAccessibilityEvent (AccessibilityEvent event) {
    switch (event.getEventType()) {
      case AccessibilityEvent.TYPE_VIEW_FOCUSED:
      case AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED:
      case AccessibilityEvent.TYPE_VIEW_TEXT_CHANGED:
        synchronized (eventLock) {
          latestNode = event.getSource();
        }
        break;
    }
  }

  public static native void setScreenProperties (int rows, int columns);

  public static void setScreenRows (List<CharSequence> rows) {
    int width = 1;

    if (rows == null) {
      rows = new ArrayList<CharSequence>();
    }

    if (rows.isEmpty()) {
      rows.add("empty screen");
    }

    for (CharSequence row : rows) {
      int length = row.length();

      if (length > width) {
        width = length;
      }
    }

    screenRows = rows;
    screenWidth = width;
    setScreenProperties(screenRows.size(), screenWidth);
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

  private static void addPropertyValue (
    List<CharSequence> values,
    boolean condition,
    CharSequence value
  ) {
    if (condition) {
      values.add(value);
    }
  }

  private static void addNodeProperties (
    List<CharSequence> rows,
    AccessibilityNodeInfo node
  ) {
    {
      List<CharSequence> values = new ArrayList<CharSequence>();
      addPropertyValue(values, node.isVisibleToUser(), "vis");
      addPropertyValue(values, node.getParent()==null, ROOT_NODE_NAME);

      {
        int childCount = node.getChildCount();
        addPropertyValue(values, childCount>0, "cld=" + childCount);
      }

      addPropertyValue(values, node.isEnabled(), "enb");
      addPropertyValue(values, node.isSelected(), "sel");
      addPropertyValue(values, node.isScrollable(), "scr");
      addPropertyValue(values, node.isPassword(), "pwd");
      addPropertyValue(values, node.isFocusable(), "fcb");
      addPropertyValue(values, node.isFocused(), "fcd");
      addPropertyValue(values, node.isAccessibilityFocused(), "fca");
      addPropertyValue(values, node.isClickable(), "clk");
      addPropertyValue(values, node.isLongClickable(), "lng");
      addPropertyValue(values, node.isCheckable(), "ckb");
      addPropertyValue(values, node.isChecked(), "ckd");
      addNodeProperty(rows, "flg", values.toArray(new CharSequence[values.size()]));
    }

    addNodeProperty(rows, "dsc", node.getContentDescription());
    addNodeProperty(rows, "txt", node.getText());
    addNodeProperty(rows, "cls", node.getClassName());
    addNodeProperty(rows, "pkg", node.getPackageName());

    {
      Rect rect = new Rect();
      node.getBoundsInScreen(rect);
      addNodeProperty(rows, "rct", rect.toShortString());
    }
  }

  private static void addSubtreeContent (
    List<CharSequence> rows,
    AccessibilityNodeInfo node,
    CharSequence name
  ) {
    addNodeProperty(rows, "name", name);
    addNodeProperties(rows, node);

    {
      int childCount = node.getChildCount();

      for (int childIndex=0; childIndex<childCount; childIndex+=1) {
        addSubtreeContent(rows, node.getChild(childIndex), name + "." + childIndex);
      }
    }
  }

  private static void logScreenContent (AccessibilityNodeInfo node) {
    List<CharSequence> rows = new ArrayList<CharSequence>();
    addSubtreeContent(rows, getRootNode(node), ROOT_NODE_NAME);

    for (CharSequence row : rows) {
      Log.d(LOG_TAG, row.toString());
    }
  }

  private static CharSequence getNodeText (AccessibilityNodeInfo node) {
    CharSequence text;

    if ((text = node.getContentDescription()) != null) {
      return text;
    }

    if ((text = node.getText()) != null) {
      return text;
    }

    if (node.isFocusable())  {
      return node.getClassName();
    }

    return null;
  }

  private static void renderSubtree (
    List<RenderedNode> nodes,
    AccessibilityNodeInfo root
  ) {
    CharSequence text = getNodeText(root);

    if (text != null) {
      nodes.add(new RenderedNode(root, text));
    }

    {
      int childCount = root.getChildCount();

      for (int childIndex=0; childIndex<childCount; childIndex+=1) {
        renderSubtree(nodes, root.getChild(childIndex));
      }
    }
  }

  private static void renderSubtree (AccessibilityNodeInfo root) {
    List<RenderedNode> nodes = new ArrayList<RenderedNode>();
    renderSubtree(nodes, root);

    {
      List<CharSequence> rows = new ArrayList<CharSequence>();

      for (RenderedNode node : nodes) {
        rows.add(node.getText());
      }

      setScreenRows(rows);
    }
  }

  private static AccessibilityNodeInfo getRootNode (AccessibilityNodeInfo node) {
    while (true) {
      AccessibilityNodeInfo parent = node.getParent();

      if (parent == null) {
        return node;
      }

      node = parent;
    }
  }

  private static void setCurrentNode (AccessibilityNodeInfo node) {
    currentNode = node;
    logScreenContent(getRootNode(node));
    renderSubtree(getRootNode(node));
  }

  public static void updateCurrentView () {
    boolean hasChanged;

    synchronized (eventLock) {
      if ((hasChanged = (latestNode != homeNode))) {
        homeNode = latestNode;
      }
    }

    if (hasChanged) {
      setCurrentNode(homeNode);
    }
  }

  public static void getRowText (char[] textBuffer, int rowNumber, int columnNumber) {
    CharSequence rowText = (rowNumber < screenRows.size())? screenRows.get(rowNumber): null;
    int rowLength = (rowText != null)? rowText.length(): 0;

    int textLength = textBuffer.length;
    int textIndex = 0;

    while (textIndex < textLength) {
      textBuffer[textIndex++] = (columnNumber < rowLength)? rowText.charAt(columnNumber++): ' ';
    }
  }

  public static boolean moveUp () {
    AccessibilityNodeInfo parent = currentNode.getParent();

    if (parent != null) {
      setCurrentNode(parent);
      return true;
    }

    return false;
  }

  public static boolean moveDown () {
    if (currentNode.getChildCount() > 0) {
      setCurrentNode(currentNode.getChild(0));
      return true;
    }

    return false;
  }

  public static boolean moveLeft () {
    AccessibilityNodeInfo parent = currentNode.getParent();

    if (parent != null) {
      int childCount = parent.getChildCount();

      for (int childIndex=parent.getChildCount()-1; childIndex>=0; childIndex-=1) {
        AccessibilityNodeInfo child = parent.getChild(childIndex);

        if (child == currentNode) {
          if (--childIndex >= 0) {
            setCurrentNode(parent.getChild(childIndex));
            return true;
          }

          break;
        }
      }
    }

    return false;
  }

  public static boolean moveRight () {
    AccessibilityNodeInfo parent = currentNode.getParent();

    if (parent != null) {
      int childCount = parent.getChildCount();

      for (int childIndex=0; childIndex<childCount; childIndex+=1) {
        AccessibilityNodeInfo child = parent.getChild(childIndex);

        if (child == currentNode) {
          if (++childIndex < childCount) {
            setCurrentNode(parent.getChild(childIndex));
            return true;
          }

          break;
        }
      }
    }

    return false;
  }

  static {
    List<CharSequence> rows = new ArrayList<CharSequence>();
    rows.add("BRLTTY on Android");
    setScreenRows(rows);
  }
}
