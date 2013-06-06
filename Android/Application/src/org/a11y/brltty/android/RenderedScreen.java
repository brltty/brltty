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

import android.os.Build;

import android.view.accessibility.AccessibilityNodeInfo;

import android.graphics.Rect;

public class RenderedScreen {
  private final AccessibilityNodeInfo eventNode;
  private final AccessibilityNodeInfo rootNode;

  private final ScreenElementList screenElements = new ScreenElementList();
  private final List<CharSequence> screenRows = new ArrayList<CharSequence>();

  private int screenWidth = 1;

  public final AccessibilityNodeInfo getRootNode () {
    return AccessibilityNodeInfo.obtain(rootNode);
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

  public final boolean performAction (int column, int row) {
    ScreenElement element = screenElements.findByBrailleLocation(column, row);

    if (element != null) {
      if (element.performAction(column - element.getBrailleLocation().left)) {
        return true;
      }
    }

    return false;
  }

  private final boolean addScreenElements (AccessibilityNodeInfo root) {
    boolean hasLabel = false;

    if (root != null) {
      {
        int childCount = root.getChildCount();

        for (int childIndex=0; childIndex<childCount; childIndex+=1) {
          AccessibilityNodeInfo child = root.getChild(childIndex);

          if (child != null) {
            if (addScreenElements(child)) {
              hasLabel = true;
            }
          }

          child.recycle();
          child = null;
        }
      }

      boolean isVisible;
      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
        isVisible = root.isVisibleToUser();
      } else {
        isVisible = true;
      }

      if (isVisible) {
        String text = ScreenUtilities.normalizeText(root.getText());
        String description = ScreenUtilities.normalizeText(root.getContentDescription());

        int actions = root.getActions() & (
          AccessibilityNodeInfo.ACTION_CLICK |
          AccessibilityNodeInfo.ACTION_LONG_CLICK |
          AccessibilityNodeInfo.ACTION_SCROLL_FORWARD |
          AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD
        );

        if (text != null) {
          if (description != null) text = description;
          if (actions == 0) hasLabel = true;
        } else if (actions != 0) {
          if (hasLabel) {
            hasLabel = false;
          } else if ((text = description) == null) {
            text = root.getClassName().toString();
            int index = text.lastIndexOf('.');
            if (index >= 0) text = text.substring(index+1);
            text = "(" + text + ")";
          }
        }

        if (text == null) text = "";
        screenElements.add(text, root);
      }
    }

    return hasLabel;
  }

  private final void finishScreenRows () {
    if (screenRows.isEmpty()) {
      screenRows.add("empty screen");
    }

    for (CharSequence row : screenRows) {
      int length = row.length();

      if (length > screenWidth) {
        screenWidth = length;
      }
    }
  }

  public RenderedScreen (AccessibilityNodeInfo node) {
    if (node != null) node = AccessibilityNodeInfo.obtain(node);

    eventNode = node;
    rootNode = ScreenUtilities.findRootNode(node);

    addScreenElements(rootNode);
    BrailleRenderer.getBrailleRenderer().renderScreenElements(screenRows, screenElements);
    finishScreenRows();
  }
}
