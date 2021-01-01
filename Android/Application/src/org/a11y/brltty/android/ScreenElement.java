/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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

  private ScreenElement backwardElement = null;
  private ScreenElement forwardElement = null;

  public final ScreenElement getBackwardElement () {
    return backwardElement;
  }

  public final ScreenElement setBackwardElement (ScreenElement element) {
    backwardElement = element;
    return this;
  }

  public final ScreenElement getForwardElement () {
    return forwardElement;
  }

  public final ScreenElement setForwardElement (ScreenElement element) {
    forwardElement = element;
    return this;
  }

  // Since null needs to be a valid cached value, use a self reference
  // to mean that the correct value hasn't been determined yet.
  private ScreenElement upwardElement = this;
  private ScreenElement downwardElement = this;
  private ScreenElement leftwardElement = this;
  private ScreenElement rightwardElement = this;

  public final ScreenElement getUpwardElement () {
    return upwardElement;
  }

  public final ScreenElement setUpwardElement (ScreenElement element) {
    upwardElement = element;
    return this;
  }

  public final ScreenElement getDownwardElement () {
    return downwardElement;
  }

  public final ScreenElement setDownwardElement (ScreenElement element) {
    downwardElement = element;
    return this;
  }

  public final ScreenElement getLeftwardElement () {
    return leftwardElement;
  }

  public final ScreenElement setLeftwardElement (ScreenElement element) {
    leftwardElement = element;
    return this;
  }

  public final ScreenElement getRightwardElement () {
    return rightwardElement;
  }

  public final ScreenElement setRightwardElement (ScreenElement element) {
    rightwardElement = element;
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

  public boolean onAccessibilityActions () {
    return false;
  }

  public boolean onDescribeElement () {
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
      case  6: return onAccessibilityActions();
      case 15: return onDescribeElement();
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
  private int[] lineOffsets = null;

  protected String[] makeBrailleText (String text) {
    if (text == null) return null;

    List<String> lines = new ArrayList<String>();
    int start = 0;

    while (true) {
      int end = text.indexOf('\n', start);
      if (end == -1) break;

      lines.add(text.substring(start, end));
      start = end + 1;
    }

    if (start > 0) text = text.substring(start);
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

  private final int[] makeLineOffsets (String[] lines) {
    int[] offsets = new int[lines.length];

    int index = 0;
    int offset = 0;

    for (String line : lines) {
      offsets[index] = offset;
      offset += lines[index].length();
      index += 1;
    }

    return offsets;
  }

  public final int getTextOffset (int column, int row) {
    String[] lines = getBrailleText();

    synchronized (this) {
      if (lineOffsets == null) {
        lineOffsets = makeLineOffsets(lines);
      }
    }

    return lineOffsets[row] + Math.min(column, (lines[row].length() - 1));
  }

  public final Point getBrailleCoordinates (int offset) {
    String[] lines = getBrailleText();

    if (lines != null) {
      int row = 0;

      for (String line : lines) {
        int length = line.length();
        if (offset < length) return new Point(offset, row);

        row += 1;
        offset -= length;
      }
    }

    return null;
  }
}
