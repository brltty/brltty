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

import android.graphics.Rect;

public class RealScreenElement extends ScreenElement {
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
    return accessibilityNode;
  }

  @Override
  public boolean isCheckable () {
    return accessibilityNode.isCheckable();
  }

  @Override
  public boolean isChecked () {
    return accessibilityNode.isChecked();
  }

  private boolean doAction (int action) {
    AccessibilityNodeInfo node = accessibilityNode;
    Rect inner = getVisualLocation();

    while (((node.getActions() & action) == 0) || !node.isEnabled()) {
      AccessibilityNodeInfo parent = node.getParent();
      if (parent == null) {
        return false;
      }

      Rect outer = new Rect();
      parent.getBoundsInScreen(outer);

      if (!outer.contains(inner)) {
        return false;
      }

      inner = outer;
      node = parent;
    }

    return node.performAction(action);
  }

  @Override
  public boolean onBringCursor () {
    return doAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS);
  }

  @Override
  public boolean onClick () {
    return doAction(AccessibilityNodeInfo.ACTION_CLICK);
  }

  @Override
  public boolean onLongClick () {
    return doAction(AccessibilityNodeInfo.ACTION_LONG_CLICK);
  }

  @Override
  public boolean onScrollBackward () {
    return doAction(AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD);
  }

  @Override
  public boolean onScrollForward () {
    return doAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD);
  }

  public RealScreenElement (String text, AccessibilityNodeInfo node) {
    super(text);
    accessibilityNode = node;
  }
}
