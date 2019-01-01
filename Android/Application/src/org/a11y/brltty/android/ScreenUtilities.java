/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2019 by The BRLTTY Developers.
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

import java.util.List;
import java.util.Arrays;
import java.util.Comparator;

import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityWindowInfo;

import android.graphics.Rect;

public abstract class ScreenUtilities {
  private ScreenUtilities () {
  }

  public static String getClassPath (AccessibilityNodeInfo node) {
    CharSequence path = node.getClassName();
    if (path == null) return null;
    return path.toString();
  }

  public static String getClassName (AccessibilityNodeInfo node) {
    String path = getClassPath(node);
    if (path == null) return null;

    int index = path.lastIndexOf('.');
    return path.substring(index+1);
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

  public static boolean isVisible (AccessibilityNodeInfo node) {
    if (ApplicationUtilities.haveJellyBean) {
      return node.isVisibleToUser();
    }

    Rect location = new Rect();
    node.getBoundsInScreen(location);
    return ScreenDriver.getWindow().contains(location);
  }

  public static boolean isSubclassOf (AccessibilityNodeInfo node, Class type) {
    String path = getClassPath(node);
    if (path == null) return false;
    return LanguageUtilities.canAssign(type, path);
  }

  public static boolean isCheckBox (AccessibilityNodeInfo node) {
    return isSubclassOf(node, android.widget.CheckBox.class);
  }

  public static boolean isSwitch (AccessibilityNodeInfo node) {
    return isSubclassOf(node, android.widget.Switch.class);
  }

  public static int getSelectionMode (AccessibilityNodeInfo node) {
    if (ApplicationUtilities.haveLollipop) {
      if (node.getCollectionItemInfo() != null) {
        AccessibilityNodeInfo parent = node.getParent();

        if (parent != null) {
          try {
            AccessibilityNodeInfo.CollectionInfo collection = parent.getCollectionInfo();

            if (collection != null) {
              return collection.getSelectionMode();
            }
          } finally {
            parent.recycle();
            parent = null;
          }
        }
      }
    }

    return node.isCheckable()?
           AccessibilityNodeInfo.CollectionInfo.SELECTION_MODE_MULTIPLE:
           AccessibilityNodeInfo.CollectionInfo.SELECTION_MODE_NONE;
  }

  public static boolean hasSelectionMode (AccessibilityNodeInfo item, int mode) {
    return getSelectionMode(item) == mode;
  }

  public static boolean isRadioButton (AccessibilityNodeInfo node) {
    if (hasSelectionMode(node, AccessibilityNodeInfo.CollectionInfo.SELECTION_MODE_SINGLE)) return true;
    return isSubclassOf(node, android.widget.RadioButton.class);
  }

  public static boolean isEditable (AccessibilityNodeInfo node) {
    if (ApplicationUtilities.haveJellyBeanMR2) {
      return node.isEditable();
    } else {
      return isSubclassOf(node, android.widget.EditText.class);
    }
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

  public static String getDescription (AccessibilityNodeInfo node) {
    return normalizeText(node.getContentDescription());
  }

  public static AccessibilityNodeInfo getRootNode () {
    if (ApplicationUtilities.haveJellyBean) {
      return BrailleService.getBrailleService().getRootInActiveWindow();
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

  public static AccessibilityNodeInfo findNode (AccessibilityNodeInfo root, NodeTester tester) {
    if (root != null) {
      if (isVisible(root)) {
        if (tester.testNode(root)) {
          return AccessibilityNodeInfo.obtain(root);
        }
      }

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
    NodeTester tester = new NodeTester() {
      @Override
      public boolean testNode (AccessibilityNodeInfo node) {
        return node.isSelected();
      }
    };

    return findNode(root, tester);
  }

  public static AccessibilityNodeInfo findFocusedNode (AccessibilityNodeInfo root) {
    NodeTester tester = new NodeTester() {
      @Override
      public boolean testNode (AccessibilityNodeInfo node) {
        return node.isFocused();
      }
    };

    return findNode(root, tester);
  }

  public static AccessibilityNodeInfo findFocusableNode (AccessibilityNodeInfo root) {
    NodeTester tester = new NodeTester() {
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
    NodeTester tester = new NodeTester() {
      @Override
      public boolean testNode (AccessibilityNodeInfo node) {
        return getText(node) != null;
      }
    };

    return findNode(root, tester);
  }

  public static AccessibilityNodeInfo findDescribedNode (AccessibilityNodeInfo root) {
    NodeTester tester = new NodeTester() {
      @Override
      public boolean testNode (AccessibilityNodeInfo node) {
        return getDescription(node) != null;
      }
    };

    return findNode(root, tester);
  }

  public static AccessibilityNodeInfo findActionableNode (AccessibilityNodeInfo node, NodeTester tester) {
    node = AccessibilityNodeInfo.obtain(node);

    while (true) {
      if (node.isEnabled()) {
        if (tester.testNode(node)) {
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

  public static AccessibilityNodeInfo findActionableNode (AccessibilityNodeInfo node, final int actions) {
    NodeTester tester = new NodeTester() {
      @Override
      public boolean testNode (AccessibilityNodeInfo node) {
        return (node.getActions() & actions) != 0;
      }
    };

    return findActionableNode(node, tester);
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

  public static AccessibilityNodeInfo findActionableNode (
    AccessibilityNodeInfo node,
    AccessibilityNodeInfo.AccessibilityAction... actions
  ) {
    final Comparator<AccessibilityNodeInfo.AccessibilityAction> comparator =
      new Comparator<AccessibilityNodeInfo.AccessibilityAction>() {
        @Override
        public int compare (
          AccessibilityNodeInfo.AccessibilityAction action1,
          AccessibilityNodeInfo.AccessibilityAction action2
        ) {
          return Integer.compare(action1.getId(), action2.getId());
        }
      };

    final AccessibilityNodeInfo.AccessibilityAction[] array = Arrays.copyOf(actions, actions.length);
    Arrays.sort(array, 0, array.length, comparator);

    NodeTester tester = new NodeTester() {
      @Override
      public boolean testNode (AccessibilityNodeInfo node) {
        for (AccessibilityNodeInfo.AccessibilityAction action : node.getActionList()) {
          if (Arrays.binarySearch(array, 0, array.length, action, comparator) >= 0) return true;
        }

        return false;
      }
    };

    return findActionableNode(node, tester);
  }

  public static boolean performAction (
    AccessibilityNodeInfo node,
    AccessibilityNodeInfo.AccessibilityAction action
  ) {
    node = findActionableNode(node, action);
    if (node == null) return false;

    try {
      return node.performAction(action.getId());
    } finally {
      node.recycle();
      node = null;
    }
  }

  public static List<AccessibilityWindowInfo> getWindows () {
    return BrailleService.getBrailleService().getWindows();
  }

  public static AccessibilityWindowInfo findWindow (int identifier) {
    AccessibilityWindowInfo found = null;

    for (AccessibilityWindowInfo window : getWindows()) {
      if (found == null) {
        if (window.getId() == identifier) {
          found = window;
          continue;
        }
      }

      window.recycle();
      window = null;
    }

    return found;
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

  public static String getSelectionModeLabel (AccessibilityNodeInfo.CollectionInfo collection) {
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
