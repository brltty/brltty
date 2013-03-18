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

import android.view.accessibility.AccessibilityNodeInfo;

import android.graphics.Rect;

public class ScreenElementList extends ArrayList<ScreenElement> {
  private int atTopCount = 0;

  public static boolean isContainer (Rect outer, int left, int top, int right, int bottom) {
    return (left >= outer.left) &&
           (right <= outer.right) &&
           (top >= outer.top) &&
           (bottom <= outer.bottom);
  }

  public static boolean isContainer (Rect outer, int x, int y) {
    return isContainer(outer, x, y, x, y);
  }

  public static boolean isContainer (Rect outer, Rect inner) {
    return isContainer(outer, inner.left, inner.top, inner.right, inner.bottom);
  }

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
      List<ScreenElement> containedElements = new ScreenElementList();

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

  public final ScreenElement find (AccessibilityNodeInfo node) {
    for (ScreenElement element : this) {
      if (element.getAccessibilityNode() == node) {
        return element;
      }
    }

    return null;
  }

  public final ScreenElement find (
    ScreenElement.LocationGetter locationGetter,
    int column, int row
  ) {
    ScreenElement bestElement = null;
    Rect bestLocation = null;

    for (ScreenElement element : this) {
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

  public final ScreenElement findByScreenLocation (int column, int row) {
    return find(ScreenElement.screenLocationGetter, column, row);
  }

  public final ScreenElement findByBrailleLocation (int column, int row) {
    return find(ScreenElement.brailleLocationGetter, column, row);
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

  public ScreenElementList () {
    super();
  }
}
