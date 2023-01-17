/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2023 by The BRLTTY Developers.
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

import java.util.Map;
import java.util.HashMap;

import android.view.accessibility.AccessibilityWindowInfo;
import android.view.accessibility.AccessibilityNodeInfo;

import android.graphics.Point;
import android.graphics.Rect;

public class ScreenWindow {
  private final int windowIdentifier;
  private AccessibilityWindowInfo windowInfo = null;

  private ScreenWindow (int identifier) {
    windowIdentifier = identifier;
  }

  public final int getWindowIdentifier () {
    return windowIdentifier;
  }

  public final AccessibilityWindowInfo getWindowInfo () {
    synchronized (this) {
      if (windowInfo != null) {
        if (APITests.haveLollipop) {
          return AccessibilityWindowInfo.obtain(windowInfo);
        }
      }
    }

    return null;
  }

  private final static Map<Integer, ScreenWindow> screenWindowCache =
               new HashMap<Integer, ScreenWindow>();

  private RenderedScreen renderedScreen = null;

  public static ScreenWindow getScreenWindow (Integer identifier) {
    synchronized (screenWindowCache) {
      ScreenWindow window = screenWindowCache.get(identifier);
      if (window != null) return window;

      window = new ScreenWindow(identifier);
      screenWindowCache.put(identifier, window);
      return window;
    }
  }

  public static ScreenWindow getScreenWindow (AccessibilityWindowInfo info) {
    ScreenWindow window = null;

    if (APITests.haveLollipop) {
      window = getScreenWindow(info.getId());

      synchronized (window) {
        if (window.windowInfo != null) window.windowInfo.recycle();
        window.windowInfo = AccessibilityWindowInfo.obtain(info);
      }
    }

    return window;
  }

  public static ScreenWindow getScreenWindow (AccessibilityNodeInfo node) {
    if (APITests.haveLollipop) {
      AccessibilityWindowInfo info = node.getWindow();

      if (info != null) {
        try {
          return getScreenWindow(info);
        } finally {
          info.recycle();
          info = null;
        }
      }
    }

    return getScreenWindow(node.getWindowId());
  }

  public final Rect getLocation () {
    synchronized (this) {
      Rect location = new Rect();

      if (windowInfo != null) {
        if (APITests.haveLollipop) {
          windowInfo.getBoundsInScreen(location);
          return location;
        }
      }

      Point size = new Point();
      ApplicationUtilities.getWindowManager().getDefaultDisplay().getSize(size);
      location.set(0, 0, size.x, size.y);
      return location;
    }
  }

  public boolean contains (Rect location) {
    return getLocation().contains(location);
  }

  public final RenderedScreen getRenderedScreen () {
    return renderedScreen;
  }

  public final ScreenWindow setRenderedScreen (RenderedScreen screen) {
    renderedScreen = screen;
    return this;
  }

  public static ScreenWindow setRenderedScreen (AccessibilityNodeInfo node) {
    ScreenWindow window = getScreenWindow(node);
    return window.setRenderedScreen(new RenderedScreen(node));
  }
}
