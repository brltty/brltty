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

import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.ArrayList;

import android.util.Log;

import android.graphics.Rect;

import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

import android.accessibilityservice.AccessibilityService;

public class ScreenDriver {
  private static final String LOG_TAG = ScreenDriver.class.getName();

  private static final String ROOT_NODE_NAME = "root";
  private static final boolean LOG_ACCESSIBILITY_EVENTS = true;

  private static boolean isContainer (Rect outer, int left, int top, int right, int bottom) {
    return (left >= outer.left) &&
           (right <= outer.right) &&
           (top >= outer.top) &&
           (bottom <= outer.bottom);
  }

  private static boolean isContainer (Rect outer, int x, int y) {
    return isContainer(outer, x, y, x, y);
  }

  private static boolean isContainer (Rect outer, Rect inner) {
    return isContainer(outer, inner.left, inner.top, inner.right, inner.bottom);
  }

  private static class RenderedNode {
    private final AccessibilityNodeInfo screenNode;
    private final String renderedText;

    private Rect screenLocation = null;
    private Rect brailleLocation = null;

    public String getRenderedText () {
      return renderedText;
    }

    public Rect getScreenLocation () {
      synchronized (this) {
        if (screenLocation == null) {
          if (screenNode != null) {
            screenLocation = new Rect();
            screenNode.getBoundsInScreen(screenLocation);
          }
        }
      }

      return screenLocation;
    }

    public Rect getBrailleLocation () {
      return brailleLocation;
    }

    public void setBrailleLocation (Rect location) {
      brailleLocation = location;
    }

    public boolean performAction (int offset) {
      final int[] actions = {
        AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS,
        AccessibilityNodeInfo.ACTION_CLICK,
        AccessibilityNodeInfo.ACTION_LONG_CLICK,
        AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD,
        AccessibilityNodeInfo.ACTION_SCROLL_FORWARD
      };

      if (offset >= actions.length) {
        return false;
      }
      int action = actions[offset];

      AccessibilityNodeInfo node = screenNode;
      Rect inner = getScreenLocation();

      while (((node.getActions() & action) == 0) || !node.isEnabled()) {
        AccessibilityNodeInfo parent = node.getParent();
        if (parent == null) {
          return false;
        }

        Rect outer = new Rect();
        parent.getBoundsInScreen(outer);
        if (!isContainer(outer, inner)) {
          return false;
        }

        inner = outer;
        node = parent;
      }

      return node.performAction(action);
    }

    public boolean isCheckable () {
      return screenNode.isCheckable();
    }

    public boolean isChecked () {
      return screenNode.isChecked();
    }

    private String makeRenderedText (String text) {
      if (isCheckable()) {
        text = text + "[" + (isChecked()? "x": " ") + "]";
      }

      return text;
    }

    RenderedNode (AccessibilityNodeInfo node, String text) {
      screenNode = node;
      renderedText = makeRenderedText(text);
    }

    public interface LocationGetter {
      public Rect getLocation (RenderedNode node);
    }

    public static RenderedNode findByLocation (
      LocationGetter locationGetter,
      int column, int row
    ) {
      RenderedNode bestNode = null;
      Rect bestLocation = null;

      for (RenderedNode node : renderedNodes) {
        Rect location = locationGetter.getLocation(node);

        if (location != null) {
          if (isContainer(location, column, row)) {
            if ((bestLocation == null) || isContainer(bestLocation, location)) {
              bestLocation = location;
              bestNode = node;
            }
          }
        }
      }

      return bestNode;
    }
  }

  private static class GlobalRenderedNode extends RenderedNode {
    private final int clickAction;

    @Override
    public boolean isCheckable () {
      return false;
    }

    @Override
    public boolean isChecked () {
      return false;
    }

    public boolean performAction (int offset) {
      if (offset > 0) {
        return false;
      }

      return BrailleService.getBrailleService().performGlobalAction(clickAction);
    }

    GlobalRenderedNode (int action, String text) {
      super(null, text);
      clickAction = action;
    }
  }

  private final static Object eventLock = new Object();
  private volatile static AccessibilityNodeInfo latestNode = null;

  private static AccessibilityNodeInfo homeNode;
  private static List<RenderedNode> renderedNodes;
  private static List<CharSequence> screenRows;
  private static int screenWidth;

  private static AccessibilityNodeInfo currentNode;
  private static int cursorColumn;
  private static int cursorRow;

