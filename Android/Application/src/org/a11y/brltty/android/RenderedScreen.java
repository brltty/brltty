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

  private int screenWidth = 1;

  public final AccessibilityNodeInfo getRootNode () {
    if (rootNode == null) return null;
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
      Rect location = element.getBrailleLocation();

      if (element.performAction((column - location.left), (row - location.top))) {
        return true;
      }
    }

    return false;
  }

  private final int addScreenElements (AccessibilityNodeInfo root) {
    final int significantActions = AccessibilityNodeInfo.ACTION_CLICK
                                 | AccessibilityNodeInfo.ACTION_LONG_CLICK
                                 | AccessibilityNodeInfo.ACTION_SCROLL_FORWARD
                                 | AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD
                                 ;
    int propagatedActions = significantActions;

    if (root != null) {
      int actions = root.getActions() & significantActions;
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

      boolean isVisible;
      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
        isVisible = root.isVisibleToUser();
      } else {
        Rect location = new Rect();
        root.getBoundsInScreen(location);
        isVisible = ScreenDriver.getWindow().contains(location);
      }

      if (isVisible) {
        String text = ScreenUtilities.normalizeText(root.getText());
        String description = ScreenUtilities.normalizeText(root.getContentDescription());

        if (text != null) {
          if (description != null) text = description;
        } else if ((childCount == 0) || ((actions & propagatedActions) != actions)) {
          if ((text = description) == null) {
            text = root.getClassName().toString();
            int index = text.lastIndexOf('.');
            if (index >= 0) text = text.substring(index+1);
            text = "(" + text + ")";
          }
        }

        screenElements.add(text, root);
      }

      propagatedActions &= ~actions;
    }

    return propagatedActions;
  }

  private final void finishScreenRows () {
    if (screenRows.isEmpty()) {
      screenRows.add("waiting for screen update");
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
    ApplicationParameters.BRAILLE_RENDERER.renderScreenElements(screenRows, screenElements);
    finishScreenRows();
  }
}
