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

import android.view.accessibility.AccessibilityNodeInfo;

import android.graphics.Rect;

public abstract class ScreenUtilities {
  private ScreenUtilities () {
  }

  public static boolean isVisible (AccessibilityNodeInfo node) {
    if (ApplicationUtilities.haveJellyBean) {
      return node.isVisibleToUser();
    }

    Rect location = new Rect();
    node.getBoundsInScreen(location);
    return ScreenDriver.getWindow().contains(location);
  }

  public static boolean isSubclassOf (AccessibilityNodeInfo node, Class type) {
    return LanguageUtilities.canAssign(type, node.getClassName().toString());
  }

  public static boolean isCheckBox (AccessibilityNodeInfo node) {
    return isSubclassOf(node, android.widget.CheckBox.class);
  }

  public static boolean isSwitch (AccessibilityNodeInfo node) {
    return isSubclassOf(node, android.widget.Switch.class);
  }

  private static boolean isSelectionMode (AccessibilityNodeInfo node, int mode) {
    if (ApplicationUtilities.haveLollipop) {
      if (node.getCollectionItemInfo() != null) {
        AccessibilityNodeInfo parent = node.getParent();

        if (parent != null) {
          try {
            AccessibilityNodeInfo.CollectionInfo collection = parent.getCollectionInfo();

            if (collection != null) {
              if (collection.getSelectionMode() == mode) {
                return true;
              }
            }
          } finally {
            parent.recycle();
            parent = null;
          }
        }
      }
    }

    return false;
  }

  public static boolean isRadioButton (AccessibilityNodeInfo node) {
    if (isSelectionMode(node, AccessibilityNodeInfo.CollectionInfo.SELECTION_MODE_SINGLE)) return true;
    return false;
  }

  public static boolean isEditable (AccessibilityNodeInfo node) {
    if (ApplicationUtilities.haveJellyBeanMR2) {
      return node.isEditable();
    } else {
      return isSubclassOf(node, android.widget.EditText.class);
    }
  }

  public static AccessibilityNodeInfo getRefreshedNode (AccessibilityNodeInfo node) {
    if (node != null) {
      if (ApplicationUtilities.haveJellyBeanMR2) {
        node = AccessibilityNodeInfo.obtain(node);

        if (!node.refresh()) {
          node.recycle();
          node = null;
        }

        return node;
      }

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

  public static AccessibilityNodeInfo findNode (AccessibilityNodeInfo root, ScreenNodeTester tester) {
    if (root != null) {
      if (tester.testNode(root)) return AccessibilityNodeInfo.obtain(root);

      {
        int childCount = root.getChildCount();

        for (int childIndex=0; childIndex<childCount; childIndex+=1) {
          AccessibilityNodeInfo child = root.getChild(childIndex);

          if (child != null) {
            AccessibilityNodeInfo node = findNode(child, tester);
            child.recycle();
            child = null;
            if (node != null) return node;
          }
        }
      }
    }

    return null;
  }

  public static AccessibilityNodeInfo findSelectedNode (AccessibilityNodeInfo root) {
    ScreenNodeTester tester = new ScreenNodeTester() {
      @Override
      public boolean testNode (AccessibilityNodeInfo node) {
        return node.isSelected();
      }
    };

    return findNode(root, tester);
  }

  public static AccessibilityNodeInfo findFocusedNode (AccessibilityNodeInfo root) {
    ScreenNodeTester tester = new ScreenNodeTester() {
      @Override
      public boolean testNode (AccessibilityNodeInfo node) {
        return node.isFocused();
      }
    };

    return findNode(root, tester);
  }

  public static AccessibilityNodeInfo findFocusableNode (AccessibilityNodeInfo root) {
    ScreenNodeTester tester = new ScreenNodeTester() {
      @Override
      public boolean testNode (AccessibilityNodeInfo node) {
        if (node.isFocusable()) {
          if (isVisible(node)) {
            return true;
          }
        }

        return false;
      }
    };

    return findNode(root, tester);
  }

  public static AccessibilityNodeInfo findTextNode (AccessibilityNodeInfo root) {
    ScreenNodeTester tester = new ScreenNodeTester() {
      @Override
      public boolean testNode (AccessibilityNodeInfo node) {
        return node.getText() != null;
      }
    };

    return findNode(root, tester);
  }

  public static AccessibilityNodeInfo findDescribedNode (AccessibilityNodeInfo root) {
    ScreenNodeTester tester = new ScreenNodeTester() {
      @Override
      public boolean testNode (AccessibilityNodeInfo node) {
        return node.getContentDescription() != null;
      }
    };

    return findNode(root, tester);
  }

  public static AccessibilityNodeInfo findActionableNode (AccessibilityNodeInfo node, int actions) {
    node = AccessibilityNodeInfo.obtain(node);

    while (true) {
      if (node.isEnabled()) {
        if ((node.getActions() & actions) != 0) {
          return node;
        }
      }

      AccessibilityNodeInfo parent = node.getParent();
      node.recycle();
      node = null;

      if (parent == null) return null;
      node = parent;
      parent = null;
    }
  }

  public static boolean performAction (AccessibilityNodeInfo node, int action) {
    node = findActionableNode(node, action);
    if (node == null) return false;

    try {
      return node.performAction(action);
    } finally {
      node.recycle();
      node = null;
    }
  }

  public static boolean performClick (AccessibilityNodeInfo node) {
    return performAction(node, AccessibilityNodeInfo.ACTION_CLICK);
  }

  public static boolean performLongClick (AccessibilityNodeInfo node) {
    return performAction(node, AccessibilityNodeInfo.ACTION_LONG_CLICK);
  }

  public static boolean performScrollForward (AccessibilityNodeInfo node) {
    return performAction(node, AccessibilityNodeInfo.ACTION_SCROLL_FORWARD);
  }

  public static boolean performScrollBackward (AccessibilityNodeInfo node) {
    return performAction(node, AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD);
  }

  public static String normalizeText (CharSequence text) {
    if (text != null) {
      String string = text.toString().trim();
      if (string.length() > 0) return string;
    }

    return null;
  }

  public static String getText (AccessibilityNodeInfo node) {
    CharSequence text = null;

    {
      TextField field = TextField.get(node);

      if (field != null) {
        synchronized (field) {
          text = field.getAccessibilityText();
        }
      }
    }

    if (text == null) text = node.getText();
    if (!isEditable(node)) return normalizeText(text);

    if (text == null) return "";
    return text.toString();
  }

  public static String getRangeValueFormat (AccessibilityNodeInfo.RangeInfo range) {
    switch (range.getType()) {
      case AccessibilityNodeInfo.RangeInfo.RANGE_TYPE_INT:
        return "%.0f";

      case AccessibilityNodeInfo.RangeInfo.RANGE_TYPE_PERCENT:
        return "%.0f%";

      default:
      case AccessibilityNodeInfo.RangeInfo.RANGE_TYPE_FLOAT:
        return "%.2f";
    }
  }

  public static String getSelectionMode (AccessibilityNodeInfo.CollectionInfo collection) {
    if (ApplicationUtilities.haveLollipop) {
      switch (collection.getSelectionMode()) {
        case AccessibilityNodeInfo.CollectionInfo.SELECTION_MODE_NONE:
          return "none";

        case AccessibilityNodeInfo.CollectionInfo.SELECTION_MODE_SINGLE:
          return "sngl";

        case AccessibilityNodeInfo.CollectionInfo.SELECTION_MODE_MULTIPLE:
          return "mult";
      }
    }

    return null;
  }
}
