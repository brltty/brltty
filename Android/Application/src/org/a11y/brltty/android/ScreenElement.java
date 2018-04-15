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

import java.util.List;
import java.util.ArrayList;

import android.view.accessibility.AccessibilityNodeInfo;

import android.graphics.Rect;
import android.graphics.Point;

public class ScreenElement {
  private final String elementText;

  public ScreenElement (String text) {
    elementText = text;
  }

  public ScreenElement (int text) {
    this(ApplicationUtilities.getResourceString(text));
  }

  public final String getElementText () {
    return elementText;
  }

  private ScreenElement previousElement = null;
  private ScreenElement nextElement = null;

  public final ScreenElement getPreviousElement () {
    return previousElement;
  }

  public final ScreenElement setPreviousElement (ScreenElement element) {
    previousElement = element;
    return this;
  }

  public final ScreenElement getNextElement () {
    return nextElement;
  }

  public final ScreenElement setNextElement (ScreenElement element) {
    nextElement = element;
    return this;
  }

  public AccessibilityNodeInfo getAccessibilityNode () {
    return null;
  }

  public boolean isEditable () {
    return false;
  }

  public boolean bringCursor () {
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

  public boolean onContextClick () {
    return false;
  }

  public boolean performAction (int column, int row) {
    switch (column) {
      case  0: return onBringCursor();
      case  1: return onClick();
      case  2: return onLongClick();
      case  3: return onScrollBackward();
      case  4: return onScrollForward();
      case  5: return onContextClick();
      default: return false;
    }
  }

  private Rect visualLocation = null;

  public final Rect getVisualLocation () {
    return visualLocation;
  }

  public final ScreenElement setVisualLocation (Rect location) {
    visualLocation = location;
    return this;
  }

  private Rect brailleLocation = null;

  public final Rect getBrailleLocation () {
    return brailleLocation;
  }

  public final ScreenElement setBrailleLocation (Rect location) {
    brailleLocation = location;
    return this;
  }

  public final ScreenElement setBrailleLocation (int left, int top, int right, int bottom) {
    return setBrailleLocation(new Rect(left, top, right, bottom));
  }

  private String[] brailleText = null;

  protected String[] makeBrailleText (String text) {
    if (text == null) return null;
    List<String> lines = new ArrayList<String>();

    while (text != null) {
      int index = text.indexOf('\n');
      if (index == -1) break;

      lines.add(text.substring(0, index));
      text = text.substring(index+1);
    }

    lines.add(text);
    return lines.toArray(new String[lines.size()]);
  }

  public final String[] getBrailleText () {
    synchronized (this) {
      if (brailleText == null) {
        brailleText = makeBrailleText(elementText);
      }
    }

    return brailleText;
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
}
