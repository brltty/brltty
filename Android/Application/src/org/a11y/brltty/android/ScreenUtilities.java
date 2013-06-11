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

import android.view.accessibility.AccessibilityNodeInfo;

public final class ScreenUtilities {
  public static AccessibilityNodeInfo getRefreshedNode (AccessibilityNodeInfo node) {
    if (node != null) {
      {
        int childCount = node.getChildCount();

        for (int childIndex=0; childIndex<childCount; childIndex+=1) {
          AccessibilityNodeInfo child = node.getChild(childIndex);

          if (child != null) {
            AccessibilityNodeInfo parent = child.getParent();

            child.recycle();
            child = null;

            if (node.equals(parent)) {
              return parent;
            }

            if (parent != null) {
              parent.recycle();
              parent = null;
            }
          }
        }
      }

      {
        AccessibilityNodeInfo parent = node.getParent();

        if (parent != null) {
          int childCount = parent.getChildCount();

          for (int childIndex=0; childIndex<childCount; childIndex+=1) {
            AccessibilityNodeInfo child = parent.getChild(childIndex);

            if (node.equals(child)) {
              parent.recycle();
              parent = null;
              return child;
            }

            if (child != null) {
              child.recycle();
              child = null;
            }
          }

          parent.recycle();
          parent = null;
        }
      }
    }

    return null;
  }

  public static AccessibilityNodeInfo findRootNode (AccessibilityNodeInfo node) {
    if (node != null) {
      AccessibilityNodeInfo child = AccessibilityNodeInfo.obtain(node);

      while (true) {
        AccessibilityNodeInfo parent = child.getParent();
        if (parent == null) break;

        child.recycle();
        child = parent;
      }

      node = child;
    }

    return node;
  }

  public static AccessibilityNodeInfo findFocusedNode (AccessibilityNodeInfo root) {
    if (root != null) {
      if (root.isFocused()) return AccessibilityNodeInfo.obtain(root);

      {
        int childCount = root.getChildCount();

        for (int childIndex=0; childIndex<childCount; childIndex+=1) {
          AccessibilityNodeInfo child = root.getChild(childIndex);

          if (child != null) {
            AccessibilityNodeInfo node = findFocusedNode(child);
            child.recycle();
            child = null;
            if (node != null) return node;
          }
        }
      }
    }

    return null;
  }

  public static String normalizeText (CharSequence text) {
    if (text != null) {
      String string = text.toString().trim();

      if (string.length() > 0) {
        return string;
      }
    }

    return null;
  }

  public static String getNodeText (AccessibilityNodeInfo node) {
    String text;

    if ((text = normalizeText(node.getContentDescription())) != null) {
      return text;
    }

    if ((text = normalizeText(node.getText())) != null) {
      return text;
    }

    if (node.getActions() != 0)  {
      return "";
    }

    return null;
  }

  private ScreenUtilities () {
  }
}
