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

  private final Map<Integer, TextEntry> textEntries = new HashMap<Integer, TextEntry>();

  public final int getWindowIdentifier () {
    return windowIdentifier;
  }

  public TextEntry getTextEntry (AccessibilityNodeInfo node, boolean canCreate) {
    Integer key = new Integer(node.hashCode());
    TextEntry value = textEntries.get(key);

    if (value == null) {
      if (canCreate) {
        value = new TextEntry();
        textEntries.put(key, value);
      }
    }

    return value;
  }

  public ScreenWindow (int identifier) {
    windowIdentifier = identifier;
  }
}
