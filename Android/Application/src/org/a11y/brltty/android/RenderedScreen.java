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

  public final ScreenElement findScreenElement (AccessibilityNodeInfo node) {
    if (node == null) return null;
    Rect location = new Rect();
    node.getBoundsInScreen(location);
    return screenElements.findByVisualLocation(location);
  }

  public final ScreenElement findRenderedScreenElement (AccessibilityNodeInfo node) {
    ScreenElement element = findScreenElement(node);

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

  public enum SearchDirection {
    FORWARD,
    BACKWARD,

    FIRST,
    LAST,
  }

  public final boolean moveFocus (SearchDirection direction) {
    AccessibilityNodeInfo node = getCursorNode();

    if (node != null) {
      ScreenElement element = findScreenElement(node);

      node.recycle();
      node = null;

      if (element != null) {
        int count = screenElements.size();
        int index = screenElements.indexOf(element);

        switch (direction) {
          case FORWARD: {
            int start = index;

            while (true) {
              if (++index == count) index = 0;
              if (index == start) break;
              if (screenElements.get(index).bringCursor()) return true;
            }

            break;
          }

          case BACKWARD: {
            int start = index;

            while (true) {
              if (--index < 0) index += count;
              if (index == start) break;
              if (screenElements.get(index).bringCursor()) return true;
            }

            break;
          }

          case FIRST: {
            int first = 0;

            while (first < index) {
              if (screenElements.get(first).bringCursor()) return true;
              first += 1;
            }

            break;
          }

          case LAST: {
            int last = count - 1;

            while (last > index) {
              if (screenElements.get(last).bringCursor()) return true;
              last -= 1;
            }

            break;
          }
        }
      }
    }

    return false;
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
        boolean found;

        if (hasSignificantActions(child)) {
          found = false;
        } else if (child.getText() != null) {
          found = true;
        } else {
          found = hasInnerText(child);
        }

        child.recycle();
        child = null;

        if (found) return true;
      }
    }

    return false;
  }

  private static String makeText (AccessibilityNodeInfo node) {
    StringBuilder sb = new StringBuilder();
    boolean hasText = false;

    if (node.isCheckable()) {
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
        hasText = true;

        if (text.length() > 0) {
          if (sb.length() > 0) sb.append(' ');
          sb.append(text);
        }
      }
    }

    if (!hasText) {
      if (sb.length() > 0) {
        String description = ScreenUtilities.getDescription(node);

        if (description != null) {
          sb.append(' ');
          sb.append(description);
        }
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

    if (!(hasText || (sb.length() > 0))) return null;
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
              text = root.getClassName().toString();
              int index = text.lastIndexOf('.');
              if (index >= 0) text = text.substring(index+1);
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
    BrailleRenderer.getBrailleRenderer().renderScreenElements(screenElements, screenRows);

    screenWidth = findScreenWidth();
    cursorNode = findCursorNode();

    if (ApplicationSettings.LOG_RENDERED_SCREEN) {
      logRenderedScreen();
    }
  }
}
