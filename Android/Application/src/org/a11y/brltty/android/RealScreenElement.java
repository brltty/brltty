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

import android.util.Log;

import android.os.Bundle;

import android.view.accessibility.AccessibilityNodeInfo;
import android.graphics.Rect;
import android.view.KeyEvent;

public class RealScreenElement extends ScreenElement {
  private static final String LOG_TAG = RealScreenElement.class.getName();

  private final AccessibilityNodeInfo accessibilityNode;

  public RealScreenElement (String text, AccessibilityNodeInfo node) {
    super(text);
    accessibilityNode = AccessibilityNodeInfo.obtain(node);

    {
      Rect location = new Rect();
      node.getBoundsInScreen(location);
      location.right -= 1;
      location.bottom -= 1;
      setVisualLocation(location);
    }
  }

  @Override
  public AccessibilityNodeInfo getAccessibilityNode () {
    return AccessibilityNodeInfo.obtain(accessibilityNode);
  }

  @Override
  public boolean isEditable () {
    return ScreenUtilities.isEditable(accessibilityNode);
  }

  private static boolean setInputFocus (AccessibilityNodeInfo node) {
    if (node.isFocused()) return true;

    if (!node.performAction(AccessibilityNodeInfo.ACTION_FOCUS)) return false;
    final long startTime = ApplicationUtilities.getAbsoluteTime();

    while (true) {
      try {
        Thread.sleep(ApplicationParameters.FOCUS_WAIT_INTERVAL);
      } catch (InterruptedException exception) {
      }

      {
        AccessibilityNodeInfo refreshed = ScreenUtilities.getRefreshedNode(node);

        if (refreshed != null) {
          try {
            if (refreshed.isFocused()) return true;
          } finally {
            refreshed.recycle();
            refreshed = null;
          }
        }
      }

      if (ApplicationUtilities.getRelativeTime(startTime) >= ApplicationParameters.FOCUS_WAIT_TIMEOUT) {
        return false;
      }
    }
  }

  private final boolean setInputFocus () {
    AccessibilityNodeInfo node = ScreenUtilities.getRefreshedNode(accessibilityNode);

    if (node != null) {
      try {
        return setInputFocus(node);
      } finally {
        node.recycle();
        node = null;
      }
    }

    return false;
  }

  private AccessibilityNodeInfo getFocusableNode () {
    return ScreenUtilities.findActionableNode(accessibilityNode,
      AccessibilityNodeInfo.ACTION_FOCUS |
      AccessibilityNodeInfo.ACTION_CLEAR_FOCUS
    );
  }

  private boolean performNodeAction (int action) {
    return ScreenUtilities.performAction(accessibilityNode, action);
  }

  private boolean performNodeAction (AccessibilityNodeInfo.AccessibilityAction action) {
    return ScreenUtilities.performAction(accessibilityNode, action);
  }

  public boolean injectKey (int keyCode, boolean longPress) {
    AccessibilityNodeInfo node = getFocusableNode();

    if (node != null) {
      try {
        if (setInputFocus(node)) {
          if (InputService.injectKey(keyCode, longPress)) {
            return true;
          }
        }
      } finally {
        node.recycle();
        node = null;
      }
    }

    return false;
  }

  @Override
  public boolean bringCursor () {
    if (ApplicationUtilities.haveJellyBean) {
      return performNodeAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS);
    }

    return false;
  }

  @Override
  public boolean onBringCursor () {
    if (ApplicationUtilities.haveJellyBean) {
      {
        AccessibilityNodeInfo node = accessibilityNode.findFocus(AccessibilityNodeInfo.FOCUS_ACCESSIBILITY);

        if (node != null) {
          try {
            return node.equals(accessibilityNode);
          } finally {
            node.recycle();
            node = null;
          }
        }
      }

      if (performNodeAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS)) return true;
    }

    if (ApplicationUtilities.haveIceCreamSandwich) {
      AccessibilityNodeInfo node = getFocusableNode();

      if (node != null) {
        boolean isFocused = node.isFocused();

        if (!isFocused) {
          if (node.performAction(AccessibilityNodeInfo.ACTION_FOCUS)) {
            isFocused = true;
          }
        }

        node.recycle();
        node = null;

        if (isFocused) return true;
      }
    }

    return super.onBringCursor();
  }

  @Override
  public boolean onClick () {
    if (ApplicationUtilities.haveIceCreamSandwich) {
      if (isEditable()) {
        return performNodeAction(AccessibilityNodeInfo.ACTION_FOCUS);
      }
    }

    if (ApplicationUtilities.haveJellyBean) {
      return performNodeAction(AccessibilityNodeInfo.ACTION_CLICK);
    }

    if (ApplicationUtilities.haveIceCreamSandwich) {
      return injectKey(KeyEvent.KEYCODE_DPAD_CENTER, false);
    }

    return super.onClick();
  }

  @Override
  public boolean onLongClick () {
    if (ApplicationUtilities.haveJellyBean) {
      return performNodeAction(AccessibilityNodeInfo.ACTION_LONG_CLICK);
    }

    if (ApplicationUtilities.haveIceCreamSandwich) {
      return injectKey(KeyEvent.KEYCODE_DPAD_CENTER, true);
    }

    return super.onLongClick();
  }

  @Override
  public boolean onScrollBackward () {
    if (ApplicationUtilities.haveJellyBean) {
      return performNodeAction(AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD);
    }

    if (ApplicationUtilities.haveIceCreamSandwich) {
      return injectKey(KeyEvent.KEYCODE_PAGE_UP, false);
    }

    return super.onScrollBackward();
  }

  @Override
  public boolean onScrollForward () {
    if (ApplicationUtilities.haveJellyBean) {
      return performNodeAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD);
    }

    if (ApplicationUtilities.haveIceCreamSandwich) {
      return injectKey(KeyEvent.KEYCODE_PAGE_DOWN, false);
    }

    return super.onScrollForward();
  }

  @Override
  public boolean onContextClick () {
    if (ApplicationUtilities.haveMarshmallow) {
      return performNodeAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_CONTEXT_CLICK);
    }

    return super.onContextClick();
  }

  @Override
  public boolean performAction (int column, int row) {
    if (ScreenUtilities.isEditable(accessibilityNode)) {
      if (!onBringCursor()) return false;
      setInputFocus();

      String[] lines = getBrailleText();
      String line;
      int index = 0;
      int offset = 0;

      while (true) {
        line = lines[index];
        if (index == row) break;

        offset += line.length();
        index += 1;
      }

      offset += Math.min(column, (line.length() - 1));
      return InputHandlers.placeCursor(accessibilityNode, offset);
    }

    return super.performAction(column, row);
  }

  @Override
  protected String[] makeBrailleText (String text) {
    String[] lines = super.makeBrailleText(text);
    if (lines == null) return null;

    if (ScreenUtilities.isEditable(accessibilityNode)) {
      int count = lines.length;

      for (int index=0; index<count; index+=1) {
        lines[index] += ' ';
      }
    }

    return lines;
  }
}
