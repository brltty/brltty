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
    return rootNode;
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
        if ((element = findRenderedScreenElement(node.getChild(childIndex))) != null) {
          return element;
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

  private final void addScreenElements (AccessibilityNodeInfo root) {
    if (root != null) {
      boolean isVisible;

      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
        isVisible = root.isVisibleToUser();
      } else {
        isVisible = true;
      }

      if (isVisible) {
        String text = ScreenUtilities.getNodeText(root);

        if (text != null) {
          screenElements.add(text, root);
        }
      }

      {
        int childCount = root.getChildCount();

        for (int childIndex=0; childIndex<childCount; childIndex+=1) {
          addScreenElements(root.getChild(childIndex));
        }
      }
    }
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
    eventNode = node;
    rootNode = ScreenUtilities.findRootNode(node);

    addScreenElements(rootNode);
    BrailleRenderer.getBrailleRenderer().renderScreenElements(screenRows, screenElements);
    finishScreenRows();
  }
}
