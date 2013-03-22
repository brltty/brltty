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

public final class ScreenUtilities {
  public static AccessibilityNodeInfo findRootNode (AccessibilityNodeInfo node) {
    if (node != null) {
      while (true) {
        AccessibilityNodeInfo parent = node.getParent();

        if (parent == null) {
          break;
        }

        node = parent;
      }
    }

    return node;
  }

  private static String normalizeText (CharSequence text) {
    if (text != null) {
      String string = text.toString().trim();

      if (string.length() > 0) {
        return string;
      }
    }

    return null;
  }

  public static String getNodeText (AccessibilityNodeInfo node) {
    String text;

    if ((text = normalizeText(node.getContentDescription())) != null) {
      return text;
    }

    if ((text = normalizeText(node.getText())) != null) {
      return text;
    }

    if (node.getActions() != 0)  {
      return "";
    }

    return null;
  }

  private ScreenUtilities () {
  }
}
