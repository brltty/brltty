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

import android.util.Log;

import java.util.List;
import java.util.ArrayList;

import android.view.accessibility.AccessibilityNodeInfo;

import android.graphics.Rect;

public class RenderedScreen {
  private static final String LOG_TAG = RenderedScreen.class.getName();

  private final AccessibilityNodeInfo eventNode;
  private final AccessibilityNodeInfo rootNode;

  private final ScreenElementList screenElements = new ScreenElementList();
  private final List<String> screenRows = new ArrayList<String>();

  private final int screenWidth;
  private final AccessibilityNodeInfo cursorNode;

  private final AccessibilityNodeInfo getNode (AccessibilityNodeInfo node) {
    if (node == null) return null;
    return AccessibilityNodeInfo.obtain(node);
  }

  public final AccessibilityNodeInfo getRootNode () {
    return getNode(rootNode);
  }

  public final int getScreenWidth () {
    return screenWidth;
  }

  public final int getScreenHeight () {
    return screenRows.size();
  }

  public final String getScreenRow (int index) {
    return screenRows.get(index);
  }

  public final AccessibilityNodeInfo getCursorNode () {
    return getNode(cursorNode);
  }

  public final ScreenElement getScreenElement (AccessibilityNodeInfo node) {
    if (node == null) return null;
    return screenElements.get(node);
  }

  public final ScreenElement findRenderedScreenElement (AccessibilityNodeInfo node) {
    ScreenElement element = getScreenElement(node);

    if (element != null) {
      if (element.getBrailleLocation() != null) {
        return element;
      }
    }

    {
      int childCount = node.getChildCount();

      for (int childIndex=0; childIndex<childCount; childIndex+=1) {
        AccessibilityNodeInfo child = node.getChild(childIndex);

        if (child != null) {
          element = findRenderedScreenElement(child);

          child.recycle();
          child = null;

          if (element != null) return element;
        }
      }
    }

    return null;
  }

  public final boolean performAction (int column, int row) {
    ScreenElement element = screenElements.findByBrailleLocation(column, row);
    if (element == null) return false;

    Rect location = element.getBrailleLocation();
    return element.performAction((column - location.left), (row - location.top));
  }

  private static final int SIGNIFICANT_NODE_ACTIONS
    = AccessibilityNodeInfo.ACTION_CLICK
    | AccessibilityNodeInfo.ACTION_LONG_CLICK
    | AccessibilityNodeInfo.ACTION_SCROLL_FORWARD
    | AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD
    ;

  private static int getSignificantActions (AccessibilityNodeInfo node) {
    return node.getActions() & SIGNIFICANT_NODE_ACTIONS;
  }

  private static boolean hasSignificantActions (AccessibilityNodeInfo node) {
    return getSignificantActions(node) != 0;
  }

  private static boolean hasInnerText (AccessibilityNodeInfo root) {
    int childCount = root.getChildCount();

    for (int childIndex=0; childIndex<childCount; childIndex+=1) {
      AccessibilityNodeInfo child = root.getChild(childIndex);

      if (child != null) {
        try {
          if (hasSignificantActions(child)) return false;
          if (ScreenUtilities.getText(child) != null) return true;
          return hasInnerText(child);
        } finally {
          child.recycle();
          child = null;
        }
      }
    }

    return false;
  }