  private static AccessibilityNodeInfo getRootNode (AccessibilityNodeInfo node) {
    if (node != null) {
      while (true) {
        AccessibilityNodeInfo parent = node.getParent();

        if (parent == null) {
          break;
        }

        node = parent;
      }
    }

    return node;
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

    if (node.getActions() != 0)  {
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

  private static void addPropertyValue (
    List<CharSequence> values,
    boolean condition,
    CharSequence value
  ) {
    if (condition) {
      values.add(value);
    }
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
      addPropertyValue(values, node.isVisibleToUser(), "vis");
      addPropertyValue(values, node.getParent()==null, ROOT_NODE_NAME);

      {
        int count = node.getChildCount();
        addPropertyValue(values, count>0, "cld=" + count);
      }

      addPropertyValue(values, node.isEnabled(), "enb");
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
        addNodeProperty(rows, "lbl", getNodeText(subnode));
      }
    }

    {
      AccessibilityNodeInfo subnode = node.getLabeledBy();

      if (subnode != null) {
        addNodeProperty(rows, "lbd", getNodeText(subnode));
      }
    }

    {
      Rect rect = new Rect();
      node.getBoundsInScreen(rect);
      addNodeProperty(rows, "locn", rect.toShortString());
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
      String text = getNodeText(node);

      if (text != null) {
        sb.append(text);
      } else {
        sb.append(node.getClassName());
      }

      Rect location = new Rect();
      node.getBoundsInScreen(location);
      sb.append(" ");
      sb.append(location.toShortString());
    }

    Log.d(LOG_TAG, sb.toString());
  }

  public static void handleAccessibilityEvent (AccessibilityEvent event) {
    if (LOG_ACCESSIBILITY_EVENTS) {
      AccessibilityNodeInfo node = event.getSource();
      logAccessibilityNode(node, "event node");
      logSubtreeProperties(getRootNode(node));
    }

    switch (event.getEventType()) {
      case AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED:
      case AccessibilityEvent.TYPE_VIEW_FOCUSED:
      case AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED:
      case AccessibilityEvent.TYPE_VIEW_TEXT_CHANGED:
        synchronized (eventLock) {
          latestNode = event.getSource();
        }
        break;

      default:
        break;
    }
  }

  private static void setScreenRows (List<CharSequence> rows) {
    if (rows == null) {
      rows = new ArrayList<CharSequence>();
    }

    if (rows.isEmpty()) {
      rows.add("empty screen");
    }

    int width = 1;
    for (CharSequence row : rows) {
      int length = row.length();

      if (length > width) {
        width = length;
      }
    }

    screenRows = rows;
    screenWidth = width;
  }

  private static RenderedNode findScreenLocation (int column, int row) {
    RenderedNode.LocationGetter locationGetter = new RenderedNode.LocationGetter() {
      @Override
      public Rect getLocation (RenderedNode node) {
        return node.getScreenLocation();
      }
    };

    return RenderedNode.findByLocation(locationGetter, column, row);
  }

  private static RenderedNode findBrailleLocation (int column, int row) {
    RenderedNode.LocationGetter locationGetter = new RenderedNode.LocationGetter() {
      @Override
      public Rect getLocation (RenderedNode node) {
        return node.getBrailleLocation();
      }
    };

    return RenderedNode.findByLocation(locationGetter, column, row);
  }

  private static RenderedNode findRenderedNode (int column, int row) {
    RenderedNode bestNode = null;
    Rect bestLocation = null;

    for (RenderedNode node : renderedNodes) {
      Rect location = node.getBrailleLocation();

      if (isContainer(location, column, row)) {
        if ((bestLocation == null) || isContainer(bestLocation, location)) {
          bestLocation = location;
          bestNode = node;
        }
      }
    }

    return bestNode;
  }

  private static void setCursorLocation (int column, int row) {
    cursorColumn = column;
    cursorRow = row;
  }

