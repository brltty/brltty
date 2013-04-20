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

import java.util.List;
import java.util.ArrayList;

import android.content.Context;

import android.os.Build;

import android.accessibilityservice.AccessibilityService;

public abstract class BrailleRenderer {
  private static final BrailleRenderer simpleBrailleRenderer = new SimpleBrailleRenderer();
  private static BrailleRenderer currentBrailleRenderer = simpleBrailleRenderer;

  public static BrailleRenderer getBrailleRenderer () {
    return currentBrailleRenderer;
  }

  protected void addVirtualElements (ScreenElementList elements) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
      Context context = ApplicationHooks.getContext();

      elements.addAtTop(
        context.getString(R.string.VIRTUAL_BUTTON_NOTIFICATIONS),
        AccessibilityService.GLOBAL_ACTION_NOTIFICATIONS
      );

      elements.addAtTop(
        context.getString(R.string.VIRTUAL_BUTTON_QUICK_SETTINGS),
        AccessibilityService.GLOBAL_ACTION_QUICK_SETTINGS
      );

      elements.addAtBottom(
        context.getString(R.string.VIRTUAL_BUTTON_BACK),
        AccessibilityService.GLOBAL_ACTION_BACK
      );

      elements.addAtBottom(
        context.getString(R.string.VIRTUAL_BUTTON_HOME),
        AccessibilityService.GLOBAL_ACTION_HOME
      );

      elements.addAtBottom(
        context.getString(R.string.VIRTUAL_BUTTON_RECENT_APPS),
        AccessibilityService.GLOBAL_ACTION_RECENTS
      );
    }
  }

  protected static List<CharSequence> makeTextLines (String text) {
    List<CharSequence> lines = new ArrayList<CharSequence>();

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

    return lines;
  }

  protected static int getTextWidth (List<CharSequence> lines) {
    int width = 1;

    for (CharSequence line : lines) {
      width = Math.max(width, line.length());
    }

    return width;
  }

  public abstract void renderScreenElements (List<CharSequence> rows, ScreenElementList elements);

  protected BrailleRenderer () {
  }
}