  private static String makeText (AccessibilityNodeInfo node) {
    StringBuilder sb = new StringBuilder();
    boolean allowZeroLength = false;
    boolean includeDescription = false;

    if (node.isCheckable()) {
      includeDescription = true;
      if (sb.length() > 0) sb.append(' ');

      if (ScreenUtilities.isSwitch(node)) {
        sb.append(Characters.SWITCH_BEGIN);
        sb.append(node.isChecked()? Characters.SWITCH_ON: Characters.SWITCH_OFF);
        sb.append(Characters.SWITCH_END);
      } else if (ScreenUtilities.isRadioButton(node)) {
        sb.append(Characters.RADIO_BEGIN);
        sb.append(node.isChecked()? Characters.RADIO_MARK: ' ');
        sb.append(Characters.RADIO_END);
      } else {
        sb.append(Characters.CHECKBOX_BEGIN);
        sb.append(node.isChecked()? Characters.CHECKBOX_MARK: ' ');
        sb.append(Characters.CHECKBOX_END);
      }
    }

    {
      String text = ScreenUtilities.getText(node);

      if (text != null) {
        allowZeroLength = true;

        if (text.length() > 0) {
          if (sb.length() > 0) sb.append(' ');
          sb.append(text);
        }
      }
    }

    if (includeDescription) {
      String description = ScreenUtilities.getDescription(node);

      if (description != null) {
        if (sb.length() > 0) sb.append(' ');
        sb.append('[');
        sb.append(description);
        sb.append(']');
      }
    }

    if (ApplicationUtilities.haveKitkat) {
      AccessibilityNodeInfo.RangeInfo range = node.getRangeInfo();

      if (range != null) {
        if (sb.length() == 0) {
          CharSequence description = ScreenUtilities.getDescription(node);
          if (description != null) sb.append(description);
        }

        if (sb.length() > 0) sb.append(' ');
        String format = ScreenUtilities.getRangeValueFormat(range);

        sb.append("@");
        sb.append(String.format(format, range.getCurrent()));
        sb.append(" (");
        sb.append(String.format(format, range.getMin()));
        sb.append(" - ");
        sb.append(String.format(format, range.getMax()));
        sb.append(")");
      }
    }

    if (!(allowZeroLength || (sb.length() > 0))) return null;
    return sb.toString();
  }

  private static String getDescription (AccessibilityNodeInfo node) {
    {
      String description = ScreenUtilities.getDescription(node);
      if (description != null) return description;
    }

    {
      StringBuilder sb = new StringBuilder();
      int childCount = node.getChildCount();

      for (int childIndex=0; childIndex<childCount; childIndex+=1) {
        AccessibilityNodeInfo child = node.getChild(childIndex);

        if (child != null) {
          if (!hasSignificantActions(child)) {
            String description = ScreenUtilities.getDescription(child);

            if (description != null) {
              if (sb.length() > 0) sb.append(' ');
              sb.append(description);
            }
          }

          child.recycle();
          child = null;
        }
      }

      if (sb.length() > 0) return sb.toString();
    }

    return null;
  }

  private final int addScreenElements (AccessibilityNodeInfo root) {
    int propagatedActions = SIGNIFICANT_NODE_ACTIONS;

    if (root != null) {
      int actions = getSignificantActions(root);
      int childCount = root.getChildCount();

      if (childCount > 0) {
        propagatedActions = 0;

        for (int childIndex=0; childIndex<childCount; childIndex+=1) {
          AccessibilityNodeInfo child = root.getChild(childIndex);

          if (child != null) {
            propagatedActions |= addScreenElements(child);

            child.recycle();
            child = null;
          }
        }
      }

      if (ScreenUtilities.isVisible(root)) {
        String text = makeText(root);

        if (text == null) {
          if ((actions != 0) && !hasInnerText(root)) {
            if ((text = getDescription(root)) == null) {
              text = ScreenUtilities.getClassName(root);
              if (text == null) text = "?";
              text = "(" + text + ")";
            }
          }
        }

        if (text != null) {
          if (!root.isEnabled()) text += " (disabled)";
          screenElements.add(text, root);
        }
      }

      propagatedActions &= ~actions;
    }

    return propagatedActions;
  }

  private final int findScreenWidth () {
    int width = 1;

    if (screenRows.isEmpty()) {
      screenRows.add("waiting for screen update");
    }

    for (String row : screenRows) {
      int length = row.length();
      if (length > width) width = length;
    }

    return width;
  }

  private final AccessibilityNodeInfo findCursorNode () {
    AccessibilityNodeInfo root = getRootNode();

    if (root != null) {
      if (ApplicationUtilities.haveJellyBean) {
        AccessibilityNodeInfo node = root.findFocus(AccessibilityNodeInfo.FOCUS_ACCESSIBILITY);

        if (node != null) {
          root.recycle();
          root = node;
          node = ScreenUtilities.findTextNode(root);

          if (node != null) {
            root.recycle();
            root = node;
            node = null;
          }

          return root;
        }
      }

      {
        AccessibilityNodeInfo node;

        if (ApplicationUtilities.haveJellyBean) {
          node = root.findFocus(AccessibilityNodeInfo.FOCUS_INPUT);
        } else if (ApplicationUtilities.haveIceCreamSandwich) {
          node = ScreenUtilities.findFocusedNode(root);
        } else {
          node = null;
        }

        if (!ApplicationUtilities.haveJellyBean) {
          if (node == null) {
            if (ApplicationUtilities.haveIceCreamSandwich) {
              if ((node = ScreenUtilities.findFocusableNode(root)) != null) {
                if (!node.performAction(AccessibilityNodeInfo.ACTION_FOCUS)) {
                  node.recycle();
                  node = null;
                }
              }
            }
          }
        }

        if (node != null) {
          root.recycle();
          root = node;
          node = ScreenUtilities.findSelectedNode(root);

          if (node != null) {
            root.recycle();
            root = node;
            node = null;
          }

          if ((node = ScreenUtilities.findTextNode(root)) == null) {
            node =  ScreenUtilities.findDescribedNode(root);
          }

          if (node != null) {
            root.recycle();
            root = node;
            node = null;
          }

          return root;
        }
      }
    }

    return root;
  }

