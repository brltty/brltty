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

import android.os.Build;
import android.os.Bundle;

import android.view.accessibility.AccessibilityNodeInfo;
import android.graphics.Rect;
import android.view.KeyEvent;

public class RealScreenElement extends ScreenElement {
  private static final String LOG_TAG = RealScreenElement.class.getName();

  private final AccessibilityNodeInfo accessibilityNode;

  @Override
  public Rect getVisualLocation () {
    synchronized (this) {
      if (visualLocation == null) {
        visualLocation = new Rect();
        accessibilityNode.getBoundsInScreen(visualLocation);
      }
    }

    return visualLocation;
  }

  @Override
  public AccessibilityNodeInfo getAccessibilityNode () {
    return AccessibilityNodeInfo.obtain(accessibilityNode);
  }

  @Override
  public boolean isCheckable () {
    return accessibilityNode.isCheckable();
  }

  @Override
  public boolean isChecked () {
    return accessibilityNode.isChecked();
  }

  @Override
  public boolean isEditable () {
    return LanguageUtilities.canAssign(android.widget.EditText.class, accessibilityNode.getClassName().toString());
  }

  @Override
  public boolean performAction (int x, int y) {
    ScreenTextEditor editor = ScreenTextEditor.getIfFocused(accessibilityNode);

    if (editor != null) {
      if (!onBringCursor()) return false;

      String[] lines = getBrailleText();
      String line;
      int index = 0;
      int offset = 0;

      while (true) {
        line = lines[index];
        if (index == y) break;

        offset += line.length();
        index += 1;
      }

      return InputService.inputCursor(offset + Math.min(x, (line.length() - 1)));
    }

    return super.performAction(x, y);
  }

  @Override
  protected String[] makeBrailleText (String text) {
    String[] lines = super.makeBrailleText(text);
    if (lines == null) return null;

    if (ScreenTextEditor.getIfFocused(accessibilityNode) != null) {
      for (int i=0; i<lines.length; i+=1) {
        lines[i] += ' ';
      }
    }

    return lines;
  }

  private AccessibilityNodeInfo getActionableNode (int action) {
    AccessibilityNodeInfo node = getAccessibilityNode();
    Rect inner = getVisualLocation();

    while (true) {
      if (node.isEnabled()) {
        if ((node.getActions() & action) != 0) {
          return node;
        }
      }

      AccessibilityNodeInfo parent = node.getParent();
      if (parent == null) break;

      Rect outer = new Rect();
      parent.getBoundsInScreen(outer);

      if (!Rect.intersects(outer, inner)) {
        parent.recycle();
        parent = null;
        break;
      }

      inner = outer;
      node.recycle();
      node = parent;
    }

    node.recycle();
    node = null;
    return null;
  }

  private AccessibilityNodeInfo getFocusableNode () {
    return getActionableNode(AccessibilityNodeInfo.ACTION_FOCUS | AccessibilityNodeInfo.ACTION_CLEAR_FOCUS);
  }

  private boolean doAction (int action) {
    AccessibilityNodeInfo node = getActionableNode(action);
    if (node == null) return false;

    boolean performed = node.performAction(action);
    node.recycle();
    node = null;
    return performed;
  }

  private boolean selectNode (AccessibilityNodeInfo root, boolean isDescendant) {
    boolean select = false;

    if (root != null) {
      {
        int childCount = root.getChildCount();

        for (int childIndex=0; childIndex<childCount; childIndex+=1) {
          AccessibilityNodeInfo child = root.getChild(childIndex);

          if (child != null) {
            if (selectNode(child, true)) {
              select = true;
            } else if (child.equals(accessibilityNode)) {
              select = true;
            }

            child.recycle();
            child = null;
          }
        }
      }

      if (isDescendant) {
        if (select != root.isSelected()) {
          int action = select? AccessibilityNodeInfo.ACTION_SELECT: AccessibilityNodeInfo.ACTION_CLEAR_SELECTION;
          root.performAction(action);
        }
      }
    }

    return select;
  }

