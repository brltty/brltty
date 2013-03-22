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

import java.util.Map;
import java.util.HashMap;

import android.view.accessibility.AccessibilityNodeInfo;

import android.graphics.Rect;

public class ScreenWindow {
  public class TextEntry {
    private int cursorOffset = 0;

    public final int getCursorOffset () {
      return cursorOffset;
    }

    public final void setCursorOffset (int offset) {
      cursorOffset = offset;
    }

    public TextEntry () {
    }
  }

  private final int windowIdentifier;

  private final Map<Rect, TextEntry> textEntries = new HashMap<Rect, TextEntry>();

  public final int getWindowIdentifier () {
    return windowIdentifier;
  }

  public TextEntry getTextEntry (AccessibilityNodeInfo node, boolean canCreate) {
    Rect key = new Rect();
    node.getBoundsInScreen(key);
    TextEntry textEntry = textEntries.get(key);

    if (textEntry == null) {
      if (canCreate) {
        textEntry = new TextEntry();
        textEntries.put(key, textEntry);
      }
    }

    return textEntry;
  }

  public ScreenWindow (int identifier) {
    windowIdentifier = identifier;
  }
}