  private static void log (String message) {
    Log.d(LOG_TAG, message);
  }

  private final void logRenderedElements () {
    log(("screen element count: " + screenElements.size()));
    int elementIndex = 0;

    for (ScreenElement element : screenElements) {
      log(
        String.format(
          "screen element %d: %s", elementIndex++, element.getElementText()
        )
      );
    }
  }

  private final void logRenderedRows () {
    log(("screen row count: " + screenRows.size()));
    log(("screen width: " + screenWidth));
    int rowIndex = 0;

    for (String row : screenRows) {
      log(
        String.format(
          "screen row %d: %s", rowIndex++, row.toString()
        )
      );
    }
  }

  public final void logRenderedScreen () {
    log("begin rendered screen");
    logRenderedElements();
    logRenderedRows();
    log("end rendered screen");
  }

  public RenderedScreen (AccessibilityNodeInfo node) {
    if (node != null) node = AccessibilityNodeInfo.obtain(node);

    eventNode = node;
    rootNode = ScreenUtilities.findRootNode(node);

    addScreenElements(rootNode);
    screenElements.finish();
    BrailleRenderer.getBrailleRenderer().renderScreenElements(screenElements, screenRows);

    screenWidth = findScreenWidth();
    cursorNode = findCursorNode();

    if (ApplicationSettings.LOG_RENDERED_SCREEN) {
      logRenderedScreen();
    }
  }

  private abstract static class NextElementGetter {
    public abstract ScreenElement getNextElement (ScreenElement from);
  }

  private final static NextElementGetter forwardElementGetter =
    new NextElementGetter() {
      @Override
      public final ScreenElement getNextElement (ScreenElement from) {
        return from.getForwardElement();
      }
    };

  private final static NextElementGetter backwardElementGetter =
    new NextElementGetter() {
      @Override
      public final ScreenElement getNextElement (ScreenElement from) {
        return from.getBackwardElement();
      }
    };

  private abstract class NextElementFinder extends NextElementGetter {
    protected abstract ScreenElement getNearestElement (ScreenElement from);
    protected abstract void setNearestElement (ScreenElement from, ScreenElement to);

    protected abstract int getLesserEdge (Rect location);
    protected abstract int getGreaterEdge (Rect location);

    protected abstract int getLesserSide (Rect location);
    protected abstract int getGreaterSide (Rect location);

    public final ScreenElement getNextElement (ScreenElement from, boolean further) {
      ScreenElement element = getNearestElement(from);
      if (element != null) return element;

      Rect fromLocation = from.getVisualLocation();
      if (fromLocation == null) return null;

      double bestDistance = -1d;
      int fromEdge = further? getGreaterEdge(fromLocation):
                              getLesserEdge(fromLocation);

      for (ScreenElement to : screenElements) {
        if (to == from) continue;

        Rect toLocation = to.getVisualLocation();
        if (toLocation == null) continue;

        double distance = further? (getLesserEdge(toLocation) - fromEdge):
                                   (fromEdge - getGreaterEdge(toLocation));

        if (distance < 0d) continue;
        double offset = getLesserSide(fromLocation) - getGreaterSide(toLocation);
        if (offset < 0d) offset = getLesserSide(toLocation) - getGreaterSide(fromLocation);
        if (offset > distance) continue;
        if (offset > 0d) distance = Math.hypot(distance, offset);

        if (element != null) {
          if (distance > bestDistance) {
            continue;
          }
        }

        bestDistance = distance;
        element = to;
      }

      if (element != null) setNearestElement(from, element);
      return element;
    }
  }