  private boolean selectNode (AccessibilityNodeInfo root) {
    return selectNode(root, false);
  }

  public boolean doKey (int keyCode, boolean longPress) {
    boolean done = false;
    AccessibilityNodeInfo node = getFocusableNode();

    if (node != null) {
      if (node.isFocused()) {
        if (InputService.inputKey(keyCode, longPress)) done = true;
      } else if (node.performAction(AccessibilityNodeInfo.ACTION_FOCUS)) {
        final long start = System.currentTimeMillis();

        while (true) {
          {
            AccessibilityNodeInfo refreshed = ScreenUtilities.getRefreshedNode(node);

            if (refreshed != null) {
              boolean stop = false;

              if (refreshed.isFocused()) {
                stop = true;
                if (InputService.inputKey(keyCode, longPress)) done = true;
              }

              refreshed.recycle();
              refreshed = null;

              if (stop) break;
            }
          }

          if ((System.currentTimeMillis() - start) >= ApplicationParameters.KEY_RETRY_TIMEOUT) {
            break;
          }

          try {
            Thread.sleep(ApplicationParameters.KEY_RETRY_INTERVAL);
          } catch (InterruptedException exception) {
          }
        }
      }

      node.recycle();
      node = null;
    }

    return done;
  }

  @Override
  public boolean onBringCursor () {
    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      {
        AccessibilityNodeInfo focusedNode = accessibilityNode.findFocus(AccessibilityNodeInfo.FOCUS_ACCESSIBILITY);

        if (focusedNode != null) {
          boolean done = focusedNode.equals(accessibilityNode);
          focusedNode.recycle();
          focusedNode = null;
          if (done) return true;
        }
      }

      return doAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS);
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.ICE_CREAM_SANDWICH)) {
      AccessibilityNodeInfo node = getFocusableNode();

      if (node != null) {
        boolean isFocused = node.isFocused();

        if (!isFocused) {
          if (node.performAction(AccessibilityNodeInfo.ACTION_FOCUS)) {
            isFocused = true;
          }
        }

        if (isFocused) selectNode(node);
        node.recycle();
        node = null;
        return isFocused;
      }
    }

    return super.onBringCursor();
  }

  @Override
  public boolean onClick () {
    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.ICE_CREAM_SANDWICH)) {
      if (isEditable()) {
        return doAction(AccessibilityNodeInfo.ACTION_FOCUS);
      }
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      return doAction(AccessibilityNodeInfo.ACTION_CLICK);
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.ICE_CREAM_SANDWICH)) {
      return doKey(KeyEvent.KEYCODE_DPAD_CENTER, false);
    }

    return super.onClick();
  }

  @Override
  public boolean onLongClick () {
    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      return doAction(AccessibilityNodeInfo.ACTION_LONG_CLICK);
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.ICE_CREAM_SANDWICH)) {
      return doKey(KeyEvent.KEYCODE_DPAD_CENTER, true);
    }

    return super.onLongClick();
  }

  @Override
  public boolean onScrollBackward () {
    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      return doAction(AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD);
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.ICE_CREAM_SANDWICH)) {
      return doKey(KeyEvent.KEYCODE_PAGE_UP, false);
    }

    return super.onScrollBackward();
  }

  @Override
  public boolean onScrollForward () {
    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      return doAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD);
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.ICE_CREAM_SANDWICH)) {
      return doKey(KeyEvent.KEYCODE_PAGE_DOWN, false);
    }

    return super.onScrollForward();
  }

  public RealScreenElement (String text, AccessibilityNodeInfo node) {
    super(text);
    accessibilityNode = AccessibilityNodeInfo.obtain(node);

    if (isEditable()) {
      ScreenTextEditor.get(accessibilityNode, true);
    }
  }
}
