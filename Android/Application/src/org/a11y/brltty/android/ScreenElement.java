/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
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

import java.util.List;
import java.util.ArrayList;

import android.view.accessibility.AccessibilityNodeInfo;

import android.graphics.Rect;
import android.graphics.Point;

public class ScreenElement {
  private final String elementText;

  private String[] brailleText = null;
  protected Rect visualLocation = null;
  private Rect brailleLocation = null;

  public String getElementText () {
    return elementText;
  }

  public final String[] getBrailleText () {
    synchronized (this) {
      if (brailleText == null) {
        brailleText = makeBrailleText(elementText);
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

  public final void setBrailleLocation (int left, int top, int right, int bottom) {
    setBrailleLocation(new Rect(left, top, right, bottom));
  }

  public final Point getBrailleCoordinate (int offset) {
    String[] lines = getBrailleText();

    if (lines != null) {
      int y = 0;

      for (String line : lines) {
        int length = line.length();
        if (offset < length) return new Point(offset, y);

        y += 1;
        offset -= length;
      }
    }

    return null;
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

  public boolean performAction (int x, int y) {
    switch (x) {
      case  0: return onBringCursor();
      case  1: return onClick();
      case  2: return onLongClick();
      case  3: return onScrollBackward();
      case  4: return onScrollForward();
      default: return false;
    }
  }

  protected String[] makeBrailleText (String text) {
    if (isCheckable()) {
      StringBuilder sb = new StringBuilder();

      sb.append('[');
      sb.append((isChecked()? "X": " "));
      sb.append(']');

      if (text != null) {
        sb.append(' ');
        sb.append(text);
      }

      text = sb.toString();
    }

    if (text == null) return null;
    List<String> lines = new ArrayList<String>();

    while (text != null) {
      String line;
      int index = text.indexOf('\n');

      if (index == -1) {
        line = text;
        text = null;
      } else {
        line = text.substring(0, index);
        text = text.substring(index+1);
      }

      lines.add(line);
    }

    return lines.toArray(new String[lines.size()]);
  }

  public ScreenElement (String text) {
    elementText = text;
  }

  public ScreenElement (int text) {
    this(ApplicationUtilities.getString(text));
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