  private static void setCursorLocation () {
    AccessibilityNodeInfo focusedNode;

    {
      AccessibilityNodeInfo root = getRootNode(homeNode);

      if ((focusedNode = root.findFocus(AccessibilityNodeInfo.FOCUS_ACCESSIBILITY)) != null) {
        logAccessibilityNode(focusedNode, "accessibility focus");
      } else if ((focusedNode = root.findFocus(AccessibilityNodeInfo.FOCUS_INPUT)) != null) {
        logAccessibilityNode(focusedNode, "input focus");
      } else {
        focusedNode = homeNode;
        logAccessibilityNode(focusedNode, "home node");
      }
    }

    Rect screenLocation = new Rect();
    focusedNode.getBoundsInScreen(screenLocation);
    RenderedNode node = findScreenLocation(screenLocation.left, screenLocation.top);

    if (node != null) {
      Rect brailleLocation = node.getBrailleLocation();
      setCursorLocation(brailleLocation.left, brailleLocation.top);
    } else {
      setCursorLocation(0, 0);
    }
  }

  public static native void exportScreenProperties (
    int number,
    int columns, int rows,
    int column, int row
  );

  private static void sortRenderedNodesByLocation (List<RenderedNode> nodes) {
    Comparator<RenderedNode> comparator = new Comparator<RenderedNode>() {
      @Override
      public int compare (RenderedNode node1, RenderedNode node2) {
        Rect location1 = node1.getScreenLocation();
        Rect location2 = node2.getScreenLocation();
        return (location1.top < location2.top)? -1:
               (location1.top > location2.top)? 1:
               (location1.left < location2.left)? -1:
               (location1.left > location2.left)? 1:
               (location1.right < location2.right)? -1:
               (location1.right > location2.right)? 1:
               (location1.bottom < location2.bottom)? -1:
               (location1.bottom > location2.bottom)? 1:
               0;
      }
    };

    Collections.sort(nodes, comparator);
  }

  private static void groupRenderedNodesByContainer (List<RenderedNode> nodes) {
    while (true) {
      int count = nodes.size();

      if (count < 3) {
        break;
      }

      Rect outer = nodes.get(0).getScreenLocation();
      List<RenderedNode> subnodes = new ArrayList<RenderedNode>();

      int index = 1;
      int to = 0;

      while (index < nodes.size()) {
        boolean contained = isContainer(outer, nodes.get(index).getScreenLocation());

        if (to == 0) {
          if (!contained) {
            to = index;
          }
        } else if (contained) {
          subnodes.add(nodes.remove(index));
          continue;
        }

        index += 1;
      }

      if (to > 0) {
        nodes.addAll(to, subnodes);
      }

      nodes = nodes.subList(1, count);
    }
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
    renderSubtree(nodes, root);
    sortRenderedNodesByLocation(nodes);
    groupRenderedNodesByContainer(nodes);

    nodes.add(0, new GlobalRenderedNode(AccessibilityService.GLOBAL_ACTION_NOTIFICATIONS, "Notifications"));
    nodes.add(1, new GlobalRenderedNode(AccessibilityService.GLOBAL_ACTION_QUICK_SETTINGS, "Quick Settings"));

    nodes.add(new GlobalRenderedNode(AccessibilityService.GLOBAL_ACTION_BACK, "Back"));
    nodes.add(new GlobalRenderedNode(AccessibilityService.GLOBAL_ACTION_HOME, "Home"));
    nodes.add(new GlobalRenderedNode(AccessibilityService.GLOBAL_ACTION_RECENTS, "Recent Apps"));

    {
      List<CharSequence> rows = new ArrayList<CharSequence>();

      for (RenderedNode node : nodes) {
        String renderedText = node.getRenderedText();
        int rowIndex = rows.size();
        node.setBrailleLocation(new Rect(0, rowIndex, renderedText.length()-1, rowIndex));
        rows.add(renderedText);
      }

      setScreenRows(rows);
    }

    renderedNodes = nodes;
    setCursorLocation();

    exportScreenProperties(
      root.getWindowId(),
      screenWidth, screenRows.size(),
      cursorColumn, cursorRow
    );
  }

  private static void setCurrentNode (AccessibilityNodeInfo node) {
    currentNode = node;
  }

  public static void updateCurrentView () {
    boolean hasChanged;

    synchronized (eventLock) {
      if ((hasChanged = (latestNode != homeNode))) {
        homeNode = latestNode;
      }
    }

    if (hasChanged) {
      renderSubtree(getRootNode(homeNode));
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

  public static boolean routeCursor (int column, int row) {
    if (row != -1) {
      RenderedNode node = findRenderedNode(column, row);

      if (node != null) {
        if (node.performAction(column - node.getBrailleLocation().left)) {
          return true;
        }
      }
    }

    return false;
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
