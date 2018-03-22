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

import android.os.Build;

import android.view.accessibility.AccessibilityNodeInfo;

import android.graphics.Rect;

public class RenderedScreen {
  private static final String LOG_TAG = RenderedScreen.class.getName();

  private final AccessibilityNodeInfo eventNode;
  private final AccessibilityNodeInfo rootNode;

  private final ScreenElementList screenElements = new ScreenElementList();
  private final List<CharSequence> screenRows = new ArrayList<CharSequence>();

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

  public final CharSequence getScreenRow (int index) {
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

  public enum ChangeFocusDirection {
    FORWARD,
    BACKWARD
  }

  public boolean changeFocus (ChangeFocusDirection direction) {
    AccessibilityNodeInfo node = getCursorNode();

    if (node != null) {
      ScreenElement element = findScreenElement(node);

      node.recycle();
      node = null;

      if (element != null) {
        int index = screenElements.indexOf(element);

        switch (direction) {
          case FORWARD: {
            int size = screenElements.size();

            while (++index < size) {
              if (screenElements.get(index).bringCursor()) return true;
            }

            break;
          }

          case BACKWARD: {
            while (--index >= 0) {
              if (screenElements.get(index).bringCursor()) return true;
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

    if (element != null) {
      Rect location = element.getBrailleLocation();

      if (element.performAction((column - location.left), (row - location.top))) {
        return true;
      }
    }

    return false;
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

  private static String getDescription (AccessibilityNodeInfo node) {
    {
      String description = ScreenUtilities.normalizeText(node.getContentDescription());
      if (description != null) return description;
    }

    {
      StringBuilder sb = new StringBuilder();
      int childCount = node.getChildCount();

      for (int childIndex=0; childIndex<childCount; childIndex+=1) {
        AccessibilityNodeInfo child = node.getChild(childIndex);

        if (child != null) {
          if (!hasSignificantActions(child)) {
            String description = ScreenUtilities.normalizeText(child.getContentDescription());

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
        String text = ScreenUtilities.getText(root);

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

        if (text != null) screenElements.add(text, root);
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

    for (CharSequence row : screenRows) {
      int length = row.length();
      if (length > width) width = length;
    }

    return width;
  }

  private final AccessibilityNodeInfo findCursorNode () {
    AccessibilityNodeInfo root = getRootNode();

    if (root != null) {
      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
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

        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          node = root.findFocus(AccessibilityNodeInfo.FOCUS_INPUT);
        } else if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.ICE_CREAM_SANDWICH)) {
          node = ScreenUtilities.findFocusedNode(root);
        } else {
          node = null;
        }

        if (!ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          if (node == null) {
            if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.ICE_CREAM_SANDWICH)) {
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

  private void logRenderedElements () {
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

  private void logRenderedRows () {
    log(("screen row count: " + screenRows.size()));
    log(("screen width: " + screenWidth));
    int rowIndex = 0;

    for (CharSequence row : screenRows) {
      log(
        String.format(
          "screen row %d: %s", rowIndex++, row.toString()
        )
      );
    }
  }

  public void logRenderedScreen () {
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