  private abstract class VerticalElementFinder extends NextElementFinder {
    @Override
    protected final int getLesserEdge (Rect location) {
      return location.top;
    }

    @Override
    protected final int getGreaterEdge (Rect location) {
      return location.bottom;
    }

    @Override
    protected final int getLesserSide (Rect location) {
      return location.left;
    }

    @Override
    protected final int getGreaterSide (Rect location) {
      return location.right;
    }
  }

  private final NextElementGetter upwardElementFinder =
    new VerticalElementFinder() {
      @Override
      protected final ScreenElement getNearestElement (ScreenElement from) {
        return from.getUpwardElement();
      }

      @Override
      protected final void setNearestElement (ScreenElement from, ScreenElement to) {
        from.setUpwardElement(to);
      }

      @Override
      public final ScreenElement getNextElement (ScreenElement from) {
        return getNextElement(from, false);
      }
    };

  private final NextElementGetter downwardElementFinder =
    new VerticalElementFinder() {
      @Override
      protected final ScreenElement getNearestElement (ScreenElement from) {
        return from.getDownwardElement();
      }

      @Override
      protected final void setNearestElement (ScreenElement from, ScreenElement to) {
        from.setDownwardElement(to);
      }

      @Override
      public final ScreenElement getNextElement (ScreenElement from) {
        return getNextElement(from, true);
      }
    };

  private abstract class HorizontalElementFinder extends NextElementFinder {
    @Override
    protected final int getLesserEdge (Rect location) {
      return location.left;
    }

    @Override
    protected final int getGreaterEdge (Rect location) {
      return location.right;
    }

    @Override
    protected final int getLesserSide (Rect location) {
      return location.top;
    }

    @Override
    protected final int getGreaterSide (Rect location) {
      return location.bottom;
    }
  }

  private final NextElementGetter leftwardElementFinder =
    new HorizontalElementFinder() {
      @Override
      protected final ScreenElement getNearestElement (ScreenElement from) {
        return from.getLeftwardElement();
      }

      @Override
      protected final void setNearestElement (ScreenElement from, ScreenElement to) {
        from.setLeftwardElement(to);
      }

      @Override
      public final ScreenElement getNextElement (ScreenElement from) {
        return getNextElement(from, false);
      }
    };

  private final NextElementGetter rightwardElementFinder =
    new HorizontalElementFinder() {
      @Override
      protected final ScreenElement getNearestElement (ScreenElement from) {
        return from.getRightwardElement();
      }

      @Override
      protected final void setNearestElement (ScreenElement from, ScreenElement to) {
        from.setRightwardElement(to);
      }

      @Override
      public final ScreenElement getNextElement (ScreenElement from) {
        return getNextElement(from, true);
      }
    };

  private final boolean moveFocus (
    ScreenElement element, NextElementGetter nextElementGetter, boolean inclusive
  ) {
    ScreenElement end = element;

    while (true) {
      if (inclusive) {
        inclusive = false;
      } else if ((element = nextElementGetter.getNextElement(element)) == end) {
        return false;
      } else if (element == null) {
        return false;
      }

      if (element.bringCursor()) return true;
    }
  }

  public enum SearchDirection {
    FIRST, LAST,
    FORWARD, BACKWARD,
    UP, DOWN,
    LEFT, RIGHT,
  }

  public final boolean moveFocus (SearchDirection direction) {
    AccessibilityNodeInfo node = getCursorNode();
    if (node == null) return false;

    try {
      ScreenElement element = getScreenElement(node);
      if (element == null) return false;

      switch (direction) {
        case FIRST: {
          ScreenElement first = screenElements.getFirstElement();
          if (element == first) return false;
          return moveFocus(first, forwardElementGetter, true);
        }

        case LAST: {
          ScreenElement last = screenElements.getLastElement();
          if (element == last) return false;
          return moveFocus(last, backwardElementGetter, true);
        }

        case FORWARD:
          return moveFocus(element, forwardElementGetter, false);

        case BACKWARD:
          return moveFocus(element, backwardElementGetter, false);

        case UP:
          return moveFocus(element, upwardElementFinder, false);

        case DOWN:
          return moveFocus(element, downwardElementFinder, false);

        case LEFT:
          return moveFocus(element, leftwardElementFinder, false);

        case RIGHT:
          return moveFocus(element, rightwardElementFinder, false);

        default:
          return false;
      }
    } finally {
      node.recycle();
      node = null;
    }
  }
}
