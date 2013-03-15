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

import android.accessibilityservice.AccessibilityService;

public class ScreenDriver {
  private static final String LOG_TAG = ScreenDriver.class.getName();

  private static class RenderedNode {
    private final AccessibilityNodeInfo node;
    private final String nodeText;

    private Rect screenLocation = null;
    private Rect brailleLocation = null;

    public boolean isClickable () {
      return node.isClickable();
    }

    public boolean isCheckable () {
      return node.isCheckable();
    }

    public boolean isChecked () {
      return node.isChecked();
    }

    public boolean performClick () {
      return node.performAction(AccessibilityNodeInfo.ACTION_CLICK);
    }

    public String getRenderedText () {
      String text = nodeText;

      if (isCheckable()) {
        text = text + "[" + (isChecked()? "x": " ") + "]";
      }

      if (isClickable()) {
        text = "{" + text + "}";
      }

      return text;
    }

    public Rect getScreenLocation () {
      synchronized (this) {
        if (screenLocation == null) {
          screenLocation = new Rect();
          node.getBoundsInScreen(screenLocation);
        }
      }

      return screenLocation;
    }

    public void setBrailleLocation (Rect location) {
      brailleLocation = location;
    }

    RenderedNode (AccessibilityNodeInfo node, String text) {
      this.node = node;
      nodeText = text;
    }
  }

  private static class GlobalRenderedNode extends RenderedNode {
    private final int clickAction;

    public boolean isClickable () {
      return true;
    }

    public boolean isCheckable () {
      return false;
    }

    public boolean isChecked () {
      return false;
    }

    public boolean performClick () {
      return BrailleService.getAccessibilityService().performGlobalAction(clickAction);
    }

    GlobalRenderedNode (int action, String text) {
      super(null, text);
      clickAction = action;
    }
  }

  private static final String ROOT_NODE_NAME = "root";

  private final static Object eventLock = new Object();
  private volatile static AccessibilityNodeInfo latestNode = null;

  private static AccessibilityNodeInfo homeNode;
  private static AccessibilityNodeInfo currentNode;

  private static List<CharSequence> screenRows;
  private static int screenWidth;

  private static AccessibilityNodeInfo getRootNode (AccessibilityNodeInfo node) {
    if (node == null) {
      return null;
    }

    while (true) {
      AccessibilityNodeInfo parent = node.getParent();

      if (parent == null) {
        return node;
      }

      node = parent;
    }
  }

  private static String normalizeTextProperty (CharSequence text) {
    if (text != null) {
      String string = text.toString().trim();

      if (string.length() > 0) {
        return string;
      }
    }

    return null;
  }

  private static String getNodeText (AccessibilityNodeInfo node) {
    String text;

    if ((text = normalizeTextProperty(node.getContentDescription())) != null) {
      return text;
    }

    if ((text = normalizeTextProperty(node.getText())) != null) {
      return text;
    }

    if (node.isFocusable())  {
      text = normalizeTextProperty(node.getClassName());

      {
        int index = text.lastIndexOf('.');

        if (index >= 0) {
          text = text.substring(index+1);
        }
      }

      return text;
    }

    return null;
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
      addNodeProperty(rows, "flgs", values.toArray(new CharSequence[values.size()]));
    }

    addNodeProperty(rows, "desc", node.getContentDescription());
    addNodeProperty(rows, "text", node.getText());
    addNodeProperty(rows, "obj", node.getClassName());
    addNodeProperty(rows, "app", node.getPackageName());

    {
      Rect rect = new Rect();
      node.getBoundsInScreen(rect);
      addNodeProperty(rows, "locn", rect.toShortString());
    }
  }

  private static void addSubtreeContent (
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
          addSubtreeContent(rows, root.getChild(childIndex), name + "." + childIndex);
        }
      }
    }
  }

  private static void logSubtreeContent (AccessibilityNodeInfo root) {
    List<CharSequence> rows = new ArrayList<CharSequence>();
    addSubtreeContent(rows, root, ROOT_NODE_NAME);

    for (CharSequence row : rows) {
      Log.d(LOG_TAG, row.toString());
    }
  }

  public static void handleAccessibilityEvent (AccessibilityEvent event) {
    logSubtreeContent(getRootNode(event.getSource()));
    switch (event.getEventType()) {
      case AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED:
      case AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED:
      case AccessibilityEvent.TYPE_VIEW_FOCUSED:
      case AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED:
      case AccessibilityEvent.TYPE_VIEW_TEXT_CHANGED:
        synchronized (eventLock) {
          latestNode = event.getSource();
        }
        break;
    }
  }

  public static native void exportScreenProperties (int rows, int columns);

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
    exportScreenProperties(screenRows.size(), screenWidth);
  }

  private static void renderSubtree (
    List<RenderedNode> nodes,
    AccessibilityNodeInfo root
  ) {
    if (root != null) {
      if (root.isVisibleToUser()) {
        String text = getNodeText(root);

        if (text != null) {
          nodes.add(new RenderedNode(root, text));
        }
      }

      {
        int childCount = root.getChildCount();

        for (int childIndex=0; childIndex<childCount; childIndex+=1) {
          renderSubtree(nodes, root.getChild(childIndex));
        }
      }
    }
  }

  private static void renderSubtree (AccessibilityNodeInfo root) {
    List<RenderedNode> nodes = new ArrayList<RenderedNode>();

    nodes.add(new GlobalRenderedNode(AccessibilityService.GLOBAL_ACTION_NOTIFICATIONS, "Notifications"));
    nodes.add(new GlobalRenderedNode(AccessibilityService.GLOBAL_ACTION_QUICK_SETTINGS, "Quick Settings"));

    renderSubtree(nodes, root);

    nodes.add(new GlobalRenderedNode(AccessibilityService.GLOBAL_ACTION_BACK, "Back"));
    nodes.add(new GlobalRenderedNode(AccessibilityService.GLOBAL_ACTION_HOME, "Home"));
    nodes.add(new GlobalRenderedNode(AccessibilityService.GLOBAL_ACTION_RECENTS, "Recent Apps"));

    {
      List<CharSequence> rows = new ArrayList<CharSequence>();

      for (RenderedNode node : nodes) {
        rows.add(node.getRenderedText());
      }

      setScreenRows(rows);
    }
  }

  private static void setCurrentNode (AccessibilityNodeInfo node) {
    currentNode = node;
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
