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

import android.os.Build;
import android.os.Bundle;
import android.view.accessibility.AccessibilityNodeInfo;

import android.text.Editable;
import android.text.SpannableStringBuilder;

public abstract class InputTextEditor {
  private final boolean editWasPerformed;

  public final boolean wasPerformed () {
    return editWasPerformed;
  }

  protected abstract Integer performEdit (Editable editor, int start, int end);

  private final boolean performEdit (AccessibilityNodeInfo node) {
    if (node.isFocused()) {
      CharSequence text = node.getText();
      int start = node.getTextSelectionStart();
      int end = node.getTextSelectionEnd();

      if (text == null) {
        text = "";
        start = end = 0;
      }

      Editable editor = (text instanceof Editable)?
                        (Editable)text:
                        new SpannableStringBuilder(text);

      if ((0 <= start) && (start <= end) && (end <= text.length())) {
        Integer position = performEdit(editor, start, end);

        if (position != null) {
          Bundle arguments = new Bundle();
          arguments.putCharSequence(AccessibilityNodeInfo.ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE, editor);

          if (node.performAction(AccessibilityNodeInfo.ACTION_SET_TEXT, arguments)) {
            if (position == editor.length()) return true;

            arguments.clear();
            arguments.putInt(AccessibilityNodeInfo.ACTION_ARGUMENT_SELECTION_START_INT, position);
            arguments.putInt(AccessibilityNodeInfo.ACTION_ARGUMENT_SELECTION_END_INT, position);
            return node.performAction(AccessibilityNodeInfo.ACTION_SET_SELECTION, arguments);
          }
        }
      }
    }

    return false;
  }

  public InputTextEditor () {
    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.LOLLIPOP)) {
      RenderedScreen screen = ScreenDriver.getScreen();

      if (screen != null) {
        AccessibilityNodeInfo node = screen.getCursorNode();

        if (node != null) {
          if (node.isEditable()) {
            editWasPerformed = performEdit(node);
            return;
          }
        }
      }
    }

    throw new UnsupportedOperationException();
  }
}
