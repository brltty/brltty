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

import android.os.Build;
import android.content.Context;
import android.view.KeyEvent;

import android.accessibilityservice.AccessibilityService;

public abstract class BrailleRenderer {
  protected void addVirtualElements (ScreenElementList elements) {
    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      elements.addAtTop(
        R.string.VIRTUAL_BUTTON_NOTIFICATIONS,
        AccessibilityService.GLOBAL_ACTION_NOTIFICATIONS
      );
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      elements.addAtTop(
        R.string.VIRTUAL_BUTTON_QUICK_SETTINGS,
        AccessibilityService.GLOBAL_ACTION_QUICK_SETTINGS
      );
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      elements.addAtBottom(
        R.string.VIRTUAL_BUTTON_BACK,
        AccessibilityService.GLOBAL_ACTION_BACK
      );
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      elements.addAtBottom(
        R.string.VIRTUAL_BUTTON_HOME,
        AccessibilityService.GLOBAL_ACTION_HOME
      );
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      elements.addAtBottom(
        R.string.VIRTUAL_BUTTON_RECENT_APPS,
        AccessibilityService.GLOBAL_ACTION_RECENTS
      );
    }
  }

  protected static int getTextWidth (String[] lines) {
    int width = 1;

    for (String line : lines) {
      width = Math.max(width, line.length());
    }

    return width;
  }

  public abstract void renderScreenElements (List<CharSequence> rows, ScreenElementList elements);

  protected BrailleRenderer () {
  }
}
