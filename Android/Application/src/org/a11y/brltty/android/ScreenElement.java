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

public class ScreenElement {
  private final String nodeText;

  private String brailleText = null;
  protected Rect visualLocation = null;
  private Rect brailleLocation = null;

  public final String getBrailleText () {
    synchronized (this) {
      if (brailleText == null) {
        brailleText = makeBrailleText(nodeText);
      }
    }

    return brailleText;
  }

  public Rect getVisualLocation () {
    return null;
  }

  public final Rect getBrailleLocation () {
    return brailleLocation;
  }

  public final void setBrailleLocation (Rect location) {
    brailleLocation = location;
  }

  public AccessibilityNodeInfo getAccessibilityNode () {
    return null;
  }

  public boolean isCheckable () {
    return false;
  }

  public boolean isChecked () {
    return false;
  }

  public boolean isEditable () {
    return false;
  }

  public boolean onBringCursor () {
    return false;
  }

  public boolean onClick () {
    return false;
  }

  public boolean onLongClick () {
    return false;
  }

  public boolean onScrollBackward () {
    return false;
  }

  public boolean onScrollForward () {
    return false;
  }

  public boolean performAction (int offset) {
    switch (offset) {
      case  0: return onBringCursor();
      case  1: return onClick();
      case  2: return onLongClick();
      case  3: return onScrollBackward();
      case  4: return onScrollForward();
      default: return false;
    }
  }

  protected String makeBrailleText (String text) {
    if (isCheckable()) {
      text = text + "[" + (isChecked()? "x": " ") + "]";
    }

    return text;
  }

  public ScreenElement (String text) {
    nodeText = text;
  }

  public interface LocationGetter {
    public Rect getLocation (ScreenElement element);
  }

  public static final LocationGetter visualLocationGetter = new LocationGetter() {
    @Override
    public Rect getLocation (ScreenElement element) {
      return element.getVisualLocation();
    }
  };

  public static final LocationGetter brailleLocationGetter = new LocationGetter() {
    @Override
    public Rect getLocation (ScreenElement element) {
      return element.getBrailleLocation();
    }
  };
}
