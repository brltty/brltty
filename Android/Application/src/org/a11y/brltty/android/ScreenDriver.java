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

  private static class ScreenElement {
    private final String renderedText;

    protected Rect screenLocation = null;
    private Rect brailleLocation = null;

    public final String getRenderedText () {
      return renderedText;
    }

    public Rect getScreenLocation () {
      return null;
    }

    public final Rect getBrailleLocation () {
      return brailleLocation;
    }

    public final void setBrailleLocation (Rect location) {
      brailleLocation = location;
    }

    public AccessibilityNodeInfo getAccessibilityNode () {
      return null;
    }

    public boolean isCheckable () {
      return false;
    }

    public boolean isChecked () {
      return false;
    }

    public boolean onBringCursor () {
      return false;
    }

    public boolean onClick () {
      return false;
    }

    public boolean onLongClick () {
      return false;
    }

    public boolean onScrollBackward () {
      return false;
    }

    public boolean onScrollForward () {
      return false;
    }

    public final boolean performAction (int offset) {
      switch (offset) {
        case  0: return onBringCursor();
        case  1: return onClick();
        case  2: return onLongClick();
        case  3: return onScrollBackward();
        case  4: return onScrollForward();
        default: return false;
      }
    }

    protected String makeRenderedText (String text) {
      if (isCheckable()) {
        text = text + "[" + (isChecked()? "x": " ") + "]";
      }

      return text;
    }

    ScreenElement (String text) {
      renderedText = makeRenderedText(text);
    }

    public interface LocationGetter {
      public Rect getLocation (ScreenElement element);
    }

    public static ScreenElement find (AccessibilityNodeInfo node) {
      for (ScreenElement element : screenElements) {
        if (element.getAccessibilityNode() == node) {
          return element;
        }
      }

      return null;
    }

    public static ScreenElement find (
      LocationGetter locationGetter,
      int column, int row
    ) {
      ScreenElement bestElement = null;
      Rect bestLocation = null;

      for (ScreenElement element : screenElements) {
        Rect location = locationGetter.getLocation(element);

        if (location != null) {
          if (isContainer(location, column, row)) {
            if ((bestLocation == null) ||
                isContainer(bestLocation, location) ||
                !isContainer(location, bestLocation)) {
              bestLocation = location;
              bestElement = element;
            }
          }
        }
      }

      return bestElement;
    }

    private static ScreenElement findByScreenLocation (int column, int row) {
      ScreenElement.LocationGetter locationGetter = new ScreenElement.LocationGetter() {
        @Override
        public Rect getLocation (ScreenElement element) {
          return element.getScreenLocation();
        }
      };

      return find(locationGetter, column, row);
    }

    private static ScreenElement findByBrailleLocation (int column, int row) {
      ScreenElement.LocationGetter locationGetter = new ScreenElement.LocationGetter() {
        @Override
        public Rect getLocation (ScreenElement element) {
          return element.getBrailleLocation();
        }
      };

      return find(locationGetter, column, row);
    }
  }

  private static class RealScreenElement extends ScreenElement {
    private final AccessibilityNodeInfo accessibilityNode;

    @Override
    public Rect getScreenLocation () {
      synchronized (this) {
        if (screenLocation == null) {
          screenLocation = new Rect();
          accessibilityNode.getBoundsInScreen(screenLocation);
        }
      }

      return screenLocation;
    }

    @Override
    public AccessibilityNodeInfo getAccessibilityNode () {
      return accessibilityNode;
    }

    @Override
    public boolean isCheckable () {
      return accessibilityNode.isCheckable();
    }

    @Override
    public boolean isChecked () {
      return accessibilityNode.isChecked();
    }

    private boolean doAction (int action) {
      AccessibilityNodeInfo node = accessibilityNode;
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

    @Override
    public boolean onBringCursor () {
      return doAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS);
    }

    @Override
    public boolean onClick () {
      return doAction(AccessibilityNodeInfo.ACTION_CLICK);
    }

    @Override
    public boolean onLongClick () {
      return doAction(AccessibilityNodeInfo.ACTION_LONG_CLICK);
    }

    @Override
    public boolean onScrollBackward () {
      return doAction(AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD);
    }

    @Override
    public boolean onScrollForward () {
      return doAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD);
    }

    public RealScreenElement (String text, AccessibilityNodeInfo node) {
      super(text);
      accessibilityNode = node;
    }
  }

  private static class VirtualScreenElement extends ScreenElement {
    private final int clickAction;

    @Override
    public boolean onClick () {
      return BrailleService.getBrailleService().performGlobalAction(clickAction);
    }

    public VirtualScreenElement (String text, int action) {
      super(text);
      clickAction = action;
    }
  }

  private static class ScreenElements extends ArrayList<ScreenElement> {
    private int atTopCount = 0;

    public final void sortByScreenLocation () {
      Comparator<ScreenElement> comparator = new Comparator<ScreenElement>() {
        @Override
        public int compare (ScreenElement element1, ScreenElement element2) {
          Rect location1 = element1.getScreenLocation();
          Rect location2 = element2.getScreenLocation();
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

      Collections.sort(this, comparator);
    }

    public final void groupByContainer () {
      List<ScreenElement> elements = this;

      while (true) {
        int count = elements.size();

        if (count < 3) {
          break;
        }

        Rect outer = elements.get(0).getScreenLocation();
        List<ScreenElement> containedElements = new ScreenElements();

        int index = 1;
        int to = 0;

        while (index < elements.size()) {
          boolean contained = isContainer(outer, elements.get(index).getScreenLocation());

          if (to == 0) {
            if (!contained) {
              to = index;
            }
          } else if (contained) {
            containedElements.add(elements.remove(index));
            continue;
          }

          index += 1;
        }

        if (to > 0) {
          elements.addAll(to, containedElements);
        }

        elements = elements.subList(1, count);
      }
    }

    public final void add (String text, AccessibilityNodeInfo node) {
      add(new RealScreenElement(text, node));
    }

    public final void addAtTop (String text, int action) {
      add(atTopCount++, new VirtualScreenElement(text, action));
    }

    public final void addAtBottom (String text, int action) {
      add(new VirtualScreenElement(text, action));
    }

    public ScreenElements () {
      super();
    }
  }

  private final static Object eventLock = new Object();
  private volatile static AccessibilityNodeInfo latestNode = null;

  private static AccessibilityNodeInfo eventNode;
  private static AccessibilityNodeInfo currentNode;

  private static ScreenElements screenElements;
  private static List<CharSequence> screenRows;
  private static int screenWidth;
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

  private static void logAccessibilityEvent (AccessibilityEvent event) {
    Log.d(LOG_TAG, "accessibility event: " + event.getEventType() + "(" + event.toString() + ")");
  }

  public static void handleAccessibilityEvent (AccessibilityEvent event) {
    if (LOG_ACCESSIBILITY_EVENTS) {
      logAccessibilityEvent(event);

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

  private static void setCursorLocation (int column, int row) {
    cursorColumn = column;
    cursorRow = row;
  }

  private static void setCursorLocation () {
    AccessibilityNodeInfo node;

    {
      AccessibilityNodeInfo root = getRootNode(eventNode);

      if ((node = root.findFocus(AccessibilityNodeInfo.FOCUS_ACCESSIBILITY)) != null) {
        logAccessibilityNode(node, "accessibility focus");
      } else if ((node = root.findFocus(AccessibilityNodeInfo.FOCUS_INPUT)) != null) {
        logAccessibilityNode(node, "input focus");
      } else {
        node = eventNode;
        logAccessibilityNode(node, "event node");
      }
    }

    Rect screenLocation = new Rect();
    node.getBoundsInScreen(screenLocation);
    ScreenElement element = ScreenElement.findByScreenLocation(screenLocation.left, screenLocation.top);

    if (element != null) {
      Rect brailleLocation = element.getBrailleLocation();
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

  private static void renderSubtree (
    ScreenElements elements,
    AccessibilityNodeInfo root
  ) {
    if (root != null) {
      if (root.isVisibleToUser()) {
        String text = getNodeText(root);

        if (text != null) {
          elements.add(text, root);
        }
      }

      {
        int childCount = root.getChildCount();

        for (int childIndex=0; childIndex<childCount; childIndex+=1) {
          renderSubtree(elements, root.getChild(childIndex));
        }
      }
    }
  }

  private static void renderSubtree (AccessibilityNodeInfo root) {
    ScreenElements elements = new ScreenElements();
    renderSubtree(elements, root);
    elements.sortByScreenLocation();
    elements.groupByContainer();

    elements.addAtTop("Notifications", AccessibilityService.GLOBAL_ACTION_NOTIFICATIONS);
    elements.addAtTop("Quick Settings", AccessibilityService.GLOBAL_ACTION_QUICK_SETTINGS);

    elements.addAtBottom("Back", AccessibilityService.GLOBAL_ACTION_BACK);
    elements.addAtBottom("Home", AccessibilityService.GLOBAL_ACTION_HOME);
    elements.addAtBottom("Recent Apps", AccessibilityService.GLOBAL_ACTION_RECENTS);

    {
      List<CharSequence> rows = new ArrayList<CharSequence>();

      for (ScreenElement element : elements) {
        String renderedText = element.getRenderedText();
        int rowIndex = rows.size();
        element.setBrailleLocation(new Rect(0, rowIndex, renderedText.length()-1, rowIndex));
        rows.add(renderedText);
      }

      setScreenRows(rows);
    }

    screenElements = elements;
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
      if ((hasChanged = (latestNode != eventNode))) {
        eventNode = latestNode;
      }
    }

    if (hasChanged) {
      renderSubtree(getRootNode(eventNode));
      setCurrentNode(eventNode);
    }
  }

  public static void getRowText (char[] textBuffer, int rowIndex, int columnIndex) {
    CharSequence rowText = (rowIndex < screenRows.size())? screenRows.get(rowIndex): null;
    int rowLength = (rowText != null)? rowText.length(): 0;

    int textLength = textBuffer.length;
    int textIndex = 0;

    while (textIndex < textLength) {
      textBuffer[textIndex++] = (columnIndex < rowLength)? rowText.charAt(columnIndex++): ' ';
    }
  }

  public static boolean routeCursor (int column, int row) {
    if (row != -1) {
      ScreenElement element = ScreenElement.findByBrailleLocation(column, row);

      if (element != null) {
        if (element.performAction(column - element.getBrailleLocation().left)) {
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
