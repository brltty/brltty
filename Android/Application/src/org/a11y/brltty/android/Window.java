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

import java.util.Map;
import java.util.HashMap;

import android.view.accessibility.AccessibilityWindowInfo;
import android.view.accessibility.AccessibilityNodeInfo;

import android.graphics.Point;
import android.graphics.Rect;

public class Window {
  private final static Map<Integer, Window> windows =
               new HashMap<Integer, Window>();

  private final int windowIdentifier;
  private AccessibilityWindowInfo windowObject = null;
  private RenderedScreen renderedScreen = null;

  private Window (int identifier) {
    windowIdentifier = identifier;
  }

  public static Window get (Integer identifier) {
    synchronized (windows) {
      Window window = windows.get(identifier);
      if (window != null) return window;

      window = new Window(identifier);
      windows.put(identifier, window);
      return window;
    }
  }

  public static Window get (AccessibilityWindowInfo object) {
    Window window = get(object.getId());

    synchronized (window) {
      if (window.windowObject != null) window.windowObject.recycle();
      window.windowObject = AccessibilityWindowInfo.obtain(object);
    }

    return window;
  }

  public static Window get (AccessibilityNodeInfo node) {
    if (ApplicationUtilities.haveLollipop) {
      AccessibilityWindowInfo object = node.getWindow();

      if (object != null) {
        try {
          return get(object);
        } finally {
          object.recycle();
          object = null;
        }
      }
    }

    return get(node.getWindowId());
  }

  public final int getIdentifier () {
    return windowIdentifier;
  }

  public final Rect getLocation () {
    synchronized (this) {
      Rect location = new Rect();

      if (windowObject != null) {
        windowObject.getBoundsInScreen(location);
      } else {
        Point size = new Point();
        ApplicationUtilities.getWindowManager().getDefaultDisplay().getSize(size);
        location.set(0, 0, size.x, size.y);
      }

      return location;
    }
  }

  public boolean contains (Rect location) {
    return getLocation().contains(location);
  }

  public final RenderedScreen getScreen () {
    return renderedScreen;
  }

  public final Window setScreen (RenderedScreen screen) {
    renderedScreen = screen;
    return this;
  }

  public static Window setScreen (AccessibilityNodeInfo node) {
    Window window = get(node);
    window.setScreen(new RenderedScreen(node));
    return window;
  }
}
