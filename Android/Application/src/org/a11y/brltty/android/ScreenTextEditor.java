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

public class ScreenTextEditor {
  private static final Map<Integer, ScreenTextEditor> textEditors = new HashMap<Integer, ScreenTextEditor>();

  private int cursorOffset = 0;

  public static ScreenTextEditor get (AccessibilityNodeInfo node, boolean canCreate) {
    Integer key = new Integer(node.hashCode());
    ScreenTextEditor value = textEditors.get(key);

    if (value == null) {
      if (canCreate) {
        value = new ScreenTextEditor();
        textEditors.put(key, value);
      }
    }

    return value;
  }

  public static ScreenTextEditor getIfFocused (AccessibilityNodeInfo node) {
    return node.isFocused()? get(node, false): null;
  }

  public final int getCursorOffset () {
    return cursorOffset;
  }

  public final void setCursorOffset (int offset) {
    cursorOffset = offset;
  }

  public ScreenTextEditor () {
  }
}
