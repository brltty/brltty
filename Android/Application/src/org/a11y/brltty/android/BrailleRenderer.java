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

import android.accessibilityservice.AccessibilityService;

public abstract class BrailleRenderer {
  protected void addVirtualElements (ScreenElementList elements) {
    elements.addAtTop("Notifications", AccessibilityService.GLOBAL_ACTION_NOTIFICATIONS);
    elements.addAtTop("Quick Settings", AccessibilityService.GLOBAL_ACTION_QUICK_SETTINGS);

    elements.addAtBottom("Back", AccessibilityService.GLOBAL_ACTION_BACK);
    elements.addAtBottom("Home", AccessibilityService.GLOBAL_ACTION_HOME);
    elements.addAtBottom("Recent Apps", AccessibilityService.GLOBAL_ACTION_RECENTS);
  }

  public abstract void renderScreenElements (
    List<CharSequence> rows,
    ScreenElementList elements
  );

  protected BrailleRenderer () {
  }
}
