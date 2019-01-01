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

import android.view.accessibility.AccessibilityNodeInfo;

public class TextField {
  public TextField () {
  }

  private CharSequence accessibilityText = null;
  private int selectionStart = 0;
  private int selectionEnd = 0;

  public final CharSequence getAccessibilityText () {
    return accessibilityText;
  }

  public final TextField setAccessibilityText (CharSequence text) {
    synchronized (this) {
      accessibilityText = text;
    }

    return this;
  }

  public final int getSelectionStart () {
    return selectionStart;
  }

  public final int getSelectionEnd () {
    return selectionEnd;
  }

  public final TextField setSelection (int start, int end) {
    synchronized (this) {
      selectionStart = start;
      selectionEnd = end;
    }

    return this;
  }

  public final TextField setCursor (int offset) {
    return setSelection(offset, offset);
  }

  private static final Map<AccessibilityNodeInfo, TextField> textFields =
               new HashMap<AccessibilityNodeInfo, TextField>();

  public static TextField get (AccessibilityNodeInfo node, boolean canCreate) {
    synchronized (textFields) {
      TextField field = textFields.get(node);
      if (field != null) return field;
      if (!canCreate) return null;

      field = new TextField();
      textFields.put(node, field);
      return field;
    }
  }

  public static TextField get (AccessibilityNodeInfo node) {
    return get(node, false);
  }
}
