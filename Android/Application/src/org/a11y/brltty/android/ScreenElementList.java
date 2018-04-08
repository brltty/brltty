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

import java.util.Arrays;
import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;

import android.util.Log;

import android.view.accessibility.AccessibilityNodeInfo;

import android.graphics.Rect;

public class ScreenElementList extends ArrayList<ScreenElement> {
  private static final String LOG_TAG = ScreenElementList.class.getName();

  public ScreenElementList () {
    super();
  }

  private final Map<AccessibilityNodeInfo, ScreenElement> nodeToScreenElement =
        new HashMap<AccessibilityNodeInfo, ScreenElement>();

  public final void add (String text, AccessibilityNodeInfo node) {
    node = AccessibilityNodeInfo.obtain(node);
    ScreenElement element = new RealScreenElement(text, node);
    add(element);
    nodeToScreenElement.put(node, element);
  }

  public final ScreenElement find (AccessibilityNodeInfo node) {
    return nodeToScreenElement.get(node);
  }

  private int atTopCount = 0;

  public final void addAtTop (int text, int action, int key) {
    add(atTopCount++, new VirtualScreenElement(text, action, key));
  }

  public final void addAtTop (int text, int action) {
    add(atTopCount++, new VirtualScreenElement(text, action));
  }

  public final void addAtBottom (int text, int action, int key) {
    add(new VirtualScreenElement(text, action, key));
  }

  public final void addAtBottom (int text, int action) {
    add(new VirtualScreenElement(text, action));
  }

  public void logElements () {
    Log.d(LOG_TAG, "begin screen element list");

    for (ScreenElement element : this) {
      Log.d(LOG_TAG, "screen element: " + element.getElementText());
    }

    Log.d(LOG_TAG, "end screen element list");
  }

  private static boolean isContainer (Rect outer, int left, int top, int right, int bottom) {
    return (left >= outer.left) &&
           (right <= outer.right) &&
           (top >= outer.top) &&
           (bottom <= outer.bottom);
  }

  private static boolean isContainer (Rect outer, Rect inner) {
    return isContainer(outer, inner.left, inner.top, inner.right, inner.bottom);
  }

  public static boolean isContainer (AccessibilityNodeInfo outer, AccessibilityNodeInfo inner) {
    if ((outer == null) || (inner == null)) return false;

    boolean found = true;
    inner = AccessibilityNodeInfo.obtain(inner);

    while (inner != outer) {
      AccessibilityNodeInfo parent = inner.getParent();

      if (parent == null) {
        found = false;
        break;
      }

      inner.recycle();
      inner = parent;
    }

    inner.recycle();
    return found;
  }

  public static boolean isContainer (ScreenElement outer, ScreenElement inner) {
    return isContainer(outer.getAccessibilityNode(), inner.getAccessibilityNode());
  }

  public final ScreenElement find (
    ScreenElement.LocationGetter locationGetter,
    int left, int top, int right, int bottom
  ) {
    ScreenElement bestElement = null;
    Rect bestLocation = null;

    for (ScreenElement element : this) {
      Rect location = locationGetter.getLocation(element);

      if (location != null) {
        if (isContainer(location, left, top, right, bottom)) {
          if ((bestLocation == null) ||
              isContainer(bestLocation, location)) {
            bestLocation = location;
            bestElement = element;
          }
        }
      }
    }

    return bestElement;
  }

  public final ScreenElement findByVisualLocation (int left, int top, int right, int bottom) {
    return find(ScreenElement.visualLocationGetter, left, top, right, bottom);
  }

  public final ScreenElement findByVisualLocation (Rect location) {
    return findByVisualLocation(location.left, location.top, location.right, location.bottom);
  }

  public final ScreenElement findByBrailleLocation (int left, int top, int right, int bottom) {
    return find(ScreenElement.brailleLocationGetter, left, top, right, bottom);
  }

  public final ScreenElement findByBrailleLocation (int column, int row) {
    do {
      ScreenElement element = findByBrailleLocation(column, row, column, row);
      if (element != null) return element;
    } while (--column >= 0);

    return null;
  }

  private final void addByContainer (NodeComparator nodeComparator, AccessibilityNodeInfo... nodes) {
    {
      final int count = nodes.length;
      if (count == 0) return;
      if (count > 1) Arrays.sort(nodes, nodeComparator);
    }

    for (AccessibilityNodeInfo node : nodes) {
      if (node == null) break;

      try {
        {
          ScreenElement element = find(node);
          if (element != null) add(element);
        }

        {
          int count = node.getChildCount();

          if (count > 0) {
            AccessibilityNodeInfo[] children = new AccessibilityNodeInfo[count];

            for (int index=0; index<count; index+=1) {
              children[index] = node.getChild(index);
            }

            addByContainer(nodeComparator, children);
          }
        }
      } finally {
        node.recycle();
        node = null;
      }
    }
  }

  public final void sortByVisualLocation () {
    if (size() < 2) return;
    AccessibilityNodeInfo node = get(0).getAccessibilityNode();

    try {
      AccessibilityNodeInfo root = ScreenUtilities.findRootNode(node);

      if (root != null) {
        NodeComparator comparator = new NodeComparator() {
          private final Map<AccessibilityNodeInfo, Rect> nodeLocations =
                new HashMap<AccessibilityNodeInfo, Rect>();

          private final Rect getLocation (AccessibilityNodeInfo node) {
            synchronized (nodeLocations) {
              Rect location = nodeLocations.get(node);
              if (location != null) return location;

              location = new Rect();
              node.getBoundsInScreen(location);

              nodeLocations.put(node, location);
              return location;
            }
          }

          @Override
          public int compare (AccessibilityNodeInfo node1, AccessibilityNodeInfo node2) {
            if (node1 == null) {
              return (node2 == null)? 0: 1;
            } else if (node2 == null) {
              return -1;
            }

            Rect location1 = getLocation(node1);
            Rect location2 = getLocation(node2);

            return (location1.top < location2.top)? -1:
                   (location1.top > location2.top)? 1:
                   (location1.left < location2.left)? -1:
                   (location1.left > location2.left)? 1:
                   (location1.right > location2.right)? -1:
                   (location1.right < location2.right)? 1:
                   (location1.bottom > location2.bottom)? -1:
                   (location1.bottom < location2.bottom)? 1:
                   0;
          }
        };

        clear();
        addByContainer(comparator, root);
      }
    } finally {
      node.recycle();
      node = null;
    }
  }
}
