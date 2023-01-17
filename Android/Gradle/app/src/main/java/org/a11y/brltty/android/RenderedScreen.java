/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2023 by The BRLTTY Developers.
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

import java.util.Locale;
import java.util.Set;
import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;

import android.util.Log;
import android.text.TextUtils;

import android.view.accessibility.AccessibilityNodeInfo;

import android.widget.ImageView;
import android.widget.LinearLayout;

import android.graphics.Rect;

public class RenderedScreen {
  private final static String LOG_TAG = RenderedScreen.class.getName();

  private final AccessibilityNodeInfo eventNode;
  private final AccessibilityNodeInfo rootNode;

  private final ScreenElementList screenElements = new ScreenElementList();
  private final List<String> screenRows = new ArrayList<>();

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

  public final ScreenElement findScreenElement (AccessibilityNodeInfo node) {
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
          element = findScreenElement(child);

          child.recycle();
          child = null;

          if (element != null) return element;
        }
      }
    }

    return null;
  }

  public final ScreenElement findScreenElement (int column, int row) {
    return screenElements.findByBrailleLocation(column, row);
  }

  public final boolean performAction (int column, int row) {
    ScreenElement element = findScreenElement(column, row);
    if (element == null) return false;

    Rect location = element.getBrailleLocation();
    return element.performAction((column - location.left), (row - location.top));
  }

  private final static NodeTester[] significantNodeFilters = new NodeTester[] {
    new NodeTester() {
      @Override
      public boolean testNode (AccessibilityNodeInfo node) {
        if (!TextUtils.equals(node.getPackageName(), "com.samsung.android.messaging")) return false;
        if (!ScreenUtilities.isSubclassOf(node, LinearLayout.class)) return false;
        if (node.getChildCount() != 1) return false;
        return true;
      }
    }
  };

  public static interface NodeActionVerifier {
    public boolean verify (AccessibilityNodeInfo node);
  }

  private final static Map<Integer, NodeActionVerifier> nodeActionVerifiers =
               new HashMap<Integer, NodeActionVerifier>()
  {
    {
      put(
        AccessibilityNodeInfo.ACTION_CLICK,
        new NodeActionVerifier() {
          @Override
          public boolean verify (AccessibilityNodeInfo node) {
            return node.isClickable();
          }
        }
      );

      put(
        AccessibilityNodeInfo.ACTION_LONG_CLICK,
        new NodeActionVerifier() {
          @Override
          public boolean verify (AccessibilityNodeInfo node) {
            return node.isLongClickable();
          }
        }
      );

      put(
        AccessibilityNodeInfo.ACTION_SCROLL_FORWARD,
        new NodeActionVerifier() {
          @Override
          public boolean verify (AccessibilityNodeInfo node) {
            return node.isScrollable();
          }
        }
      );

      put(
        AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD,
        new NodeActionVerifier() {
          @Override
          public boolean verify (AccessibilityNodeInfo node) {
            return node.isScrollable();
          }
        }
      );
    }
  };

  public static Set<Integer> getVerifiableNodeActions () {
    return nodeActionVerifiers.keySet();
  }

  private final static int SIGNIFICANT_NODE_ACTIONS;
  static {
    int actions = 0;

    for (Integer action : getVerifiableNodeActions()) {
      actions |= action;
    }

    SIGNIFICANT_NODE_ACTIONS = actions;
  }

  public static int getSignificantActions (AccessibilityNodeInfo node) {
    int actions = node.getActions() & SIGNIFICANT_NODE_ACTIONS;

    if (actions != 0) {
      for (Integer action : getVerifiableNodeActions()) {
        if ((actions & action) != 0) {
          if (!nodeActionVerifiers.get(action).verify(node)) {
            if ((actions &= ~action) == 0) {
              break;
            }
          }
        }
      }
    }

    return actions;
  }

  public static boolean hasSignificantAction (AccessibilityNodeInfo node) {
    return getSignificantActions(node) != 0;
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
      } else if (ScreenUtilities.isSubclassOf(node, ImageView.class)) {
        includeDescription = true;
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

    if (APITests.haveKitkat) {
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

    if (!(allowZeroLength || (sb.length() > 0))) {
      for (NodeTester filter : significantNodeFilters) {
        if (filter.testNode(node)) {
          String description = ScreenUtilities.getDescription(node);
          if (description != null) sb.append(description);
          break;
        }
      }

      if (sb.length() == 0) return null;
    }

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
          if (!hasSignificantAction(child)) {
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
    int propagatedActions = 0;

    if (root != null) {
      {
        int childCount = root.getChildCount();

        for (int childIndex=0; childIndex<childCount; childIndex+=1) {
          AccessibilityNodeInfo child = root.getChild(childIndex);

          if (child != null) {
            try {
              propagatedActions |= addScreenElements(child);
            } finally {
              child.recycle();
              child = null;
            }
          }
        }
      }

      int actions = getSignificantActions(root);
      boolean hasActions = (actions & ~propagatedActions) != 0;
      propagatedActions &= ~actions;

      if (ScreenUtilities.isVisible(root)) {
        String text = makeText(root);
        boolean hasText = text != null;
        boolean isSkippable = false;

        {
          String label = ChromeRole.getLabel(root);

          if (label != null) {
            if (label.isEmpty()) {
              isSkippable = true;
            } else {
              label = String.format("(%s)", label);

              if (text != null) {
                label += ' ';
                label += text;
              }

              text = label;
            }
          }
        }

        if (hasActions && !hasText) {
          String description = getDescription(root);

          if (!isSkippable) {
            if ((description == null) && (text == null)) {
              description = ScreenUtilities.getClassName(root);

              if (APITests.haveJellyBeanMR2) {
                String name = root.getViewIdResourceName();

                if (name != null) {
                  String marker = ":id/";
                  int index = name.indexOf(marker);
                  name = (index < 0)? null: name.substring(index + marker.length());
                }

                if (description == null) {
                  description = name;
                } else if (name != null) {
                  description += " " + name;
                }
              }

              if (description == null) description = "?";
              description = "(" + description + ")";
            }

            if (description != null) {
              if (text == null) {
                text = description;
              } else {
                text += ' ';
                text += description;
              }
            }
          }
        }

        if (text != null) {
          propagatedActions |= SIGNIFICANT_NODE_ACTIONS & ~actions;
          if (!root.isEnabled()) text += " (disabled)";
          screenElements.add(text, root);
        }
      }
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
      if (APITests.haveJellyBean) {
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

        if (APITests.haveJellyBean) {
          node = root.findFocus(AccessibilityNodeInfo.FOCUS_INPUT);
        } else if (APITests.haveIceCreamSandwich) {
          node = ScreenUtilities.findFocusedNode(root);
        } else {
          node = null;
        }

        if (!APITests.haveJellyBean) {
          if (node == null) {
            if (APITests.haveIceCreamSandwich) {
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
        String.format(Locale.ROOT,
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
        String.format(Locale.ROOT,
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
    protected abstract ScreenElement getCachedElement (ScreenElement from);
    protected abstract void setCachedElement (ScreenElement from, ScreenElement to);

    protected abstract boolean goForward ();
    protected abstract int getLesserEdge (Rect location);
    protected abstract int getGreaterEdge (Rect location);
    protected abstract int getLesserSide (Rect location);
    protected abstract int getGreaterSide (Rect location);

    private final ScreenElement findNextElement (ScreenElement from) {
      final Rect fromLocation = from.getVisualLocation();
      if (fromLocation == null) return null;

      final boolean forward = goForward();
      final int fromEdge = forward? getGreaterEdge(fromLocation):
                                    getLesserEdge(fromLocation);

      double bestAngle = Double.MAX_VALUE;
      double bestDistance = Double.MAX_VALUE;
      ScreenElement bestElement = null;

      for (ScreenElement to : screenElements) {
        if (to == from) continue;

        Rect toLocation = to.getVisualLocation();
        if (toLocation == null) continue;

        double edgeDistance = forward? (getLesserEdge(toLocation) - fromEdge):
                                       (fromEdge - getGreaterEdge(toLocation));

        if (edgeDistance < 0d) continue;
        double sideDistance = getLesserSide(fromLocation) - getGreaterSide(toLocation);
        if (sideDistance < 0d) sideDistance = getLesserSide(toLocation) - getGreaterSide(fromLocation);
        if (sideDistance < 0d) sideDistance = 0d;

        double angle = Math.atan2(sideDistance, edgeDistance);
        double distance = Math.hypot(sideDistance, edgeDistance);

        if (bestElement != null) {
          if (angle > bestAngle) continue;

          if (angle == bestAngle) {
            if (distance >= bestDistance) {
              continue;
            }
          }
        }

        bestAngle = angle;
        bestDistance = distance;
        bestElement = to;
      }

      return bestElement;
    }

    @Override
    public final ScreenElement getNextElement (ScreenElement from) {
      ScreenElement to = getCachedElement(from);

      if (to == from) {
        to = findNextElement(from);
        setCachedElement(from, to);
      }

      return to;
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
      protected final ScreenElement getCachedElement (ScreenElement from) {
        return from.getUpwardElement();
      }

      @Override
      protected final void setCachedElement (ScreenElement from, ScreenElement to) {
        from.setUpwardElement(to);
      }

      @Override
      protected final boolean goForward () {
        return false;
      }
    };

  private final NextElementGetter downwardElementFinder =
    new VerticalElementFinder() {
      @Override
      protected final ScreenElement getCachedElement (ScreenElement from) {
        return from.getDownwardElement();
      }

      @Override
      protected final void setCachedElement (ScreenElement from, ScreenElement to) {
        from.setDownwardElement(to);
      }

      @Override
      protected final boolean goForward () {
        return true;
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
      protected final ScreenElement getCachedElement (ScreenElement from) {
        return from.getLeftwardElement();
      }

      @Override
      protected final void setCachedElement (ScreenElement from, ScreenElement to) {
        from.setLeftwardElement(to);
      }

      @Override
      protected final boolean goForward () {
        return false;
      }
    };

  private final NextElementGetter rightwardElementFinder =
    new HorizontalElementFinder() {
      @Override
      protected final ScreenElement getCachedElement (ScreenElement from) {
        return from.getRightwardElement();
      }

      @Override
      protected final void setCachedElement (ScreenElement from, ScreenElement to) {
        from.setRightwardElement(to);
      }

      @Override
      protected final boolean goForward () {
        return true;
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
